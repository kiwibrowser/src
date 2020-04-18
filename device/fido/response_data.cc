// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/response_data.h"

#include <utility>

#include "base/base64url.h"
#include "base/strings/string_piece.h"
#include "device/fido/fido_parsing_utils.h"

namespace device {

ResponseData::~ResponseData() = default;

ResponseData::ResponseData() = default;

ResponseData::ResponseData(std::vector<uint8_t> raw_credential_id)
    : raw_credential_id_(std::move(raw_credential_id)) {}

ResponseData::ResponseData(ResponseData&& other) = default;

ResponseData& ResponseData::operator=(ResponseData&& other) = default;

std::string ResponseData::GetId() const {
  std::string id;
  base::Base64UrlEncode(base::StringPiece(reinterpret_cast<const char*>(
                                              raw_credential_id_.data()),
                                          raw_credential_id_.size()),
                        base::Base64UrlEncodePolicy::OMIT_PADDING, &id);
  return id;
}

bool ResponseData::CheckRpIdHash(const std::string& rp_id) const {
  return GetRpIdHash() == fido_parsing_utils::CreateSHA256Hash(rp_id);
}

}  // namespace device
