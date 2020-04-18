#!/bin/bash
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Updates the files which depend on net/data/ssl/certificates.

try() {
  "$@" || {
    e=$?
    echo "*** ERROR $e ***  $@  " > /dev/stderr
    exit $e
  }
}

net_certs_dir=../../../../../../net/data/ssl/certificates

try openssl x509 -in "${net_certs_dir}/client_1.pem" -outform DER -out \
  client_1.der
try openssl x509 -in "${net_certs_dir}/client_2.pem" -outform DER -out \
  client_2.der
try openssl rsa -in "${net_certs_dir}/client_1.key" -inform PEM -out \
  client_1_spki.der -pubout -outform DER
try openssl asn1parse -in client_1.der -inform DER -strparse 32 -out \
  client_1_issuer_dn.der
try echo -n "hello world" > data
try openssl rsautl -inkey "${net_certs_dir}/client_1.key" -sign -in \
   data -pkcs -out signature_nohash_pkcs
try openssl dgst -sha1 -sign "${net_certs_dir}/client_1.key" -out \
   signature_client1_sha1_pkcs data
try openssl dgst -sha1 -sign "${net_certs_dir}/client_2.key" -out \
   signature_client2_sha1_pkcs data
