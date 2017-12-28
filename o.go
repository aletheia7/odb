// Copyright 2016 aletheia7. All rights reserved. Use of this source code is
// governed by a BSD-2-Clause license that can be found in the LICENSE file.

package odb

/*
#cgo CFLAGS:  -I${SRCDIR}/vendor/odbtp
#cgo LDFLAGS: -L${SRCDIR}/vendor/odbtp/.libs -lm -lc -lodbtp
#include <stdlib.h>
#include "odbtp.h"
*/
import "C"
import (
	"bytes"
	"context"
	"database/sql"
	"database/sql/driver"
	"errors"
	"fmt"
	"io"
	"net"
	"sort"
	"strconv"
	"strings"
	"sync/atomic"
	"text/template"
	"text/template/parse"
	"time"
	"unsafe"
)

type Driver_odbc string

const (
	Default_port                  = 2799
	Msaccess          Driver_odbc = `DRIVER=Microsoft Access Driver (*.mdb)`
	Foxpro                        = `DRIVER={Microsoft Visual FoxPro Driver}`
	Sql_get_type_info             = "||SQLGetTypeInfo"
)

type Login C.odbUSHORT

const (
	Normal   Login = C.ODB_LOGIN_NORMAL
	Reserved       = C.ODB_LOGIN_RESERVED
	Single         = C.ODB_LOGIN_SINGLE
)

type Data int16

func (o Data) String() string {
	s, ok := data2s[o]
	if ok {
		return s
	}
	return fmt.Sprintf("invalid ODB data type: %v", o)
}

var data2s = map[Data]string{
	Binary:   "Binary",
	Wchar:    "Wchar",
	Smallint: "Smallint",
	Int:      "Int",
	Bigint:   "Bigint",
	Bit:      "Bit",
	Char:     "Char",
	Double:   "Double",
	Datetime: "Datetime",
	// Guid:      "Guid",
	// Usmallint: "Usmallint",
	// Uint:      "Uint",
	// Bigint:    "Bigint",
	// Tinyint:   "Timyint",
	// Ubigint:   "Ubigint",
	// Utinyint:  "Utinyint",
	// Numeric:   "Numeric",
	// Real:      "Real",
	// Date:      "Date",
	// Time:      "Time",
}

func (o *Data) short() C.odbSHORT {
	return C.odbSHORT(*o)
}

const (
	Binary   Data = C.ODB_BINARY   // (-2)
	Wchar         = C.ODB_WCHAR    // (-8) Use for unicode
	Smallint      = C.ODB_SMALLINT // (-15)
	Int           = C.ODB_INT      // (-16)
	Bigint        = C.ODB_BIGINT   // (-25)
	Bit           = C.ODB_BIT      // (-7)
	Char          = C.ODB_CHAR     // 1
	Double        = C.ODB_DOUBLE   // 8 same as C.ODB_FLOAT
	Datetime      = C.ODB_DATETIME // 93
	// Guid           = C.ODB_GUID      // (-11)
	// Usmallint      = C.ODB_USMALLINT // (-17)
	// Uint           = C.ODB_UINT      // (-18)
	// Tinyint        = C.ODB_TINYINT   // (-26)
	// Ubigint        = C.ODB_UBIGINT   // (-27)
	// Utinyint       = C.ODB_UTINYINT  // (-28)
	// Numeric        = C.ODB_NUMERIC   // 2
	// Real           = C.ODB_REAL      // 7
	// Date           = C.ODB_DATE      // 91
	// Time           = C.ODB_TIME      // 92
)

type Sql_type int16

func (o Sql_type) String() string {
	s, ok := sql_type2s[o]
	if ok {
		return s
	}
	return fmt.Sprintf("invalid SQL data type: %v", o)
}

