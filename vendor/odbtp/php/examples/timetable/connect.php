<?php
    require( 'common.php' );
?>
<html>
<head>
<title>Connect</title>
</head>
<body>
<?php
    if( $con )
    {
        echo "Already Connected<p>";
    }
    else
    {
        $con = odbtp_rconnect( $odbtpserver, $connstring ) or die;

        $sql = "CREATE TABLE #TimeTable ( "
             . "TimeId int, "
             . "SqlTime datetime, "
             . "StrTime varchar(255), "
             . "SqlTimeNextYear datetime, "
             . "SqlTimeServer datetime DEFAULT (getdate()) )";

        $qry = odbtp_query( $sql ) or die;

        echo "Connected<p>";
        $id = odbtp_connect_id();
        $_SESSION['id'] = $id;
    }
    odbtp_close();
?>
<a href="connect.php">Connect</a><p>
<a href="insert.php">Insert Data</a><p>
<a href="list.php">List Data</a><p>
<a href="close.php">Close Connection <?= $id ?></a>
</body>
</html>
