# `LUCI_CONTEXT`

`LUCI_CONTEXT` is a generic way for LUCI services to pass contextual
information to each other. It has a very simple protocol:
  * Application writes a JSON file out (usually a temp file)
  * Application sets `LUCI_CONTEXT` environment variable to point to that file.
  * Subprocesses are expected to read contents from `LUCI_CONTEXT` in whole or
    part.
  * If any subprocess needs to add/modify information in the context, it copies
    the existing `LUCI_CONTEXT` entirely, then makes its modifications on the
    copy before writing it out to a different file (and updating the envvar
    appropriately.)

The `LUCI_CONTEXT` JSON file is always a JSON object (e.g. `{...}`), and
applications are cooperative in terms of the top-level keys (all known keys for
`LUCI_CONTEXT` and their meaning should be documented in this file). Every
top-level key also corresponds to a JSON object (never a primitive), to avoid
the temptation to pollute the top-level namespace with multiple
related-but-not-grouped data items.

No implementation should make the assumption that it knows the full set of keys
and/or schemas (hence the 'copy-and-modify' portion of the protocol).

Parents should keep any `LUCI_CONTEXT` files they write out alive for the
subprocess to read them (>= observable lifetime of the subprocess). If
a subprocess intends to outlive its parent, it MUST make its own copy of the
`LUCI_CONTEXT` file.

Example contents:
```
{
  "local_auth": {
    "rpc_port": 10000,
    "secret": "aGVsbG8gd29ybGQK",
    ...
  },
  "swarming": {
    "secret_bytes": "cmFkaWNhbGx5IGNvb2wgc2VjcmV0IHN0dWZmCg=="
  }
}
```

# Library support
There is an easy-to-use library for accessing the contents of `LUCI_CONTEXT`, as
well as producing new contexts, located
[here][./libs/luci_context/luci_context.py].

# Known keys

For precision, the known keys should be documented with a block of protobuf
which, when encoded in jsonpb, result in the expected values. Implementations
will typically treat `LUCI_CONTEXT` as pure JSON, but we'd like to make the
implementation more rigorous in the future (hence the strict schema
descriptions). Currently implementing `LUCI_CONTEXT` in terms of actual
protobufs would be onerous, given the way that this repo is deployed and used.

It's assumed that all of the field names in the proto snippets below EXACTLY
correspond to their encoded JSON forms. When encoding in golang, this would be
equivalent to specifying the 'OrigName' parameter in the Marshaller.

## `local_auth`

Local auth specifies where subprocesses can obtain OAuth2 tokens to use when
calling other services. It is a reference to a local RPC port, along with
some configuration of what this RPC service (called "local auth service") can
provide.

```
message LocalAuth {
  message Account {
    string id = 1;
    string email = 2;
  }

  int rpc_port = 1;
  bytes secret = 2;

  repeated Account accounts = 3;
  string default_account_id = 4;
}
```

...

The returned tokens MUST have expiration duration longer than 150 sec. Clients
of the protocol rely on this.

...

The email may be a special string `"-"` which means tokens produced by the auth
server are not associated with any particular known email. This may happen when
using tokens that don't have `userinfo.email` OAuth scope.

...

TODO(vadimsh): Finish this.


## `swarming`

This section describes data passed down from the
[swarming service](../appengine/swarming) to scripts running within swarming.

```
message Swarming {
  // The user-supplied secret bytes specified for the task, if any. This can be
  // used to pass application or task-specific secret keys, JSON, etc. from the
  // task triggerer directly to the task. The bytes will not appear on any
  // swarming UI, or be visible to any users of the swarming service.
  byte secret_bytes = 1;
}
```
