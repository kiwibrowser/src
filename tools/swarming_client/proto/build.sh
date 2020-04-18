# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

# Creates the python bindings to the proto file. The grpcio and grpcio_tools
# Python packages must be installed, or else you'll get an error. I recommend
# using virtualenv to do this.
#
# Call this script every time you modify the .proto file and check in the
# resulting *_pb2.py files as well.
python -m grpc.tools.protoc \
  --python_out=. \
  --grpc_python_out=. \
  bytestream.proto \
  -I.
