<?php
    require( 'connect.php' );
    $con = odbtp_connect( $odbtpserver, $connstring ) or die;
?>
<html>
<head>
<title>SP_WHO</title>
</head>
<body>
<?php
    $qry = odbtp_query( "EXEC sp_who" ) or die;
    odbtp_attach_field( $qry, "spid", $spid ) or die;
    odbtp_attach_field( $qry, "loginame", $loginame ) or die;
    odbtp_attach_field( $qry, "dbname", $dbname ) or die;

    echo "<table cellpadding=2 cellspacing=0 border=1>\n";
    echo "<tr><td>&nbsp;spid&nbsp;</td>"
       . "<td>&nbsp;loginame&nbsp;</td>"
       . "<td>&nbsp;dbname&nbsp;</td></tr>";

    while( odbtp_fetch( $qry ) ) {
        if( is_null( $dbname ) ) $dbname = "NULL";
        echo "<tr><td>&nbsp;$spid&nbsp;</td>"
           . "<td>&nbsp;$loginame&nbsp;</td>"
           . "<td>&nbsp;$dbname&nbsp;</td></tr>";

    }
    echo "</table>";

    odbtp_close();
?>
</body>
</html>
