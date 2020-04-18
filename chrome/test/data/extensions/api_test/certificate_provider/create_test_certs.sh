#!/bin/bash

# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Generates the following tree of certificates:
#     root (self-signed root)
#      \
#       \--> l1_leaf (end-entity)

try() {
  "$@" || {
    e=$?
    echo "*** ERROR $e ***  $@  " > /dev/stderr
    exit $e
  }
}

# Create a self-signed CA cert with CommonName CN and store it at $1.pem .
root_cert() {
  try /bin/sh -c "echo 01 > out/${1}-serial"
  try touch out/${1}-index.txt
  try openssl genrsa -out out/${1}.key 2048

  CA_ID=$1 \
    try openssl req \
      -new \
      -key out/${1}.key \
      -out out/${1}.req \
      -config ca.cnf

  CA_ID=$1 \
    try openssl x509 \
      -req -days 3650 \
      -in out/${1}.req \
      -signkey out/${1}.key \
      -extfile ca.cnf \
      -extensions ca_cert > out/${1}.pem

  try cp out/${1}.pem ${1}.pem
}

# Create a cert with CommonName CN signed by CA_ID and store it at $1.der .
# $2 must either be "leaf_cert" (for a server/user cert) or "ca_cert" (for a
# intermediate CA).
# Stores the private key at $1.pk8 .
issue_cert() {
  if [[ "$2" == "ca_cert" ]]
  then
    try /bin/sh -c "echo 01 > out/${1}-serial"
    try touch out/${1}-index.txt
  fi
  try openssl req \
    -new \
    -keyout out/${1}.key \
    -out out/${1}.req \
    -config ca.cnf

  try openssl ca \
    -batch \
    -extensions $2 \
    -in out/${1}.req \
    -out out/${1}.pem \
    -config ca.cnf

  try openssl pkcs8 -topk8 -in out/${1}.key -out ${1}.pk8 -outform DER -nocrypt

  try openssl x509 -in out/${1}.pem -outform DER -out out/${1}.der
  try cp out/${1}.der ${1}.der
}

try rm -rf out
try mkdir out

CN=root \
  try root_cert root

CA_ID=root CN=l1_leaf \
  try issue_cert l1_leaf leaf_cert
