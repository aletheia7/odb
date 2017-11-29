<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;
?>
<html>
<head>
<title>ODBTP Test - Database Listing</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
<?php
    $qry = odbtp_query( "EXEC ListEmployees" ) or die;

    $cols = odbtp_num_fields( $qry );

    echo "<table cellpadding=2 cellspacing=0 border=1>\n";
    echo "<tr>";
    for( $col = 0; $col < $cols; $col++ )
        echo "<th>&nbsp;" . odbtp_field_name( $qry, $col ) . "&nbsp;</th>";
    echo "</tr>\n";

    odbtp_bind_field( $qry, "DateEntered", ODB_CHAR ) or die;
    odbtp_bind_field( $qry, "DateModified", ODB_CHAR ) or die;

    while( ($rec = odbtp_fetch_array($qry)) ) {
        echo "<tr>";
        for( $col = 0; $col < $cols; $col++ )
            echo "<td><nobr>&nbsp;$rec[$col]&nbsp;</nobr></td>";
        echo "</tr>\n";
    }
    echo "</table>\n";

    odbtp_close();
?>
</body>
</html>
