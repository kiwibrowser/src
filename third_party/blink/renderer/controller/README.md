# The controller/ directory

controller/ contains the system infrastructure of the renderer process that uses or drives the web platform. controller/ can directly use core/ and modules/ without using Web types (but with some DEPS rules). Examples are RenderProcess, RenderThread, Android View, Extensions, Native Client etc.

We should avoid making controller/ a "catch-all" directory. Things that are part of core/ should go to core/. Things that are part of modules/ should go to modules/. Only things that lives outside the web platform should go to controller/.
