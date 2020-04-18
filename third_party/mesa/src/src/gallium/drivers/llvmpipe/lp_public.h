#ifndef LP_PUBLIC_H
#define LP_PUBLIC_H

struct pipe_screen;
struct sw_winsys;

struct pipe_screen *
llvmpipe_create_screen(struct sw_winsys *winsys);

#endif
