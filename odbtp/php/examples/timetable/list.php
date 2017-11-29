<?php
    require( 'common.php' );
?>
<html>
<head>
<title>List Data</title>
</head>
<body>
<?php
    if( !$con )
    {
        echo 'Not Connected<p>';
    }
    else
    {
        $sql = "SELECT * FROM #TimeTable";
        $qry = odbtp_query( $sql ) or die;

        odbtp_bind_field( $qry, 'SqlTime', ODB_CHAR ) or die;

        echo "<table cellpadding=2 cellspacing=0 border=1>\n";
        echo "<tr><td>&nbsp;TimeId&nbsp;</td><td>&nbsp;SqlTime&nbsp;</td>"
           . "<td>&nbsp;StrTime&nbsp;</td><td>&nbsp;SqlTimeNextYear&nbsp;</td>"
           . "<td>&nbsp;SqlTimeServer&nbsp;</td></tr>";

        while( ($rec = odbtp_fetch_array( $qry )) )
        {
            $SqlTimeNextYear
                = date( "l dS of F Y h:i:s A",
                        odbtp_datetime2ctime( $rec['SqlTimeNextYear'] ) );

            $SqlTimeServer
                = sprintf( "%02d/%02d/%d %02d:%02d:%02d.%d",
                           $rec['SqlTimeServer']->month,
                           $rec['SqlTimeServer']->day,
                           $rec['SqlTimeServer']->year,
                           $rec['SqlTimeServer']->hour,
                           $rec['SqlTimeServer']->minute,
                           $rec['SqlTimeServer']->second,
                           $rec['SqlTimeServer']->fraction );

            echo "<tr>";
            echo "<td><nobr>&nbsp;$rec[TimeId]&nbsp;</nobr></td>";
            echo "<td><nobr>&nbsp;$rec[SqlTime]&nbsp;</nobr></td>";
            echo "<td><nobr>&nbsp;$rec[StrTime]&nbsp;</nobr></td>";
            echo "<td><nobr>&nbsp;$SqlTimeNextYear&nbsp;</nobr></td>";
            echo "<td><nobr>&nbsp;$SqlTimeServer&nbsp;</nobr></td>";
            echo "</tr>";
        }
        echo "</table><p>\n";

        odbtp_close();
    }
?>
<a href="connect.php">Connect</a><p>
<a href="insert.php">Insert Data</a><p>
<a href="list.php">List Data</a><p>
<a href="close.php">Close Connection <?= $id ?></a>
</body>
</html>
