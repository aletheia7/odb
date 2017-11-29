// Copyright 2016 aletheia7. All rights reserved. Use of this source code is
// governed by a BSD-2-Clause license that can be found in the LICENSE file.

// +build linux,cgo

package odb

/*
#cgo CFLAGS:  -I${SRCDIR}/odbtp
#cgo LDFLAGS: -L${SRCDIR}/odbtp/.libs -lm -lc -lodbtp
#include <stdlib.h>
#include "odbtp.h"

int get_size() {
	return sizeof(odbLONG);
}
*/
import "C"
import (
	"errors"
	"fmt"
	"net"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"
	"unsafe"
)

var lock sync.Mutex

type Driver string

const (
	Default_port        = 2799
	Msaccess     Driver = `DRIVER=Microsoft Access Driver (*.mdb)`
	Foxpro              = `DRIVER={Microsoft Visual FoxPro Driver}`
)

// Set_truncate_seconds will call time.Time.Truncate(time.Seconds) for
// Query.Set_param_time(). msaccess does not return results with seconds
//
func Truncate_seconds(enable bool) option {
	return func(o *Con) (option, error) {
		prev := o.truncate_seconds
		return Truncate_seconds(prev), nil
	}
}

type login C.odbUSHORT

const (
	Normal   login = C.ODB_LOGIN_NORMAL
	Reserved       = C.ODB_LOGIN_RESERVED
	Single         = C.ODB_LOGIN_SINGLE
)

type Data int

func (o *Data) String() string {
	if s, ok := dt[*o]; ok {
		return s
	}
	return strconv.Itoa(int(*o))
}

const (
	Char                 = Data(C.ODB_CHAR)
	Null                 = Data(C.ODB_NULL)
	Datetime             = Data(C.ODB_DATETIME)
	Int                  = Data(C.ODB_INT)
	Double               = Data(C.ODB_DOUBLE)
	Bit                  = Data(C.ODB_BIT)
	Smallint             = Data(C.ODB_SMALLINT)
	Binary               = Data(C.ODB_BINARY)
	Binary_msaccess_memo = Data(-100)
)

var odb2sql = map[Driver]map[Data]string{
	Msaccess: {
		Char:                 "char",
		Datetime:             "datetime",
		Int:                  "integer",
		Double:               "double",
		Bit:                  "bit",
		Smallint:             "smallint",
		Binary:               "Text",
		Binary_msaccess_memo: "Text",
	},
	Foxpro: { // https://msdn.microsoft.com/en-us/library/z3y7feks(v=vs.80).aspx TYPE()
		Char:                 "C",
		Datetime:             "T",
		Int:                  "N",
		Double:               "N",
		Bit:                  "L",
		Smallint:             "N",
		Binary:               "W",
		Binary_msaccess_memo: "M",
	},
}

var dt = map[Data]string{
	Char:                 "char",
	Null:                 "null",
	Datetime:             "datetime",
	Int:                  "int",
	Double:               "double",
	Bit:                  "bit",
	Smallint:             "smallint",
	Binary:               "W",
	Binary_msaccess_memo: "binary msaccess memo",
}

type Cursor int

const (
	Cursor_forward = Cursor(C.ODB_CURSOR_FORWARD)
	Cursor_static  = Cursor(C.ODB_CURSOR_STATIC)
	Cursor_keyset  = Cursor(C.ODB_CURSOR_KEYSET)
	Cursor_dynamic = Cursor(C.ODB_CURSOR_DYNAMIC)
)

type Concur int

const (
	Concur_default  = Concur(C.ODB_CONCUR_DEFAULT)
	Concur_readonly = Concur(C.ODB_CONCUR_READONLY)
	Concur_lock     = Concur(C.ODB_CONCUR_LOCK)
	Concur_rowver   = Concur(C.ODB_CONCUR_ROWVER)
	Concur_values   = Concur(C.ODB_CONCUR_VALUES)
)

type Param int

func (o Param) ushort() C.odbUSHORT {
	return C.odbUSHORT(o)
}

const Input = Param(C.ODB_PARAM_INPUT)

