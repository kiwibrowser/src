#!/usr/bin/env python
from __future__ import print_function
from tlslite import HTTPTLSConnection, HandshakeSettings

settings = HandshakeSettings()
settings.useExperimentalTackExtension = True

h = HTTPTLSConnection("localhost", 4443, settings=settings)    
h.request("GET", "/index.html")
r = h.getresponse()
print(r.read())
