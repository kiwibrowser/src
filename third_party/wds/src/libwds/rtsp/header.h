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


#ifndef LIBWDS_RTSP_HEADER_H_
#define LIBWDS_RTSP_HEADER_H_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "libwds/rtsp/constants.h"
#include "libwds/rtsp/transportheader.h"

typedef std::map<std::string, std::string> GenericHeaderMap;

namespace wds {
namespace rtsp {

class Header {
  public:
    Header();
    virtual ~Header();

    int cseq() const;
    void set_cseq(int cseq);

    int content_length() const;
    void set_content_length(int content_length);

    const std::string& content_type() const;
    void set_content_type(const std::string& content_type);

    const std::string& session() const;
    void set_session(const std::string& session);

    unsigned int timeout() const;
    void set_timeout(int timeout);

    TransportHeader& transport() const;
    void set_transport(TransportHeader* transport);

    bool require_wfd_support() const;
    void set_require_wfd_support(bool require_wfd_support);

    const std::vector<Method>& supported_methods() const;
    void set_supported_methods(const std::vector<Method>& supported_methods);
    bool has_method(const Method& method) const;

    void add_generic_header(const std::string& key ,const std::string& value);
    const GenericHeaderMap& generic_headers () const;

    std::string ToString() const;

 private:
    int cseq_;
    int content_length_;
    unsigned int timeout_;
    std::string session_;
    mutable std::unique_ptr<TransportHeader> transport_;
    std::string content_type_;
    bool require_wfd_support_;
    std::vector<Method> supported_methods_;
    GenericHeaderMap generic_headers_;
};

} // namespace rtsp
} // namespace wds

#endif // LIBWDS_RTSP_HEADER_H_
