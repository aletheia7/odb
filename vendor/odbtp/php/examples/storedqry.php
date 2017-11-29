<html>
<head>
<title>MS Access Stored Query</title>
</head>
<body>
<?php
    $dbc = 'DRIVER={Microsoft Access Driver (*.mdb)};DBQ=c:\NorthWind.mdb;UID=admin;PWD=;';
    $con = odbtp_connect( 'localhost', $dbc ) or die;

    $qry = odbtp_prepare_proc( "Employee Sales by Country" ) or die;

    odbtp_attach_param( $qry, "[Beginning Date]", $BegDate ) or die;
    odbtp_attach_param( $qry, "[Ending Date]", $EndDate ) or die;

    $BegDate = odbtp_new_datetime();
    $EndDate = odbtp_new_datetime();

    $BegDate->year = 1996;
    $BegDate->month = 1;
    $BegDate->day = 1;

    $EndDate->year = 1996;
    $EndDate->month = 12;
    $EndDate->day = 31;

    odbtp_execute( $qry ) or die;

    $cols = odbtp_num_fields( $qry ) or die( 'UGH' );

    echo "<table cellpadding=2 cellspacing=0 border=1>\n";
    echo "<tr>";
    while( ($f = odbtp_fetch_field( $qry )) ) {
        echo "<td><nobr>&nbsp;" . $f->name . " (" . $f->type . ")&nbsp;</nobr></td>";
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

    $BegDate->year = 1997;
    $BegDate->month = 1;
    $BegDate->day = 1;

    $EndDate->year = 1997;
    $EndDate->month = 12;
    $EndDate->day = 31;

    odbtp_execute( $qry ) or die;

    $cols = odbtp_num_fields( $qry ) or die( 'UGH' );

    echo "<table cellpadding=2 cellspacing=0 border=1>\n";
    echo "<tr>";
    while( ($f = odbtp_fetch_field( $qry )) ) {
        echo "<td><nobr>&nbsp;" . $f->name . " (" . $f->type . ")&nbsp;</nobr></td>";
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

    odbtp_close();
?>
</body>
</html>
