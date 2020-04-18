#!/usr/bin/env python

import cgi

print('Content-type: text/plain')
print('')

form = cgi.FieldStorage()

print('Filename: %s' % form['up'].filename)
print('Name: %s' % form['name'].value)
print('Content: %s' % form['up'].file.read())
