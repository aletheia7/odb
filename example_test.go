package odb_test

import (
	"database/sql"
	"fmt"

	"github.com/aletheia7/odb"
)

func ExampleRegister() {
	// Returns the registered driver name to use in sql.Open(). The driver name pattern is
	// odbtp_msaccess_1, odbtp_msaccess_...
	driver_name := odb.Register(
		"<host_name>",
		odb.Normal,
		`DRIVER=Microsoft Access Driver (*.mdb);DBQ=c:/<file path to mdb>;ImplicitCommitSync=Yes`,
		odb.Int_opt(odb.Query_timeout, 20),
		odb.Bool_opt(odb.Unicodesql, true),
		odb.Bool_opt(odb.Describe_params, true),
		odb.Bool_opt(odb.Mapchar2wchar, true),
		odb.Bool_opt(odb.Prepare_is_template, true),
	)
	db, err := sql.Open(driver_name, ``)
	if err != nil {
		fmt.Print(err)
		return
	}
	defer db.Close()
}