// duplicates are commented-out
var sql_type2s = map[Sql_type]string{
	Sql_bit:      "SQL_BIT ",
	Sql_tinyint:  "SQL_TINYINT",
	Sql_smallint: "SQL_SMALLINT",
	// Sql_integer:        "SQL_INTEGER",
	Sql_int:       "SQL_INT",
	Sql_bitint:    "SQL_BIGINT",
	Sql_numeric:   "SQL_NUMERIC",
	Sql_real:      "SQL_REAL",
	Sql_float:     "SQL_FLOAT",
	Sql_double:    "SQL_DOUBLE",
	Sql_decimal:   "SQL_DECIMAL",
	Sql_date:      "SQL_DATE",
	Sql_time:      "SQL_TIME",
	Sql_timestamp: "SQL_TIMESTAMP",
	Sql_type_date: "SQL_TYPE_DATE",
	Sql_type_time: "SQL_TYPE_TIME",
	// Sql_type_timestamp: "SQL_TYPE_TIMESTAMP",
	Sql_datetime: "SQL_DATETIME",
	Sql_char:     "SQL_CHAR",
	Sql_varchar:  "SQL_VARCHAR",
	// Sql_longvarchar:   "SQL_LONGVARCHAR",
	Sql_text:  "SQL_TEXT",
	Sql_wchar: "SQL_WCHAR",
	// Sql_nchar:         "SQL_NCHAR",
	Sql_wvarchar: "SQL_WVARCHAR",
	// Sql_nvarchar:      "SQL_NVARCHAR",
	// Sql_wlongvarchar: "SQL_WLONGVARCHAR",
	Sql_ntext:         "SQL_NTEXT",
	Sql_binary:        "SQL_BINARY",
	Sql_varbinary:     "SQL_VARBINARY",
	Sql_longvarbinary: "SQL_LONGVARBINARY",
	// Sql_image:         "SQL_IMAGE",
	Sql_guid:    "SQL_GUID",
	Sql_variant: "SQL_VARIANT",
}

const (
	Sql_bit            Sql_type = C.SQL_BIT            // (-7)
	Sql_tinyint                 = C.SQL_TINYINT        // (-6)
	Sql_smallint                = C.SQL_SMALLINT       // 5
	Sql_integer                 = C.SQL_INTEGER        // 4
	Sql_int                     = C.SQL_INT            // 4
	Sql_bitint                  = C.SQL_BIGINT         // (-5)
	Sql_numeric                 = C.SQL_NUMERIC        // 2
	Sql_real                    = C.SQL_REAL           // 7
	Sql_float                   = C.SQL_FLOAT          // 6
	Sql_double                  = C.SQL_DOUBLE         // 8
	Sql_decimal                 = C.SQL_DECIMAL        // 3
	Sql_date                    = C.SQL_DATE           // 9
	Sql_time                    = C.SQL_TIME           // 10
	Sql_timestamp               = C.SQL_TIMESTAMP      // 11
	Sql_type_date               = C.SQL_TYPE_DATE      // 91
	Sql_type_time               = C.SQL_TYPE_TIME      // 92
	Sql_type_timestamp          = C.SQL_TYPE_TIMESTAMP // 93
	Sql_datetime                = C.SQL_DATETIME       // 93
	Sql_char                    = C.SQL_CHAR           // 1
	Sql_varchar                 = C.SQL_VARCHAR        // 12
	Sql_longvarchar             = C.SQL_LONGVARCHAR    // (-1)
	Sql_text                    = C.SQL_TEXT           // (-1)
	Sql_wchar                   = C.SQL_WCHAR          // (-8)
	Sql_nchar                   = C.SQL_NCHAR          // (-8)
	Sql_wvarchar                = C.SQL_WVARCHAR       // (-9)
	Sql_nvarchar                = C.SQL_NVARCHAR       // (-9)
	Sql_wlongvarchar            = C.SQL_WLONGVARCHAR   // (-10)
	Sql_ntext                   = C.SQL_NTEXT          // (-10)
	Sql_binary                  = C.SQL_BINARY         // (-2)
	Sql_varbinary               = C.SQL_VARBINARY      // (-3)
	Sql_longvarbinary           = C.SQL_LONGVARBINARY  // (-4)
	Sql_image                   = C.SQL_IMAGE          // (-4)
	Sql_guid                    = C.SQL_GUID           // (-11)
	Sql_variant                 = C.SQL_VARIANT        // (-150)
)

