/*
 * Copyright © 2012 Linaro Limited
 * Copyright © 2013 Canonical Ltd
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Simon Que 
 *  Jesse Barker
 *  Alexandros Frantzis
 */
#ifndef GLMARK2_NATIVE_STATE_DRM_H_
#define GLMARK2_NATIVE_STATE_DRM_H_

#include "native-state.h"
#include <csignal>
#include <cstring>
#include <gbm.h>
#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

class NativeStateDRM : public NativeState
{
public:
    NativeStateDRM() :
        fd_(0),
        resources_(0),
        connector_(0),
        encoder_(0),
        crtc_(0),
        mode_(0),
        dev_(0),
        surface_(0),
        bo_(0),
        fb_(0),
        crtc_set_(false) {}
    ~NativeStateDRM() { cleanup(); }

    bool init_display();
    void* display();
    bool create_window(WindowProperties const& properties);
    void* window(WindowProperties& properties);
    void visible(bool v);
    bool should_quit();
    void flip();

private:
    struct DRMFBState
    {
        int fd;
        gbm_bo* bo;
        uint32_t fb_id;
    };

    static void page_flip_handler(int fd, unsigned int frame, unsigned int sec,
                                  unsigned int usec, void* data);
    static void fb_destroy_callback(gbm_bo* bo, void* data);
    static void quit_handler(int signum);
    static volatile std::sig_atomic_t should_quit_;

    DRMFBState* fb_get_from_bo(gbm_bo* bo);
    bool init_gbm();
    bool init();
    void cleanup();

    int fd_;
    drmModeRes* resources_;
    drmModeConnector* connector_;
    drmModeEncoder* encoder_;
    drmModeCrtcPtr crtc_;
    drmModeModeInfo* mode_;
    gbm_device* dev_;
    gbm_surface* surface_;
    gbm_bo* bo_;
    DRMFBState* fb_;
    bool crtc_set_;
};

#endif /* GLMARK2_NATIVE_STATE_DRM_H_ */
