<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;
?>
<html>
<head>
<title>Insert The Ints</title>
</head>
<body>
<?php

    // Execute a stored procedure using attached parameters
    $qry = odbtp_prepare_proc( "AddTheInts" ) or die;
    odbtp_attach_param( $qry, "@RETURN_VALUE", $Id ) or die;
    odbtp_attach_param( $qry, "@TheTinyInt", $TheTinyInt ) or die;
    odbtp_attach_param( $qry, "@TheSmallInt", $TheSmallInt ) or die;
    odbtp_attach_param( $qry, "@TheInt", $TheInt ) or die;
    odbtp_attach_param( $qry, "@TheBigInt", $TheBigInt ) or die;

    $TheTinyInt = $_REQUEST['TheTinyInt'];
    $TheSmallInt = $_REQUEST['TheSmallInt'];
    $TheInt = $_REQUEST['TheInt'];
    $TheBigInt = $_REQUEST['TheBigInt'];
    odbtp_execute( $qry ) or die;
    echo "Id: $Id<p>";

    // Execute a stored procedure without using attached parameters
    $qry = odbtp_prepare_proc( "GetTheIntsString" ) or die;
    odbtp_set( $qry, "@Id", $Id ) or die;
    odbtp_execute( $qry ) or die;
    echo "Inserted: " . odbtp_get( $qry, "@TheIntsString" );

    odbtp_close();
?>
</body>
</html>