type desc struct {
	sql_type   C.odbSHORT
	col_size   C.odbULONG
	dec_digits C.odbSHORT
}

var odb2sql = map[Driver_odbc]map[Data]desc{
	Msaccess: {
		Binary:   desc{C.SQL_BINARY, 255, 0},
		Wchar:    desc{C.SQL_NTEXT, 1073741823, 0}, // memo
		Smallint: desc{C.SQL_SMALLINT, 5, 0},
		Int:      desc{C.SQL_INT, 10, 0},
		Bigint:   desc{C.SQL_DOUBLE, 53, 0},
		// Bigint:   desc{C.SQL_BIGINT, 19, 0}, // missing
		Bit:      desc{C.SQL_BIT, 1, 0},
		Char:     desc{C.SQL_CHAR, 255, 0},
		Double:   desc{C.SQL_DOUBLE, 53, 0},
		Datetime: desc{C.SQL_DATETIME, 23, 3},
	},
	// Foxpro: { // https://msdn.microsoft.com/en-us/library/z3y7feks(v=vs.80).aspx TYPE()
	// 	Char:     "C",
	// 	Datetime: "T",
	// 	Int:      "N",
	// 	Double:   "N",
	// 	Bit:      "L",
	// 	Smallint: "N",
	// 	Binary:   "W",
	// 	// Binary_msaccess_memo: "M",
	// },
}

type Conn struct {
	driver              Driver_odbc
	h                   C.odbHANDLE
	stmts               map[*Stmt]bool
	prepare_is_template bool
}

// host example: locust | locust:2799
//
func new_con(host string, login Login, dsn string) (*Conn, error) {
	if !strings.Contains(host, ":") {
		host += ":" + strconv.Itoa(Default_port)
	}
	tcp, err := net.ResolveTCPAddr("tcp", host)
	if err != nil {
		return nil, err
	}
	r := &Conn{
		stmts: map[*Stmt]bool{},
	}
	switch {
	case strings.HasPrefix(dsn, string(Msaccess)):
		r.driver = Msaccess
	case strings.HasPrefix(dsn, string(Foxpro)):
		r.driver = Foxpro
	default:
		return nil, fmt.Errorf("unknown driver", dsn)
	}
	r.h = C.odbAllocate(nil)
	if r.h == nil {
		return nil, errors.New("allocate failed")
	}
	h := new_s(tcp.IP.String())
	defer h.free()
	p := new_s(dsn)
	defer p.free()
	if !dB2b(C.odbLogin(r.h, h.p, C.odbUSHORT(tcp.Port), C.odbUSHORT(login), p.p)) {
		err = oe2err(r.h)
		C.odbFree(r.h)
		return nil, err
	}
	return r, err
}

func (o *Conn) Ping(ctx context.Context) (err error) {
	select {
	case <-ctx.Done():
		return ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		if dB2b(C.odbIsConnected(o.h)) {
			return
		}
		err = driver.ErrBadConn
	}()
	select {
	case <-ctx.Done():
		return driver.ErrBadConn
	case <-done:
	}
	return
}

