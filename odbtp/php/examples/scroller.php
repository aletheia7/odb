<?php
    session_start();
?>
<html>
<head>
<title>ODBTP Scroller</title>
</head>
<body>
<?php
    require( 'connect.php' );
    if( !$_SESSION['id'] ) {
        $con = odbtp_rconnect( $odbtpserver, $connstring ) or die;
        $_SESSION['id'] = odbtp_connect_id();;
        $qry = odbtp_allocate_query( $con ) or die;
        odbtp_set_cursor( $qry, ODB_CURSOR_STATIC, 0, TRUE ) or die;
    }
    else {
        if( !($con = odbtp_rconnect( $odbtpserver, $_SESSION['id'] )) ) {
            $_SESSION['id'] = NULL;
            die;
        }
        $qry = odbtp_get_query( $con );
    }
    switch( $_REQUEST['action'] ) {
        case 'QUERY': odbtp_query( $_REQUEST['query'], $qry ) or die;
                      $fetch_type = ODB_FETCH_NEXT;
                      $fetch_param = 0;
                      break;

        case 'FIRST': $fetch_type = ODB_FETCH_FIRST;
                      $fetch_param = 0;
                      break;

        case 'PREV': $fetch_type = ODB_FETCH_PREV;
                     $fetch_param = 0;
                     break;

        case 'NEXT': $fetch_type = ODB_FETCH_NEXT;
                     $fetch_param = 0;
                     break;

        case 'LAST': $fetch_type = ODB_FETCH_LAST;
                     $fetch_param = 0;
                     break;

        case 'ABSOLUTE': $fetch_type = ODB_FETCH_ABS;
                         $fetch_param = $_REQUEST['absolute_param'];
                         break;

        case 'RELATIVE': $fetch_type = ODB_FETCH_REL;
                         $fetch_param = $_REQUEST['relative_param'];
                         break;

        case 'BOOKMARK': $fetch_type = ODB_FETCH_BOOKMARK;
                         $fetch_param = $_REQUEST['bookmark_param'];
                         break;

        case 'BOOKMARK ROW': odbtp_row_bookmark( $qry ) or die;
                             $fetch_type = ODB_FETCH_BOOKMARK;
                             $fetch_param = 0;
                             break;

        case 'CLOSE': odbtp_close( $con, TRUE );
                      $_SESSION['id'] = NULL;
                      die( 'Connection Closed.<p><a href="scroller.htm">New Query</a>' );
                      break;
    }
    $cols = odbtp_num_fields( $qry ) or die( 'No Rows. <a href="scroller.htm">New Query</a>' );
?>
<form action="scroller.php" method="POST">
<table cellpadding=0 cellspacing=8 border=0>
<tr>
<td><nobr>
<input type="submit" name="action" value="FIRST">
<input type="submit" name="action" value="PREV">
<input type="submit" name="action" value="NEXT">
<input type="submit" name="action" value="LAST">
&nbsp;
<input type="submit" name="action" value="BOOKMARK ROW">
<input type="submit" name="action" value="CLOSE">&nbsp;
<a href="scroller.htm">New Query</a>
</nobr></td>
<tr>
<td><nobr>
<input type="text" size="5" name="absolute_param" value="1">
<input type="submit" name="action" value="ABSOLUTE">&nbsp;
<input type="text" size="5" name="relative_param" value="0">
<input type="submit" name="action" value="RELATIVE">&nbsp;
<input type="text" size="5" name="bookmark_param" value="0">
<input type="submit" name="action" value="BOOKMARK">
</nobr></td>
</tr>
</table>

<?php
    $rec = odbtp_fetch_array( $qry, $fetch_type, $fetch_param );

    echo "<p>";
    echo odbtp_affected_rows( $qry );
    echo " rows<p>\n";
    echo "<table cellpadding=2 cellspacing=0 border=0>\n";

    if( $rec ) {
        for( $col = 0; $col < $cols; $col++ ) {
            echo "<tr>";
            echo '<td align="right" valign="top"><small><nobr><b>';
            echo odbtp_field_name( $qry, $col );
            echo ':&nbsp;&nbsp;</b></nobr></small></td>';
            if( !is_null( $rec[$col] ) )
                echo "<td>$rec[$col]</td>";
            else
                echo '<td>NULL</td>';
            echo "</tr>\n";
        }
    }
    else {
        for( $col = 0; $col < $cols; $col++ ) {
            echo "<tr>";
            echo '<td align="right" valign="top"><small><nobr><b>';
            echo odbtp_field_name( $qry, $col );
            echo ':&nbsp;&nbsp;</b></nobr></small></td>';
            if( $col == 0 )
                echo "<td rowspan=\"$cols\" valign=\"top\">NO DATA</td>";
            echo "</tr>\n";
        }
    }
    echo "</table><p>\n";

    odbtp_close( $con );
?>
</body>
</html>
