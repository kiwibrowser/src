# -*- coding: utf-8 -*-
#
#  Copyright 2011 Sybren A. Stüvel <sybren@stuvel.eu>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

"""Tests prime functions."""

import unittest

import rsa.prime


class PrimeTest(unittest.TestCase):
    def test_is_prime(self):
        """Test some common primes."""

        # Test some trivial numbers
        self.assertFalse(rsa.prime.is_prime(-1))
        self.assertFalse(rsa.prime.is_prime(0))
        self.assertFalse(rsa.prime.is_prime(1))
        self.assertTrue(rsa.prime.is_prime(2))
        self.assertFalse(rsa.prime.is_prime(42))
        self.assertTrue(rsa.prime.is_prime(41))

        # Test some slightly larger numbers
        self.assertEqual(
            [907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997],
            [x for x in range(901, 1000) if rsa.prime.is_prime(x)]
        )

        # Test around the 50th millionth known prime.
        self.assertTrue(rsa.prime.is_prime(982451653))
        self.assertFalse(rsa.prime.is_prime(982451653 * 961748941))
