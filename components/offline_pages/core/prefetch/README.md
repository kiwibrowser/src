# Prefetching Offline Pages

## Architecture overview

### PrefetchService

Is the ownership holder for the main components of the prefetching system and
controls their lifetime.

### PrefetchDispatcher

Manages the prefetching pipeline tasks. It receives signals from external
clients and creates the appropriate tasks to execute them. It _might_ at some
point execute advanced task management operations like canceling queued tasks or
changing their order of execution.

### \*Task(s) (i.e. AddUniqueUrlsTask)

They are the main wrapper of pipeline steps and interact with different 
abstracted components (Downloads, persistent store, GCM, etc) to execute them.
They implement TaskQueue's Task API so that they can be exclusively executed.

## Prefetch store

* The PrefetchStore depends publicly on SQL and acts as a gateway to the SQLite
  database.
* It defines specific method signatures used to create callbacks that are passed
  to the store for the proper execution of SQL commands.
* SQL access resources are granted to those callbacks only when needed and in an
  appropriate environment (correct thread, etc).
* Pipeline tasks define methods following those signatures that contain the SQL
  commands they require to do their work.
* Tasks receive a pointer to the store to be able to execute their SQL commands.

More detailed instructions of how to use Prefetch store can be found [here](
store/README.md)

## Development guidelines

* Implementations that are injected dependencies during service creation should
  have lightweight construction and postpone heavier initialization (i.e. DB
  connection) to a later moment. Lazy initialization upon first actual usage is
  recommended.
