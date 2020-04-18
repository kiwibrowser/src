#!/usr/bin/env python

# Authors: 
#   Trevor Perrin
#   Martin von Loewis - python 3 port
#
# See the LICENSE file for legal information regarding use of this file.

from __future__ import print_function
import sys
import os
import socket
import math

if __name__ != "__main__":
    raise "This must be run as a command, not used as a module!"


from tlslite import *
from tlslite import __version__

if len(sys.argv) == 1 or (len(sys.argv)==2 and sys.argv[1].lower().endswith("help")):
    print("")
    print("Version: %s" % __version__)
    print("")
    print("RNG: %s" % prngName)
    print("")
    print("Modules:")
    if m2cryptoLoaded:
        print("  M2Crypto    : Loaded")
    else:
        print("  M2Crypto    : Not Loaded")
    if pycryptoLoaded:
        print("  pycrypto    : Loaded")
    else:
        print("  pycrypto    : Not Loaded")
    if gmpyLoaded:
        print("  GMPY        : Loaded")
    else:
        print("  GMPY        : Not Loaded")
    print("")
    print("Commands:")
    print("")
    print("  createsrp       <db>")
    print("")
    print("  add    <db> <user> <pass> [<bits>]")
    print("  del    <db> <user>")
    print("  check  <db> <user> [<pass>]")
    print("  list   <db>")
    sys.exit()

cmd = sys.argv[1].lower()

class Args:
    def __init__(self, argv):
        self.argv = argv
    def get(self, index):
        if len(self.argv)<=index:
            raise SyntaxError("Not enough arguments")
        return self.argv[index]
    def getLast(self, index):
        if len(self.argv)>index+1:
            raise SyntaxError("Too many arguments")
        return self.get(index)

args = Args(sys.argv)

def reformatDocString(s):
    lines = s.splitlines()
    newLines = []
    for line in lines:
        newLines.append("  " + line.strip())
    return "\n".join(newLines)

try:
    if cmd == "help":
        command = args.getLast(2).lower()
        if command == "valid":
            print("")
        else:
            print("Bad command: '%s'" % command)

    elif cmd == "createsrp":
        dbName = args.get(2)

        db = VerifierDB(dbName)
        db.create()

    elif cmd == "add":
        dbName = args.get(2)
        username = args.get(3)
        password = args.get(4)

        db = VerifierDB(dbName)
        db.open()
        if username in db:
            print("User already in database!")
            sys.exit()
        bits = int(args.getLast(5))
        N, g, salt, verifier = VerifierDB.makeVerifier(username, password, bits)
        db[username] = N, g, salt, verifier

    elif cmd == "del":
        dbName = args.get(2)
        username = args.getLast(3)
        db = VerifierDB(dbName)
        db.open()
        del(db[username])

    elif cmd == "check":
        dbName = args.get(2)
        username = args.get(3)
        if len(sys.argv)>=5:
            password = args.getLast(4)
        else:
            password = None

        db = VerifierDB(dbName)
        db.open()

        try:
            db[username]
            print("Username exists")

            if password:
                if db.check(username, password):
                    print("Password is correct")
                else:
                    print("Password is wrong")
        except KeyError:
            print("Username does not exist")
            sys.exit()

    elif cmd == "list":
        dbName = args.get(2)
        db = VerifierDB(dbName)
        db.open()

        print("Verifier Database")
        def numBits(n):
            if n==0:
                return 0
            return int(math.floor(math.log(n, 2))+1)
        for username in db.keys():
            N, g, s, v = db[username]
            print(numBits(N), username)
    else:
        print("Bad command: '%s'" % cmd)
except:
    raise
