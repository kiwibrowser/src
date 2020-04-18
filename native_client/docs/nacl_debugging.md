# Introduction

## Goal

The goal is to provide a mechanism for symbolic debugging of the untrusted
aspects of Native Client applications, while minimizing impact on the Service
Run-time development and security. The trusted service run-time should be
invisible to the user much the way that OS system calls are invisible during
casual application debugging.

## Summary

Both for speed of development, and portability, we have decided to use the GDB
serial debug protocol (a.k.a. RSP - Remote Serial Protocol). This protocol runs
between the target (the application being debugged) and the host (the debugger
instance, usually GDB). Using RSP allows us to maintain compatibility with GDB
based debuggers while focusing on the larger windows audience to enable windows
based debugging tools. The stub and link libraries discussed here are a portion
of the overall solution.

## Components

The entire debugging solution includes connection management, packetization of
exchange of messages, control of the target, synchronization with the host,
loading, processing, and interpreting debugging (DWARF) information, and
interaction with the user through a Visual Studio Plug-In. This document focuses
on the communication and target control aspects which together form the
dynamically loadable debug stub.
![http://nativeclient.googlecode.com/svn/wiki/images/nacl-dbg-components.png]
(http://nativeclient.googlecode.com/svn/wiki/images/nacl-dbg-components.png)

**Component Name**      | **Description**
:---------------------- | :-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
**gdb\_rsp**            | Gdb\_rsp is a static standalone library providing an abstraction of the communication between a GDB target and GDB host.  Communication happens over a GDBTransport, which can be either socket or system pipe based.  Messages are exchanged in the form of discrete GDBPacket objects which are capable of streaming values in and out of them.  Data traversing the GDBTransport conforms to the GDB Remote Serial Protocol to allow a standard GDB process to connect via socket. The library provides an abstract class for defining the host and target, from which implementers can derive a stub or debugger and ensure compatibility.  The gdb\_rsp library provides additional support for GDB/RSP related data such as architecture specific thread context structures, defining register order, enumerating signal/exception, etc...
**nacl\_target\_stub**  | The nacl\_target\_stub library implements the back end of the GDB debugging stub, handling exceptions, processing queries, and generating responses.  The nacl\_debug\_stub is loaded dynamically and provides a single interface similar an ioctl which the Service Run-time can use to provide debugging support.  This reduces the impact and footprint of the debug\_stub on the service Run-time.
**Dwarf2Reader**        | Dwarf2reader is a stand alone library for parsing Dwarf debugging information.  The dwarf2reader provides an interface from which one can implement callbacks to process the dwarf data.
**Dwarf2XML**           | Dwarf2XML is a helper utility which will take a NEXE and extract the Dwarf data in a more human readable form.
**NaClvs.DebugHelpers** | This library provides a mechanism for the C# based PlugIn to communicate with the gdb\_rsp library as well as provide a mechanism to load and process the debugging information.
**NaClvs.Package**      | The NaClvs.Package implements the plug-in which communicates with Visual Studio to provide a native development and debugging environment within the VS framework.

# The Debug Stub

The debug stub is activated by checking for a debugging environment variable. If
the environment variable is set, the service run-time will attempt to load and
use the debugging module. Once loaded, the debugging module works in
co-operation with the parent application (most likely Chrome), for logging
debugging information and events. It is advisable that the parent application do
something to make the logging obvious such as opening a console style window, to
ensure users to do not accidentally weaken the security of the application by
accidentally enabling debugging.

## Communication between the Stub and Service Run-time

Communication with the stub happens through a single library function in the
service run-time. This function is in the form of: `int
NaClDebugStubCommand(uint32 CMD, void *data, uint32 size);
` Underneath, this function checks for the presence of the debug stub module. If
found, it automatically prepends version information before calling the dispatch
function in the DLL. The function will return zero on success, or an error code
on failure. It is not expected that the service run-time will need to handle
error cases since the debug stub is not require for correct execution of the
Service Run-time. In addition, any events will be logged, providing the user a
means to determine if the debugging system is working. However, a non-zero error
allows us to indicate that data which would normally be passed back within the
data block may not be valid. The actual function exported by the library is:
`int NaClDebugStubDispatch(uint32 version, uint32 CMD, void *data, uint32 size);
` This technique allows us to centralize checking for the presence of the debug
stub as well as validating and or dealing with version compatibility. Within the
debug stub, the dispatch function is responsible for validation of the
parameters before dispatching to the individual handlers.

## Debugging Events

The debugging stub is interested in several events including load of the NEXE,
load of additional modules, creation of threads, and destruction of threads.
These events are raised from the TCB (trusted codebase) by calling
NaClDebugStubCommand(). Of course, this function is a no-op if debugging is not
enabled.

**Initialization**  | On initialization of the debugging stub, the stub must be configured with parameters for communication with the outside world, as well as the untrusted memory range.  This includes the address and choice of socket or pipe.  This needs to be the first debug stub command called.  The debug stub will instantiate a new thread which will be responsible for communicating with the debugger.
:------------------ | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
**Module Load**     | After load and verification of the NEXE, but before its execution, the Service Run-time must inform the debug stub of the name and path of the NEXE.  Additionally, any dynamic module loader will also need to inform the debug stub so that it can notify the debugger. _In the future, we plan to provide an hash of the data and executable sections of the NEXE (excluding debugging sections).  This information can be queried by the debugger to help validate the loaded symbols match the NEXE even if it has been stripped._
**Thread Creation** | On creation of a NaClAppThread, the new thread must call into the debug stub before it switches to the untrusted context.  This allows the debug stub to track the thread as well as change it's signal/exception handler.  To ensure the debugger catches the signal/exception, the Service Run-time must guaranty it calls the debug\_stub after any other code (such as breakpad) has inserted a handler.
**Exception**       | When an exception (e.g. breakpoint or access violation) is thrown in debugged code, the debug stub must be notified. See the section on "Processing Exceptions," below, for details.

## Processing Exceptions

When an exception takes place, the inserted exception handler will catch the
exception or signal, and update it's state in the debug thread object which
tracks it (such as storing registers). The thread will then block on an OS
synchronization object. The debugging thread created at initialization of the
debug stub will eventually detect the exception by scanning through the list of
active thread objects at which point it will sleep the other threads and signal
to the debugger that the NEXE is in an exception state. When the debugger
signals it is time to continue, the debugging thread will mark the debug thread
structure with a flag to signal it may continue, it should step, or if the
thread should be killed. It will then signal the thread to allow to clean itself
up or destroy itself. If the thread continues, it will pull the new register
state (in case it was modified during debugging) and update the exception
context before returning.

### Security Issues

Unfortunately under Windows, the thread context of the excepted thread is stored
within the untrusted stack of the thread which took this exception. This opens a
security hole where another malicious thread could modify either the exception
context itself, or the return pointers within the stack making, any function
return unsafe. In the absence of a Windows sigaltstack() implementation, it
seems difficult if not impossible to reliably plug this hole.

One possible choice is to allow the user to chose if it wants to support safe or
unsafe debugging. In the safe case, all excepted threads would be terminated,
while in the unsafe case the debuggers provides the stepping and continue
features of a normal debugger.

### System Calls

While exceptions should never happen within a system call, it is possible that
while one thread takes an exception, another is in a system call. It is also
possible that the debugger has signaled a force break and the thread of interest
is within a system call. When the debug stub is asked for context info for a
thread, it checks first to see if the app thread was in a system call. If so, it
uses the thread context preserved on entry into the system call.

When a thread is stopped in user code, we allow alterations to its current
context. This is not true for threads stopped in syscalls. We do not copy the
context information from the debug thread object into the app thread before
waking it up, since this would lead to unexpected behavior. Instead the stub
will signal the debugger with an error if an attempt is made to modify the
registers of a thread within a system call.

# GDB Remote Serial Protocol

see:
http://www.redhat.com/docs/manuals/enterprise/RHEL-4-Manual/gdb/remote-protocol.html

The RSP is in the form of transactions, consisting of a command issued by the
host, which is then acked by the target. The target then responses which in turn
is acked by the host. Generally transactions originate only from the Host to the
Target, and the protocol can be processed in lock step. This is common for
debugging stubs, since it simplifies the implementation. However the protocol
supports an optional sequence field which allows for alignment of responses. The
exception to this rule is ”Step” or “Continue” commands and “Stop” Packets. Step
and Continue commands do not expect an immediate response. Instead they expect
the host to resuming running and not reply until execution is halted again.
Encoding Data travels between the Host and Target in the form of packets. A
packet will begin with the “Ready” signal which is ‘**$**’ (dollar). It then can
be followed by an optional two hex digit sequence number and ‘**:**’ (colon).
Next is the command or response code and the optional data payload. Finally the
packet is terminated with a number sign ‘**#**’ followed by a two hex digit
check sum.

This protocol assumes that: 1. ‘**:**’ as the third character in the packet
denotes a packet with a sequence. 1. ‘**#**’ denotes the end of the packet and
beginning of the checksum. 1. All characters are standard ASCII values between
32 and 127. 1. Non alpha numeric characters have special meaning and are usually
used to split fields in the stream. Usually these are ‘**:**’, ‘**,**’ (comma),
or ‘**.**’ (period). 1. Fields which could contain non-alpha numeric characters
are typically encoded as pairs of hexadecimal digits representing a string of 8
bit values.

## Protocol Layers

The communication library divides the protocol into three layers link,
transport, application. The link layer is provided by a vitalized GDBLink object
which could be either a socket or system pipe. The transport layer is managed by
the GDBTransport which computes the checksum and provides the appropriate
ACK/NAK. Finally the application layer involves the sending and receiving the
payload in the form of GDBMessage objects which provides a mechanism for
streaming various data types in and out of a packet.

Stop packets are packets generated by the target when it enters a stopped state.
For example, when the target hits a signal or exception, it will emit a Stop
packet which the debugger uses as a signal to know the target is stopped and is
ready to receive debugging commands.

![http://nativeclient.googlecode.com/svn/wiki/images/gdb_rsp_exchange.png]
(http://nativeclient.googlecode.com/svn/wiki/images/gdb_rsp_exchange.png)

REQUEST: `$[<SQ>:]<C>[<data>]#<XS>`

*   $ signals a payload follows
*   SQ is an optional two hex digit sequence number followed by ‘:’
*   C is a single ASCII letter command
*   data is an optional range of parameters for the command
*   # signals the end of the payload
*   XS is a two hex digit (8 bit) accumulation of the payload

ACK/NAK: * `+[<SQ>]` The message was received and the sequence if present is
echoed back. * `-` The message contained and invalid check sum.

RESPONSE `$<data>#<XS>` * `<data>` can be any valid payload and varies based on
original command.

ACK/NAK: * `+[<SQ>]` The message was received and the sequence if present is
echoed back. * `-` The message contained and invalid check sum.

## Supported Commands

Minimal debugging support can be accomplished through reading and writing both
registers and memory, as well as signalling when a host should continue, or a
when it is stopped. While it is possible to support windows debugging with a
minimal set, modern versions of GDB expect certain other commands to be
available as well, and the lack of those features may cause GDB to incorrectly
determine that the target is 32 bit instead of 64.

#### ‘g’  Get Registers

Retrieve registers from ‘Active’ thread context.

Command

*   `$g#<XS>`

Response

*   `$XX..XX#<XS>` where XX..XX is all bytes of the first 16 registers

#### ‘G’  Set Registers

Set registers of ‘Active’ thread context. Command * `G<XX..XX>#<XS>` - where
XX..XX is all bytes of the first 16 registers

Response * `$OK#<XS>` - Set correctly * `$E01#<XS>` - The thread is unavailible

#### ‘H’ Select Thread

Select the thread for subsequent operations (`m',`M', `g',`G', et.al.). c
depends on the operation to be performed: it should be `c' for step and continue
operations,`g' for other operations. The thread designator t... may be -1,
meaning all the threads, a thread number, or zero which means pick any thread.
Reply: Command

*   `$Hc<thread>#<XS>` - Set thread for continue operations.
*   `$Hg<thread>#<XS>` - Set thread for register and other operations.

Response

*   `$OK#<XS>` - Thread is alive
*   `$E01#<XS>` - Failed to find thread

#### ‘m’ Get Memory

Retrieve memory range from the target process. Command *
`$m<addr32/64>,<len16>#<XS>` - Retrieve len bytes from address addr

Response * `$<XX..XX>#<XS>` - Hexidecimal payload * `$E01#<XS>` - Failed to
parse request * `$E02#<XS>` - Failed to read memory (pointer error?)

#### ‘M’ Set Memory

Set memory range in the target process.

Command * `$M<addr>,<len>:<data>#<XS>` - Set memory range

Response * `$OK#<XS>` * `$E01#<XS>` - Failed to parse request * `$E03 #<XS>`-
Failed to write to memory (pointer error?)

#### ‘T’ Thread Alive

Determine if a particular thread ID is alive Command * `T<thread>#<XS>` - Query
if thread is alive

Response * `$OK#<XS>` * `$E01#<XS>` - Failed to find thread

#### ‘?’ Get Last Signal

It is possible (and expected in the GDB case) that the target is already stopped
when the host connects. Command * `?#<XS>` - Fetch last stop reason.

Response * Stop Packet - See stop packets bellow

## Query Packets

Query packets are an extension of the basic debugging. The query mechanism is
extensible, and while custom queries are allowed, they would not be understood
by a vanilla GDB. The following is the minimum set of standard queries found to
be required by a 64b GDB. qSupported - Supported Queries Requests a list of
supported queries. While in theory it is safe to send an empty response to a
query to signal it is not supported, in practice, 64b GDB appears to require the
Xfer:features:read query to determine if the target is 32 or 64 bit. Command *
`qSupported{:<feature>{;<feature>...}}#<XS>` - Fetch last stop reason.

Response * `PacketSize=3ff;qXfer:features:read+#<XS>` - Return a max packet size
of 0x3FFF and the ability to respond to Xfer feature read requests, such as the
target architecture query

#### qfThreadInfo,  qsThreadInfo - Request list of active threads

Obtain a comma separated list of active thread ids from the target. Since there
may be too many active threads to fit into one reply packet, this query works
iteratively. The qfThreadInfo requests the list begining at the first ID.
qsThreadInfo requests a list of subsequent threads. The list is terminated with
a lower case ‘l’ (Lower case “L”). Command * `qfThreadInfo#<XS>` - Retreive list
of thread from the first * `qsThreadInfo#<XS>` - Retreive subsequent list of
thread

Response * `m<thread>{,<thread>...} #<XS>` - more availible threads * `l#<XS>` -
End of list

#### qXfer - Query transfer

Requests a list of supported queries. While in theory it is safe to send an
empty response to a query to signal it is not supported, in practice, 64b GDB
appears to require the Xfer:features:read query to determine if the target is 32
or 64 bit. Command * `qXfer:features:read:target.xml:<range>#<XS>` - Request
target.xml

Response * `l<target><architecture>i386:x86-64</architecture></target>` - Return
64b target.

### Break/Step/Continue Requests

‘Ctrl-C’ Break, ‘s’ Step, or ‘c’ Continue (Stop Requests) Both step and continue
with cause execution in the target to start. A response will not be sent until
execution of the target is halted again. Break is a request to stop a currently
running system. Output “responses” may be sent while execution is taking place.
These responses are informational, and the host should wait for an actual stop
response. Command

*   `<Ctrl-C>` - Send a break request
*   `s[<addr32/64>]` - Step from current position or optional address
*   `c[<addr32/64>]` - Continue form current possition or optional address

Response

*   No immediate response, but a stop packet will be issued when the target next
    stops.

### Stop Packets

The following packets are expected while the target is either running, or
stopping. These reponses are also returned when

*   `$S<AA>#<XS>` - AA is the signal number
*   `$T<AA>n...:r...;n...:r...;n...:r...;#<XS>` - AA = two hex digit signal
    number; n... = register number (hex), r... = target byte ordered register
    contents, size defined by DEPRECATED\_REGISTER\_RAW\_SIZE; n... = `thread',
    r... = thread process ID, this is a hex integer; n... = (`watch' | `rwatch'
    |`awatch', r... = data address, this is a hex integer; n... = other string
    not starting with valid hex digit. GDB should ignore this n..., r... pair
    and go on to the next. This way we can extend the protocol.
*   `$W<AA>#<XS>` -The process exited, and AA is the exit status. This is only
    applicable to certain targets.
*   `$X<AA>#<XS>` -The process terminated with signal AA.
*   `<XS> - XX..XX` is hex encoding of ASCII data. This can happen at any time
    while the program is running and the debugger should continue to wait for
    `W',`T', etc.

### Asynchronous Communication

While RSP was developed originally for serial (UART) communication, it is often
run over TCP. Since TCP is a loss less transport, the ACKs are superfluous. In
addition, since GDB supports the concept of a sequence number per packet, it
becomes possible for the Host and the Target to communicate asynchronously
instead of in lockstep. The target is required to respond to a particular
request with it’s sequence number, and since Target side packets do not expect a
response (see Stop Packets), it is possible to support a multi-threaded system
so long as each packet itself is sent atomically.
