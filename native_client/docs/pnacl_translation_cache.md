_Draft_

# Table of Contents

# Introduction

For Portable Native Client [PNaCl]
(http://nativeclient.googlecode.com/svn/data/site/pnacl.pdf), we wish to be able
to cache the translation of portable **bitcode files** _to_
architecture-specific **ELF files**. The question we wish to answer in this
document is: "What should manage the cache"? Should we implement caching as a
web app, or implement it in the browser?

Here is a typical scenario: a user just opened multiple window tabs, each trying
to load the same PNaCl app composed of N bitcode files from M different domains.
These bitcode files need to be translated to ELF files that are eventually used
by the dynamic linker (a nexe). The system should cache the ELF files to avoid
re-translating the bitcode when unchanged.

Here is the high-level flow for a single bitcode file:

```

  function getPnaclTranslation(url, hash) {
    var elf_file = GetFromCache(hash);
    if (!elf_file) {
      var pnacl_file = plugin.fetchUrl(url);

      // Compute hash, since bitcode may have changed in the meantime.
      var hash2 = CalcHash(pnacl_file);

      // May want to simply stop and retry if hash != hash2

      var elf_file_w = CreateCacheEntry(hash2);

      Translate(pnacl_file, elf_file_w);

      elf_file = MakeReadOnly(elf_file_w);
    }
    return elf_file;
  }
  // Dynamic Linker will use the elf_file...

```

Note: for details on specifying URLs of libraries, see
NameResolutionForDynamicLibraries.

Here is a diagram of how the components interact:

```

 [ Bitcode1 ] <------\
 [ Bitcode2 ] <-------\
 [ PNaCl App.html ] <-----> [ Cache Manager ] <--> [ Translator.nexe ]
 [ Dyn Linker.nexe ] <--/

```

All boxes besides the Cache Manager represent data and nexes that are likely
from different domains.

## Requirements

This cache and translation system must at the very least satisfy the following
requirements:

*   Handle concurrency.
    *   Avoid redundant concurrent translations.
    *   Avoid races in writing to the cache.
*   Supply file descriptors to the translator nexe.
*   The cross-domain fetch permission should propagate to the derived data
    (translated ELF files in the cache).
    *   I.e., the ELF files should be available to the consumer (dynamic linker)
        that is pulled from another domain.
*   Supply file descriptors of cache entries to dynamic linker.
*   Evict and unlink files when at capacity.
*   Use immutable files so that the linker will be able to map files directly
    instead of copying the data.

Other (important) requirements:

*   Be sure not to block the UI threads.
*   Handle crashes: Cache "put" operation should be atomic. Put another way, the
    cache should never return a partially-written file.
*   Do not cache the bitcode files (only the ELF files), since the bitcode files
    can be quite large and are redundant.
*   Clear cache when browser cache is cleared, or via other mechanisms.
    *   Useful for deterministic testing.
    *   Possibly relevant to privacy / not revealing browsing history via
        timing.
*   Retain ability to test translator as a standalone program.
*   Compatibility with third-party translator.nexes (e.g., one with fewer
    optimizations but faster startup), or multiple versions of translator.nexes.
*   Reuse cache mechanism for other purposes (e.g., Mono assemblies).
*   Have per-app, per-domain, per-X quotas.

Other issues to consider:

*   Portability of approach to other browsers.
*   Do we reuse translator instances to translate multiple bitcode files?

# Options

There are at least two options to consider:

*   Option A: Manage the cache via a web app and back it with HTML FS.
*   Option B: Manage the cache in a browser broker process and back it with real
    files in a reserved directory.

Let's consider how the requirements will be satisfied by each option.

## Option A: Manage the cache via a web app, backed by HTML FS

Being a Web App, it will need storage. The only HTML storage option that appears
to have the capacity and capabilities required is the HTML Filesystem API
([spec](http://dev.w3.org/2009/dap/file-system/pub/FileSystem/), [demo]
(http://www.html5rocks.com/tutorials/file/filesystem/)). This filesystem
provides domain-specific storage.

Let's go over what is needed:

*   Manage concurrent requests

    *   [SharedWorkers]
        (http://dev.w3.org/html5/workers/#shared-workers-introduction) allow us
        to refer to a single Javascript worker across different windows. This
        single Javascript worker can handle serialization.

*   Get file descriptors out of files in cache

    *   Ideally we would have urlAsNaClDesc apply to HTML FS based URLs.
    *   i.e., fd = plugin.urlAsNaClDesc(fileEntry.toURI())

*   Pass file descriptors out of the cache to dynamic linker and translator
    nexes.

    *   Since the Cache Web App is in a different domain from the other
        components, we need to extend [postMessage]
        (http://www.whatwg.org/specs/web-apps/current-work/multipage/comms.html#web-messaging)
        to send FDs across domains.
    *   FDs will need to be wrapped to pass them around in Javascript.
    *   In that case, will FDs close when JS ref-counts go to zero?

*   Unlink files

    *   Use [fileEntry.remove]
        (http://dev.w3.org/2009/dap/file-system/pub/FileSystem/#widl-Entry-remove)(succCB,
        errCB). The spec doesn't specify the semantics if a file is still in
        use. A worry is that, depending on the implementation on Windows, you
        may an error, and this would introduce platform differences (if the
        files are created on Windows without FILE\_SHARE\_DELETE).
    *   Perhaps we can just gracefully handle errors.
    *   We can only gracefully handle the case where remove() gets the error,
        not the reader/writer getting the error.
    *   If the cache is full, we would have to delay a cache-put until a remove
        is finally successful. If we cannot unlink anything (all files are
        busy), then we would have to fail to add to the cache and the subsequent
        load will fail.

*   Immutable files

    *   The Cache Web App does have a domain-isolated HTML FS, but how do we
        feel about making it trusted code?

Extensions made will be available to all Web Apps and not used solely by the
translation cache.

In summary, the extensions appear to be:

*   plugin.urlAsNaClDesc(fileEntry.toURI())
*   window.postMessage for an FD
*   Make semantics of unlinking concrete

## Option B: Manage the cache in a browser broker process, backed by a single directory

With this option, the browser will have a Cache Broker Process that is given
access to a single directory in the native file system. We do not extend the
capabilities of web apps (besides supplying a bitcode translation cache).

*   Manage concurrent requests

    *   We will need a broker process in the browser.

*   Get file descriptors out of files in cache

    *   Trivial to get.

*   Pass file descriptors out of the cache to dynamic linker and translator
    nexes.

    *   Use chrome IPC between cache broker and NaCl plugins.
    *   Re: garbage collection -- ownership transfer from cache to consumer.

*   Unlink files

    *   Chrome correctly uses FILE\_SHARE\_DELETE so that in-use files can still
        be marked for deletion.

*   Immutable files

    *   The cache broker manages the directory. One problem is that files are
        externally visible, but if that is the only way to modify cached files
        then you have already been owned?

# Variants of Options

Here, we explore variations on the above options.

## Option B-prime: Manage the cache in a browser broker process, backed by browser cache

We could go with Option B, but instead of giving the cache broker ownership of a
fresh/isolated directory we could reuse the browser cache ([e.g., the Chromium
Disk Cache]
(http://www.chromium.org/developers/design-documents/network-stack/disk-cache)).

*   The chrome cache has an option to store entries as as individual files, so
    we can still get FDs.
    *   Will need to investigate.
*   Chrome already has separate caches for video, etc., so we can still tune the
    size of the cache, don't have to worry about competing with other types of
    files for cache space, etc.
*   We get to reuse the chrome cache meta-data for LRU, journaling, clearing the
    cache happens naturally, etc.

## Option B-double-prime: Manage the cache in the browser process, backed by browser cache

This is a variation of Option B where we add logic to the browser process
instead of having a separate broker process.

*   May be easiest for a first-cut.
*   Do not need to wait for a general Pepper Broker Abstraction.

# Current Leanings

Go with Option B double-prime, since it does not need to face unspecified
behavior (unlink behavior in the FS), and does not require extensions to the web
app platform. We also get to reuse much of the chrome code including its cache.
The drawback is that this appears to be chrome-specific. However, every browser
has a browser cache, so presumably this can be done for other browsers. Once
Pepper has a broker abstraction, we could move this cache logic out of the
browser process into a broker process.

# Other Requirements / Issues

This section explores the secondary requirements and issues related to
client-side translation caches.

## Compatiblity with Alternative Translator.nexes

We can create a separate cache instance based on a hash of the translator.nexe.
TODO: Look into the details of doing so with each option.

## The Translator May Consist of Multiple Components

The "translator" component will actually involve two nexes (llc and ld) for
quite a while. It will not be a single nexe until llc is taught all of the
linker logic -- optimize TLS accesses, generate GOT and PLT entries, generate
DT\_NEEDED, etc.. This also means that:

*   We may need two files -- one more for the intermediate file -- instead of
    just the one for the final output. Alternatively, we can try to use shared
    memory tagged with an explicit file size for the intermediate data.
*   We need to be able to atomically update the translator and linker. This is
    possible if we specify them in the manifest file and update the hashes
    (assuming we use the hashing scheme) atomically.

## Notes on Persisting Instances of the Translator to Translate Multiple BC files

Persisting and reusing instances of the translator nexe will save on startup
cost. However, even if we limit this persistence/sharing to a single domain
there are ways in which a malicious individual could affect multiple domains.
Mark's example:

*   Translator starts up
*   Translator receives a malicious.bc and gets owned
*   Translator behavior is now changed
*   Changed translator persists and mistranslates domainA:libgood.so.bc, which
    has hash X. The mistranslation doubles numeric literals that may refer to
    prices.
*   Next time another app wants to use domainB:libgood.so.bc which also has hash
    X. Since we only check hashes in the cache, the dynamic linker will be
    supplied with the cached mistranslated version.

## Not Storing Bitcode vs Not Using Hashes

See the ShareByHash document for background. It is possible to detect that files
have changed without ShareByHash using the typical Browser Cache mechanisms
(e.g., Timestamp + [ETags](http://en.wikipedia.org/wiki/HTTP_ETag)). That option
has the following implications:

*   We would need to store the bitcode file in the browser cache, since we need
    to know if the bitcode has changed. The bitcode files are typically larger
    than the ELF files and are redundant.
*   This would imply using Option B, since it is unlikely that the Web App
    Platform would be extended with an API for checking if data is already in
    the browser cache.
*   ETags have zero guarantees in the case that the same bitcode file is hosted
    at different URLs (we would not be able to share cached ELF files).
*   Unknown: How to allow developers to opt-in to silent updates of libraries
    like libpepper and libc with the content-hash scheme.

# Related

*   DSO name resolution design document: NameResolutionForDynamicLibraries
*   Share-by-hash design document: ShareByHash
