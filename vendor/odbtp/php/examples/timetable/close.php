<?php
    require( 'common.php' );
?>
<html>
<head>
<title>Close Connection</title>
</head>
<body>
<?php
    if( !$con )
    {
        echo 'Not Connected<p>';
    }
    else
    {
        odbtp_close( $con, TRUE );
        echo "Connection Closed<p>";
        $id = NULL;
        $_SESSION['id'] = $id;
    }
?>
<a href="connect.php">Connect</a><p>
<a href="insert.php">Insert Data</a><p>
<a href="list.php">List Data</a><p>
<a href="close.php">Close Connection <?= $id ?></a>
</body>
</html>
