This tool is designed to measure and aggregate a few key performance metrics
for all Chrome processes over time. The tool supports other browsers such as
Edge or Firefox, or in fact any other processes. The inclusion of processes
is based on a simple partial process image name match against the image name
argument (optional, Chrome.exe is the default image name used when no
argument is provided).

Sample usage:

"IdleWakeups.exe" to match all Chrome.exe processes.
"IdleWakeups.exe" firefox to match Firefox process.
"IdleWakeups.exe MicrosoftEdge" to match all Edge processes.

When the tool starts it begins gathering and aggregating CPU usage, private
working set size, number of context switches / sec, and power usage for all
matched processes. Hit Ctrl+C to stop the measurements and print average and
median values over the entire measurement interval.

CPU usage is normalized to one CPU core with 100% meaning one CPU core is
fully utilized. 

Intel Power Gadget is required to allow IdleWakeups tool to query power usage.
https://software.intel.com/en-us/articles/intel-power-gadget-20
