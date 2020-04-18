/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
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


#ifndef LIBWDS_RTSP_MACROS_H_
#define LIBWDS_RTSP_MACROS_H_

#include <stdio.h>

#define MAKE_HEX_STRING_2(NAME, PROPERTY) \
  char NAME[3]; \
  std::snprintf(NAME, sizeof(NAME), "%02X", PROPERTY) \

#define MAKE_HEX_STRING_4(NAME, PROPERTY) \
  char NAME[5]; \
  std::snprintf(NAME, sizeof(NAME), "%04X", PROPERTY) \

#define MAKE_HEX_STRING_6(NAME, PROPERTY) \
  char NAME[7]; \
  std::snprintf(NAME, sizeof(NAME), "%06X", PROPERTY) \

#define MAKE_HEX_STRING_8(NAME, PROPERTY) \
  char NAME[9]; \
  std::snprintf(NAME, sizeof(NAME), "%08X", PROPERTY) \

#define MAKE_HEX_STRING_10(NAME, PROPERTY) \
  char NAME[11]; \
  std::snprintf(NAME, sizeof(NAME), "%010llX", PROPERTY) \

#define MAKE_HEX_STRING_12(NAME, PROPERTY) \
  char NAME[13]; \
  std::snprintf(NAME, sizeof(NAME), "%012llX", PROPERTY) \

#define MAKE_HEX_STRING_16(NAME, PROPERTY) \
  char NAME[17]; \
  std::snprintf(NAME, sizeof(NAME), "%016llX", PROPERTY) \

#endif  // LIBWDS_RTSP_MACROS_H_

