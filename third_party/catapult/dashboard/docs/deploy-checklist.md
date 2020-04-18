# Deploy process check list

## Background

There are two types of versions of
the dashboard in the [app engine versions
list](https://appengine.google.com/deployment?&app_id=s~chromeperf):
"dev" versions and "clean" versions. There is always one default version
that's being used, and multiple other versions that could be set as
default, if the current version is broken. Dev versions contain "dev-"
in the name, and contain changes that haven't been reviewed and checked
in; in general, dev versions should never be set as default.

## General procedure

Every Tuesday and Thursday (or whenever required), a new "clean" version
should be uploaded and checked (see below) to see if there are any
problems. If any problems are seen, the previous clean version should be
set as default When setting a new version as default, in order to avoid
outages, the basic functionality of the dashboard should be checked to
make sure it's not broken.

If the new clean version passes the checklist, add the version name to the
top of
[the list of default Perf Dashboard versions](https://docs.google.com/spreadsheets/d/1cf4OnANqjCqcYbo3e6QbY2nWL601a3V_56pa0vyguU0).
Then in the app engine versions list, select the checkbox next to the
clean version and click on the "Migrate Version" button.

## The check list

### Alerts page functionality

[/alerts](https://chromeperf.appspot.com/alerts)

- If there are untriaged alerts, a table of alerts should be shown.
- If there are no alerts, try
  [/alerts?triaged=true](https://chromeperf.appspot.com/alerts?triaged=true);
  it should show the last 500 alerts.
- After checking a row and clicking "graph", a page with should open
  with the selected alerts and their graphs.
- After clicking the link inside a row, a page should open with a graph
  that shows the revision where the alert occurred.

### Report page functionality

[/report](https://chromeperf.appspot.com/report)

- After using the menu to select a test suite, bot and test (e.g. kraken,
  ChromiumPerf/linux-release, Total) and clicking "add", a chart should
  be shown.
- A sub-series chart can also be added by selecting a sub-test and
  clicking "add".
- After dragging the revision range slider, the revision range of the data
  shown should be changed and the and URL should be updated.

### Graph functionality

On a page with a graph, e.g.
[/group\_report?bug\_id=509851](https://chromeperf.appspot.com/group_report?bug_id=509851) or
[/report?sid=89a4bd60...](https://chromeperf.appspot.com/report?sid=89a4bd60efbaf838455514aef4f6487e2e782888b1787a420b2f694e539e90da),
check:

- The buttons in the legend should change which items are selected.
- The items which aren't loaded by default should items should be
  loaded later than core items.

### Triaging and bisect functionality

On a page graph with an alert, e.g.
[/group\_report?keys=agxz...](https://chromeperf.appspot.com/group_report?keys=agxzfmNocm9tZXBlcmZyFAsSB0Fub21hbHkYgIDAwY_K9QgM), check:

- One can un-triage the alert on the graph by clicking it then clicking X
- One can mark an alert as ignored by clicking on it and clicking "Ignore".
- One can start a bisect job by clicking any point in the graph, clicking
  "bisect", then submitting the form.
