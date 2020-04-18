Protobuf definitons
===================

annotations.proto [source of truth](https://github.com/luci/luci-go/tree/master/common/proto/milo): copied from commit [326c6be009be4018defde9a147536ee31c7ac515](https://github.com/luci/luci-go/commit/326c6be009be4018defde9a147536ee31c7ac515); [current commit](https://github.com/luci/luci-go/commit/master)

To regenerate:
```bash
$ cd chromite/lib/protos
$ wget https://github.com/luci/luci-go/raw/master/common/proto/milo/annotations.proto
$ protoc --python_out=. *.proto
# Update current commit hash in README.
$ vi README.md
```
