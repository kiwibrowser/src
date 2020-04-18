#!/usr/bin/env python
# -*- coding: utf-8 -*-
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

from setuptools import setup

if __name__ == '__main__':
    setup(name='rsa',
          version='3.4.2',
          description='Pure-Python RSA implementation',
          author='Sybren A. Stuvel',
          author_email='sybren@stuvel.eu',
          maintainer='Sybren A. Stuvel',
          maintainer_email='sybren@stuvel.eu',
          url='https://stuvel.eu/rsa',
          packages=['rsa'],
          license='ASL 2',
          classifiers=[
              'Development Status :: 5 - Production/Stable',
              'Intended Audience :: Developers',
              'Intended Audience :: Education',
              'Intended Audience :: Information Technology',
              'License :: OSI Approved :: Apache Software License',
              'Operating System :: OS Independent',
              'Programming Language :: Python',
              'Programming Language :: Python :: 2',
              'Programming Language :: Python :: 2.6',
              'Programming Language :: Python :: 2.7',
              'Programming Language :: Python :: 3',
              'Programming Language :: Python :: 3.3',
              'Programming Language :: Python :: 3.4',
              'Programming Language :: Python :: 3.5',
              'Topic :: Security :: Cryptography',
          ],
          install_requires=[
              'pyasn1 >= 0.1.3',
          ],
          entry_points={'console_scripts': [
              'pyrsa-priv2pub = rsa.util:private_to_public',
              'pyrsa-keygen = rsa.cli:keygen',
              'pyrsa-encrypt = rsa.cli:encrypt',
              'pyrsa-decrypt = rsa.cli:decrypt',
              'pyrsa-sign = rsa.cli:sign',
              'pyrsa-verify = rsa.cli:verify',
              'pyrsa-encrypt-bigfile = rsa.cli:encrypt_bigfile',
              'pyrsa-decrypt-bigfile = rsa.cli:decrypt_bigfile',
          ]},

          )
