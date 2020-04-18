# How to disable a failing benchmark or story in Telemetry

## Modify [`tools/perf/expectations.config`](https://cs.chromium.org/chromium/src/tools/perf/expectations.config?q=expectations.config&sq=package:chromium&dr)

Start a fresh branch in an up-to-date Chromium checkout. If you're unsure of how to do this, [see these instructions](https://www.chromium.org/developers/how-tos/get-the-code).

In your editor, open up [`tools/perf/expectations.config`](https://cs.chromium.org/chromium/src/tools/perf/expectations.config?q=expectations.config&sq=package:chromium&dr).

You'll see that the file is divided into sections sorted alphabetically by benchmark name. Find the section for the benchmark in question. (If it doesn't exist, add it in the correct alphabetical location.)

Each line in this file looks like:

    crbug.com/12345 [ conditions ] benchmark_name/story_name [ Skip ]

and consists of:

* A crbug tracking the story failure

* A list of space-separated tags describing the platforms on which the story will be disabled. A full list of these tags is available [at the top of the file](https://cs.chromium.org/chromium/src/tools/perf/expectations.config?type=cs&q=tools/perf/expectations.config&sq=package:chromium&g=0&l=5). (Note that these conditions are combined via a logical AND, so a platform must meet all conditions to be disabled.)

* The benchmark name followed by a "/"

* The story name, or an asterisk if the entire benchmark should be disabled

* The string "`[ Skip ]`", which denotes that the test should be skipped

Add a new line for each story that you need to disable, or an asterisk if you're disabling the entire benchmark. Multiple lines are also necessary to disable a single story on multiple platforms that lack a common tag.

For example, an entry disabling a particular story might look like:

    crbug.com/738453 [ Nexus_6 ] blink_perf.canvas/putImageData.html [ Skip ]


whereas an entry disabling a benchmark on an entire platform might look like:

    crbug.com/593973 [ Android_Svelte ] blink_perf.canvas/* [ Skip ]

## Submit changes

Once you've committed your changes locally, your CL can be submitted with:

- `NOTRY=true`
- `TBR=`someone from [`tools/perf/OWNERS`](https://cs.chromium.org/chromium/src/tools/perf/OWNERS?q=tools/perf/owners&sq=package:chromium&dr)
- `CC=`benchmark owner found in [this spreadsheet](https://docs.google.com/spreadsheets/u/1/d/1xaAo0_SU3iDfGdqDJZX_jRV0QtkufwHUKH3kQKF3YQs/edit#gid=0)
- `BUG=`tracking bug

*Please make sure to CC the benchmark owner so that they're aware that they've lost coverage.*

The `TBR` and `NOTRY` are permitted and recommended so long as the only file changed is `tools/perf/expectations.config`. If your change touches real code rather than just that configuration data, you'll need a real review before submitting it.
