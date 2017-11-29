<?php
    session_start();

    require( '../connect.php' );

    if( ($id = $_SESSION['id']) )
    {
        if( !($con = odbtp_rconnect( $odbtpserver, $id )) )
        {
            $_SESSION['id'] = NULL;
            die;
        }
    }
    else
    {
        $con = NULL;
    }
?>