func (o *Conn) row_cache(enable bool, size uint64) (err error) {
	if !dB2b(C.odbUseRowCache(o.h, b2B(enable), C.odbULONG(size))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Conn) get_attr_bool(opt bool_option) (bool, error) {
	var long C.odbULONG
	if !dB2b(C.odbGetAttrLong(o.h, C.odbLONG(opt), &long)) {
		return false, oe2err(o.h)
	}
	if long == 1 {
		return true, nil
	}
	return false, nil
}

func (o *Conn) set_attr_bool(opt bool_option, enable bool) (err error) {
	var long C.odbULONG
	if enable {
		long = 1
	}
	if !dB2b(C.odbSetAttrLong(o.h, C.odbLONG(opt), long)) {
		err = oe2err(o.h)
	}
	return
}

func (o *Conn) set_attr_int(opt int_option, i uint64) (err error) {
	if !dB2b(C.odbSetAttrLong(o.h, C.odbLONG(opt), C.odbULONG(i))) {
		err = oe2err(o.h)
	}
	return
}

func (o *Conn) logout(disconnect bool) {
	if o.h != nil {
		C.odbLogout(o.h, b2B(disconnect))
		C.odbFree(o.h)
		o.h = nil
	}
}

func (o *Conn) drop() (err error) {
	for q := range o.stmts {
		if !dB2b(C.odbDropQry(q.h)) {
			return oe2err(q.h)
		}
	}
	o.stmts = map[*Stmt]bool{}
	return
}

func (o *Conn) free() {
	if o.h != nil {
		C.odbFree(o.h)
		o.h = nil
	}
	o.stmts = map[*Stmt]bool{}
}

func (o *Conn) allocate_query() *Stmt {
	r := &Stmt{
		con: o,
		h:   C.odbAllocate(o.h),
	}
	if r.h == nil {
		r = nil
	}
	if r != nil {
		o.stmts[r] = true
	}
	return r
}

func (o *Conn) prepare(sql string) (*Stmt, error) {
	r := &Stmt{
		con:   o,
		h:     C.odbAllocate(o.h),
		bindp: map[C.odbUSHORT]Data{},
	}
	if r.h == nil {
		return nil, fmt.Errorf("odbAllocate failed")
	}
	switch sql {
	case Sql_get_type_info:
		r.special = sql
	default:
		s := new_s(sql)
		defer s.free()
		if !dB2b(C.odbPrepare(r.h, s.p)) {
			err := oe2err(r.h)
			C.odbFree(r.h)
			return nil, err
		}
	}
	o.stmts[r] = true
	return r, nil
}

type Stmt struct {
	con            *Conn
	h              C.odbHANDLE
	bindp          map[C.odbUSHORT]Data
	fetch_err      error
	bound          bool
	ns             *named_sql
	special        string
	identity_table string // only msaccess
}

// only one string allowed
//
func (o *Stmt) execute(sql ...string) error {
	o.fetch_err = nil
	r := false
	if sql != nil && 0 < len(sql) {
		s := new_s(sql[0])
		defer s.free()
		r = dB2b(C.odbExecute(o.h, s.p))
	} else {
		r = dB2b(C.odbExecute(o.h, C.odbPCSTR(nil)))
	}
	if !r {
		return oe2err(o.h)
	}
	return nil
}

func (o *Stmt) fetch_row() error {
	if dB2b(C.odbFetchRow(o.h)) {
		return nil
	}
	return oe2err(o.h)
}

func (o *Stmt) free() {
	if o.h != nil {
		C.odbDropQry(o.h)
		C.odbFree(o.h)
		delete(o.con.stmts, o)
		o.h = nil
		o.bindp = map[C.odbUSHORT]Data{}
	}
}

// Convert a Foxpro Memo string to a map

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

// col: 1 based
func (o *Stmt) bind(col C.odbUSHORT, data Data, last C.odbBOOL) (err error) {
	desc := odb2sql[o.con.driver][data]
	if !dB2b(C.odbBindParamEx(
		o.h,
		col,
		C.ODB_PARAM_INPUT,
		data.short(),
		0,
		desc.sql_type,
		desc.col_size,
		desc.dec_digits,
		last,
	)) {
		return oe2err(o.h)
	}
	return
}

// col: 1 based
func (o *Stmt) set(col C.odbUSHORT, i interface{}, last C.odbBOOL) (err error) {
	switch v := i.(type) {
	case []byte:
		s := new_b(v)
		if !dB2b(C.odbSetParam(o.h, col, (C.odbPVOID)(s.p), C.odbLONG(len(v)), last)) {
			s.free()
			return oe2err(o.h)
		}
		s.free()
	case string:
		s := new_s(v)
		if !dB2b(C.odbSetParam(o.h, col, (C.odbPVOID)(s.p), C.odbLONG(len(v)), last)) {
			s.free()
			return oe2err(o.h)
		}
		s.free()
	case time.Time:
		var ts C.odbTIMESTAMP
		ts.sYear = C.odbSHORT(v.Year())
		ts.usMonth = C.odbUSHORT(v.Month())
		ts.usDay = C.odbUSHORT(v.Day())
		ts.usHour = C.odbUSHORT(v.Hour())
		ts.usMinute = C.odbUSHORT(v.Minute())
		ts.usSecond = C.odbUSHORT(v.Second())
		ts.ulFraction = C.odbULONG(v.Nanosecond())
		if !dB2b(C.odbSetParamTimestamp(o.h, col, &ts, last)) {
			return oe2err(o.h)
		}
	case nil:
		if !dB2b(C.odbSetParamNull(o.h, col, last)) {
			return oe2err(o.h)
		}
	case int64:
		if o.con.driver == Msaccess {
			if !dB2b(C.odbSetParamDouble(o.h, col, C.odbDOUBLE(float64(v)), last)) {
				return oe2err(o.h)
			}
		} else {
			if !dB2b(C.odbSetParamLongLong(o.h, col, C.odbULONGLONG(v), last)) {
				return oe2err(o.h)
			}
		}
	case float64:
		if !dB2b(C.odbSetParamDouble(o.h, col, C.odbDOUBLE(v), last)) {
			return oe2err(o.h)
		}
	case bool:
		var b C.odbBYTE
		if v == true {
			b = C.odbBYTE(1)
		}
		if !dB2b(C.odbSetParamByte(o.h, col, b, last)) {
			return oe2err(o.h)
		}
	default:
		return fmt.Errorf("invalid type: parama: %v, %T", col, v)
	}
	return
}

type nchar struct {
	p (*C.odbCHAR)
}

func new_s(s string) *nchar {
	return &nchar{(*C.odbCHAR)(C.CString(s))}
}

func new_b(b []byte) *nchar {
	return &nchar{(*C.odbCHAR)(C.CBytes(b))}
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

func dB2b(b C.odbBOOL) bool {
	if b == 0 {
		return false
	}
	return true
}

func oe2err(h C.odbHANDLE) error {
	return errors.New(C.GoString((*C.char)(unsafe.Pointer(C.odbGetErrorText(h)))))
}

// Driver name
const Driver_msaccess = "odbtp_msaccess"

var driver_name_ct uint32

// Returns the registered driver name to use in sql.Open(). The driver name pattern is
// odbtp_msaccess_1, odbtp_msaccess_...
//    driver_name := odb.Register(
//     	"<host_name>",
//     	odb.Normal,
//     	`DRIVER=Microsoft Access Driver (*.mdb);DBQ=c:/<file path to mdb>;ImplicitCommitSync=Yes`,
//     	odb.Int_opt(odb.Query_timeout, 20),
//     	odb.Bool_opt(odb.Unicodesql, true),
//     	odb.Bool_opt(odb.Describe_params, true),
//     	odb.Bool_opt(odb.Mapchar2wchar, true),
//     	odb.Bool_opt(odb.Prepare_is_template, true),
//    )
//   db, err := sql.Open(driver_name, ``)
//   if err != nil {
//    	j.Err(err)
//    	return
//   }
//   defer db.Close()
//
func Register(address string, login Login, odbc_dsn string, opt ...option) (driver_name string) {
	driver_name = fmt.Sprintf("%v_%v", Driver_msaccess, atomic.AddUint32(&driver_name_ct, 1))
	sql.Register(driver_name, &Driver{
		addr:     address,
		login:    login,
		odbc_dsn: odbc_dsn,
		opt:      opt,
	})
	return
}

type option func(o *Conn) error

type bool_option C.odbLONG

const (
	// Example in query: {{.id}} becomes sql.Named("id", <value)
	Prepare_is_template bool_option = 1 << iota * -1 // ODB_ATTR are positive
	Cache_procs                     = C.ODB_ATTR_CACHEPROCS
	Mapchar2wchar                   = C.ODB_ATTR_MAPCHARTOWCHAR
	Describe_params                 = C.ODB_ATTR_DESCRIBEPARAMS
	Unicodesql                      = C.ODB_ATTR_UNICODESQL
)

func Bool_opt(opt bool_option, enable bool) option {
	return func(o *Conn) error {
		switch opt {
		case Prepare_is_template:
			o.prepare_is_template = enable
		default:
			return o.set_attr_bool(opt, enable)
		}
		return nil
	}
}

type int_option C.odbLONG

const (
	Query_timeout int_option = C.ODB_ATTR_QUERYTIMEOUT
	Vardatasize              = C.ODB_ATTR_VARDATASIZE
)

func Int_opt(opt int_option, i uint64) option {
	return func(o *Conn) error {
		return o.set_attr_int(opt, i)
	}
}

type Driver struct {
	addr     string
	odbc_dsn string
	login    Login
	opt      []option
	con      *Conn
}

// dsn is not used. Use Register()
//
func (o *Driver) Open(dsn string) (driver.Conn, error) {
	if 0 < len(dsn) {
		return nil, fmt.Errorf("use Register")
	}
	if len(o.odbc_dsn) == 0 {
		return nil, fmt.Errorf("missing Dsn doption")
	}
	con, err := new_con(o.addr, o.login, o.odbc_dsn)
	if err != nil {
		return nil, err
	}
	if o.opt != nil {
		for _, oo := range o.opt {
			if err := oo(con); err != nil {
				con.logout(true)
				return nil, err
			}
		}
	}
	return con, nil
}
func (o *Conn) Prepare(query string) (ds driver.Stmt, err error) {
	return o.PrepareContext(context.Background(), query)
}

func (o *Conn) PrepareContext(ctx context.Context, query string) (ds driver.Stmt, err error) {
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		switch query {
		case Sql_get_type_info:
		default:
		}
		var ns *named_sql
		if o.prepare_is_template {
			ns = new_named_sql(query)
			if ns.err != nil {
				err = ns.err
				return
			}
			query = ns.sql
		}
		var st *Stmt
		st, err = o.prepare(query)
		if err != nil {
			return
		}
		st.ns = ns
		ds = st
		return
	}()
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-done:
	}
	return
}

