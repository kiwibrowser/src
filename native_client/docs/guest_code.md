# Guest code in Native Client

## Introduction

Web apps may wish to run _guest code_ under Native Client -- that is, code which
runs with less authority than the web app itself. The guest code is less trusted
than the main web app.

For example, guest code might not be allowed to use the cookies for the web
app's origin. Guest code might have limited access to the DOM.

## Use cases

*   A web app may wish to embed and run NaCl code written by other people, e.g.
    gadgets that extend the web app's functionality.
*   A web app may wish to use code that it controls without fully trusting it,
    such as complex legacy libraries (perhaps for image or video decoding) that
    might have buffer overrun vulnerabilities. The web app does not want an
    attacker (who provides a malicious video file) to gain access to the web
    app's cookies.

## Requirements

*   No sources of ambient authority
    *   In NaCl, this means there should be no system calls that provide
        authority without requiring a descriptor argument.
    *   open(): On Unix, this is a source of ambient authority, but in NaCl it
        is only available outside the browser when in debug mode, which is OK.
*   No unavoidably-granted authority
    *   In NaCl, this means it should be possible to choose what descriptors a
        NaCl process is started with.
    *   Currently, the `<embed>` element always starts NaCl processes with a
        descriptor that provides access to the DOM, providing access to all of
        the web app's ambient authority (cookies, etc.). There is no way to
        start a process other than `<embed>`, so no way to start a new process
        without this descriptor.

## See also

*   DeterministicExecution: a restriction that could be applied to some guest
    code
*   SystemCalls
*   Caja's [Flash bridge]
    (http://code.google.com/p/google-caja/wiki/FlashBridge): this indicates that
    Flash has the ability to run guest code to some extent
