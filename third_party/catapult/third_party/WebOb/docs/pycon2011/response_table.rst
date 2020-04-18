===========================
 Response Comparison Table
===========================

b=WebBob
z=Werkzeug
x=both
 =neither

WEBOB NAME                         write  read  WERKZEUG NAME                      NOTES
=================================  =====  ====  =================================  ===========================================
default_content_type                 x      x   default_mimetype                   wb default: "text/html", wz: "text/plain"
default_charset                      b      b  	                                   wz uses class var default for charset
charset                              x      x   charset
unicode_errors                       b      b
default_conditional_response         b      b
from_file() (classmethod)            b      b
copy                                 b      b
status (string)                      x      x   status
status_code                          x      x   status_code
                                            z   default_status
headers                              b      b
body                                 b      b
unicode_body                         x      x   data
body_file                                   b                                      File-like obj returned is writeable
app_iter                             b      x   get_app_iter()
                                            z   iter_encoded()
allow                                b      x   allow
vary                                 b      x   vary
content_type                         x      x   content_type
content_type_params                  x      x   mime_type_params
                                     z      z   mime_type                          content_type str wo parameters
content_length                       x      x   content_length
content_encoding                     x      x   content_encoding
content_language                     b      x   content_language
content_location                     x      x   content_location
content_md5                          x      x   content_md5
content_disposition                  b      b
accept_ranges                        b      b
content_range                        b      b
date                                 x      x   date
expires                              x      x   expires
last_modified                        x      x   last_modified
cache_control                        b      z   cache_control
cache_expires (dwim)                 b      b
conditional_response (bool)          b      x   make_conditional()
etag                                 b      x   add_etag()
etag                                 b      x   get_etag()
etag                                 b      x   set_etag()
                                            z   freeze()
location                             x      x   location
pragma                               b      b
age                                  x      x   age
retry_after                          x      x   retry_after
server                               b      b
www_authenticate                     b      z   www_authenticate
                                     x      x   date
retry_after                          x      x   retry_after
set_cookie()                                    set_cookie()
delete_cookie()                                 delete_cookie()
unset_cookie()
                                            z   is_streamed
                                            z   is_sequence
body_file                                   x   stream
                                                close()
                                                get_wsgi_headers()
                                                get_wsgi_response()
__call__()                                      __call__()