type named_sql struct {
	sql  string
	uniq map[string]string
	pos  map[int]string
	err  error
}

func new_named_sql(tp_s string) (r *named_sql) {
	r = &named_sql{
		uniq: map[string]string{},
		pos:  map[int]string{},
	}
	if !strings.Contains(tp_s, "{{") {
		r.sql = tp_s
		return
	}
	var tree map[string]*parse.Tree
	tree, r.err = parse.Parse(``, tp_s, ``, ``, map[string]interface{}{})
	if r.err != nil {
		return
	}
	i := 0
	for _, n := range tree[""].Root.Nodes {
		if an, ok := n.(*parse.ActionNode); ok {
			if an.Pipe == nil {
				continue
			}
			if len(an.Pipe.Cmds) == 1 {
				name := an.Pipe.Cmds[0].String()[1:] // remove dot
				r.uniq[name] = `?`
				r.pos[i] = name
				i++
			}
		}
	}
	var buf bytes.Buffer
	tp := template.Must(template.New(``).Option("missingkey=error").Parse(tp_s))
	r.err = tp.Execute(&buf, r.uniq)
	r.sql = buf.String()
	return
}

func (o *Conn) Close() error {
	o.logout(true)
	return nil
}

func (o *Conn) Begin() (driver.Tx, error) {
	return o.BeginTx(context.Background(), driver.TxOptions{})
}

