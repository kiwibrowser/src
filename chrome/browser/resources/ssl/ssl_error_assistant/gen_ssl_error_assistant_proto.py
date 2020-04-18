#!/usr/bin/python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
 Convert the ASCII ssl_error_assistant.asciipb proto into a binary resource.
"""

import base64
import os
import sys

# Import the binary proto generator. Walks up to the root of the source tree
# which is six directories above, and finds the protobufs directory from there.
proto_generator_path = os.path.normpath(os.path.join(os.path.abspath(__file__),
    *[os.path.pardir] * 6 + ['chrome/browser/resources/protobufs']))
sys.path.insert(0, proto_generator_path)
from binary_proto_generator import BinaryProtoGenerator


class SSLErrorAssistantProtoGenerator(BinaryProtoGenerator):
  def ImportProtoModule(self):
    import ssl_error_assistant_pb2
    globals()['ssl_error_assistant_pb2'] = ssl_error_assistant_pb2

  def EmptyProtoInstance(self):
    return ssl_error_assistant_pb2.SSLErrorAssistantConfig()

  def ValidatePb(self, opts, pb):
    assert pb.version_id > 0
    assert len(pb.captive_portal_cert) > 0
    for cert in pb.captive_portal_cert:
      assert(cert.sha256_hash.startswith("sha256/"))
      decoded_hash = base64.b64decode(cert.sha256_hash[len("sha256/"):])
      assert(len(decoded_hash) == 32)

  def ProcessPb(self, opts, pb):
    binary_pb_str = pb.SerializeToString()
    outfile = os.path.join(opts.outdir, opts.outbasename)
    open(outfile, 'wb').write(binary_pb_str)


def main():
  return SSLErrorAssistantProtoGenerator().Run()

if __name__ == '__main__':
  sys.exit(main())
