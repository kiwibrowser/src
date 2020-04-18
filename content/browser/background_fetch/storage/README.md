# Background fetch state storage

`content/browser/background_fetch/storage` contains a set of tasks that read to
and write from the service worker database, to store the state of the currently
running background fetches.

`DatabaseTask` is the abstract base class that all other tasks extend.

## Service Worker DB UserData schema

[Design doc](https://docs.google.com/document/d/1-WPPTP909Gb5PnaBOKP58tPVLw2Fq0Ln-u1EBviIBns/edit)

- Each key will be stored twice by the Service Worker DB, once as a
  "REG\_HAS\_USER\_DATA:", and once as a "REG\_USER\_DATA:" - see
  content/browser/service\_worker/service\_worker\_database.cc for details.
- Non-padded integer values are serialized as a string by base::Int\*ToString().
### Keys and values
```
key: "bgfetch_active_registration_unique_id_<developer_id>"
value: "<unique_id>"
```

```
key: "bgfetch_registration_<unique_id>"
value: "<serialized content::proto::BackgroundFetchMetadata>"
```

```
key: "bgfetch_title_<unique_id>"
value: "<ui_title>"
```

```
key: "bgfetch_pending_request_<unique_id>_<request_index>"
value: "<serialized content::proto::BackgroundFetchPendingRequest>"
```

```
key: "bgfetch_active_request_<unique_id>_<request_index>"
value: "<serialized content::proto::BackgroundFetchActiveRequest>"
```

```
key: "bgfetch_completed_request_<unique_id>_<request_index>"
value: "<serialized content::proto::BackgroundFetchCompletedRequest>"
```

### Expansions
* `<unique_id>` is a GUID (v4) that identifies a background fetch registration.
E.g.  `17467386-60b4-4c5b-b66c-aabf793fd39b`
* `<developer_id>` is a string provided by the developer to differentiate
background fetches within a service worker. As such it may contain any
characters and so it should be used very carefully within keys as it may
introduce ambiguity.
* `<request_index>` is an `int` containing the index of a request within a
multi-part fetch. These must be padded with zeros to ensure that the ordering
is maintain when reading back from the database, e.g. `0000000000`.
* `<ui_title>` is the notification title provided by the developer. It can also
be updated by calling `BackgroundFetchUpdateEvent.updateUI`.

