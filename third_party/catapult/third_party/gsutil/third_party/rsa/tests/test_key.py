"""
Some tests for the rsa/key.py file.
"""

import unittest

import rsa.key
import rsa.core


class BlindingTest(unittest.TestCase):
    def test_blinding(self):
        """Test blinding and unblinding.

        This is basically the doctest of the PrivateKey.blind method, but then
        implemented as unittest to allow running on different Python versions.
        """

        pk = rsa.key.PrivateKey(3727264081, 65537, 3349121513, 65063, 57287)

        message = 12345
        encrypted = rsa.core.encrypt_int(message, pk.e, pk.n)

        blinded = pk.blind(encrypted, 4134431)  # blind before decrypting
        decrypted = rsa.core.decrypt_int(blinded, pk.d, pk.n)
        unblinded = pk.unblind(decrypted, 4134431)

        self.assertEqual(unblinded, message)


class KeyGenTest(unittest.TestCase):
    def test_custom_exponent(self):
        priv, pub = rsa.key.newkeys(16, exponent=3)

        self.assertEqual(3, priv.e)
        self.assertEqual(3, pub.e)

    def test_default_exponent(self):
        priv, pub = rsa.key.newkeys(16)

        self.assertEqual(0x10001, priv.e)
        self.assertEqual(0x10001, pub.e)
