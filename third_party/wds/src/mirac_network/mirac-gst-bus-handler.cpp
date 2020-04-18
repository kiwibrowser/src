/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2015 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <gst/gst.h>
#include "libwds/public/logging.h"


gboolean
mirac_gstbus_callback (GstBus     *bus,
                 GstMessage *message,
                 gpointer    data)
{
    GError *err = NULL;
    gchar *debug = NULL;

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error (message, &err, &debug);
        WDS_VLOG (debug);
        WDS_ERROR ("[%s] %s", g_quark_to_string(err->domain), err->message);
        g_error_free (err);
        g_free (debug);
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning (message, &err, &debug);
        WDS_VLOG (debug);
        WDS_WARNING ("[%s] %s", g_quark_to_string(err->domain), err->message);
        g_error_free (err);
        g_free (debug);
        break;
    case GST_MESSAGE_INFO:
        gst_message_parse_info (message, &err, &debug);
        WDS_VLOG (debug);
        WDS_LOG ("[%s] %s", g_quark_to_string(err->domain), err->message);
        g_error_free (err);
        g_free (debug);
        break;
    default:
        /* unhandled message */
        break;
    }

    return TRUE;
}
