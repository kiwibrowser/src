/*
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
 *  Alexandros Frantzis
 */
#include "native-state-mir.h"
#include "log.h"

/******************
 * Public methods *
 ******************/

namespace
{

const MirDisplayOutput*
find_active_output(const MirDisplayConfiguration* conf)
{
    const MirDisplayOutput *output = NULL;

    for (uint32_t d = 0; d < conf->num_outputs; d++)
    {
        const MirDisplayOutput* out = &conf->outputs[d];

        if (out->used && out->connected &&
            out->num_modes && out->current_mode < out->num_modes)
        {
            output = out;
            break;
        }
    }

    return output;
}

MirPixelFormat
find_best_surface_format(MirConnection* connection)
{
    static const unsigned int formats_size = 10;
    MirPixelFormat formats[formats_size];
    unsigned int num_valid_formats = 0;
    MirPixelFormat best_format = mir_pixel_format_invalid;

    mir_connection_get_available_surface_formats(connection,
                                                 formats,
                                                 formats_size,
                                                 &num_valid_formats);

    /*
     * Surface formats come sorted in largest active bits order.
     * Prefer opaque formats over formats with alpha, and largest
     * formats over smaller ones.
     */
    for (unsigned int i = 0; i < num_valid_formats; i++)
    {
        if (formats[i] == mir_pixel_format_xbgr_8888 ||
            formats[i] == mir_pixel_format_xrgb_8888 ||
            formats[i] == mir_pixel_format_bgr_888)
        {
            best_format = formats[i];
            break;
        }
        else if (best_format == mir_pixel_format_invalid)
        {
            best_format = formats[i];
        }
    }

    return best_format;
}

class DisplayConfiguration
{
public:
    DisplayConfiguration(MirConnection* connection)
        : display_config(mir_connection_create_display_config(connection))
    {
    }

    ~DisplayConfiguration()
    {
        if (display_config)
            mir_display_config_destroy(display_config);
    }

    bool is_valid()
    {
        return display_config != 0;
    }

    operator MirDisplayConfiguration*() const
    {
        return display_config;
    }

private:
    MirDisplayConfiguration* display_config;
};
}

volatile sig_atomic_t NativeStateMir::should_quit_(false);

NativeStateMir::~NativeStateMir()
{
    if (mir_surface_)
        mir_surface_release_sync(mir_surface_);
    if (mir_connection_)
        mir_connection_release(mir_connection_);
}

bool
NativeStateMir::init_display()
{
    struct sigaction sa;
    sa.sa_handler = &NativeStateMir::quit_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    mir_connection_ = mir_connect_sync(NULL, "glmark2");

    if (!mir_connection_is_valid(mir_connection_)) {
        Log::error("Couldn't connect to the Mir display server\n");
        return false;
    }

    return true;
}

void*
NativeStateMir::display()
{
    if (mir_connection_is_valid(mir_connection_))
        return static_cast<void*>(mir_connection_get_egl_native_display(mir_connection_));

    return 0;
}

bool
NativeStateMir::create_window(WindowProperties const& properties)
{
    static const char *win_name("glmark2 " GLMARK_VERSION);

    if (!mir_connection_is_valid(mir_connection_)) {
        Log::error("Cannot create a Mir surface without a valid connection "
                   "to the Mir display server!\n");
        return false;
    }

    /* Recreate an existing window only if it has actually been resized */
    if (mir_surface_) {
        if (properties_.fullscreen != properties.fullscreen ||
            (properties.fullscreen == false &&
             (properties_.width != properties.width ||
              properties_.height != properties.height)))
        {
            mir_surface_release_sync(mir_surface_);
            mir_surface_ = 0;
        }
        else
        {
            return true;
        }
    }

    uint32_t output_id = mir_display_output_id_invalid;

    properties_ = properties;

    if (properties_.fullscreen) {
        DisplayConfiguration display_config(mir_connection_);
        if (!display_config.is_valid()) {
            Log::error("Couldn't get display configuration from the Mir display server!\n");
            return false;
        }

        const MirDisplayOutput* active_output = find_active_output(display_config);
        if (active_output == NULL) {
            Log::error("Couldn't find an active output in the Mir display server!\n");
            return false;
        }

        const MirDisplayMode* current_mode =
            &active_output->modes[active_output->current_mode];

        properties_.width = current_mode->horizontal_resolution;
        properties_.height = current_mode->vertical_resolution;
        output_id = active_output->output_id;

        Log::debug("Making Mir surface fullscreen on output %u (%ux%u)\n",
                   output_id, properties_.width, properties_.height);
    }

    MirPixelFormat surface_format = find_best_surface_format(mir_connection_);
    if (surface_format == mir_pixel_format_invalid) {
        Log::error("Couldn't find a pixel format to use for the Mir surface!\n");
        return false;
    }

    Log::debug("Using pixel format %u for the Mir surface\n", surface_format);

    MirSurfaceSpec* spec =
        mir_connection_create_spec_for_normal_surface(mir_connection_,
                                                      properties_.width,
                                                      properties_.height,
                                                      surface_format);
    mir_surface_spec_set_name(spec, win_name);
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
    if (output_id != mir_display_output_id_invalid)
        mir_surface_spec_set_fullscreen_on_output(spec, output_id);

    mir_surface_ = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    if (!mir_surface_ || !mir_surface_is_valid(mir_surface_)) {
        Log::error("Failed to create Mir surface!\n");
        return false;
    }

    return true;
}

void*
NativeStateMir::window(WindowProperties& properties)
{
    properties = properties_;

    if (mir_surface_)
    {
        MirBufferStream* bstream = mir_surface_get_buffer_stream(mir_surface_);
        return static_cast<void*>(mir_buffer_stream_get_egl_native_window(bstream));
    }

    return 0;
}

void
NativeStateMir::visible(bool /*visible*/)
{
}

bool
NativeStateMir::should_quit()
{
    return should_quit_;
}

/*******************
 * Private methods *
 *******************/

void
NativeStateMir::quit_handler(int /*signum*/)
{
    should_quit_ = true;
}
