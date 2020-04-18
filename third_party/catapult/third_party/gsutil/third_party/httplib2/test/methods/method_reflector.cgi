#!/usr/bin/env python
import os

# Always returns an empty response body
# and adds in the X-Method: header with the
# method that was sent to the CGI

print "Status: 200 Ok"
print "X-Method: %s" % os.environ['REQUEST_METHOD']
print ""


