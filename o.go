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
	"strconv"
	"strings"
	"sync/atomic"
	"text/template"
	"text/template/parse"
	"time"
	"unsafe"
)

type driver_odbc string

const (
	Default_port = 2799

	// Queries with this prefix will be executed and not prepared
	// The prefix is removed
	Execute_prefix = `odbexecute`

	// Special odbtp query. Use in QueryContext().
	// https://docs.microsoft.com/en-us/sql/odbc/reference/develop-app/catalog-functions-in-odbc
	Sql_get_type_info = Execute_prefix + "||SQLGetTypeInfo"
	Sql_tables        = Execute_prefix + "||SQLTables|||"
	Sql_columns       = Execute_prefix + "||SQLColumns|||" // must append "<table>", and optional "|column"
)

type Login C.odbUSHORT

const (
	Normal   Login = C.ODB_LOGIN_NORMAL
	Reserved       = C.ODB_LOGIN_RESERVED
	Single         = C.ODB_LOGIN_SINGLE
)

type Conn struct {
	driver              driver_odbc
	h                   C.odbHANDLE
	prepare_is_template bool
	zero_scan           bool
	debug               io.Writer
	identity_table      string
}

func (o *Conn) Prepare(query string) (ds driver.Stmt, err error) {
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.Prepare:")
	}
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
		var st *Stmt
		st, err = o.prepare(query)
		if err != nil {
			return
		}
		ds = st
		if o.debug != nil {
			fmt.Fprintf(o.debug, "Conn.PrepareContext:")
		}
		return
	}()
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-done:
	}
	return
}

func (o *Conn) Ping(ctx context.Context) (err error) {
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.Ping:")
	}
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

func (o *Conn) Close() error {
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.Close:")
	}
	o.logout(true)
	return nil
}

func (o *Conn) Begin() (driver.Tx, error) {
	return o.BeginTx(context.Background(), driver.TxOptions{})
}

// msaccess: sql.LevelReadCommitted or sql.LevelDefault (usually none)
func (o *Conn) BeginTx(ctx context.Context, opts driver.TxOptions) (tx driver.Tx, err error) {
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.BeginTx:")
	}
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		if opts.ReadOnly {
			err = fmt.Errorf("ReadOnly transaction is not supported")
			return
		}
		var isolation C.odbULONG
		switch o.driver {
		case msaccess, mssql:
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
		case foxpro:
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
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.Commit:")
	}
	if !dB2b(C.odbCommit(o.h)) {
		return oe2err(o.h)
	}
	return
}

func (o *Conn) Rollback() (err error) {
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.Rollback:")
	}
	if !dB2b(C.odbRollback(o.h)) {
		return oe2err(o.h)
	}
	return
}

func (o *Conn) Exec(args []driver.Value) (dr driver.Result, err error) {
	return nil, driver.ErrSkip
}

func (o *Conn) Query(args []driver.Value) (dr driver.Rows, err error) {
	return nil, driver.ErrSkip
}

func (o *Conn) CheckNamedValue(nv *driver.NamedValue) (err error) {
	switch t := nv.Value.(type) {
	case *int64, *float64, *bool, *[]byte, *string, *time.Time: // These are used for nil
	case Identity_table:
		o.identity_table = string(t)
		err = driver.ErrRemoveArgument
	case *Identity_table:
		o.identity_table = string(*t)
		err = driver.ErrRemoveArgument
	case int:
		if o.driver == msaccess {
			nv.Value = float64(t) // msaccess does not have long/int64
		} else {
			nv.Value, err = driver.DefaultParameterConverter.ConvertValue(nv.Value)
		}
	case int64:
		if o.driver == msaccess {
			nv.Value = float64(t) // msaccess does not have long/int64
		} else {
			nv.Value, err = driver.DefaultParameterConverter.ConvertValue(nv.Value)
		}
	default:
		nv.Value, err = driver.DefaultParameterConverter.ConvertValue(nv.Value)
	}
	return
}

type stresult struct {
	id      int64
	id_err  error
	row     int64
	row_err error
}

func (o *stresult) LastInsertId() (int64, error) {
	return o.id, o.id_err
}

