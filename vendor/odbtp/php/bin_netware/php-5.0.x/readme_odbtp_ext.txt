Quick Install of the PHP Odbtp Extension
========================================

PHP version    : 5.0.4
Apache version : 2.x

This extension requires that you use latest Novell LibC v.7.0.0 or later!
It is build for PHP 5.0.4 as it ships with NetWare 65 SP3/4 and will not
work with other versions!
Also it is required that you download the complete odbtp package since
this archive contains only the PHP odbtp client extension for NetWare:
http://odbtp.sourceforge.net/

Extract the files from this archive with full pathnames so that subdirs 
are created; then just copy the extension to sys:/php5/ext/.
If you need the mssql aliased version of odbtp then copy the suffixed
extension and rename it.
Finally edit sys:/php5/php.ini and add this line to load the extension:
extension=php_odbtp.nlm

and add the section below to configure odbtp:
[odbtp]
odbtp.interface_file = "sys:/php5/odbtp.conf"
odbtp.datetime_format = mdyhmsf
odbtp.detach_default_queries = yes

then reload PHP or reboot your server.
Then check with phpinfo() that the module is loaded and has correctly
parsed the config section.

Known bugs:
- none yet ...

