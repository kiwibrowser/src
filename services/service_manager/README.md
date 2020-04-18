# Service Manager User Guide

[TOC]

## What is the Service Manager?

The **Service Manager** is a tool that brokers connections and capabilities
between --  and manages instances of -- system components referred to henceforth
as **services**.

The Service Manager performs the following functions:

* Brokers interface requests between service instances, enforcing static
  capability policies declared by the services involved.
* Launches and manages the lifecycle of services and processes.
* Isolates service instances and interface requests among them according to
  user identity.
* Tracks running service instances and exposes privileged APIs for querying
  system state.

The Service Manager presents a series of Mojo
[interfaces](https://cs.chromium.org/chromium/src/services/service_manager/public/mojom/)
to services, though in practice most interaction with the Service Manager is
made simpler by using its corresponding
[C++ client library](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/).

## Mojo Recap

The Mojo system provides two key components of interest here - a lightweight
message pipe concept allowing two endpoints to communicate, and a bindings layer
that allows interfaces to be described to bind to those endpoints, with
ergonomic bindings for languages used in Chrome.

Mojo message pipes are designed to be lightweight and may be read from/written
to and passed around from one process to another. In most situations a developer
won't interact with the pipes directly, but rather with bindings types generated
to encapsulate a bound interface. To use the bindings, a developer defines their
interface in the [Mojom IDL format](/mojo/public/tools/bindings). With some
build magic, the generated definitions can then be referenced from C++,
JavaScript and Java code.

See the [Mojo documentation](/mojo) for a complete overview, detailed
explanations, and API references.

## Services

A **service** is a collection of one or more private implementations of public
Mojo interfaces which are reachable via the Service Manager. Every service is
comprised of the following pieces:

* A set of public Mojo interface definitions
* A **service manifest** declarating arbitrarily named capabilities which are
  each comprised of one or more exposed Mojo interfaces.
* Private implementation code which responds to lifecycle events and incoming
  interface requests, all driven by the Service Manager.

The Service Manager is responsible for starting new service instances on-demand,
and a given service many have any number of concurrently running instances. The
Service Manager disambiguates service instances by their unique **identity**. A
service's identity is represented by the 3-tuple of its **service name**, **user
ID**, and **instance qualifier**:

* The service name is a free-form -- typically short -- string identifying the
  the specific service being run in the instance.
* The user ID is a GUID string representing the identity of a user in the system.
  Every running service instance is associated with a specific user ID.
* Finally, the instance qualifier is an arbitrary free-form string used to
  disambiguate multiple instances of a service for the same user.

As long as a service instance is running it must maintain an implementation of
the
[`service_manager.mojom.Service`](https://cs.chromium.org/chromium/src/services/service_manager/public/mojom/service.mojom)
interface. Typically this is done in C++ code by implementing the C++ client
library's
[`service_manager::Service`](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/service.h)
interface. This interface is driven by messages from the Service Manager and is
used to receive incoming interface requests the Service Manager brokers from
other services.

Every service instance also has an outgoing link back to the Service Manager
which it can use to make interface requests to other services in the system.
This is the
[`service_manager.mojom.Connector`](https://cs.chromium.org/chromium/src/services/service_manager/public/mojom/connector.mojom)
interface, and it's commonly used via the C++ client library's
[`service_manager::Connector`](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/connector.h)
class.

## A Simple Service Example

This section walks through the creation of a simple skeleton service.

### Private Implementation

Consider this implementation of the `service_manager::Service` interface:

**`//services/my_service/my_service.h`**
``` cpp
#include "base/macros.h"
#include "services/service_manager/public/cpp/service.h"

namespace my_service {

class MyService : public service_manager::Service {
 public:
  MyService();
  ~MyService() override;

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::ServiceInfo& remote_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle handle) override;
 private:
  DISALLOW_COPY_AND_ASSIGN(MyService);
};

}  // namespace my_service
```

**`//services/my_service/my_service.cc`**
``` cpp
#include "services/my_service/my_service.h"

namespace my_service {

MyService::MyService() = default;

MyService::~MyService() = default;

void MyService::OnStart() {
}

void MyService::OnBindInterface(const service_manager::ServiceInfo& remote_info,
                                const std::string& interface_name,
                                mojo::ScopedMessagePipeHandle handle) {
}

}  // namespace my_service
```

### Main Entry Point

While services do not need to define a main entry point -- *e.g.* they may only
intend to be embedded in other running processes -- for the sake of completeness
we also define a `ServiceMain` definition so that the service can be run in its
own process:

**`//services/my_service/my_service_main.cc`**
``` cpp
#include "services/my_service/my_service.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"

MojoResult ServiceMain(MojoHandle service_request_handle) {
  return service_manager::ServiceRunner(new MyService).Run(
      service_request_handle);
}
```

### Manifest

A static manifest is provided to the Service Manager by each service to declare
the capabilities exposed and required by the service:

**`//services/my_service/manifest.json`**
``` json
{
  "name": "my_service",
  "display_name": "My Service",
  "interface_provider_specs": {
    "service_manager:connector": {}
  }
}
```

See [Service Manifests](#Service-Manifests) for more information.

### Build Targets

Finally some build targets corresponding to the above things:

**`//services/my_service/BUILD.gn`**
``` python
import("//services/service_manager/public/cpp/service.gni")
import("//services/service_manager/public/service_manifest.gni")

source_set("lib") {
  public = [ "my_service.h" ]
  sources = [ "my_service.cc" ]

  public_deps = [
    "//base",
    "//services/service_manager/public/cpp",
  ]
}

service("my_service") {
  sources = [
    "my_service_main.cc",
  ]
  deps = [
    ":lib",
    "//services/service_manager/public/c",
  ]
}

service_manifest("manifest") {
  name = "my_service"
  source = "manifest.json"
}
```

Building the `my_service` target produces a `my_service.service` (or on Windows,
`my_service.service.exe`) binary in the output directory. This can be run as
a standalone executable, but it will exit immediately without doing anything
interesting, because it won't have a `Service` pipe to drive it. The Service
Manager knows how to provide such a pipe when launching a service executable.

This service doesn't do much of anything. It will simply run forever (or at
least until the Service Manager itself shuts down), ignoring all incoming
messages. Before we expand on the definition of this service, let's look at some
of the details of the `service_manager::Service` interface.

### OnStart

The `Service` implementation is guaranteed to receive a single `OnStart()`
invocation from the Service Manager before anything else hapens. Once this
method is called, the implementation can access its
`service_manager::ServiceContext` via `context()`. This object itself exposes a
few values:

* `service_info()` is a `service_manager::ServiceInfo` structure describing the
  running service from the Service Manager's perspective. This includes the
  `service_manager::Identity` which uniquely identifies the running instance,
  as well as the `service_manager::InterfaceProviderSpec` describing the
  capability specifications outlined in the service's manifest.
* `identity()` is a shortcut to the `Identity` stored in the
  `ServiceInfo`.
* `connector()` is a `service_manager::Connector` which can be used to make
  outgoing interface requests to other services.

For example, we could modify `MyService` to connect out to logger service on
startup:

``` cpp
void MyService::OnStart() {
  logger::mojom::LoggerPtr logger;
  context()->connector()->BindInterface("logger", &logger);
  logger->Log("Started MyService!");
}
```

### OnBindInterface

The `OnBindInterface` method on `service_manager::Service` is invoked by the
Service Manager any time another service instance uses its own `Connector` to
request an interface from this `my_service` instance. The Service Manager only
invokes this method once it has already validated that the request meets the
mutual constraints specified in each involved service's manifest.

The arguments to `OnBindInterface` are as follows:

* `remote_info` is the `service_manager::ServiceInfo` corresponding to the
  remote service which is requesting this interface. The information in this
  structure is provided authoritatively by the Service Manager and can be
  trusted in any context.
* `interface_name` is the (`std::string`) name of the interface being requested
  by the remote service. The Service Manager has already validated that the
  remote service requires at least one capability which exposes this interface
  from the local service.
* `handle` is the `mojo::ScopedMessagePipeHandle` of an interface pipe which
  the remote service expects us to bind to a concrete implementation of
  the requested interface.

The Service Manager client library provides a `service_manager::BinderRegistry`
class definition which can make it easier for services to bind incoming
interface requests. Typesafe binding callbacks are added to an `BinderRegistry`
ahead of time, and the incoming arguments to `OnBindInterface` can be forwarded
to the registry, which will bind the message pipe if it knows how. For example,
we could modify our `MyService` implementation as follows:

``` cpp
namespace {

void BindDatabase(my_service::mojom::DatabaseRequest request) {
  mojo::MakeStrongBinding(std::make_unique<my_service::DatabaseImpl>(),
                          std::move(request));
}

}  // namespace

MyService::MyService() {
  // Imagine |registry_| is added as a member of MyService, with type
  // service_manager::BinderRegistry.

  // The |my_service::mojom::Database| interface type is inferred by the
  // compiler in the AddInterface call, and this effectively adds the bound
  // function to an internal map keyed on the interface name, i.e.
  // "my_service::mojom::Database" in this case.
  registry_.AddInterface(base::Bind(&BindDatabase));
}

void MyService::OnBindInterface(const service_manager::ServiceInfo& remote_info,
                                const std::string& interface_name,
                                mojo::ScopedMessagePipeHandle handle) {
  registry_.BindInterface(interface_name, std::move(handle));
}
```

For more details regarding the definition of Mojom interfaces, implementing them
in C++, and working with C++ types like `InterfaceRequest`, see the
[Mojom IDL and Bindings Generator](/mojo/public/tools/bindings) and
[Mojo C++ Bindings API](/mojo/public/cpp/bindings) documentation.

## Service Manifests

If some service were to come along and attempt to connect to `my_service` and
bind the `my_service::mojom::Database` interface, we might see the Service
Manager spit out an error log complaining that `InterfaceProviderSpec` prevented
a connection to `my_service`.

In order for the interface to be reachable by other services, we must first fix
its manifest's **interface provider spec**. The interface provider spec is
a dictionary keyed by **interface provider name**, with each value representing
the **capability spec** for that provider.

Each capability spec defines an optional `"provides"` key and an optional
`"requires"` key.

The `provides` key value is a dictionary which is itself keyed by arbitrary
free-form strings (capability names, implicitly scoped to the manifest's own
service) whose values are lists of Mojom interface names exposed as part of that
capability.

The `requires` key value is also a dictionary, but it's one which is keyed by
remote service name. Each value is a list of capabilities required from the
corresponding remote service.

Finally, every interface provider spec (often exclusively) contains one standard
capability spec named "service_manager:connector". This is the capability spec
enforced when inter-service connections are made from a service's `Connector`
interface.

Let's update the `my_service` manifest as follows:

**`//services/my_service/manifest.json`**
``` json
{
  "name": "my_service",
  "display_name": "My Service",
  "interface_provider_specs": {
    "service_manager:connector": {
      "provides": {
        "database": [
          "my_service::mojom::Database"
        ]
      }
    }
  }
}
```

This means that `my_service` has defined a `database` capability comprised
solely of the `my_service::mojom::Database` interface. Any service which
requires this capability can bind that interface from `my_service`.

For the sake of this example, let's define another service manifest:

**`//services/other_service/manifest.json`**
``` json
{
  "name": "other_service",
  "display_name": "Other Service",
  "interface_provider_specs": {
    "service_manager:connector": {
      "requires": {
        "my_service": [ "database" ]
      }
    }
  }
}
```

Now if `other_service` attempts to bind the database interface:

``` cpp
void OtherService::OnStart() {
  my_service::mojom::DatabasePtr database;
  context()->connector()->BindInterface("my_service", &database);
  database->AddTable(...);
}
```

The Service Manager will approve of the request and forward it on to the
`my_service` instance's `OnBindInterface` method.

## Testing

Now that we've built a simple service it's time to write a test for it.
The Service Manager client library provides a test fixture base class in
[`service_manager::test::ServiceTest`](https://cs.chromium.org/chromium/src/services/service_manager/public/cpp/service_test.h) that makes writing service integration tests straightforward. This test fixture
runs an in-process Service Manager on a background thread which allows test
service instances to be injected at runtime.

Let's look at a simple test of our service:

**`//services/my_service/my_service_unittest.cc`**
``` cpp
#include "base/bind.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "path/to/some_interface.mojom.h"

class MyServiceTest : public service_manager::test::ServiceTest {
 public:
  // Our tests run as service instances themselves. In this case each instance
  // identifies as the service named "my_service_unittests".
  MyServiceTest() : service_manager::test::ServiceTest("my_service_unittests") {
  }

  ~MyServiceTest() override {}
}

TEST_F(MyServiceTest, Basic) {
  my_service::mojom::DatabasePtr database;
  connector()->BindInterface("my_service", &database);

  base::RunLoop loop;

  // This assumes DropTable expects a response with no arguments. When the
  // response is received, the RunLoop is quit.
  database->DropTable("foo", loop.QuitClosure());

  loop.Run();
}
```

If adding a new test binary for these tests, we can augment our `BUILD.gn` to
use the `service_test` GN template like so:

**`//services/my_service/BUILD.gn`**
``` cpp
import("//services/catalog/public/tools/catalog.gni")
import("//services/service_manager/public/tools/test/service_test.gni")

service_test("my_service_unittests") {
  sources = [
    "my_service_unittest.cc",
  ]
  deps = [
    "//services/my_service/public/interfaces",
  ]
  catalog = ":my_service_unittests_catalog"
}

service_manifest("my_service_unittests_manifest") {
  name = "my_service_unittests"
  manifest = "my_service_unittests_manifest.json"
}

catalog("my_service_unittests_catalog") {
  testonly = true
  embedded_services = [ ":my_service_unittests_manifest" ]
  standalone_services = [ ":manifest" ]
}
```

Alright, there's a lot going on here. First we also have to create a service
manifest for the test service itself, as the Service Manager needs to be able
to reason about the test's own required capabilities with respect to the
service-under-test.

We can do something like:

**`//services/my_service/my_service_unittests_manifest.json`**
``` json
{
  "name": "my_service_unittests",
  "display_name": "my_service tests",
  "interface_provider_specs": {
    "service_manager:connector": {
      "requires": {
        "my_service": [ "database" ]
      }
    }
  }
}
```

You may also notice that we have suddenly introduced a **catalog** in the
`service_test` target incantation. Any runtime environment which hosts a
Service Manager must provide the Service Manager implementation with a catalog
of service manifests. This catalog defines the complete set of services
recognized by the Service Manager instance and can be used in all kinds of
interesting ways to control how various services are started in the system. See
[Service Manager Catalogs](#Service-Manager-Catalogs) for more information.

For now let's just accept that we have to create a `catalog` rule for our test
suite and plug it into the `service_test` target.

In practice, we typically try to avoid introducing new unittest binaries for
individual services. Instead we have an aggregate `service_unittests` target
defined in [`//services/BUILD.gn`](https://cs.chromium.org/chromium/src/services/BUILD.gn).
There are several examples of other services adding their service tests to this
suite.

## Service Manager Catalogs

A **catalog** is an aggregation of service manifests which comprises a complete
runtime configuration of the Service Manager.

The GN `catalog` target template defined in
[`//services/catalog/public/tools/catalog.gni`](https://cs.chromium.org/chromium/src/services/catalog/public/tools/catalog.gni).
provides a simple means of aggregating service manifests into a single build
artifact. See the comments on the template for detailed documentation.

This GNI also defines a `catalog_cpp_source` target which can generate a static
C++ representation of an aggregated catalog manifest so that it can be passed
the Service Manager at runtime.

In general, service developers should never be concerned with creating new
catalogs or instantiating the Service Manager, but it's important to be aware
of these concepts. When introducing a new service into any runtime environment
-- including Chrome, Content, or various unit test suites such as
`service_unittests` discussed in the previous section -- your service manifest
must be added to the catalog used in that environment.

TODO - expand on this

## Packaging Services

TODO

## Chrome and Chrome OS Service Manager Integration

TODO