type Con struct {
	driver           Driver
	h                handle
	qrys             map[*Query]bool
	convert_all      bool
	row_cache_size   uint
	truncate_seconds bool
}

// host example: locust | locust:2799
//
func New(host string, login login, dsn string, opt ...option) (*Con, error) {
	lock.Lock()
	defer lock.Unlock()
	if !strings.Contains(host, ":") {
		host += ":" + strconv.Itoa(Default_port)
	}
	tcp, err := net.ResolveTCPAddr("tcp", host)
	if err != nil {
		return nil, err
	}
	r := &Con{
		qrys: map[*Query]bool{},
	}
	switch {
	case strings.HasPrefix(dsn, string(Msaccess)):
		r.driver = Msaccess
	case strings.HasPrefix(dsn, string(Foxpro)):
		r.driver = Foxpro
	default:
		return nil, fmt.Errorf("unknown driver", dsn)
	}
	r.h = handle(C.odbAllocate(nil))
	if r.h == nil {
		return nil, errors.New("allocate failed")
	}
	h := new_s(tcp.IP.String())
	defer h.free()
	p := new_s(dsn)
	defer p.free()
	if !B2b(C.odbLogin(r.h, h.p, C.odbUSHORT(tcp.Port), C.odbUSHORT(login), p.p)) {
		err = oe2err(r.h)
		C.odbFree(r.h)
		return nil, err
	}
	for _, o := range opt {
		if _, err = o(r); err != nil {
			r.Logout(true)
			r.Free()
			return nil, err
		}
	}
	return r, err
}

type option func(o *Con) (option, error)

func Load_data_types() option {
	return func(o *Con) (option, error) {
		if o.h != nil {
			if !B2b(C.odbLoadDataTypes(o.h)) {
				return nil, oe2err(o.h)
			}
		}
		return Load_data_types(), nil
	}
}

func Unicode(enable bool) option {
	return func(o *Con) (option, error) {
		var long C.odbULONG
		var err error
		if !B2b(C.odbGetAttrLong(o.h, C.odbLONG(C.ODB_ATTR_UNICODESQL), &long)) {
			return nil, oe2err(o.h)
		}
		var prev bool
		if long == 1 {
			prev = true
		}
		if enable {
			long = 1
		} else {
			long = 0
		}
		if !B2b(C.odbSetAttrLong(o.h, C.odbLONG(C.ODB_ATTR_UNICODESQL), long)) {
			return nil, oe2err(o.h)
		}
		return Unicode(prev), err
	}
}

func Cache_procs(enable bool) option {
	return func(o *Con) (option, error) {
		var long C.odbULONG
		var err error
		if !B2b(C.odbGetAttrLong(o.h, C.odbLONG(C.ODB_ATTR_CACHEPROCS), &long)) {
			return nil, oe2err(o.h)
		}
		var prev bool
		if long == 1 {
			prev = true
		}
		if enable {
			long = 1
		} else {
			long = 0
		}
		if !B2b(C.odbSetAttrLong(o.h, C.odbLONG(C.ODB_ATTR_CACHEPROCS), long)) {
			return nil, oe2err(o.h)
		}
		return Cache_procs(prev), err
	}
}

func Querytimeout(timeout uint) option {
	return func(o *Con) (option, error) {
		var prev C.odbULONG
		if !B2b(C.odbGetAttrLong(o.h, C.odbLONG(C.ODB_ATTR_QUERYTIMEOUT), &prev)) {
			return nil, oe2err(o.h)
		}
		if !B2b(C.odbSetAttrLong(o.h, C.odbLONG(C.ODB_ATTR_QUERYTIMEOUT), C.odbULONG(timeout))) {
			return nil, oe2err(o.h)
		}
		return Querytimeout(uint(prev)), nil
	}
}

func Row_cache(enable bool, size uint) option {
	return func(o *Con) (option, error) {
		prev := B2b(C.odbIsUsingRowCache(o.h))
		prev_size := o.row_cache_size
		o.row_cache_size = size
		if !B2b(C.odbUseRowCache(o.h, b2B(enable), C.odbULONG(o.row_cache_size))) {
			return nil, oe2err(o.h)
		}
		return Row_cache(prev, prev_size), nil
	}
}

