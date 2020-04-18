# Services in Content

The //content layer implements the core process model for Chrome, including
major process types: `browser`, `renderer`, `gpu`, `utility`, `plugin`. From the
perspective of the service manager, each process type is a service, and each
process instance is an instantiation of that service. For a renderer process,
its `service_manager::Identity` is constructed as follows:

```
name: content_renderer
userid: <guid, from BrowserContext that spawned this renderer>
instance: <string, generated from the RenderProcesHost's ID>
```

These services express the set of capabilities they expose to one another using
service manifests (see [Service Manager README](https://chromium.googlesource.com/chromium/src/+/master/services/service_manager/README.md)). For //content, the service manifests live in
`//content/public/app/mojo`. 

Every `content::BrowserContext` has a user id generated for it upon
construction, and the services run with that BrowserContext use that user id as
part of their instance identity. Where there are multiple instances of the same
service for the same user, the instance field in the Identity disambiguates
them.

Launching code for each process type is currently ad-hoc & specific per type,
and lives in `//content/browser`. In the medium-long term, we'll work to
generalize this and move it all into the service manager. 
Each content process type is launched by host code in `//content/browser`,
though eventually all process launching will be moved to the service manager.

The canonical service for each process type is represented by an implementation
of the `service_manager::Service` interface which lives on the IO thread. This
implementation is shared, and is a detail of `content::ServiceManagerConnection`
which you will find in `//content/public/common`. This implementation receives
the `OnStart()` and `OnBindInterface()` calls from the service manager.

The rest of this document talks about things you might like to do and how to
accomplish them.

### Expose Mojo interfaces from one of the existing content-provided services.

To expose interfaces at the service-level from one of the existing content-
provided services, you will need to add a `content::ConnectionFilter` to the
`content::ServiceManagerConnection` in the relevant process. See
`//content/public/common/connection_filter.h`. You implement this interface to
handle `OnBindInterface()` requests on the IO thread. You can construct a
`service_manager::BinderRegistry` on any other thread and move it to the IO
thread using `//content/public/common/connection_filter_impl.h`. When you add
bind callbacks to the binder registry you can specify what task runner you
would like incoming interface requests to be bound on.

### Expose Mojo interfaces at the frame level between browser & renderer.

You can add bind callbacks to the `service_manager::InterfaceRegistry` owned by
the `RenderFrame` and the `RenderFrameHost`. See the various content client
interfaces also for signals to embedders allowing them to add additional
interfaces.

### Expose a named service from an existing process.

If you want to expose a named service (i.e. a service other than the ones
provided by content) from a process provided by content, you can "embed" a
service in one of the content-provided services. You do this by calling
`AddEmbeddedService()` on `ServiceManagerConnection`.

