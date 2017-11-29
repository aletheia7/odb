<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;

    odbtp_set_attr( ODB_ATTR_TRANSACTIONS, ODB_TXN_READCOMMITTED ) or die;

    $qry1 = odbtp_prepare( "{? = call AddEmployee( ?, ?, ?, ?, ?, ? )}" ) or die;
    odbtp_output( $qry1, 1 );
    odbtp_input( $qry1, 2 );
    odbtp_input( $qry1, 3 );
    odbtp_input( $qry1, 4 );
    odbtp_input( $qry1, 5 );
    odbtp_input( $qry1, 6 );
    odbtp_input( $qry1, 7 );

    $qry2 = odbtp_allocate_query() or die;
    odbtp_prepare( "{call SetEmployeeNotes( ?, ? )}", $qry2 ) or die;
    odbtp_input( $qry2, 1 );
    odbtp_input( $qry2, 2 );

    function AddEmployee( $qry, $ExtId, $LastName, $FirstName, $Title, $Salary, $JobDesc )
    {
        odbtp_set( $qry, 2, $ExtId );
        odbtp_set( $qry, 3, $LastName );
        odbtp_set( $qry, 4, $FirstName );
        odbtp_set( $qry, 5, $Title );
        odbtp_set( $qry, 6, $Salary );
        if( $JobDesc != ODB_DEFAULT )
            odbtp_set( $qry, 7, $JobDesc );
        else
            odbtp_set( $qry, 7 );

        odbtp_execute( $qry ) or die;
        $out = odbtp_get( $qry, 1 ) or die;
        return $out;
    }
    function SetEmployeeNotes( $qry, $Id, $Notes )
    {
        odbtp_set( $qry, 1, $Id );
        odbtp_set( $qry, 2, $Notes );
        odbtp_execute( $qry ) or die;
    }
    $Id = AddEmployee( $qry1, "233522563477555", "Falk", "Peter", "CFO", "200000", "Cooks the books" );
    SetEmployeeNotes( $qry2, $Id, "He likes to manage money." );

    $Id = AddEmployee( $qry1, "999992111454333", "Rudman", "Viola", "Administrative Assistant", "41222.4", "Keeps boss in line" );
    SetEmployeeNotes( $qry2, $Id, "Wants more pay for all the work she does." );
?>
<html>
<head>
<title>ODBTP Test - Rollback</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
<?php
    odbtp_query( "SELECT * FROM Employees" ) or die;

    $cols = odbtp_num_fields( $qry1 );

    echo "<table cellpadding=2 cellspacing=0 border=1>\n";
    echo "<tr>";
    for( $col = 0; $col < $cols; $col++ )
        echo "<th>&nbsp;" . odbtp_field_name( $qry1, $col ) . "&nbsp;</th>";
    echo "</tr>\n";

    odbtp_bind_field( $qry1, "DateEntered", ODB_CHAR ) or die;

    while( ($rec = odbtp_fetch_array($qry1)) ) {
        echo "<tr>";
        for( $col = 0; $col < $cols; $col++ )
            echo "<td><nobr>&nbsp;$rec[$col]&nbsp;</nobr></td>";
        echo "</tr>\n";
    }
    echo "</table><p>\n";

    echo "ROLLBACK<p>\n";
    odbtp_rollback();

    odbtp_query( "SELECT * FROM Employees" ) or die;

    $cols = odbtp_num_fields( $qry1 );

    echo "<table cellpadding=2 cellspacing=0 border=1>\n";
    echo "<tr>";
    for( $col = 0; $col < $cols; $col++ )
        echo "<th>&nbsp;" . odbtp_field_name( $qry1, $col ) . "&nbsp;</th>";
    echo "</tr>\n";

    odbtp_bind_field( $qry1, "DateEntered", ODB_CHAR ) or die;

    while( ($rec = odbtp_fetch_array($qry1)) ) {
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
