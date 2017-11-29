<?php
    require( 'common.php' );
?>
<html>
<head>
<title>Insert Data</title>
</head>
<body>
<?php
    if( !$con )
    {
        echo 'Not Connected<p>';
    }
    else
    {
        $time = time();
        $strtime = date ("l dS of F Y h:i:s A", $time );
        $objtime = odbtp_ctime2datetime( $time );
        $objtime->year += 1;

        $sql = "INSERT INTO #TimeTable( TimeId, SqlTime, StrTime, SqlTimeNextYear ) "
             . "VALUES( ?, ?, ?, ? )";

        $qry = odbtp_prepare( $sql ) or die;
        odbtp_input( $qry, 1 ) or die;
        odbtp_input( $qry, 2 ) or die;
        odbtp_input( $qry, 3 ) or die;
        odbtp_input( $qry, 4 ) or die;

        odbtp_set( $qry, 1, $time ) or die;
        odbtp_set( $qry, 2, $time ) or die;
        odbtp_set( $qry, 3, $strtime ) or die;
        odbtp_set( $qry, 4, $objtime ) or die;
        odbtp_execute( $qry ) or die;

        echo "Data Inserted: $strtime<p>";

        odbtp_close();
    }
?>
<a href="connect.php">Connect</a><p>
<a href="insert.php">Insert Data</a><p>
<a href="list.php">List Data</a><p>
<a href="close.php">Close Connection <?= $id ?></a>
</body>
</html>
