/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2016 Intel Corporation.
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

#include "libwds/public/logging.h"

namespace wds {

static void Dummy(const char*, ...) {}
LogSystem::LogFunction LogSystem::log_func_ = &Dummy;
LogSystem::LogFunction LogSystem::vlog_func_ = &Dummy;
LogSystem::LogFunction LogSystem::warning_func_ = &Dummy;
LogSystem::LogFunction LogSystem::error_func_ = &Dummy;

void LogSystem::set_log_func(LogFunction func) {
  log_func_ = func;
}

LogSystem::LogFunction LogSystem::log_func() {
  return log_func_;
}

void LogSystem::set_vlog_func(LogFunction func) {
  vlog_func_ = func;
}

LogSystem::LogFunction LogSystem::vlog_func() {
  return vlog_func_;
}

void LogSystem::set_warning_func(LogFunction func) {
  warning_func_ = func;
}

LogSystem::LogFunction LogSystem::warning_func() {
  return warning_func_;
}

void LogSystem::set_error_func(LogFunction func) {
  error_func_ = func;
}

LogSystem::LogFunction LogSystem::error_func() {
  return error_func_;
}

}  // namespace wds
