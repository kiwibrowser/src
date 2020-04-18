#!/usr/bin/env python
from distutils.core import setup

VERSION = "1.02"

setup(
name = "SocksiPy-branch",
version = VERSION,
description = "A Python SOCKS module",
long_description = "This Python module allows you to create TCP connections through a SOCKS proxy without any special effort.",
url = "http://socksipy.sourceforge.net/",
author = "Dan-Haim",
author_email="negativeiq@users.sourceforge.net",
license = "BSD",
py_modules=["socks"]
)

