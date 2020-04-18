## Introduction

Pepper is an interface that web browsers (specifically, Chromium) provide for
implementing browser plugins. Pepper started out as an extension to Mozilla's
widely-used [NPAPI](http://en.wikipedia.org/wiki/NPAPI) interface, but Pepper v2
provides a new API that is not an extension to NPAPI.

[NaCl's integration with Chromium](chromium_integration.md) is implemented using
Pepper. However, NaCl also makes a Pepper-based interface available to web apps
over [IPC](imc_sockets.md).

## References

*   http://code.google.com/p/ppapi/: Project containing the interface
    definitions (C header files)
    *   [Concepts](http://code.google.com/p/ppapi/wiki/Concepts) - wiki page
*   https://wiki.mozilla.org/Plugins:PlatformIndependentNPAPI
*   [plugin-futures](https://mail.mozilla.org/listinfo/plugin-futures) mailing
    list