func (o *stresult) RowsAffected() (int64, error) {
	return o.row, o.row_err
}

// msaccess: Must add an Identity_table arg to call LastInsertId()
func (o *Conn) ExecContext(ctx context.Context, query string, args []driver.NamedValue) (dr driver.Result, err error) {
	if o.debug != nil {
		fmt.Fprintln(o.debug, "Conn.ExecContext:")
	}
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		var st *Stmt
		st, err = o.prepare(query)
		if err != nil {
			return
		}
		st.identity_table = o.identity_table
		_, err = st.QueryContext(ctx, args)
		if err != nil {
			return
		}
		r := &stresult{}
		r.id, r.id_err = st.LastInsertId()
		r.row, r.row_err = st.RowsAffected()
		dr = r
		st.Close()
	}()
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-done:
	}
	return
}

func (o *Conn) QueryContext(ctx context.Context, query string, args []driver.NamedValue) (dr driver.Rows, err error) {
	if o.debug != nil {
		// q := shrink.ReplaceAllLiteralString(strings.Map(func(r rune) rune {
		// 	switch r {
		// 	case '\t':
		// 		return ' '
		// 	case '\n':
		// 		return -1
		// 	default:
		// 		return r
		// 	}
		// }, query), ` `)
		fmt.Fprintln(o.debug, "Conn.QueryContext:")
	}
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		var st *Stmt
		st, err = o.prepare(query)
		if err != nil {
			return
		}
		st.identity_table = o.identity_table
		dr, err = st.QueryContext(ctx, args)
	}()
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	case <-done:
	}
	return
}

// host example: pine | pine:2799
func new_con(host string, login Login, dsn string) (*Conn, error) {
	if !strings.Contains(host, ":") {
		host += ":" + strconv.Itoa(Default_port)
	}
	tcp, err := net.ResolveTCPAddr("tcp", host)
	if err != nil {
		return nil, err
	}
	r := &Conn{}
	switch {
	case strings.HasPrefix(dsn, string(msaccess)):
		r.driver = msaccess
	case strings.HasPrefix(dsn, string(foxpro)):
		r.driver = foxpro
	case strings.HasPrefix(dsn, string(mssql)):
		r.driver = mssql
	default:
		return nil, fmt.Errorf("unknown driver", dsn)
	}
	r.h = C.odbAllocate(nil)
	if r.h == nil {
		return nil, oe2err(r.h)
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

func (o *Conn) prepare(query string) (st *Stmt, err error) {
	r := &Stmt{
		con:      o,
		bindp:    map[C.odbUSHORT]data{},
		prepared: true,
	}
	r.h = C.odbAllocate(o.h)
	if r.h == nil {
		err = oe2err(o.h)
		if o.debug != nil {
			fmt.Fprintln(o.debug, "Conn.prepare:", oe2err)
		}
		return nil, err
	}
	if strings.HasPrefix(query, Execute_prefix) {
		r.prepared = false
		r.ns = new_named_sql(query[len(Execute_prefix):])
		if r.ns.err != nil {
			err = r.ns.err
			r.Close()
			return
		}
		st = r
		return
	}
	if o.prepare_is_template {
		r.ns = new_named_sql(query)
		if r.ns.err != nil {
			err = r.ns.err
			r.Close()
			return
		}
		query = r.ns.sql
	}
	s := new_s(query)
	defer s.free()
	if !dB2b(C.odbPrepare(r.h, s.p)) {
		err := oe2err(r.h)
		r.Close()
		return nil, err
	}
	return r, nil
}

type Stmt struct {
	con            *Conn
	h              C.odbHANDLE
	bindp          map[C.odbUSHORT]data
	fetch_err      error
	ns             *named_sql
	prepared       bool
	identity_table string // only msaccess
}

func (o *Stmt) execute() error {
	o.fetch_err = nil
	r := false
	if o.prepared {
		r = dB2b(C.odbExecute(o.h, C.odbPCSTR(nil)))
	} else {
		s := new_s(o.ns.sql)
		defer s.free()
		r = dB2b(C.odbExecute(o.h, s.p))
	}
	if !r {
		return oe2err(o.h)
	}
	return nil
}

// col: 1 based
func (o *Stmt) bind(col C.odbUSHORT, data data) (err error) {
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
		0,
	)) {
		return oe2err(o.h)
	}
	return
}

