<?php

// You must change $server and $db_connect to match your installation.

$server     = 'odbtpserver.somewhere.com';
$db_connect = 'DRIVER={SQL Server};SERVER=myserver;UID=myuid;PWD=mypwd;DATABASE=pubs;';
$connection = odbtp_connect( $server, $db_connect );

?>
