# Share-by-hash: Decentralised sharing of identical files

Mark Seaborn, February 2011

_Draft_

**Please send comments to [native-client-discuss]
(https://groups.google.com/group/native-client-discuss)** rather than adding
them inline or using the comment form, because the wiki does not send
notifications about changes.

## Proposed scheme

Back in 2008, Douglas Crockford proposed the following browser extension:

> Any HTML tag that accepts a `src=` or `href=` attribute should also be allowed
> to take a `hash=` attribute. The value of a hash attribute would be the [base
> 32 encoding](http://www.crockford.com/wrmg/base32.html) of the [SHA]
> (http://en.wikipedia.org/wiki/SHA_hash_functions) of the object that would be
> retrieved. This does a couple of useful things.
>
> First, it gives us confidence that the file that we receive is the one that we
> asked for, that it was not replaced or tampered with in transit.
>
> Second, browsers can cache by hash code. If the cache contains a file that
> matches the requested `hash=`, then there is no need to go to the network
> regardless of the url. This would improve the performance of Ajax libraries
> because you would only have to download the library once for all of the sites
> you visit, even if every site links to its own copy.
>
> Posted by: Douglas Crockford on 03/25/2008

(I am quoting this in full because it is short and because it disappeared from
its original location. I retrieved it from
http://www.therichwebexperience.com/blog/douglas_crockford/2008/03/hash on
2011/02/04.)

In this document, we focus specifically on the second property, which we dub
_share-by-hash_.

## Potential problems

*   Privacy: This provides a way for a sneaky site to work out if you have
    visited some other site. Suppose alice.net links to a file with hash H.
    mallet.com can host a page P linking to hash H. If visiting page P doesn't
    produce a request for H from mallet.com, mallet.com can deduce that you've
    visited alice.net.

    *   Mitigation: alice.net could opt out of the share-by-hash scheme for file
        H. (However, alice.net would still want the benefit of using the hash
        for an integrity check.)

*   Leakability: It is easier to leak the hash of a file over a low-bandwidth
    covert channel than to leak the full contents of the file. A share-by-hash
    scheme could allow an attacker to retrieve the full contents after leaking
    the hash from a process that is supposed to be confined.

*   Hash collisions: Suppose many sites rely on a library file F with hash H. If
    there is a vulnerability in the crypto-hash scheme, an attacker might be
    able to construct a malicious library, F2, with the same hash H. If the
    attacker can get the user to visit their site, they can poison the cache to
    replace F with F2, and run code in the context of the sites that rely on F.

*   Site testability: Suppose example.net hosts a web app that uses a popular
    library. The developers link to the library as
    `href=http://example.net/poplib.js hash=H` but they forget to make the file
    available under `/poplib.js`. Since it's such a popular library, most users,
    including the developers, already have hash H in their browser cache. The
    developers don't notice the problem, but for some users, the web app
    mysteriously fails.

    *   Mitigation: Create tools to lint for these dangling links. Use
        development tools for constructing web apps that avoid this problem.

*   Inverting service: The browser cache could be used to invert hashes. If
    other systems make the assumption that hashes are not invertable,
    share-by-hash would violate this assumption. Is this assumption ever valid
    to make?

*   Upgrading hash scheme: What kind of plans do we need to make to change to a
    stronger hash algorithm if/when flaws are found in the current one?

## Extension: Direct cache queries

Share-by-hash as proposed by Douglas Crockford exposes a composite operation in
which the system queries the cache and falls back to fetching a URL:

```
function getCachedByHashAndUrl(hash, url) {
    var file = getCachedByHash(hash);
    if (file == null) {
        file = fetchUrl(url);
        if (hashData(file) != hash) {
            throwHashMismatchError();
        }
        addToHashCache(hash, file);
    }
    return file;
}
```

What are the implications of exposing the cache query, getCachedByHash(hash),
directly? Initially it appears there are none, because the information revealed
by getCachedByHash(hash) is also revealed by getCachedByHashAndUrl(hash, url).
We probably cannot enforce anything by forcing a web app to use
getCachedByHashAndUrl() rather than getCachedByHash(). However, we might be able
to impose costs for malfeasance.

Suppose an attacker is trying to invert a hash H, to find the data X for which
hash(X) = H. Suppose the attacker has access to getCachedByHashAndUrl() but not
getCachedByHash(). The attacker will not be able to supply a URL that returns X.
We could log cases where getCachedByHashAndUrl() does not return successfully.
Potentially, we could warn the user. However, this would trigger in innocent
cases such as when URLs fail to respond.

## Extension: PNaCl translation cache

The [PNaCl translation cache](pnacl_translation_cache.md) differs from the
browser cache in that we want to cache a function of a file, rather than the
file itself. A web app specifies a portable bitcode file as a (hash, URL) pair,
and PNaCl caches the translated version of the bitcode file (an
architecture-specific ELF file).

In this scenario, there is no need to keep the bitcode file around: it can be
thrown away after the translated version is computed.

## Related work

Related concepts: * "self-certifying names": I believe this term was introduced
by the Self-certifying File System (SFS) * "strong names": I believe this term
was introduced by Microsoft in .NET * content-addressable filesystems

Related systems: * Share-by-hash was implemented by the EC Habitats system, and
this is described by Mark Miller in "Anonymous Unspoofable Cyclic Distributed
Static Linking" (January 2000): [part 1]
(http://www.eros-os.org/pipermail/e-lang/2000-January/003188.html), [part 2]
(http://www.eros-os.org/pipermail/e-lang/2000-January/003194.html). * Tahoe-LAFS
is a content-addressable filesystem. It can provide share-by-hash, but it also
provides a way to turn it off on privacy grounds. * Git, Mercurial, Monotone:
These DVCSes implement content-addressable filesystems. However, share-by-hash
is scoped to within a repository.

Discussions relevant to share-by-hash: * [An Analysis of Compare-by-hash]
(http://valerieaurora.org/review/hash.html) ([PDF]
(http://valerieaurora.org/review/hash.pdf)), Val Henson, 2003. *
[Compare-by-Hash: A Reasoned Analysis]
(http://www.cs.colorado.edu/~jrblack/papers/cbh.html), J. Black, April 2006 *
Brendan Eich [responded](http://brendaneich.com/2008/04/popularity/) to Douglas
Crockford's proposal: "One idea, mooted by many folks, most recently here by
Doug, entails embedding crypto-hashes in potentially very long-lived script tag
attributes. Is this a good idea? Probably not, based both on theoretical
soundness concerns about crypto-hash algorithms, and on well-known poisoning
attacks."
