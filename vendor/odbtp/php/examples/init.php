<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;

    odbtp_set_attr( ODB_ATTR_TRANSACTIONS, ODB_TXN_READCOMMITTED ) or die;

    $qry1 = odbtp_query( "TRUNCATE TABLE Employees" ) or die;

    odbtp_prepare( "{? = call AddEmployee( ?, ?, ?, ?, ?, ? )}", $qry1 ) or die;
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

        if( !odbtp_execute( $qry ) ) {
            odbtp_rollback();
            die;
        }
        return odbtp_get( $qry, 1 );
    }
    function SetEmployeeNotes( $qry, $Id, $Notes )
    {
        odbtp_set( $qry, 1, $Id );
        odbtp_set( $qry, 2, $Notes );

        if( !odbtp_execute( $qry ) ) {
            odbtp_rollback();
            die;
        }
    }
    $Id = AddEmployee( $qry1, "111222333444555", "Rogers", "Frank", "CFO", "222444.88", ODB_DEFAULT );
    SetEmployeeNotes( $qry2, $Id, "He's one of the gugs who cooks the books." );

    $Id = AddEmployee( $qry1, "311322343454685", "Johnson", "Mary", "Supervisor", 100000, "Supervises Web Developers" );
    SetEmployeeNotes( $qry2, $Id, "She can read greek: αΑ βΒ γΓ δΔ εΕ ζΖ ηΗ θΘ ιΙ κΚ λΛ μΜ νΝ ξΞ οΟ πΠ ρΡ σΣ τΤ υΥ φΦ χΧ ψΨ ωΩ" );

    $Id = AddEmployee( $qry1, "568899902133452", "Smith", "John", "CEO", "38020979.46", NULL );
    SetEmployeeNotes( $qry2, $Id, "He's the guy that makes all the money." );

    $Id = AddEmployee( $qry1, "100000900000000", "Lopez", "Susan", "Computer Programmer", 73144.57, "Makes web-based applications" );
    SetEmployeeNotes( $qry2, $Id, "She prefers to use PHP for web development instead of ASP or JSP." );

    odbtp_commit();
?>
<html>
<head>
<title>ODBTP Test - Initialization</title>
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
    odbtp_bind_field( $qry1, "DateModified", ODB_CHAR ) or die;

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