func Convert_all(enable bool) option {
	return func(o *Con) (option, error) {
		prev := o.convert_all
		o.convert_all = enable
		if !B2b(C.odbConvertAll(o.h, b2B(enable))) {
			return nil, oe2err(o.h)
		}
		return Convert_all(prev), nil
	}
}

func (o *Con) Logout(disconnect bool) {
	lock.Lock()
	defer lock.Unlock()
	if o.h != nil {
		C.odbLogout(o.h, b2B(disconnect))
		C.odbFree(o.h)
		o.h = nil
	}
}

func (o *Con) Free() {
	lock.Lock()
	defer lock.Unlock()
	if o.h != nil {
		C.odbFree(o.h)
		o.h = nil
	}
	o.qrys = map[*Query]bool{}
}

func (o *Con) Allocate_query() *Query {
	lock.Lock()
	defer lock.Unlock()
	r := &Query{
		con: o,
		h:   handle(C.odbAllocate(o.h)),
	}
	if r.h == nil {
		r = nil
	}
	if r != nil {
		o.qrys[r] = true
	}
	return r
}

func (o *Con) Prepare(sql string) (*Query, error) {
	return o.prepare_all(sql, false)
}

// Do not use with Single
//
func (o *Con) Prepare_proc(sql string) (*Query, error) {
	return o.prepare_all(sql, true)
}

func (o *Con) Version() string {
	lock.Lock()
	defer lock.Unlock()
	p := C.odbGetVersion(o.h)
	if p == nil {
		return ``
	} else {
		return C.GoString((*C.char)(unsafe.Pointer(p)))
	}
}

func (o *Con) remove(qry *Query) {
	if _, ok := o.qrys[qry]; ok {
		delete(o.qrys, qry)
	}
}

type Query struct {
	con *Con
	h   handle
}