// msaccess: sql.LevelReadCommitted or sql.LevelDefault (usually none)
//
func (o *Conn) BeginTx(ctx context.Context, opts driver.TxOptions) (tx driver.Tx, err error) {
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		// if err = o.drop(); err != nil {
		// 	return
		// }
		if opts.ReadOnly {
			err = fmt.Errorf("ReadOnly transaction is not supported")
			return
		}
		var isolation C.odbULONG
		switch o.driver {
		case Msaccess:
			switch sql.IsolationLevel(opts.Isolation) {
			case sql.LevelDefault:
				isolation = C.ODB_TXN_DEFAULT
			case sql.LevelReadUncommitted:
				isolation = C.ODB_TXN_READUNCOMMITTED
			case sql.LevelReadCommitted:
				isolation = C.ODB_TXN_READCOMMITTED
			case sql.LevelRepeatableRead:
				isolation = C.ODB_TXN_REPEATABLEREAD
			case sql.LevelSerializable:
				isolation = C.ODB_TXN_SERIALIZABLE
			default:
				err = fmt.Errorf("unsupported isolation level: %v", opts.Isolation)
				return
			}
		case Foxpro:
			err = fmt.Errorf("unsupported isolation level: %v", opts.Isolation)
			return
		default:
			err = fmt.Errorf("unkown odbc driver and isolation level")
			return
		}
		if !dB2b(C.odbSetAttrLong(o.h, C.ODB_ATTR_TRANSACTIONS, isolation)) {
			err = oe2err(o.h)
			return
		}
		tx = o
	}()
	select {
	case <-ctx.Done():
		o.logout(true)
		return nil, ctx.Err()
	case <-done:
	}
	return
}

