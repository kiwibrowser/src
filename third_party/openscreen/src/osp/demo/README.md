# Presentation API Demo

This directory contains a demo of a Presentation API controller and receiver.
The demo supports flinging a URL to start a presentation and stopping the
presentation.

## Command line options

The same executable is run for the controller and receiver; only the command
line options affect the behavior.  The command line options are:

``` bash
    $ demo [-v] [friendly_name]
```

 - `-v` enables verbose logging.
 - Specifying `friendly_name` puts the demo in receiver mode and sets its name
   to `friendly_name`.  Currently, `friendly_name` won't appear in any of the
   controller-side output related to this screen though.  If no friendly name is
   given, the demo runs as a controller.

## Log output

Because the demo acts like a shell and accepts commands on `stdin`, the logging
output is redirected to a separate file so it doesn't flood the same display.
You have to create these files on your machine before running the demo.  For the
controller, this file should be named `_cntl_fifo` and for the receiver, it
should be named `_recv_fifo`.  The simplest way to do this is so you can see the
output while the demo is running is to make these named pipes like so:

``` bash
    $ mkfifo _cntl_fifo _recv_fifo
```

Then `cat` them in separate terminals while the demo is running.

## Controller commands

 - `avail <url>`: Begin listening for receivers that support the presentation of
   `url`.
 - `start <url> <service_id>`: Start a presentation of `url` on the receiver
   specified by the ID `service_id`.  `service_id` will be printed in the output
   log once `avail` has been run.  The demo only supports starting one
   presentation at a time.
 - `msg <string>`: Sends a string message on the open presentation connection.
 - `term`: Terminate the previously started presentation.

## Receiver commands

 - `avail`: Toggle whether the receiver is publishing itself as an available
   screen.  The receiver starts in the publishing state.
 - `close`: Close the open presentation connection without terminating the
   presentation.
 - `msg <string>`: Sends a string message on the open presentation connection.
 - `term`: Terminate the running presentation.
