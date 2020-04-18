#!/usr/bin/env python
import os
import time

# Always returns an empty response body
# and adds in the X-Method: header with the
# method that was sent to the CGI
time.sleep(3)

print "Status: 200 Ok"
print "Content-type: text/plain"
print ""
print "3 seconds later"