func (o *Conn) Commit() (err error) {
	if !dB2b(C.odbCommit(o.h)) {
		return oe2err(o.h)
	}
	return nil
}

func (o *Conn) Rollback() (err error) {
	if !dB2b(C.odbRollback(o.h)) {
		return oe2err(o.h)
	}
	return nil
}

func (o *Stmt) Close() error {
	return nil
}

func (o *Stmt) NumInput() int {
	if o.ns != nil && 0 < len(o.ns.uniq) {
		return len(o.ns.uniq)
	}
	return int(C.odbGetTotalParams(o.h))
}

func (o *Stmt) LastInsertId() (id int64, err error) {
	if o.con.driver != Msaccess {
		return 0, driver.ErrSkip
	}
	if len(o.identity_table) == 0 {
		return 0, fmt.Errorf("missing Identity_table arg")
	}
	st, err := o.con.prepare(fmt.Sprintf("select @@IDENTITY from [%v]", o.identity_table))
	if err != nil {
		return
	}
	defer st.free()
	if err = st.execute(); err != nil {
		return
	}
	dv := make([]driver.Value, 1)
	if err = st.Next(dv); err != nil {
		return
	}
	var ok bool
	id, ok = dv[0].(int64)
	if !ok {
		return 0, fmt.Errorf("could not convert %v (%T) to int64", dv[0], dv[0])
	}
	return
}

func (o *Stmt) RowsAffected() (int64, error) {
	return 0, driver.ErrSkip
}

// RowsAffected() is not implemented by msaccess.
//
func (o *Stmt) Exec(args []driver.Value) (dr driver.Result, err error) {
	return nil, driver.ErrSkip
}

func (o *Stmt) Query(args []driver.Value) (dr driver.Rows, err error) {
	return nil, driver.ErrSkip
}

type Identity_table string

func (o *Stmt) CheckNamedValue(nv *driver.NamedValue) (err error) {
	switch t := nv.Value.(type) {
	case int:
		nv.Value = int64(t)
	case Identity_table:
		o.identity_table = string(t)
		err = driver.ErrRemoveArgument
	case *Identity_table:
		o.identity_table = string(*t)
		err = driver.ErrRemoveArgument
	default:
	}
	return
}

// Must add an Identity_table arg to retrieve the LastInsertId for msaccess
//
func (o *Stmt) ExecContext(ctx context.Context, args []driver.NamedValue) (dr driver.Result, err error) {
	_, err = o.QueryContext(ctx, args)
	if err == nil {
		dr = o
	}
	return
}

func (o *Stmt) QueryContext(ctx context.Context, args []driver.NamedValue) (dr driver.Rows, err error) {
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		var col C.odbUSHORT
		var last C.odbBOOL
		o.bound = false
		if !o.bound {
			o.bound = true
			if o.ns != nil && 0 < len(o.ns.uniq) {
				args, err = o.named2pos(args)
				if err != nil {
					return
				}
			}
			for i, na := range args {
				col++
				if (i + 1) == len(args) {
					last = 1
				}
				switch t := na.Value.(type) {
				case []byte, string:
					err = o.bind(col, Wchar, last)
				case int64:
					if o.con.driver == Msaccess {
						err = o.bind(col, Double, last)
					} else {
						err = o.bind(col, Bigint, last)
					}
				case time.Time:
					err = o.bind(col, Datetime, last)
				case bool:
					err = o.bind(col, Bit, last)
				case float64:
					err = o.bind(col, Double, last)
				default:
					err = fmt.Errorf("type unsupported: %T", t)
				}
				if err != nil {
					return
				}
			}
		}
		col = 0
		for i, v := range args {
			col++
			if (i + 1) == len(args) {
				last = 1
			}
			if err = o.set(col, v.Value, last); err != nil {
				return
			}
		}
		if err = o.execute(o.special); err != nil {
			return
		}
		dr = o
		return
	}()
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-done:
	}
	return
}

