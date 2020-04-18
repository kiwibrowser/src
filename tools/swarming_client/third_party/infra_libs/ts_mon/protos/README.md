Updating the *.proto files: see go/updating-tsmon-protos

To generate the `*_pb2.py` files from the `*proto` files:

    cd infra_libs/ts_mon/protos/new
    protoc --python_out=. *.proto

protoc version tested: libprotoc 3.0.0

