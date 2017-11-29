<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;
?>
<html>
<head>
<title>ODBTP Database Query Results</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
<?php
    odbtp_set_attr( ODB_ATTR_FULLCOLINFO, TRUE );

    $query = stripslashes($_REQUEST['query']);
    echo "Query: " . $query . "<p>";
    $qry = odbtp_query( $query ) or die;

    do {
        if( ($msg = odbtp_get_message( $qry )) ) {
            echo "MESSAGE: $msg<p>";
            continue;
        }
        if( ($cols = odbtp_num_fields( $qry )) == 0 ) {
            echo odbtp_affected_rows( $qry );
            echo " rows affected<p>\n";
            continue;
        }
        echo "<table cellpadding=2 cellspacing=0 border=1>\n";
        echo "<tr>";
        for( $col = 0; $col < $cols; $col++ ) {
            echo "<td><nobr>&nbsp;" . odbtp_field_name( $qry, $col );
            echo " (" . odbtp_field_type( $qry, $col ) . ")&nbsp;</nobr></td>";
            if( odbtp_field_bindtype( $qry, $col ) == ODB_DATETIME )
                odbtp_bind_field( $qry, $col, ODB_CHAR );
        }
        echo "</tr>\n";

        while( ($rec = odbtp_fetch_array($qry)) ) {
            echo "<tr>";
            for( $col = 0; $col < $cols; $col++ ) {
                if( is_null( $rec[$col] ) ) $rec[$col] = "NULL";
                echo "<td><nobr>&nbsp;$rec[$col]&nbsp;</nobr></td>";
            }
            echo "</tr>\n";
        }
        echo "</table><p>\n";

        echo odbtp_affected_rows( $qry );
        echo " rows affected<p>\n";
    }
    while( odbtp_next_result( $qry ) );

    odbtp_close();
?>
</body>
</html>
