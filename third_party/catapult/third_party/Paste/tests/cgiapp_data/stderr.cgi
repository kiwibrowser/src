#!/usr/bin/env python
from __future__ import print_function
import sys
print('Status: 500 Server Error')
print('Content-type: text/html')
print()
print('There was an error')
print('some data on the error', file=sys.stderr)