func (o *Stmt) named2pos(args []driver.NamedValue) (pos []driver.NamedValue, err error) {
	if len(args) != len(o.ns.uniq) {
		return nil, fmt.Errorf("template uniq args != args: %v vs %v", len(o.ns.uniq), len(args))
	}
	n2nv := map[string]*driver.NamedValue{}
	leftover := map[string]bool{}
	for k := range o.ns.uniq {
		leftover[k] = true
	}
	for i, nv := range args {
		if _, ok := o.ns.uniq[nv.Name]; !ok {
			return nil, fmt.Errorf("missing arg Name in template: %v", nv.Name)
		}
		n2nv[nv.Name] = &args[i]
		delete(leftover, nv.Name)
	}
	if len(leftover) != 0 {
		return nil, fmt.Errorf("template args not used: %v", leftover)
	}
	pos = make([]driver.NamedValue, len(o.ns.pos))
	for i := range pos {
		pos[i] = *n2nv[o.ns.pos[i]]
	}
	return
}

func (o *Stmt) Columns() (s []string) {
	total := int(C.odbGetTotalCols(o.h))
	s = make([]string, 0, total)
	for i := 1; i <= total; i++ {
		if r := C.odbColName(o.h, C.odbUSHORT(i)); r != nil {
			s = append(s, C.GoString((*C.char)(unsafe.Pointer(r))))
		} else {
			return
		}
	}
	return
}

func (o *Stmt) Next(dest []driver.Value) (err error) {
	if err = o.fetch_row(); err != nil {
		return err
	}
	if dB2b(C.odbNoData(o.h)) {
		err = io.EOF
		return
	}
	var col C.odbUSHORT
	for i := range dest {
		col++
		byte_len := C.odbColDataLen(o.h, col)
		if byte_len == C.ODB_NULL {
			dest[i] = nil
			continue
		}
		dt := Data(C.odbColDataType(o.h, col))
		if _, ok := odb2sql[o.con.driver][dt]; !ok {
			return fmt.Errorf("unknown Data type: %v", dt)
		}
		switch dt {
		case Char, Wchar:
			// todo: size is limited to C.int (2 GB), but odbColDataText is C.LONG
			// on amd64. Bigger []byte could be delivered
			if len(o.special) == 0 {
				dest[i] = C.GoBytes(unsafe.Pointer(C.odbColDataText(o.h, col)), (C.int)(byte_len))
			} else {
				// This is need for SQLGetTypeInfo. The column size returns 4G but the data is
				// null terminated and is not ODB_NULL. Some of the fields have zero bytes.
				dest[i] = C.GoString((*C.char)(unsafe.Pointer(C.odbColDataText(o.h, col))))
			}
		case Int:
			dest[i] = int64(C.odbColDataLong(o.h, col))
		case Bigint:
			dest[i] = int64(C.odbColDataLongLong(o.h, col))
		case Datetime:
			ts := C.odbColDataTimestamp(o.h, col)
			dest[i] = time.Date(
				int(ts.sYear),
				time.Month(ts.usMonth),
				int(ts.usDay),
				int(ts.usHour),
				int(ts.usMinute),
				int(ts.usSecond),
				int(ts.ulFraction),
				time.Local,
			)
		case Double:
			dest[i] = float64(C.odbColDataDouble(o.h, col))
		case Bit:
			if C.odbColDataByte(o.h, col) == 0 {
				dest[i] = false
			} else {
				dest[i] = true
			}
		case Smallint:
			dest[i] = int64(C.odbColDataShort(o.h, col))
		default:
			return fmt.Errorf("invalid type: column: %v, %v", i+1, dt)
		}
	}
	return nil
}
