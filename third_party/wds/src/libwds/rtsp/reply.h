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


#ifndef LIBWDS_RTSP_REPLY_H_
#define LIBWDS_RTSP_REPLY_H_

#include "libwds/rtsp/message.h"

namespace wds {
namespace rtsp {

class Reply: public Message {
 public:
  explicit Reply(int response_code = STATUS_OK);
  ~Reply() override;

  int response_code() const { return response_code_; }
  void set_response_code(int response_code) { response_code_ = response_code; }

  std::string ToString() const override;

 private:
  int response_code_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_REPLY_H_