// defaults: odb.Cursor_forward, Concur_default, false
//
func (o *Query) Set_cursor(cursor Cursor, concur Concur, enable_bookmarks bool) (err error) {
	lock.Lock()
	defer lock.Unlock()
	if !B2b(C.odbSetCursor(o.h, C.odbUSHORT(cursor), C.odbUSHORT(concur), b2B(enable_bookmarks))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) Param_sql_type_name(param interface{}) string {
	lock.Lock()
	defer lock.Unlock()
	return C.GoString((*C.char)(unsafe.Pointer(C.odbParamSqlTypeName(o.h, o.get_param(o, param)))))
}

func (o *Query) Input(param interface{}, dt Data, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.input(param, dt, odb2sql[o.con.driver][dt], final)
}

type In struct {
	Dt Data
}

type Inp struct {
	Col interface{}
	Dt  Data
}

// Inputs calls Input using an array of In.
//
func (o *Query) Inputs(in []In) error {
	lock.Lock()
	defer lock.Unlock()
	final := false
	var err error
	for i, p := range in {
		if i == len(in)-1 {
			final = true
		}
		if err = o.input(i+1, p.Dt, odb2sql[o.con.driver][p.Dt], final); err != nil {
			return fmt.Errorf("%v, %v, %v, %v", i+1, p.Dt, odb2sql[o.con.driver][p.Dt], err)
		}
	}
	return nil
}

func (o *Query) Inputs_p(in []Inp) error {
	lock.Lock()
	defer lock.Unlock()
	final := false
	var err error
	for i, p := range in {
		if i == len(in)-1 {
			final = true
		}
		if err = o.input(p.Col, p.Dt, odb2sql[o.con.driver][p.Dt], final); err != nil {
			return fmt.Errorf("%v, %v, %v, %v", p.Col, p.Dt, odb2sql[o.con.driver][p.Dt], err)
		}
	}
	return nil
}

type Set struct {
	Data interface{} // can be nil for null
}

type Setp struct {
	Col  interface{}
	Data interface{}
}

// Set_param calls Set_param_<date type> receivers using an array of Set. Set
// Set.Param to nil to use one based incrementing column number.
//
func (o *Query) Set_param(set []Set) error {
	lock.Lock()
	defer lock.Unlock()
	final := false
	var err error
	var param int
	for i, p := range set {
		if i == len(set)-1 {
			final = true
		}
		param = i + 1
		switch t := p.Data.(type) {
		case string:
			err = o.set_param_text(param, t, final)
		case int:
			err = o.set_param_long(param, t, final)
		case time.Time:
			if t.IsZero() {
				err = o.set_param_null(param, final)
			} else {
				err = o.set_param_timestamp(param, t, final)
			}
		case float64:
			err = o.set_param_double(param, t, final)
		case byte:
			err = o.set_param_byte(param, t, final)
		case nil:
			err = o.set_param_null(param, final)
		default:
			return fmt.Errorf("data type not supported: %v", t)
		}
		if err != nil {
			return fmt.Errorf("param: %v, err: %v, data: %v", param, err, p.Data)
		}
	}
	return nil
}

func (o *Query) Set_param_p(set []Setp) error {
	lock.Lock()
	defer lock.Unlock()
	final := false
	var err error
	for i, p := range set {
		if i == len(set)-1 {
			final = true
		}
		switch t := p.Data.(type) {
		case string:
			err = o.set_param_text(p.Col, t, final)
		case int:
			err = o.set_param_long(p.Col, t, final)
		case time.Time:
			err = o.set_param_timestamp(p.Col, t, final)
		case float64:
			err = o.set_param_double(p.Col, t, final)
		case byte:
			err = o.set_param_byte(p.Col, t, final)
		case int16:
			err = o.set_param_short(p.Col, t, final)
		case nil:
			err = o.set_param_null(p.Col, final)
		default:
			return fmt.Errorf("data type not supported: %v", t)
		}
		if err != nil {
			return fmt.Errorf("%v, %v, %v", p.Col, err)
		}
	}
	return nil
}

func (o *Query) Set_param_text(param interface{}, v string, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_text(param, v, final)
}

func (o *Query) Set_param_long(param interface{}, v int, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_long(param, v, final)
}

func (o *Query) Set_param_short(param interface{}, v int16, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_short(param, v, final)
}

func (o *Query) Set_param_double(param interface{}, v float64, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_double(param, v, final)
}

func (o *Query) Set_param_byte(param interface{}, v byte, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_byte(param, v, final)
}

// msaccess does not like fractional seconds. Use Truncate_seconds(true) option.
//
func (o *Query) Set_param_timestamp(param interface{}, v time.Time, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_timestamp(param, v, final)
}

func (o *Query) Set_param_null(param interface{}, final bool) error {
	lock.Lock()
	defer lock.Unlock()
	return o.set_param_null(param, final)
}

func (o *Query) Get_total_rows() int {
	lock.Lock()
	defer lock.Unlock()
	return int(C.odbGetTotalRows(o.h))
}

func (o *Query) Detach() (err error) {
	lock.Lock()
	defer lock.Unlock()
	if !B2b(C.odbDetachQry(o.h)) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) Get_total_params() int {
	lock.Lock()
	defer lock.Unlock()
	return int(C.odbGetTotalParams(o.h))
}

func (o *Query) Execute(sql string) error {
	lock.Lock()
	defer lock.Unlock()
	var r bool
	if 0 < len(sql) {
		s := new_s(sql)
		defer s.free()
		r = B2b(C.odbExecute(o.h, s.p))
	} else {
		r = B2b(C.odbExecute(o.h, C.odbPCSTR(nil)))
	}
	if r {
		return nil
	}
	return oe2err(o.h)
}

func (o *Query) Fetch_row() error {
	lock.Lock()
	defer lock.Unlock()
	if B2b(C.odbFetchRow(o.h)) {
		return nil
	}
	return oe2err(o.h)
}

func (o *Query) Fetch_row_abs(row int) (err error) {
	if !B2b(C.odbFetchRowEx(o.h, C.ODB_FETCH_ABS, C.odbLONG(row))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) Free() {
	lock.Lock()
	defer lock.Unlock()
	if o.h != nil {
		C.odbDropQry(o.h)
		C.odbFree(o.h)
		o.h = nil
	}
}

// col: int | string
//
func (o *Query) Col_data_text(col interface{}) (v string) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		v = C.GoString((*C.char)(unsafe.Pointer(C.odbColDataText(o.h, col))))
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_time(col interface{}) (v time.Time) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		ts := C.odbColDataTimestamp(o.h, col)
		v = time.Date(
			int(ts.sYear),
			time.Month(ts.usMonth),
			int(ts.usDay),
			int(ts.usHour),
			int(ts.usMinute),
			int(ts.usSecond),
			int(ts.ulFraction),
			time.Local,
		)
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_byte(col interface{}) (v byte) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		v = byte(C.odbColDataByte(o.h, col))
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_short(col interface{}) (v int16) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		v = int16(C.odbColDataShort(o.h, col))
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_double(col interface{}) (v float64) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		v = float64(C.odbColDataDouble(o.h, col))
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_long(col interface{}) (v int) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		v = int(C.odbColDataLong(o.h, col))
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_type(col interface{}) (v Data) {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) != C.ODB_NULL {
		v = Data((C.odbColDataType(o.h, col)))
	}
	return
}

// col: int | string
//
func (o *Query) Col_data_len(col interface{}) int {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col {
		return int(C.odbColDataLen(o.h, col))
	}
	return -1
}

// col: int | string
//
func (o *Query) Col_is_null(col interface{}) bool {
	lock.Lock()
	defer lock.Unlock()
	if col := o.get_param(o, col); 0 < col && C.odbColDataLen(o.h, col) == C.ODB_NULL {
		return true
	}
	return false
}

func (o *Query) No_data() bool {
	lock.Lock()
	defer lock.Unlock()
	return B2b(C.odbNoData(o.h))
}

func (o *Query) Get_total_columns() int {
	lock.Lock()
	defer lock.Unlock()
	return int(C.odbGetTotalCols(o.h))
}

func (o *Query) Col_name(col int) string {
	lock.Lock()
	defer lock.Unlock()
	if r := C.odbColName(o.h, C.odbUSHORT(col)); r == nil {
		return ``
	} else {
		return C.GoString((*C.char)(unsafe.Pointer(r)))
	}
}

// Convert a Foxpro Memo string to a map
//
func S2memo(s string) (r map[string]string) {
	r = map[string]string{}
	if 0 < len(s) {
		for _, kv := range strings.Split(s, "\r") {
			a := strings.Split(kv, "=")
			if len(a) == 2 && 0 < len(a[0]) && 0 < len(a[1]) {
				r[a[0]] = a[1]
			}
		}
	}
	return
}

// Convert a map to Foxpro Memo string
//
func Memo2s(m map[string]string) string {
	keys := make([]string, len(m))
	i := 0
	for k := range m {
		keys[i] = k
		i++
	}
	sort.Strings(keys)
	a := make([]string, 0, len(m))
	for _, k := range keys {
		a = append(a, k+"="+m[k])
	}
	return strings.Join(a, "\r")
}

type handle C.odbHANDLE

type dcmd uint8

var efalse = errors.New("false")

func Is_amd64() int {
	return int(C.get_size())
	// return C.sizeof_odbLONG
}

func (o *Query) get_param(q *Query, param interface{}) C.odbUSHORT {
	switch t := param.(type) {
	case int:
		return C.odbUSHORT(t)
	case string:
		s := new_s(t)
		defer s.free()
		return C.odbUSHORT(C.odbColNum(q.h, s.p))
	default:
		return C.odbUSHORT(0)
	}
}

func (o *Con) prepare_all(sql string, proc bool) (q *Query, err error) {
	lock.Lock()
	defer lock.Unlock()
	q = &Query{
		con: o,
		h:   handle(C.odbAllocate(o.h)),
	}
	if q.h == nil {
		q = nil
	} else {
		s := new_s(sql)
		defer s.free()
		if proc {
			if !B2b(C.odbPrepareProc(q.h, s.p)) {
				err = oe2err(q.h)
				C.odbFree(q.h)
				q = nil
			}
		} else {
			if !B2b(C.odbPrepare(q.h, s.p)) {
				err = oe2err(q.h)
				C.odbFree(q.h)
				q = nil
			}
		}
	}
	if q != nil {
		o.qrys[q] = true
	}
	return
}

func (o *Query) input(param interface{}, dt Data, sql_type string, final bool) (err error) {
	var st_num C.odbSHORT
	var col_size C.odbULONG
	var dec_dig C.odbSHORT
	if dt == Binary_msaccess_memo {
		col_size = 65536
		st_num = C.SQL_TEXT
	} else {
		st := new_s(sql_type)
		defer st.free()
		if !B2b(C.odbDescribeSqlType(o.con.h, st.p, &st_num, &col_size, &dec_dig)) {
			err = oe2err(o.h)
			return
		}
	}
	if !B2b(C.odbBindParamEx(o.h, o.get_param(o, param), Input.ushort(), C.odbSHORT(dt), 0, st_num, col_size, dec_dig, b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param(param interface{}, v string, final bool) (err error) {
	s := new_s(v)
	defer s.free()
	l := len(v)
	if !B2b(C.odbSetParam(o.h, o.get_param(o, param), (C.odbPVOID)(s.p), C.odbLONG(l), b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_text(param interface{}, v string, final bool) (err error) {
	s := new_s(v)
	defer s.free()
	if !B2b(C.odbSetParamText(o.h, o.get_param(o, param), s.p, b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_long(param interface{}, v int, final bool) (err error) {
	if !B2b(C.odbSetParamLong(o.h, o.get_param(o, param), C.odbULONG(v), b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_short(param interface{}, v int16, final bool) (err error) {
	if !B2b(C.odbSetParamShort(o.h, o.get_param(o, param), C.odbUSHORT(v), b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_double(param interface{}, v float64, final bool) (err error) {
	if !B2b(C.odbSetParamDouble(o.h, o.get_param(o, param), C.odbDOUBLE(v), b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_byte(param interface{}, v byte, final bool) (err error) {
	if !B2b(C.odbSetParamByte(o.h, o.get_param(o, param), C.odbBYTE(v), b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_timestamp(param interface{}, v time.Time, final bool) (err error) {
	var ts C.odbTIMESTAMP
	ts.sYear = C.odbSHORT(v.Year())
	ts.usMonth = C.odbUSHORT(v.Month())
	ts.usDay = C.odbUSHORT(v.Day())
	ts.usHour = C.odbUSHORT(v.Hour())
	ts.usMinute = C.odbUSHORT(v.Minute())
	ts.usSecond = C.odbUSHORT(v.Second())
	if o.con.truncate_seconds {
		ts.ulFraction = 0
	} else {
		ts.ulFraction = C.odbULONG(v.Nanosecond())
	}
	if !B2b(C.odbSetParamTimestamp(o.h, o.get_param(o, param), &ts, b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Query) set_param_null(param interface{}, final bool) (err error) {
	if !B2b(C.odbSetParamNull(o.h, o.get_param(o, param), b2B(final))) {
		err = oe2err(o.h)
	}
	return
}

type nchar struct {
	p (*C.odbCHAR)
}

func new_s(s string) *nchar {
	return &nchar{(*C.odbCHAR)(C.CString(s))}
}

func (o *nchar) free() {
	if o.p != nil {
		C.free(unsafe.Pointer(o.p))
		o.p = nil
	}
}

func b2B(b bool) C.odbBOOL {
	if b {
		return C.odbBOOL(1)
	}
	return C.odbBOOL(0)
}

func B2b(b C.odbBOOL) bool {
	if b == 0 {
		return false
	}
	return true
}

func B2e(b C.odbBOOL) error {
	if b == 0 {
		return efalse
	}
	return nil
}

func oe2err_p(h C.odbHANDLE, format string, a ...interface{}) error {
	e := C.GoString((*C.char)(unsafe.Pointer(C.odbGetErrorText(h))))
	return fmt.Errorf("%v:"+format, e, a)
}

func oe2err(h handle) error {
	return errors.New(C.GoString((*C.char)(unsafe.Pointer(C.odbGetErrorText(h)))))
}