// col: 1 based
func (o *Stmt) set(col C.odbUSHORT, i interface{}) (err error) {
	switch v := i.(type) {
	case *[]byte:
		if v == nil {
			if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
				return oe2err(o.h)
			}
		} else {
			s := new_b(*v)
			defer s.free()
			if !dB2b(C.odbSetParam(o.h, col, (C.odbPVOID)(s.p), C.odbLONG(len(*v)), 0)) {
				return oe2err(o.h)
			}
		}
	case []byte:
		s := new_b(v)
		defer s.free()
		if !dB2b(C.odbSetParam(o.h, col, (C.odbPVOID)(s.p), C.odbLONG(len(v)), 0)) {
			return oe2err(o.h)
		}
	case *string:
		if v == nil {
			if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
				return oe2err(o.h)
			}
		} else {
			s := new_s(*v)
			defer s.free()
			if !dB2b(C.odbSetParam(o.h, col, (C.odbPVOID)(s.p), C.odbLONG(len(*v)), 0)) {
				return oe2err(o.h)
			}
		}
	case string:
		s := new_s(v)
		defer s.free()
		if !dB2b(C.odbSetParam(o.h, col, (C.odbPVOID)(s.p), C.odbLONG(len(v)), 0)) {
			return oe2err(o.h)
		}
	case *time.Time:
		if v == nil {
			if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
				return oe2err(o.h)
			}
		} else {
			var ts C.odbTIMESTAMP
			ts.sYear = C.odbSHORT(v.Year())
			ts.usMonth = C.odbUSHORT(v.Month())
			ts.usDay = C.odbUSHORT(v.Day())
			ts.usHour = C.odbUSHORT(v.Hour())
			ts.usMinute = C.odbUSHORT(v.Minute())
			ts.usSecond = C.odbUSHORT(v.Second())
			ts.ulFraction = C.odbULONG(v.Nanosecond())
			if !dB2b(C.odbSetParamTimestamp(o.h, col, &ts, 0)) {
				return oe2err(o.h)
			}
		}
	case time.Time:
		var ts C.odbTIMESTAMP
		ts.sYear = C.odbSHORT(v.Year())
		ts.usMonth = C.odbUSHORT(v.Month())
		ts.usDay = C.odbUSHORT(v.Day())
		ts.usHour = C.odbUSHORT(v.Hour())
		ts.usMinute = C.odbUSHORT(v.Minute())
		ts.usSecond = C.odbUSHORT(v.Second())
		ts.ulFraction = C.odbULONG(v.Nanosecond())
		if !dB2b(C.odbSetParamTimestamp(o.h, col, &ts, 0)) {
			return oe2err(o.h)
		}
	case nil:
		if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
			return oe2err(o.h)
		}
	case *int64:
		if v == nil {
			if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
				return oe2err(o.h)
			}
		} else {
			if o.con.driver == msaccess {
				if !dB2b(C.odbSetParamDouble(o.h, col, C.odbDOUBLE(float64(*v)), 0)) {
					return oe2err(o.h)
				}
			} else {
				if !dB2b(C.odbSetParamLongLong(o.h, col, C.odbULONGLONG(*v), 0)) {
					return oe2err(o.h)
				}
			}
		}
	case int64:
		if o.con.driver == msaccess {
			if !dB2b(C.odbSetParamDouble(o.h, col, C.odbDOUBLE(float64(v)), 0)) {
				return oe2err(o.h)
			}
		} else {
			if !dB2b(C.odbSetParamLongLong(o.h, col, C.odbULONGLONG(v), 0)) {
				return oe2err(o.h)
			}
		}
	case *float64:
		if v == nil {
			if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
				return oe2err(o.h)
			}
		} else {
			if !dB2b(C.odbSetParamDouble(o.h, col, C.odbDOUBLE(*v), 0)) {
				return oe2err(o.h)
			}
		}
	case float64:
		if !dB2b(C.odbSetParamDouble(o.h, col, C.odbDOUBLE(v), 0)) {
			return oe2err(o.h)
		}
	case *bool:
		if v == nil {
			if !dB2b(C.odbSetParamNull(o.h, col, 0)) {
				return oe2err(o.h)
			}
		} else {
			var b C.odbBYTE
			if *v == true {
				b = C.odbBYTE(1)
			}
			if !dB2b(C.odbSetParamByte(o.h, col, b, 0)) {
				return oe2err(o.h)
			}
		}
	case bool:
		var b C.odbBYTE
		if v == true {
			b = C.odbBYTE(1)
		}
		if !dB2b(C.odbSetParamByte(o.h, col, b, 0)) {
			return oe2err(o.h)
		}
	default:
		return fmt.Errorf("invalid type: parama: %v, %T", col, v)
	}
	return
}

