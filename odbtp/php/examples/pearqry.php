<html>
<head>
<title>ODBTP PEAR DB Query Results</title>
</head>
<body>
<?php

    require( "DB.php" );

    $dsn = 'odbtp(mydbsyntax)://myuid:mypwd@odbtpsrvr/mydb?option1=val1';

    $dbh = DB::connect( $dsn );

    if( PEAR::isError( $dbh ) ) {
        echo "Error message: " . $dbh->getMessage() . "<br>";
        echo "Detailed error description: " . $dbh->getDebugInfo() . "<br>";
        die;
    }
    $res = $dbh->query( $_REQUEST['query'] );
    if( PEAR::isError( $res ) ) {
        echo "Error message: " . $res->getMessage() . "<br>";
        echo "Detailed error description: " . $res->getDebugInfo() . "<br>";
        die;
    }
    do {
        if( !($cols = $res->numCols()) ) continue;

        echo $res->numRows() . " rows<p>\n";

        $colinfo = $res->tableInfo();
        if( PEAR::isError( $colinfo ) ) {
            echo "Error message: " . $colinfo->getMessage() . "<br>\n";
            echo "Detailed error description: " . $colinfo->getDebugInfo() . "<br>\n";
            die;
        }
        echo "<table cellpadding=2 cellspacing=0 border=1>\n";
        echo "<tr>";
        echo "<td>&nbsp;<b>Table</b>&nbsp;</td>";
        echo "<td>&nbsp;<b>Name</b>&nbsp;</td>";
        echo "<td>&nbsp;<b>Type</b>&nbsp;</td>";
        echo "<td>&nbsp;<b>Length</b>&nbsp;</td>";
        echo "<td>&nbsp;<b>Flags</b>&nbsp;</td>";
        echo "</tr>\n";


        for( $i = 0; $i < $cols; $i++ ) {
            echo "<tr>";
            echo "<td>&nbsp;". $colinfo[$i]['table'] . "&nbsp;</td>";
            echo "<td>&nbsp;". $colinfo[$i]['name'] . "&nbsp;</td>";
            echo "<td>&nbsp;". $colinfo[$i]['type'] . "&nbsp;</td>";
            echo "<td>&nbsp;". $colinfo[$i]['len'] . "&nbsp;</td>";
            echo "<td>&nbsp;". $colinfo[$i]['flags'] . "&nbsp;</td>";
            echo "</tr>\n";
        }
        echo "</table><p>\n";
        echo "<table cellpadding=2 cellspacing=0 border=1>\n";
        echo "<tr>";

        for( $col = 0; $col < $cols; $col++ ) {
            echo "<td><nobr>&nbsp;" . $colinfo[$col]['name'] . "&nbsp;</nobr></td>";
        }
        echo "</tr>\n";

        while( $row = $res->fetchRow() ) {
            echo "<tr>";
            for( $col = 0; $col < $cols; $col++ ) {
                if( is_null( $row[$col] ) ) $row[$col] = "NULL";
                echo "<td valign=\"top\"><pre>";
                print_r( $row[$col] );
                echo "</pre></td>";
            }
            echo "</tr>\n";
        }
        echo "</table><p>\n\n";
    }
    while( $res->nextResult() );

    $dbh->disconnect();
?>
</body>
</html>
