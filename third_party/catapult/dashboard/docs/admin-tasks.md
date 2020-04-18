# Admin Tasks for the Chrome Performance Dashboard

## "Dashboard is down" check list

- Is app engine up? If not, just have to sit tight. You can check the status
here: [https://code.google.com/status/appengine](https://code.google.com/status/appengine)
- Check the [main app engine dashboard page](https://console.developers.google.com/appengine?project=chromeperf&moduleId=default).
- Are we over quota?
- Look at the error rates on the dashboard.
- Check the task queues.
- Test data not showing up
  - Check [/new\_tests](https://chromeperf.appspot.com/new_tests).
  - Search the logs
  - Is the test internal-only and the user is logged out?

## Handling stackdriver alerts

We use stackdriver monitoring to check if the dashboard is running as expected.
When you get an alert mail from stackdriver, you should do the following:

**Understand what alerted**, and find the relevant code. We have two main types
of alerts:

1. **Metric Absence on Custom Metrics**. When we have some piece of code which
   absolutely must run regularly, we call `utils.TickMonitoringCustomMetric`
   every time the code completes. If that call is **not** made, it generally
   means the code failed, and we send an alert. You'll want to search the code
   for the call to tick the metric with that name, so you can understand where
   the likely failure is.
2. **Metric Threshold on Task Queue new-points-queue**. When this happens, we are
   seeing too many errors adding data to the datastore in `add_point_queue.py`.

**Analyze the logs for errors**. Once you have a basic idea what codepath is
failing, you'll want to look at the logs. There are two main entry points for
this:

1. **[Error Reporting Page](http://go/chromeperf-errors)**. This page
   lists common errors that have occurred recently, grouped together. You'll
   want to look out especially for ones marked `NEW ERROR`. Click through to
   look at callstacks and relevant logs.
2. **[Logs page](http://go/chromeperf-logs)**. This page allows you to search
   all the logs. You'll want to try and find log entries on the URLs where the
   problem occurred.
   ([Logs page help](https://cloud.google.com/logging/docs/view/logs_viewer))

**File a bug and follow up**. The bug should be labeled `P0`, `Perf Dashboard`,
`Bug`. If it is clear the problem is with bisect, add that label as well. Reply
to the email and link the bug, and update the bug with your findings as you
understand the problem better.

## Scheduled downtime

If it's necessary at some point to have scheduled downtime, announce
it ahead of time. At least 2 days before the downtime (ideally more),
announce in these ways:

 1. Send an email to any Chromium perf sheriffs who will be affected,
    or all perf sheriffs (`perf-sheriffs@chromium.org`).
 2. Send an email to `chrome-perf-dashboard-announce@google.com`.

If possible, it's probably best to schedule it for Saturday, when usage
is likely to be relatively low.

## Routine tasks

There are several routine tasks to do to set up the dashboard for a
user. The official process for this is to file bugs on crbug.com
with labels:

- `Performance-Dashboard-IPWhitelist`
- `Performance-Dashboard-BotWhitelist`
- `Performance-Dashboard-MonitoringRequest`

### Editing sheriff rotations

You can view, create and edit sheriff rotations
at [/edit\_sheriffs](https://chromeperf.appspot.com/edit_sheriffs).

#### Adding a new sheriff

It’s fine to add a new sheriff rotation any time a team wants alerts
to go to a new email address. It’s fine to make a temporary sheriff
rotation for monitoring new tests before they are stable. Here are the
fields that need to be filled out:

 - **Name**: This is the name of the sheriff
   rotation. It will be listed in the drop-down
   at [/alerts](https://chromeperf.appspot.com/alerts).
 - **Rotation URL**: Some sheriff rotations have a URL for specifying
   the email of the sheriff. For example, the Chromium Perf Sheriff URL
   is [http://chromium-build.appspot.com/p/chromium/sheriff\_perf.js](http://chromium-build.appspot.com/p/chromium/sheriff_perf.js).
   Most sheriff rotations don’t have a URL, and if not it’s fine to leave
   this blank and just specify an email address.
 - **Notification Email**:
   This is usually a mailing list that alerts should go to. However,
   there’s nothing stopping it from being an individual’s email
   account. It must be specified if there is no Rotation URL, but it’s
   optional otherwise.
 - **Internal-only**: If the tests this sheriff is monitoring are internal-only,
   or the name of the sheriff rotation is sensitive, please
   set this to "Yes". If set to "Yes", the sheriff rotation will only
   show up on the alerts page for users logged in with google.com accounts.
 - **Summarize Email**: By default, the perf dashboard sends one email
   for each alert, as soon as it gets the alert. If that will add up to
   too much mail, setting this to "Yes" will switch to a daily summary.

#### Monitoring tests

After creating a sheriffing rotation, you need to add the individual
tests to monitor. You do this by clicking on "Set a sheriff for a
group of tests". It asks for a pattern. Patterns match test paths,
which are of the form "Master/Bot/test-suite/graph/trace". You can replace
any part of the test path with a `*` for a wildcard.

The dashboard will list the matching tests before allowing you to apply
the pattern, so you’ll be able to check if the pattern is correct.

To remove a pattern, click "Remove a sheriff from a group of tests".

If you want to keep alerting on most of the tests in a pattern and
just disable alerting on a few noisy ones, you can add the "Disable
Alerting" anomaly threshold config to the noisy tests (see "Modify
anomaly threshold configs" below).

### Setting up alert threshold configs

The default alert thresholds should work reasonably well for most test
data, but there are some graphs for which it may not be correct. If
there are invalid alerts, or the dashboard is not sending alerts when
you expect them, you may want to modify an alert threshold config.

To edit alert threshold configs, go
to [/edit\_anomaly\_configs](https://chromeperf.appspot.com/edit_anomaly_configs).
Add a new config with a descriptive name and a JSON mapping of parameters
to values.

### Anomaly config debugger page

Start off by using the anomaly threshold debugging
page: [/debug\_alert](https://chromeperf.appspot.com/debug_alert). The
page shows the segmentation of the data that was given by the anomaly
finding algorithm. Based on the documentation, change the config
parameters to get the alerts where you want them.

### Automatically applying labels to bugs

The dashboard can automatically apply labels to bugs filed on alerts,
based on which test triggered the alert. This is useful for flagging
the relevant teams attention. For example, the dashboard automatically
applies the label "Cr-Blink-JavaScript" to dromaeo regressions,
which cuts down on a lot of CC-ing by hand.

To make a label automatically applied to a bug, go
to [/edit\_sheriffs](https://chromeperf.appspot.com/edit_sheriffs) and
click "Set a bug lable to automatically apply to a group of
tests". Then type in a pattern as described in "Edit Sheriff
Rotations -&gt; Monitoring Tests" section above, and type in the bug
label. You’ll see a list of tests the label will be applied to before
you confirm.

To remove a label, go
to [/edit\_sheriffs](https://chromeperf.appspot.com/edit_sheriffs) and
click "Remove a bug label that automatically applies to a group of
tests".

### Migrating and renaming data

When a test name changes, it is possible to migrate
the existing test data to use the new name. You
can do this by entering a pattern for the test name
at [/migrate\_test\_names](https://chromeperf.appspot.com/migrate_test_names).

### Whitelisting senders of data

There are two types of whitelisting on the perf dashboard:

The IP whitelist is a list of IP addresses of machines which
are allowed to post data to /add\_point. This is to prevent
/add\_point from being spammed. You can add a bot to the IP whitelist
at [/ip\_whitelist](https://chromeperf.appspot.com/ip_whitelist). If
you’re seeing 403 errors on your buildbots, the IPs to add are likely
already in the logs. Note that if you are seeing 500 errors, those are
not related to IP whitelisting. They are usually caused by an error in
the JSON data sent by the buildbot. If you can’t tell by looking at
the JSON data what is going wrong, the easiest thing to do is to add a
unit test with the JSON to `add_point_test.py` and debug it from there.

The bot whitelist is a list of bot names which are publicly visible. If a
bot is not on the list, users must be logged into google.com accounts to
see the data for that bot. You can add or remove a bot from the whitelist
at [/bot\_whitelist](https://chromeperf.appspot.com/bot_whitelist),
and make a bot’s existing data publicly visible (or internal\_only)
at [/change\_internal\_only](https://chromeperf.appspot.com/change_internal_only).
