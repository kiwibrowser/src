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


#include "libwds/rtsp/header.h"

#include <algorithm>

namespace wds {
namespace rtsp {

namespace {
  const char kContentLenght[] = "Content-Length: ";
  const char kContentType[] = "Content-Type: ";
  const char kContentLength[] = "Content-Length: ";
  const char kCSeq[] = "CSeq: ";
  const char kSession[] = "Session: ";
  const char kTimeout[] = ";timeout=";
  const char kPublic[] = "Public: ";
  const char kRequire[] = "Require: org.wfa.wfd1.0";
}

Header::Header() :
    cseq_(0),
    content_length_(0),
    timeout_(0),
    require_wfd_support_(false) {
}

Header::~Header() {
}

int Header::cseq() const {
  return cseq_;
}

void Header::set_cseq(int cseq) {
  cseq_ = cseq;
}

int Header::content_length() const {
  return content_length_;
}

void Header::set_content_length(int content_length) {
  content_length_ = content_length;
}

const std::string& Header::content_type() const {
  return content_type_;
}

void Header::set_content_type(const std::string& content_type) {
  content_type_ = content_type;
}

const std::string& Header::session() const {
    return session_;
}

void Header::set_session(const std::string& session) {
    session_ = session;
}

unsigned int Header::timeout() const {
    return timeout_;
}

void Header::set_timeout(int timeout) {
    timeout_ = timeout;
}

TransportHeader& Header::transport() const {
    if (!transport_)
        transport_.reset(new TransportHeader());
    return *transport_;
}

void Header::set_transport(TransportHeader* transport) {
    transport_.reset(transport);
}

bool Header::require_wfd_support() const {
  return require_wfd_support_;
}

void Header::set_require_wfd_support(bool require_wfd_support) {
  require_wfd_support_ = require_wfd_support;
}

const std::vector<Method>& Header::supported_methods() const {
  return supported_methods_;
}

void Header::add_generic_header(const std::string& key ,const std::string& value) {
    generic_headers_[key] = value;
}

const GenericHeaderMap& Header::generic_headers() const {
    return generic_headers_;
}

void Header::set_supported_methods(
    const std::vector<Method>& supported_methods) {
  supported_methods_ = supported_methods;
}

bool Header::has_method(const Method& method) const {
  return std::find (supported_methods_.begin(),
      supported_methods_.end(),
      method) != supported_methods_.end();
}

std::string Header::ToString() const {
  std::string ret;

  ret += kCSeq + std::to_string(cseq_) + CRLF;

  if (!session_.empty()) {
      if (timeout_ > 0)
          ret += kSession + session_
            + kTimeout + std::to_string(timeout_) + CRLF;
      else
          ret += kSession + session_ + CRLF;
  }

  if (content_type_.length())
    ret += kContentType + content_type_ + CRLF;

  if (content_length_)
    ret += kContentLenght + std::to_string(content_length_) + CRLF;

  ret += transport().ToString();

  if (supported_methods_.size()) {
    auto i = supported_methods_.begin();
    auto end = supported_methods_.end();
    ret += kPublic;
    while (i != end) {
      ret += MethodName::name[*i];
      if (i != --supported_methods_.end()) {
        ret += ", ";
      }
      ++i;
    }
    ret += CRLF;
  }

  if (require_wfd_support_)
    ret += std::string(kRequire) + CRLF;

  for (auto it = generic_headers_.begin(); it != generic_headers_.end(); it++)
    ret += (*it).first + ": " + (*it).second + CRLF;

  return ret + CRLF;
}

} // namespace rtsp
} // namespace wds
