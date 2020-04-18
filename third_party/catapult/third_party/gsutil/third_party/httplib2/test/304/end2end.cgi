#!/usr/bin/env python
import os


etag = os.environ.get("HTTP_IF_NONE_MATCH", None)
if etag:
    print "Status: 304 Not Modified"
else:
    print "Status: 200 Ok"
    print 'ETag: "123456779"'
    print "Content-Type: text/html"
    print ""
    print "<html></html>"
