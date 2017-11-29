Quick Install of the PHP Odbtp Extension
========================================

PHP version    : 4.2.3
Apache version : 2.x / 1.3.x

This extension requires that you use latest Novell LibC v.7.0.0 or later!
Also it is required that you download the complete odbtp package since
this archive contains only the PHP odbtp client extension for NetWare:
http://odbtp.sourceforge.net/

Extract the files from this archive with full pathnames so that subdirs 
are created; then just copy the right extension for your Apache (release_1.3
for Apache 1.3.x, or release_2.0 for Apache 2.0.x) to sys:/php/ext/.
If you need the mssql aliased version of odbtp then copy the suffixed
extension and rename it.
Finally edit sys:/php/php.ini and add this line to load the extension:
extension=phpodbtp.nlm

and add the section below to configure odbtp:
[odbtp]
odbtp.interface_file = "sys:/php/odbtp.conf"
odbtp.datetime_format = mdyhmsf
odbtp.detach_default_queries = yes

then reload PHP or reboot your server.
Then check with phpinfo() that the module is loaded and has correctly
parsed the config section.

Known bugs:
- none yet ...

