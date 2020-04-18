# -*- coding: utf-8 -*-
#
#  Copyright 2011 Sybren A. Stüvel <sybren@stuvel.eu>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

'''Unittest for saving and loading keys.'''

import base64
import unittest
import os.path
import pickle

from rsa._compat import b

import rsa.key

B64PRIV_DER = b('MC4CAQACBQDeKYlRAgMBAAECBQDHn4npAgMA/icCAwDfxwIDANcXAgInbwIDAMZt')
PRIVATE_DER = base64.standard_b64decode(B64PRIV_DER)

B64PUB_DER = b('MAwCBQDeKYlRAgMBAAE=')
PUBLIC_DER = base64.standard_b64decode(B64PUB_DER)

PRIVATE_PEM = b('''
-----BEGIN CONFUSING STUFF-----
Cruft before the key

-----BEGIN RSA PRIVATE KEY-----
Comment: something blah

%s
-----END RSA PRIVATE KEY-----

Stuff after the key
-----END CONFUSING STUFF-----
''' % B64PRIV_DER.decode("utf-8"))

CLEAN_PRIVATE_PEM = b('''\
-----BEGIN RSA PRIVATE KEY-----
%s
-----END RSA PRIVATE KEY-----
''' % B64PRIV_DER.decode("utf-8"))

PUBLIC_PEM = b('''
-----BEGIN CONFUSING STUFF-----
Cruft before the key

-----BEGIN RSA PUBLIC KEY-----
Comment: something blah

%s
-----END RSA PUBLIC KEY-----

Stuff after the key
-----END CONFUSING STUFF-----
''' % B64PUB_DER.decode("utf-8"))

CLEAN_PUBLIC_PEM = b('''\
-----BEGIN RSA PUBLIC KEY-----
%s
-----END RSA PUBLIC KEY-----
''' % B64PUB_DER.decode("utf-8"))


class DerTest(unittest.TestCase):
    """Test saving and loading DER keys."""

    def test_load_private_key(self):
        """Test loading private DER keys."""

        key = rsa.key.PrivateKey.load_pkcs1(PRIVATE_DER, 'DER')
        expected = rsa.key.PrivateKey(3727264081, 65537, 3349121513, 65063, 57287)

        self.assertEqual(expected, key)

    def test_save_private_key(self):
        """Test saving private DER keys."""

        key = rsa.key.PrivateKey(3727264081, 65537, 3349121513, 65063, 57287)
        der = key.save_pkcs1('DER')

        self.assertEqual(PRIVATE_DER, der)

    def test_load_public_key(self):
        """Test loading public DER keys."""

        key = rsa.key.PublicKey.load_pkcs1(PUBLIC_DER, 'DER')
        expected = rsa.key.PublicKey(3727264081, 65537)

        self.assertEqual(expected, key)

    def test_save_public_key(self):
        """Test saving public DER keys."""

        key = rsa.key.PublicKey(3727264081, 65537)
        der = key.save_pkcs1('DER')

        self.assertEqual(PUBLIC_DER, der)


class PemTest(unittest.TestCase):
    """Test saving and loading PEM keys."""

    def test_load_private_key(self):
        """Test loading private PEM files."""

        key = rsa.key.PrivateKey.load_pkcs1(PRIVATE_PEM, 'PEM')
        expected = rsa.key.PrivateKey(3727264081, 65537, 3349121513, 65063, 57287)

        self.assertEqual(expected, key)

    def test_save_private_key(self):
        """Test saving private PEM files."""

        key = rsa.key.PrivateKey(3727264081, 65537, 3349121513, 65063, 57287)
        pem = key.save_pkcs1('PEM')

        self.assertEqual(CLEAN_PRIVATE_PEM, pem)

    def test_load_public_key(self):
        """Test loading public PEM files."""

        key = rsa.key.PublicKey.load_pkcs1(PUBLIC_PEM, 'PEM')
        expected = rsa.key.PublicKey(3727264081, 65537)

        self.assertEqual(expected, key)

    def test_save_public_key(self):
        """Test saving public PEM files."""

        key = rsa.key.PublicKey(3727264081, 65537)
        pem = key.save_pkcs1('PEM')

        self.assertEqual(CLEAN_PUBLIC_PEM, pem)

    def test_load_from_disk(self):
        """Test loading a PEM file from disk."""

        fname = os.path.join(os.path.dirname(__file__), 'private.pem')
        with open(fname, mode='rb') as privatefile:
            keydata = privatefile.read()
        privkey = rsa.key.PrivateKey.load_pkcs1(keydata)

        self.assertEqual(15945948582725241569, privkey.p)
        self.assertEqual(14617195220284816877, privkey.q)


class PickleTest(unittest.TestCase):
    """Test saving and loading keys by pickling."""

    def test_private_key(self):
        pk = rsa.key.PrivateKey(3727264081, 65537, 3349121513, 65063, 57287)

        pickled = pickle.dumps(pk)
        unpickled = pickle.loads(pickled)
        self.assertEqual(pk, unpickled)

    def test_public_key(self):
        pk = rsa.key.PublicKey(3727264081, 65537)

        pickled = pickle.dumps(pk)
        unpickled = pickle.loads(pickled)

        self.assertEqual(pk, unpickled)
