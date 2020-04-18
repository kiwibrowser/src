# protoc plugin for JSON converter

These protoc plugins use a simple JSON encoding.

An instance of the following protobuf:

```
message Foo {
  message Bar {
    repeated string rabbits = 1;
  }
  optional string cat = 1;
  repeated int32 dog = 2;
  optional Bar rabbit_den = 3;
}
```

could be encoded to something like:

```
{
  "1": "kitty",
  "2": [4, 16, 9],
  "3": { "1": ["thumper", "oreo", "daisy"] }
}
```

Only a limited part of the protocol buffer IDL is supported.

*   Supported field types: `float`, `double`, `int32`, `bool`, `string`, `message`, and `enum`

*   Supported field rules: `optional`, `repeated`

*   Unsupported features:

    *   default values
    *   imports
    *   extensions
    *   services
    *   non-file-level options
