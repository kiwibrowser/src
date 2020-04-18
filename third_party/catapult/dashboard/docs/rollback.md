# Rolling back from a broken deployment

If the current default version is broken

## Find the target version to rollback to

Check the [list of default versions](https://docs.google.com/spreadsheets/d/1cf4OnANqjCqcYbo3e6QbY2nWL601a3V_56pa0vyguU0)
of the Perf Dashboard. The current version should be at the top of the
list, and the previous version should be directly under it.

You should rollback to the previous version. Please add that version to
the top of the list after you rollback to it.

## Rollback to the previous version

Find the target version in the [app engine versions
list](https://appengine.google.com/deployment?&app_id=s~chromeperf).
Select the checkbox next to it, and then hit the "Migrate Traffic" button.
