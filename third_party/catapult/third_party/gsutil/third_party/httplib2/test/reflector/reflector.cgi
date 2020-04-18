#!/usr/bin/env python
import os

# Always returns an empty response body
# and adds in the X-Method: header with the
# method that was sent to the CGI

print "Status: 200 Ok"
print "Content-type: text/plain"
print 'ETag: "alsjflaksjfasj"'
print ""
print "\n".join(["%s=%s" % (key, value) for key, value in  os.environ.iteritems()])


