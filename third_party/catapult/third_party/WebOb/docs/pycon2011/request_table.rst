==========================
 Request Comparison Table
==========================

b=WebBob
z=Werkzeug
x=both


WEBOB NAME                         write  read  WERKZEUG NAME                      NOTES
=================================  =====  ====  =================================  ===========================================

Read-Write Properties                           Read-Write Properties
+++++++++++++++++++++                           +++++++++++++++++++++

content_type                                    content_type                       CommonRequestDescriptorMixin
charset                                         charset "utf-8"
headers                                         headers cached_property
urlvars
urlargs
host                                            host cached_property
body
unicode_errors 'strict'                         encoding_errors 'ignore'
decode_param_names F
request_body_tempfile_limit 10*1024             max_content_length None            Not sure if these are the same
                                                is_behind_proxy F
                                                max_form_memory_size None
                                                parameter_storage_class            ImmutableMultiDict
                                                list_storage_class                 ImmutableList
                                                dict_storage_class                 ImmutableTypeConversionDict
environ                                         environ
                                                populate_request T
                                                shallow F


Environ Getter Properties
+++++++++++++++++++++++++

body_file_raw
scheme
method                                          method
http_version
script_name                                     script_root cached_property
path_info                                       ???path cached_property
content_length                                  content_type                       CommonRequestDescriptorMixin
remote_user                                     remote_user
remote_addr                                     remote_addr
query_string                                    query_string
server_name                                     host (with port)
server_port                                     host (with name)
uscript_name
upath_info
is_body_seekable
authorization                                   authorization cached_property
pragma                                          pragma cached_property
date                                            date                               CommonRequestDescriptorMixin
max_forwards                                    max_forwards                       CommonRequestDescriptorMixin
range
if_range
referer/referrer                                referrer                           CommonRequestDescriptorMixin
user_agent                                      user_agent cached_property
                                                input_stream
                                                mimetype                           CommonRequestDescriptorMixin


Read-Only Properties
++++++++++++++++++++

host_url                                        host_url cached_property
application_url                                 base_url cached_property        Not sure if same
path_url                                        ???path cached_property
path                                            ???path cached_property
path_qs                                         ???path cached_property
url                                             url cached_property
is_xhr                                          is_xhr
str_POST
POST
str_GET
GET
str_params
params
str_cookies
cookies                                         cookies cached_property
                                                url_charset
                                                stream cached_property
                                                args cached_property            Maybe maps to params
                                                data cached_property
                                                form cached_property
                                                values cached_property          Maybe maps to params
                                                files  cached_property
                                                url_root cached_property
                                                access_route cached_property
                                                is_secure
                                                is_multithread
                                                is_multiprocess
                                                is_run_once


Accept Properties
+++++++++++++++++

accept                                          accept_mimetypes
accept_charset                                  accept_charsets
accept_encoding                                 accept_encodings
accept_language                                 accept_languages

Etag Properties
+++++++++++++++

cache_control                                   cache_control cached_property
if_match                                        if_match cached_property
if_none_match                                   if_none_match cached_property
if_modified_since                               if_modified_since cached_property
if_unmodified_since                             if_unmodified_since cached_property

Methods
++++++

relative_url
path_info_pop
path_info_peek
copy
copy_get
make_body_seekable
copy_body
make_tempfile
remove_conditional_headers
as_string (__str__)
call_application
get_response

Classmethods
++++++++++++

from_string (classmethod)
from_file
blank
                                                from_values
                                                application

Notes
-----

 <mitsuhiko> mcdonc: script_root and path in werkzeug are not quite script_name and path_info in webob
[17:51] <mitsuhiko> the behavior regarding slashes is different for easier url joining
