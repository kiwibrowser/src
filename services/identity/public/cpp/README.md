IdentityManager is the next-generation C++ API for interacting with Google
identity. It is currently backed by //components/signin (see IMPLEMENTATION
NOTES below); in the long-term it will serve as the primary client-side
interface to the Identity Service, encapsulating a connection to a remote
implementation of identity::mojom::IdentityManager. It provides conveniences
over the bare Identity Service Mojo interfaces such as:

- Synchronous access to the information of the primary account (via caching)

PrimaryAccountTokenFetcher is the primary client-side interface for obtaining
access tokens for the primary account. In particular, it takes care of waiting
until the primary account is available.

IMPLEMENTATION NOTES

The Identity Service client library is being developed in parallel with the
implementation and interfaces of the Identity Service itself. The motivation is
to allow clients to be converted to use this client library in a parallel and
pipelined fashion with building out the Identity Service as the backing
implementation of the library.

In the near term, this library is backed directly by //components/signin
classes. We are striving to make the interactions of this library with those
classes as similar as possible to its eventual interaction with the Identity
Service. In places where those interactions vary significantly from the
envisioned eventual interaction with the Identity Service, we have placed TODOs.
