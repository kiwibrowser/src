The code is this directory is neither endorsed nor supported by the Chromium
benchmarking team.

To add code in this directory:
1. First check the list of available benchmarking harnesses at
bit.ly/chrome-benchmark-harnesses and make sure that your use case cannot fit
any of the supported harness. If there is a related harness, please reach out
to us and extend existing functionality.
2. If your test case cannot fit into any existing harness, create a
sub-directory with yourself and at least one another person as the OWNERS. Send
the CL containing ONLY the new OWNERS file to the owners of tools/perf/. Your
CL description should describe what the code you plan to add do. If it's an
ephemeral benchmark used to drive a perf project, you need an accompanied bug
(assigned to you) to clean up the benchmarks once your perf project is launched.

**NOTE**
1. Benchmarks in this directory will not be scheduled for running on the
perf waterfall.
2. Chromium benchmarking team will NOT review nor maintain any code under this
directory. It is the responsiblity of the owners of each sub-directory to
maintain them.
