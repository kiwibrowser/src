#ifndef GLXINIT_INCLUDED
#define GLXINIT_INCLUDED

#include <X11/Xlib.h>
#include <GL/gl.h>

typedef struct {
   __GLcontextModes *configs;
   char *serverGLXexts;
} __GLXscreenConfigs;

typedef struct {
   Display *dpy;
   __GLXscreenConfigs **screenConfigs;
   char *serverGLXversion;
   int majorOpcode;
   struct x11_screen *xscr;
} __GLXdisplayPrivate;

extern __GLXdisplayPrivate *__glXInitialize(Display * dpy);

#endif /* GLXINIT_INCLUDED */