type Rows struct {
	*Stmt
}

func (o *Rows) Close() error {
	if o.con.debug != nil {
		fmt.Fprintln(o.con.debug, "Rows.Close:")
	}
	return o.Stmt.Close()
}

// Driver name
const Driver_msaccess = "odbtp_msaccess"

var driver_name_ct uint32

// Returns the registered driver name to use in sql.Open(). The driver name
// pattern is odbtp_msaccess_1, odbtp_msaccess_2, odbtp_msaccess_...
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
	// Example in query: {{.id}} becomes sql.Named("id", <value>)
	Prepare_is_template bool_option = 1 << iota * -1 // ODB_ATTR are positive

	// Zero_scan will cause Stmt.Scan() to return go zero values in place of nil
	// for database null
	Zero_scan

	// http://odbtp.sourceforge.net/clilib.html#attributes
	Cache_procs     = C.ODB_ATTR_CACHEPROCS
	Mapchar2wchar   = C.ODB_ATTR_MAPCHARTOWCHAR
	Describe_params = C.ODB_ATTR_DESCRIBEPARAMS
	Unicodesql      = C.ODB_ATTR_UNICODESQL
	Right_trim_text = C.ODB_ATTR_RIGHTTRIMTEXT
)

func Bool_opt(opt bool_option, enable bool) option {
	return func(o *Conn) error {
		switch opt {
		case Prepare_is_template:
			o.prepare_is_template = enable
		case Zero_scan:
			o.zero_scan = enable
		default:
			return o.set_attr_bool(opt, enable)
		}
		return nil
	}
}

