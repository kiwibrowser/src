This is where old perf machinery used to live, keeping track of binary sizes,
etc. Now that lives elsewhere and has a team to support it (see
https://www.chromium.org/developers/tree-sheriffs/perf-sheriffs). This code
remains to ensure that no static initializers get into Chromium.

Because this code has this history, it's far more complicated than it needs to
be. TODO(dpranke): Simplify it. https://crbug.com/572393

In the meanwhile, if you're trying to update perf_expectations.json, there are
no instructions for doing so, and the tools that you used to use don't work
because they rely on data files that were last updated at the end of 2015. So
here's what to do to reset the expected static initializer count value.

The expected static initializer count value is in the "regress" field for the
platform. In addition, each platform has a checksum in the "sha1" field to
ensure that you properly used the magic tools. Since the magic tools don't work
anymore, dpranke added a bypass to the verification. If you run:

> tools/perf_expectations/make_expectations.py --checksum --verbose

the script will tell you what the checksum *should* be. Alter the "sha1" field
to be that value, and you can commit changes to that file.

Please see https://crbug.com/572393 for more information.
