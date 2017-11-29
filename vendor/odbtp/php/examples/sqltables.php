<html>
<head>
<title>SQLTables</title>
</head>
<body>
<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;

    odbtp_use_row_cache() or die;

    // Call ODBC API catalog function SQLTables to get list of tables.
    $qryTables = odbtp_query( "||SQLTables" ) or die;

    // Detach result so that another query can be executed on
    // the same connection without losing this one.
    odbtp_detach( $qryTables ) or die;

    odbtp_attach_field( $qryTables, 2, $table ) or die;

    echo "<dl>";
    while( odbtp_fetch( $qryTables ) ) {
        // Skip system tables.
        if( !strncmp( $table, 'sys', 3 ) ) continue; // ignore system tables

        echo "<dt>$table<dd>";

        // Call ODBC API catalog function SQLColumns to get
        // table columns.
        $qryColumns = odbtp_query( "||SQLColumns|||$table" ) or die;
        odbtp_attach_field( $qryColumns, 3, $column );
        odbtp_attach_field( $qryColumns, 5, $type );

        while( odbtp_fetch( $qryColumns ) ) {
            echo "$column&nbsp;&nbsp;$type<br>";
        }
        echo "&nbsp;<br></dd>";
    }
    echo "</dl>";

    odbtp_close();
?>
</body>
</html>
