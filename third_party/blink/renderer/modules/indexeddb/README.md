# IndexedDB

IndexedDB is a browser storage mechanism that can efficiently store and retrieve
large amounts of structured data with a subset of the
[ACID](https://en.wikipedia.org/wiki/ACID) guarantees that are generally
associated with relational databases. IndexedDB
[enjoys wide cross-browser adoption](https://caniuse.com/#feat=indexeddb).

The [IndexedDB specification](https://w3c.github.io/IndexedDB/) is
[maintained on GitHub](https://github.com/w3c/IndexedDB/). The specification's
[issue tracker](https://github.com/w3c/IndexedDB/issues/) is the recommended
forum for discussing new feature proposals and changes that would apply to all
browsers implementing IndexedDB.

Mozilla's [IndexedDB documentation](https://developer.mozilla.org/en-US/docs/Web/API/IndexedDB_API)
is a solid introduction to IndexedDB for Web developers. Learning the IndexedDB
concepts is also a good first step in understanding Blink's IndexedDB
implementation.

## Documentation

Please add documents below as you write it.

* [Overview for Database People](/third_party/blink/renderer/modules/indexeddb/docs/idb_overview.md)
* [IndexedDB Data Path](/third_party/blink/renderer/modules/indexeddb/docs/idb_data_path.md)

## Design Docs

Please complete the list below with new or existing design docs.

* [Handling Large Values in IndexedDB](https://goo.gl/VncHrw)
