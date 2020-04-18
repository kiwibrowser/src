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

#include "mirac-glib-logging.hpp"

#include <glib.h>

namespace {

void MiracGlibLog(const char* format, ...) {
    va_list va;
    va_start(va, format);
    g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, format, va);
    va_end(va);
}

void MiracGlibVLog(const char* format, ...) {
    va_list va;
    va_start(va, format);
    g_logv("rtsp", G_LOG_LEVEL_DEBUG, format, va);
    va_end(va);
}

void MiracGlibWarning(const char* format, ...) {
    va_list va;
    va_start(va, format);
    g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, format, va);
    va_end(va);
}

void MiracGlibError(const char* format, ...) {
    va_list va;
    va_start(va, format);
    g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, format, va);
    va_end(va);
}

}  // namespace

void InitGlibLogging() {
    wds::LogSystem::set_log_func(&MiracGlibLog);
    wds::LogSystem::set_vlog_func(&MiracGlibVLog);
    wds::LogSystem::set_warning_func(&MiracGlibWarning);
    wds::LogSystem::set_error_func(&MiracGlibError);
}

