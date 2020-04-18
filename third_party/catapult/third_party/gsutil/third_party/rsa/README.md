Pure Python RSA implementation
==============================

[![PyPI](https://img.shields.io/pypi/v/rsa.svg)](https://pypi.python.org/pypi/rsa)
[![Build Status](https://travis-ci.org/sybrenstuvel/python-rsa.svg?branch=master)]
    (https://travis-ci.org/sybrenstuvel/python-rsa)
[![Coverage Status](https://coveralls.io/repos/github/sybrenstuvel/python-rsa/badge.svg?branch=master)]
    (https://coveralls.io/github/sybrenstuvel/python-rsa?branch=master)
[![Code Climate](https://img.shields.io/codeclimate/github/sybrenstuvel/python-rsa.svg)]
    (https://codeclimate.com/github/sybrenstuvel/python-rsa)

[Python-RSA](https://stuvel.eu/rsa) is a pure-Python RSA implementation. It supports
encryption and decryption, signing and verifying signatures, and key
generation according to PKCS#1 version 1.5. It can be used as a Python
library as well as on the commandline. The code was mostly written by
Sybren A.  Stüvel.

Documentation can be found at the [Python-RSA homepage](https://stuvel.eu/rsa).

Download and install using:

    pip install rsa

or download it from the [Python Package Index](https://pypi.python.org/pypi/rsa).

The source code is maintained at [Github](https://github.com/sybrenstuvel/python-rsa/) and is
licensed under the [Apache License, version 2.0](https://www.apache.org/licenses/LICENSE-2.0)


Plans for the future
--------------------

Version 3.4 is the last version in the 3.x range. Version 4.0 will drop the following modules,
as they are insecure:

- `rsa._version133`
- `rsa._version200`
- `rsa.bigfile`
- `rsa.varblock`

Those modules are marked as deprecated in version 3.4.

Furthermore, in 4.0 the I/O functions will be streamlined to always work with bytes on all
supported versions of Python.
