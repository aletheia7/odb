BEGIN {

  # fetch version number from input file and writes them to STDOUT

  while ((getline < ARGV[1]) > 0) {
    if (match ($0, /^#define ODBTP_LIB_VERSION "[^"]+"/)) {
      ver_str = substr($3, 2, length($3) - 2);
      split(ver_str, v, ".");
      if (!v[3]) {
        v[3] = 0;
      }
    }
  }
  
  # fetch Apache version numbers from input file and writes them to STDOUT

  if (ARGV[2]) {
    if (match (ARGV[2], /ap_release.h/)) {
      while ((getline < ARGV[2]) > 0) {
        if (match ($0, /^#define AP_SERVER_MAJORVERSION "[^"]+"/)) {
          ap_ver_major = substr($3, 2, length($3) - 2);
        }
        else if (match ($0, /^#define AP_SERVER_MINORVERSION "[^"]+"/)) {
          ap_ver_minor = substr($3, 2, length($3) - 2);
        }
        else if (match ($0, /^#define AP_SERVER_PATCHLEVEL/)) {
          ap_ver_str_patch = substr($3, 2, length($3) - 2);
          if (match (ap_ver_str_patch, /[0-9][0-9]*/)) {
            ap_ver_patch = substr(ap_ver_str_patch, RSTART, RLENGTH); 
          }
        }
      }
      ap_ver_str = ap_ver_major "." ap_ver_minor "." ap_ver_str_patch;
    }
    if (match (ARGV[2], /httpd.h/)) {
      while ((getline < ARGV[2]) > 0) {
        if (match ($0, /^#define SERVER_BASEREVISION "[^"]+"/)) {
          ap_ver_str = substr($3, 2, length($3) - 2);
        }
      }
    }
    print "AP_VERSION_STR = " ap_ver_str "";
  }

  print "VMAJ = " v[1] "";
  print "VMIN = " v[2] "";
  print "VREV = " v[3] "";
  print "VERSION_STR = " ver_str "";

}
