<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;
?>
<html>
<head>
<title>MSSQL Database Query Results</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
<?php
    $qry = odbtp_query( "SELECT NEWID()" ) or die;
    $guid1 = odbtp_result( $qry, 0, 0 );
    $sguid1 = odbtp_guid_string( $guid1 );

    $qry = odbtp_prepare_proc( "EchoGUID" ) or die;

    // Binary format input (the default)
    odbtp_attach_param( $qry, "@GUIDIn", $guid1 ) or die;
    odbtp_attach_param( $qry, "@GUIDOut", $guid2 ) or die;
    odbtp_execute( $qry ) or die;
    echo odbtp_guid_string( $guid1 ) . "<br>\n";
    echo odbtp_guid_string( $guid2 ) . "<p>\n\n";

    // String format input
    odbtp_attach_param( $qry, "@GUIDIn", $sguid1, ODB_CHAR ) or die;
    odbtp_attach_param( $qry, "@GUIDOutStr", $sguid2 ) or die;
    odbtp_execute( $qry ) or die;
    echo $sguid1 . "<br>\n";
    echo $sguid2 . "\n";

    odbtp_close();
?>
</body>
</html>
