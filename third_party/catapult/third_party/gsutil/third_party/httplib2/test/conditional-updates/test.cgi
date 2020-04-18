#!/usr/bin/env python
import os

# Always returns an empty response body
# and adds in the X-Method: header with the
# method that was sent to the CGI

method = os.environ['REQUEST_METHOD']
if "GET" == method:
    if "123456789" == os.environ.get('HTTP_IF_NONE_MATCH', ''):
        print "Status: 304 Not Modified"
    else:
        print "Status: 200 Ok"
        print "ETag: 123456789"
        print ""
elif method in ["PUT", "PATCH", "DELETE"]:
    if "123456789" == os.environ.get('HTTP_IF_MATCH', ''):
        print "Status: 200 Ok"
        print ""
    else:
        print "Status: 412 Precondition Failed"
        print ""
else:
    print "Status: 405 Method Not Allowed"
    print ""



