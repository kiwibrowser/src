### First things first

*   Follow the steps in the [Getting Started]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/getting_started.html)
    guide.
*   Browse the rest of the [documentation]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/README.html#doc).
*   Read the [research paper]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/nacl_paper.pdf)
    (PDF).

### Please try these

*   Run all the [examples and tests]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/examples.html).
*   [Build Native Client]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/building.html)
    from scratch.
*   Build Quake and XaoS for Native Client; run them. For more information, see
    the [examples and tests]
    (http://nativeclient.googlecode.com/svn/trunk/src/native_client/documentation/examples.html)
    page.

### For even more fun

*   Try NativeClientInGoogleChrome.
*   Pick something from the [Ideas](ideas.md) page.
*   Port existing open-source packages to run as Native Client module
    components.
*   Write new Native Client modules that use Native Client's reduced system call
    interface, NPAPI, and SRPC to communicate with the browser.
*   Defeat the Native Client sandbox. Can you create a Native Client module that
    creates a file in the local file system, makes a network connection that
    subverts browser domain restrictions, or directly executes a system call?
    Exploits using <tt>sel_ldr</tt> from the command line or from the browser
    plug-in are both of interest. Don't use the -d debug flag — that would be
    too easy! > Some specific areas to explore:
    *   **The inner sandbox** - A defect in our decoder table or validation
        logic could make it possible for the validator to miss a system call
        instruction or other disallowed instruction that could then break out of
        the sandbox.
    *   **The outer sandbox** - If the inner sandbox were ever compromised, the
        outer sandbox provides a second line of defense to limit file system and
        networking system calls. This sandbox isn't ready yet so you can't
        really break out.
    *   **Hardware errata** - Can you write a program that causes segmented
        memory protection to fail, or control transfer to the wrong address? Can
        you write a program that causes the machine to hang? These are all
        things Native Client needs to prevent.
    *   **Service runtime binary loader** - Can you create a Native Client
        module that causes the service runtime to fail in such a way that it can
        be exploited?
    *   **Service runtime trampoline/springboard mechanisms** - Can you create a
        Native Client module that causes the trampoline or springboard to fail?
        The result might be a control transfer to an unsafe instruction,
        unintended exposure of the trusted stack, or a browser crash or hang.
    *   **IMC (inter-module communication) interface** - Can you find a defect
        in the IMC interface that allows you to cause some unintended
        side-effect outside of the Native Client module?
    *   **NPAPI interface** - Can you find a defect in our NPAPI implementation
        that allows you to cause some unintended side-effect outside of the
        Native Client module?