func Debug(w io.Writer) option {
	return func(o *Conn) error {
		o.debug = w
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
	if con.debug != nil {
		fmt.Fprintln(con.debug, "Driver.Open:")
	}
	return con, nil
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

func (o *Stmt) Close() error {
	if o.con.debug != nil {
		fmt.Fprintln(o.con.debug, "Stmt.Close:")
	}
	if o.h != nil {
		C.odbDropQry(o.h)
		C.odbFree(o.h)
		o.h = nil
		o.bindp = map[C.odbUSHORT]data{}
	}
	return nil
}

func (o *Stmt) NumInput() int {
	if o.ns != nil && 0 < len(o.ns.uniq) {
		return len(o.ns.uniq)
	}
	return int(C.odbGetTotalParams(o.h))
}

// Only usefull with msaccess
func (o *Stmt) LastInsertId() (id int64, err error) {
	if o.con.driver != msaccess {
		return 0, driver.ErrSkip
	}
	if len(o.identity_table) == 0 {
		return 0, fmt.Errorf("missing Identity_table arg")
	}
	st, err := o.con.prepare(fmt.Sprintf("select @@IDENTITY from [%v]", o.identity_table))
	if err != nil {
		return
	}
	defer st.Close()
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

// Not implemented
func (o *Stmt) RowsAffected() (int64, error) {
	return 0, driver.ErrSkip
}

func (o *Stmt) Exec(args []driver.Value) (dr driver.Result, err error) {
	return nil, driver.ErrSkip
}

func (o *Stmt) Query(args []driver.Value) (dr driver.Rows, err error) {
	return nil, driver.ErrSkip
}

// Used with msaccess.  See ExecContext()
type Identity_table string

func (o *Stmt) CheckNamedValue(nv *driver.NamedValue) (err error) {
	switch t := nv.Value.(type) {
	case *int64, *float64, *bool, *[]byte, *string, *time.Time: // These are used for nil
	case Identity_table:
		o.identity_table = string(t)
		err = driver.ErrRemoveArgument
	case *Identity_table:
		o.identity_table = string(*t)
		err = driver.ErrRemoveArgument
	case int:
		if o.con.driver == msaccess {
			nv.Value = float64(t) // msaccess does not have long/int64
		} else {
			nv.Value, err = driver.DefaultParameterConverter.ConvertValue(nv.Value)
		}
	case int64:
		if o.con.driver == msaccess {
			nv.Value = float64(t) // msaccess does not have long/int64
		} else {
			nv.Value, err = driver.DefaultParameterConverter.ConvertValue(nv.Value)
		}
	default:
		nv.Value, err = driver.DefaultParameterConverter.ConvertValue(nv.Value)
	}
	return
}

var (
	// string or []byte input parameter to be set to database null
	Ns *string

	// int64 input parameter to be set to database null
	Ni *int64

	// float64 input parameter to be set to database null
	Nf *float64

	// bool input parameter to be set to database null
	Nb *bool

	// time.Time input parameter to be set to database null
	Nt *time.Time
)

// msaccess: Must add an Identity_table arg to call LastInsertId()
func (o *Stmt) ExecContext(ctx context.Context, args []driver.NamedValue) (dr driver.Result, err error) {
	if o.con.debug != nil {
		fmt.Fprintln(o.con.debug, "Stmt.ExecContext:")
	}
	_, err = o.QueryContext(ctx, args)
	if err == nil {
		dr = o
	}
	return
}

func (o *Stmt) QueryContext(ctx context.Context, args []driver.NamedValue) (dr driver.Rows, err error) {
	if o.con.debug != nil {
		fmt.Fprintln(o.con.debug, "Stmt.QueryContext:")
	}
	select {
	case <-ctx.Done():
		return nil, ctx.Err()
	default:
	}
	done := make(chan struct{})
	go func() {
		defer close(done)
		var col C.odbUSHORT
		if o.ns != nil && 0 < len(o.ns.uniq) {
			args, err = o.named2pos(args)
			if err != nil {
				return
			}
		}
		for _, na := range args {
			col++
			switch t := na.Value.(type) {
			case string, *string, []byte, *[]byte:
				err = o.bind(col, owchar)
			case int64, *int64:
				err = o.bind(col, obigint)
			case time.Time, *time.Time:
				err = o.bind(col, odatetime)
			case bool, *bool:
				err = o.bind(col, obit)
			case float64, *float64:
				err = o.bind(col, odouble)
			default:
				err = fmt.Errorf("type unsupported: %T %v %v %v", t, na.Ordinal, na.Name, na.Value)
			}
			if err != nil {
				return
			}
		}
		if 0 < col && !dB2b(C.odbFinalizeRequest(o.h)) {
			err = oe2err(o.h)
			return
		}
		col = 0
		for _, v := range args {
			col++
			if err = o.set(col, v.Value); err != nil {
				return
			}
		}
		if 0 < col && !dB2b(C.odbFinalizeRequest(o.h)) {
			err = oe2err(o.h)
			return
		}
		if err = o.execute(); err != nil {
			return
		}
		dr = &Rows{o}
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
	if !dB2b(C.odbFetchRow(o.h)) {
		return oe2err(o.h)
	}
	if dB2b(C.odbNoData(o.h)) {
		err = io.EOF
		return
	}
	col := C.odbUSHORT(0)
	is_nil := false
	for i := range dest {
		col++
		is_nil = false
		byte_len := int32(C.odbColDataLen(o.h, col))
		if byte_len == C.ODB_NULL {
			is_nil = true
		}
		if is_nil && !o.con.zero_scan {
			dest[i] = nil
			continue
		}
		dt := data(C.odbColDataType(o.h, col))
		if _, ok := odb2sql[o.con.driver][dt]; !ok {
			return fmt.Errorf("unknown Data type: %v, col: %v", dt, col)
		}
		switch dt {
		case ochar, owchar:
			if is_nil && o.con.zero_scan {
				dest[i] = []byte{}
			} else {
				dest[i] = C.GoBytes(unsafe.Pointer(C.odbColDataText(o.h, col)), (C.int)(byte_len))
			}
		case oint:
			if is_nil && o.con.zero_scan {
				dest[i] = int64(0)
			} else {
				dest[i] = int64(C.odbColDataLong(o.h, col))
			}
		case obigint:
			if is_nil && o.con.zero_scan {
				dest[i] = int64(0)
			} else {
				dest[i] = int64(C.odbColDataLongLong(o.h, col))
			}
		case odatetime:
			if is_nil && o.con.zero_scan {
				dest[i] = time.Time{}
			} else {
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
			}
		case odouble:
			if is_nil && o.con.zero_scan {
				dest[i] = float64(0)
			} else {
				dest[i] = float64(C.odbColDataDouble(o.h, col))
			}
		case oreal:
			if is_nil && o.con.zero_scan {
				dest[i] = float64(0)
			} else {
				dest[i] = float64(C.odbColDataFloat(o.h, col))
			}
		case obit:
			if is_nil && o.con.zero_scan {
				dest[i] = false
			} else {
				if C.odbColDataByte(o.h, col) == 0 {
					dest[i] = false
				} else {
					dest[i] = true
				}
			}
		case osmallint:
			dest[i] = int64(C.odbColDataShort(o.h, col))
		default:
			return fmt.Errorf("invalid type: column: %v, %v", i+1, dt)
		}
	}
	return
}

// Convert a Foxpro Memo string to a map
// func S2memo(s string) (r map[string]string) {
// 	r = map[string]string{}
// 	if 0 < len(s) {
// 		for _, kv := range strings.Split(s, "\r") {
// 			a := strings.Split(kv, "=")
// 			if len(a) == 2 && 0 < len(a[0]) && 0 < len(a[1]) {
// 				r[a[0]] = a[1]
// 			}
// 		}
// 	}
// 	return
// }

// Convert a map to Foxpro Memo string
// func Memo2s(m map[string]string) string {
// 	keys := make([]string, len(m))
// 	i := 0
// 	for k := range m {
// 		keys[i] = k
// 		i++
// 	}
// 	sort.Strings(keys)
// 	a := make([]string, 0, len(m))
// 	for _, k := range keys {
// 		a = append(a, k+"="+m[k])
// 	}
// 	return strings.Join(a, "\r")
// }

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

type data int16

func (o data) String() string {
	s, ok := data2s[o]
	if ok {
		return s
	}
	return fmt.Sprintf("invalid ODB data type: %d", o)
}

var data2s = map[data]string{
	obinary:   "Binary",
	owchar:    "Wchar",
	osmallint: "Smallint",
	oint:      "Int",
	obigint:   "Bigint",
	obit:      "Bit",
	ochar:     "Char",
	odouble:   "Double",
	oreal:     "Real",
	odatetime: "Datetime",
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

func (o *data) short() C.odbSHORT {
	return C.odbSHORT(*o)
}

const (
	obinary   data = C.ODB_BINARY   // (-2) 65534
	owchar         = C.ODB_WCHAR    // (-8) Use for unicode
	osmallint      = C.ODB_SMALLINT // (-15)
	oint           = C.ODB_INT      // (-16)
	obigint        = C.ODB_BIGINT   // (-25)
	obit           = C.ODB_BIT      // (-7)
	ochar          = C.ODB_CHAR     // 1
	oreal          = C.ODB_REAL     // 7
	odouble        = C.ODB_DOUBLE   // 8 same as C.ODB_FLOAT
	odatetime      = C.ODB_DATETIME // 93
	// Guid           = C.ODB_GUID      // (-11)
	// Usmallint      = C.ODB_USMALLINT // (-17)
	// Uint           = C.ODB_UINT      // (-18)
	// Tinyint        = C.ODB_TINYINT   // (-26)
	// Ubigint        = C.ODB_UBIGINT   // (-27)
	// Utinyint       = C.ODB_UTINYINT  // (-28)
	// Numeric        = C.ODB_NUMERIC   // 2
	// Date           = C.ODB_DATE      // 91
	// Time           = C.ODB_TIME      // 92
)

// Used with Sql_get_type_info query DATA_TYPE column
func Get_sql_type(i int16) string {
	return sql_type(i).String()
}

type sql_type int16

func (o sql_type) String() string {
	s, ok := sql_type2s[o]
	if ok {
		return s
	}
	return fmt.Sprintf("invalid SQL data type: %v", o)
}

// duplicates are commented-out
var sql_type2s = map[sql_type]string{
	sql_bit:      "SQL_BIT ",
	sql_tinyint:  "SQL_TINYINT",
	sql_smallint: "SQL_SMALLINT",
	// sql_integer:        "SQL_INTEGER",
	sql_int:       "SQL_INT",
	sql_bitint:    "SQL_BIGINT",
	sql_numeric:   "SQL_NUMERIC",
	sql_real:      "SQL_REAL",
	sql_float:     "SQL_FLOAT",
	sql_double:    "SQL_DOUBLE",
	sql_decimal:   "SQL_DECIMAL",
	sql_date:      "SQL_DATE",
	sql_time:      "SQL_TIME",
	sql_timestamp: "SQL_TIMESTAMP",
	sql_type_date: "SQL_TYPE_DATE",
	sql_type_time: "SQL_TYPE_TIME",
	// sql_type_timestamp: "SQL_TYPE_TIMESTAMP",
	sql_datetime: "SQL_DATETIME",
	sql_char:     "SQL_CHAR",
	sql_varchar:  "SQL_VARCHAR",
	// sql_longvarchar:   "SQL_LONGVARCHAR",
	sql_text:  "SQL_TEXT",
	sql_wchar: "SQL_WCHAR",
	// sql_nchar:         "SQL_NCHAR",
	sql_wvarchar: "SQL_WVARCHAR",
	// sql_nvarchar:      "SQL_NVARCHAR",
	// sql_wlongvarchar: "SQL_WLONGVARCHAR",
	sql_ntext:         "SQL_NTEXT",
	sql_binary:        "SQL_BINARY",
	sql_varbinary:     "SQL_VARBINARY",
	sql_longvarbinary: "SQL_LONGVARBINARY",
	// sql_image:         "SQL_IMAGE",
	sql_guid:    "SQL_GUID",
	sql_variant: "SQL_VARIANT",
}

const (
	sql_bit            sql_type = C.SQL_BIT            // (-7)
	sql_tinyint                 = C.SQL_TINYINT        // (-6)
	sql_smallint                = C.SQL_SMALLINT       // 5
	sql_integer                 = C.SQL_INTEGER        // 4
	sql_int                     = C.SQL_INT            // 4
	sql_bitint                  = C.SQL_BIGINT         // (-5)
	sql_numeric                 = C.SQL_NUMERIC        // 2
	sql_real                    = C.SQL_REAL           // 7
	sql_float                   = C.SQL_FLOAT          // 6
	sql_double                  = C.SQL_DOUBLE         // 8
	sql_decimal                 = C.SQL_DECIMAL        // 3
	sql_date                    = C.SQL_DATE           // 9
	sql_time                    = C.SQL_TIME           // 10
	sql_timestamp               = C.SQL_TIMESTAMP      // 11
	sql_type_date               = C.SQL_TYPE_DATE      // 91
	sql_type_time               = C.SQL_TYPE_TIME      // 92
	sql_type_timestamp          = C.SQL_TYPE_TIMESTAMP // 93
	sql_datetime                = C.SQL_DATETIME       // 93
	sql_char                    = C.SQL_CHAR           // 1
	sql_varchar                 = C.SQL_VARCHAR        // 12
	sql_longvarchar             = C.SQL_LONGVARCHAR    // (-1)
	sql_text                    = C.SQL_TEXT           // (-1)
	sql_wchar                   = C.SQL_WCHAR          // (-8)
	sql_nchar                   = C.SQL_NCHAR          // (-8)
	sql_wvarchar                = C.SQL_WVARCHAR       // (-9)
	sql_nvarchar                = C.SQL_NVARCHAR       // (-9)
	sql_wlongvarchar            = C.SQL_WLONGVARCHAR   // (-10)
	sql_ntext                   = C.SQL_NTEXT          // (-10)
	sql_binary                  = C.SQL_BINARY         // (-2)
	sql_varbinary               = C.SQL_VARBINARY      // (-3)
	sql_longvarbinary           = C.SQL_LONGVARBINARY  // (-4)
	sql_image                   = C.SQL_IMAGE          // (-4)
	sql_guid                    = C.SQL_GUID           // (-11)
	sql_variant                 = C.SQL_VARIANT        // (-150)
)

type desc struct {
	sql_type   C.odbSHORT
	col_size   C.odbULONG
	dec_digits C.odbSHORT
}

var odb2sql = map[driver_odbc]map[data]desc{
	msaccess: {
		obinary:   desc{C.SQL_BINARY, 255, 0},
		owchar:    desc{C.SQL_NTEXT, 1073741823, 0}, // memo
		osmallint: desc{C.SQL_SMALLINT, 5, 0},
		oint:      desc{C.SQL_INT, 10, 0},
		obigint:   desc{C.SQL_DOUBLE, 53, 0},
		// Bigint:   desc{C.SQL_BIGINT, 19, 0}, // missing
		obit:      desc{C.SQL_BIT, 1, 0},
		ochar:     desc{C.SQL_CHAR, 255, 0},
		odouble:   desc{C.SQL_DOUBLE, 53, 0},
		oreal:     desc{C.SQL_REAL, 24, 0},
		odatetime: desc{C.SQL_DATETIME, 23, 3},
	},
	mssql: {
		obinary:   desc{C.SQL_BINARY, 8000, 0},
		owchar:    desc{C.SQL_NVARCHAR, 4000, 0}, // memo
		osmallint: desc{C.SQL_SMALLINT, 5, 0},
		oint:      desc{C.SQL_INT, 10, 0},
		obigint:   desc{C.SQL_DOUBLE, 53, 0},
		// Bigint:   desc{C.SQL_BIGINT, 19, 0}, // missing
		obit:      desc{C.SQL_BIT, 1, 0},
		ochar:     desc{C.SQL_VARCHAR, 8000, 0},
		odouble:   desc{C.SQL_DOUBLE, 53, 0},
		oreal:     desc{C.SQL_REAL, 24, 0},
		odatetime: desc{C.SQL_DATETIME, 23, 3},
	},
	foxpro: {
		obinary:   desc{C.SQL_BINARY, 8000, 0},
		owchar:    desc{C.SQL_NVARCHAR, 4000, 0}, // memo
		osmallint: desc{C.SQL_SMALLINT, 5, 0},
		oint:      desc{C.SQL_INT, 10, 0},
		obigint:   desc{C.SQL_DOUBLE, 53, 0},
		// Bigint:   desc{C.SQL_BIGINT, 19, 0}, // missing
		obit:      desc{C.SQL_BIT, 1, 0},
		ochar:     desc{C.SQL_VARCHAR, 8000, 0},
		odouble:   desc{C.SQL_DOUBLE, 53, 0},
		oreal:     desc{C.SQL_REAL, 24, 0},
		odatetime: desc{C.SQL_DATETIME, 23, 3},
	},
	// foxpro: { // https://msdn.microsoft.com/en-us/library/z3y7feks(v=vs.80).aspx TYPE()
	// 	ochar:     "C",
	// 	odatatime: "T",
	// 	oint:      "N",
	// 	odouble:   "N",
	// 	obit:      "L",
	// 	osmallint: "N",
	// 	obinary:   "W",
	// 	// Binary_msaccess_memo: "M",
	// },
}

const (
	msaccess driver_odbc = `DRIVER=Microsoft Access Driver (*.mdb)`
	foxpro               = `DRIVER={Microsoft Visual FoxPro Driver}`
	mssql                = `DRIVER={SQL Server}`
)
