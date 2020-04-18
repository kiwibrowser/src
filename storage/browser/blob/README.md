# Chrome's Blob Storage System Design

Elaboration of the blob storage system in Chrome.

# What are blobs?

Please see the [FileAPI Spec](https://www.w3.org/TR/FileAPI/) for the full
specification for Blobs, or [Mozilla's Blob documentation](
https://developer.mozilla.org/en-US/docs/Web/API/Blob) for a description of how
Blobs are used in the Web Platform in general. For the purposes of this
document, the important aspects of blobs are:

1. Blobs are immutable.
2. Blob can be made using one or more of: bytes, files, or other blobs.
3. Blobs can be ['sliced'](
https://developer.mozilla.org/en-US/docs/Web/API/Blob/slice), which creates a
blob that is a subsection of another blob.
4. Reading blobs is asynchronous.
5. Reading blob metadata (like size) is synchronous.
6. Blobs can be passed to other browsing contexts, such as Javascript workers
or other tabs.

In Chrome, after blob creation the actual blob 'data' gets transported to and
lives in the browser process. The renderer just holds a reference -
specifically a string UUID - to the blob, which it can use to read the blob or
pass it to other processes.

# Summary & Terminology

Blobs are created in a renderer process, where their data is temporarily held
for the browser (while Javascript execution can continue). When the browser has
enough memory quota for the blob, it requests the data from the renderer. All
blob data is transported from the renderer to the browser. Once complete, any
pending reads for the blob are allowed to complete. Blobs can be huge (GBs), so
quota is necessary.

If the in-memory space for blobs is getting full, or a new blob is too large to
be in-memory, then the blob system uses the disk. This can either be paging old
blobs to disk, or saving the new too-large blob straight to disk.

Blob reading goes through the network layer, where the renderer dispatches a
network request for the blob and the browser responds with the
`BlobURLRequestJob`.

General Chrome terminology:

* **Renderer, Browser, and IPCs**: See the [Multi-Process Architecture](
https://www.chromium.org/developers/design-documents/multi-process-architecture)
document to learn about these concepts.
* **Shared Memory**: Memory that both the browser and renderer process can read
& write. Created only between 2 processes.

Blob system terminology:

* **Blob**: This is a blob object, which can consist of bytes or files, as
described above.
* **BlobItem**:
This is a primitive element that can basically be a File, Bytes, or another
Blob. It also stores an offset and size, so this can be a part of a file. (This
can also represent a "future" file and "future" bytes, which is used to signify
a bytes or file item that has not been transported yet).
* **dependent blobs**: These are blobs that our blob is dependent on to be
constructed. As in, a blob is constructed with a dependency on another blob
(maybe it is a slice or just a blob in our constructor), and before the new
blob can be constructed it might need to wait for the "dependent" blobs to
complete. (This can sound backwards, but it's how it's referenced in the code.
So think "I am dependent on these other blobs")
* **transportation strategy**: a method for sending the data in a BlobItem from
a renderer to the browser. The system currently implements three strategies:
IPC, Shared Memory, and Files.
* **blob description**: the inital data sychronously sent to the browser that
describes the items (content and sizes) of the new blob. This can
optimistically include the blob data if the size is less than the maximimum IPC
size.

# Blob Storage Limits

We calculate the storage limits [here](
https://cs.chromium.org/chromium/src/storage/browser/blob/blob_memory_controller.cc?q=CalculateBlobStorageLimitsImpl&sq=package:chromium).

**In-Memory Storage Limit**

* If the architecture is x64 and NOT Chrome OS or Android: `2GB`
* If Chrome OS: `total_physical_memory / 5`
* If Android: `total_physical_memory / 100`


**Disk Storage Limit**

* If Chrome OS: `disk_size / 2`
* If Android: `6 * disk_size / 100`
* Else: `disk_size / 10`

Note: Chrome OS's disk is part of the user partition, which is separate from the
system partition.

**Minimum Disk Availability**

We limit our disk limit to accomidate a minimum disk availability. The equation
we use is:

`min_disk_availability = in_memory_limit * 2`

## Example Limits

| Device | Ram | In-Memory Limit | Disk | Disk Limit | Min Disk Availability |
| --- | --- | --- | --- | --- | --- |
| Cast | 512 MB | 102 MB | 0 | 0 | 0 |
| Android Minimal | 512 MB | 5 MB | 8 GB | 491 MB | 10 MB |
| Android Fat | 2 GB | 20 MB | 32 GB | 1.9 GB | 40 MB |
| CrOS | 2 GB | 409 MB | 8 GB | 4 GB | 0.8 GB |
| Desktop 32 | 3 GB | 614 MB | 500 GB | 50 GB | 1.2 GB |
| Desktop 64 | 4 GB | 2 GB | 500 GB | 50 GB | 4 GB |

# Common Pitfalls

## Creating Large Blobs Too Fast

Creating a lot of blobs, especially if they are very large blobs, can cause
the renderer memory to grow too fast and result in an OOM on the renderer side.
This is because the renderer temporarily stores the blob data while it waits
for the browser to request it. Meanwhile, Javascript can continue executing.
Transfering the data can take a lot of time if the blob is large enough to save
it directly to a file, as this means we need to wait for disk operations before
the renderer can get rid of the data.

## Leaking Blob References

If the blob object in Javascript is kept around, then the data will never be
cleaned up in the backend. This will unnecessarily use memory, so make sure to
dereference blob objects if they are no longer needed.

Similarily if a URL is created for a blob, this will keep the blob data around
until the URL is revoked (and the blob object is dereferenced). However, the
URL is automatically revoked when the browser context is destroyed.

# How to use Blobs (Browser-side)

## Building
All blob interaction should go through the `BlobStorageContext`. Blobs are
built using a `BlobDataBuilder` to populate the data and then calling
`BlobStorageContext::AddFinishedBlob` or `::BuildBlob`. This returns a
`BlobDataHandle`, which manages reading, lifetime, and metadata access for the
new blob.

If you have known data that is not available yet, you can still create the blob
reference, but see the documentation in `BlobDataBuilder::AppendFuture* or
::Populate*` methods on the builder, the callback usage on
`BlobStorageContext::BuildBlob`, and
`BlobStorageContext::NotifyTransportComplete` to facilitate this construction.

## Accessing / Reading

All blob information should come from the `BlobDataHandle` returned on
construction. This handle is cheap to copy. Once all instances of handles for
a blob are destructed, the blob is destroyed.

`BlobDataHandle::RunOnConstructionComplete` will notify you when the blob is
constructed or broken (construction failed due to not enough space, filesystem
error, etc).

The `BlobReader` class is for reading blobs, and is accessible off of the
`BlobDataHandle` at any time.

# Blob Creation & Transportation (Renderer)

**This process is outlined with diagrams and illustrations [here](
https://docs.google.com/presentation/d/1MOm-8kacXAon1L2tF6VthesNjXgx0fp5AP17L7XDPSM/edit#slide=id.g75c319281_0_681).**

This outlines the renderer-side responsabilities of the blob system. The
renderer needs to:

 1. Consolidate small bytes items into larger chunks (avoiding a huge array of
 1 byte items).
 2. Communicate the blob description to the browser immediately on
 construction.
 3. Populate shared memory or files sent from the browser with the consolidated
 blob data items.
 4. Hold the blob data until the browser is finished requesting it.

The meat of blob construction starts in the [WebBlobRegistryImpl](
https://cs.chromium.org/chromium/src/content/renderer/blob_storage/webblobregistry_impl.h)'s
`createBuilder(uuid, content_type)`.

## Blob Data Consolidation

Since blobs are often constructed with arrays with single bytes, we try to
consolidate all **adjacent** memory blob items into one. This is done in
[BlobConsolidation](https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_consolidation.h).
The implementation doesn't actually do any copying or allocating of new memory
buffers, instead it facilitates the transformation between the 'consolidated'
blob items and the underlying bytes items. This way we don't waste any memory.

## Blob Transportation (Renderer)

After the blob has been 'consolidated', it is given to the
[BlobTransportController](https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_transport_controller.h).
This class:

1. Immediately communicates the blob description to the Browser. We also
[optimistically send](https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_transport_controller.cc?l=325)
the blob data if the total memory is less than our IPC threshold.
2. Stores the blob consolidation for data requests from the browser.
3. Answers requests from the browser to populate or send the blob data. The
browser can request the renderer:
  1. Send items and populate the data in IPC ([code](
https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_transport_controller.cc?q="case+IPCBlobItemRequestStrategy::IPC")).
  2. Populate items in shared memory and notify the browser when population is
complete ([code](https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_transport_controller.cc?q="case+IPCBlobItemRequestStrategy::SHARED_MEMORY")).
  3. Populate items in files and notify the browser when population is complete
([code](https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_transport_controller.cc?q="case+IPCBlobItemRequestStrategy::FILE")).
4. Destroys the blob consolidation when the browser says it's done.

The transport controller also tries to keep the renderer alive while we are
sending blobs, as if the renderer is closed then we would lose any pending blob
data. It does this the [incrementing and decrementing the process reference
count](https://cs.chromium.org/chromium/src/content/renderer/blob_storage/blob_transport_controller.cc?l=62),
which should prevent fast shutdown.

# Blob Transportation & Storage (Browser)

The browser side is a little more complicated. We are thinking about:

1. Do we have enough space for this blob?
2. Pick transportation strategy for blob's components.
3. Is there enough free memory to transport the blob right now? Or does older
blob data to be paged to disk first?
4. Do I need to wait for files to be created?
5. Do I need to wait for dependent blobs?

## Summary

We follow this general flow for constructing a blob on the browser side:

1. Does the blob fit, and what transportation strategy should be used.
2. Create our browser-side representation of the blob data, including the data
items from dependent blobs. We try to share items as much as possible to save
memory, and allow for the dependent blob items to be not populated yet.
3. Request memory and/or file quota from the BlobMemoryController, which
manages our blob storage limits. Quota is necessary for both transportation and
any copies we have to do from dependent blobs.
4. If transporation quota is needed and when it is granted:
  1. Tell the BlobTransportHost to start asking for blob data given the earlier
  decision of strategy.
    * The BlobTransportHost populates the browser-side blob data item.
  2. When transportation is done we notify the BlobStorageContext
5. When transportation is done, copy quota is granted, and dependent blobs are
complete, we finish the blob.
  1. We perform any pending copies from dependent blobs
  2. We notify any listeners that the blob has been completed.

Note: The transportation sections (steps 1, 2, 3) of this process are described
(without accounting for blob dependencies) with diagrams and details in [this
presentation](https://docs.google.com/presentation/d/1MOm-8kacXAon1L2tF6VthesNjXgx0fp5AP17L7XDPSM/edit#slide=id.g75d5729ce_0_105).

## BlobTransportHost

The `BlobTransportHost` is in charge of the actual transportation of the data
from the renderer to the browser. When the initial description of the blob is
sent to the browser, the BlobTransportHost asks the BlobMemoryController which
strategy (IPC, Shared Memory, or File) it should use to transport the file.
Based on this strategy it can translate the memory items sent from the renderer
into a browser represetation to facilitate the transportation. See [this](
https://docs.google.com/presentation/d/1MOm-8kacXAon1L2tF6VthesNjXgx0fp5AP17L7XDPSM/edit#slide=id.g75d5729ce_0_145)
slide, which illustrates how the browser might segment or split up the
renderer's memory into transportable chunks.

Once the transport host decides its strategy, it will create its own transport
state for the blob, including a `BlobDataBuilder` using the transport's data
segment representation. Then it will tell the `BlobStorageContext` that it is
ready to build the blob.

When the `BlobStorageContext` tells the transport host that it is ready to
transport the blob data, the transport host requests all of the data from the
renderer, populates the data in the `BlobDataBuilder`, and then signals the
storage context that it is done.

## BlobStorageContext

The `BlobStorageContext` is the hub of the blob storage system. It is
responsible for creating & managing all the state of constructing blobs, as
well as all blob handle generation and general blob status access.

When a `BlobDataBuilder` is given to the context, whether from the
`BlobTransportHost` or from elsewhere, the context will do the following:

1. Find all dependent blobs in the new blob (any blob reference in the blob
item list), and create a 'slice' of their items for the new blob.
2. Create the final blob item list representation, which creates a new blob
item list which inserts these 'slice' items into the blob reference spots. This
is 'flattening' the blob.
3. Ask the `BlobMemoryManager` for file or memory quota for the transportation
if necessary
  * When the quota request is granted, notify the `BlobTransportHost` that to
  begin transporting the data.
4. Ask the `BlobMemoryManager` for memory quota for any copies necessary for
blob slicing.
5. Adds completion callbacks to any blobs our blob depends on.

When all of the following conditions are met:

1. The `BlobTransportHost` tells us it has transported all the data (or we
don't need to transport data),
2. The `BlobMemoryManager` approves our memory quota for slice copies (or we
don't need slice copies), and
3. All dependent blobs are completed (or we don't have dependent blobs),

The blob can finish constructing, where any pending blob slice copies are
performed, and we set the status of the blob.

### BlobStatus lifecycle

The BlobStatus tracks the construction procedure (specifically the transport
process), and the copy memory quota and dependent blob process is encompassed
in `PENDING_INTERNALS`.

Once a blob is finished constructing, the status is set to `DONE` or any of
the `ERR_*` values.

### BlobSlice

During construction, slices are created for dependent blobs using the given
offset and size of the reference. This slice consists of the relevant blob
items, and metadata about possible copies from either end. If blob items can
entirely be used by the new blob, then we just share the item between the. But
if there is a 'slice' of the first or last item, then our resulting BlobSlice
representation will create a new bytes item for the new blob, and store
necessary copy data for later.

### BlobFlattener

The `BlobFlattener` takes the new blob description (including blob references),
creates blob slices for all the referenced blobs, and constructs a 'flat'
representation of the new blob, where all blob references are replaced with the
`BlobSlice` items. It also stores any copy data from the slices.

## BlobMemoryController

The `BlobMemoryController` is responsable for:

1. Determining storage quota limits for files and memory, including restricting
file quota when disk space is low.
2. Determining whether a blob can fit and the transportation strategy to use.
3. Tracking memory quota.
4. Tracking file quota and creating files.
5. Accumulating and evicting old blob data to files to disk.

