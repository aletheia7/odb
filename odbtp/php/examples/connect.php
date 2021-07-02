<?php
    // Configure here.
    $odbtpserver = 'odbtpsvr.somewhere.com';
    $mssqldriver = '{SQL Server}';
    $mssqlserver = 'MYMSSQLSERVER';
    $mssqluser   = 'sa';
    $mssqlpass   = '';
    $database    = 'OdbtpTest';

    // Let's build up the connection string from the vars for later use.
    $connstring  = "DRIVER=$mssqldriver;SERVER=$mssqlserver;UID=$mssqluser;PWD=$mssqlpass;DATABASE=$database;";

    // Let's check for our extension and bail out if not loaded.
    extension_loaded('odbtp') or die ("Error: odbtp extension not loaded! Make sure you have the extension enabled in '" . get_cfg_var('cfg_file_path') . "' !");
?>


