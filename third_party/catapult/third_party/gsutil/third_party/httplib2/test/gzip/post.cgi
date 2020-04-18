#!/usr/bin/env python
import zlib
import os
from StringIO import StringIO

# Always returns a gzipped response body
print "Status: 200 Ok"
print ""

print(zlib.compress('This is a compressed string'))


