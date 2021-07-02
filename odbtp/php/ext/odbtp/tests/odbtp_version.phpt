--TEST--
odbtp_version() function
--SKIPIF--
<?php if (!extension_loaded('odbtp')) print 'skip'; ?>
--POST--
--GET--
--FILE--
<?php
    require 'connect.inc';

    if( $connection ) $version = odbtp_version( $connection );

    if( $version )
        echo "$version\n";
    else
        echo "Unable to get version\n";
?>
--EXPECT--
ODBTP/1.1
