// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/dhcp_pac_file_fetcher_factory_chromeos.h"

#include <memory>

#include "chromeos/network/dhcp_pac_file_fetcher_chromeos.h"

namespace chromeos {

DhcpPacFileFetcherFactoryChromeos::DhcpPacFileFetcherFactoryChromeos() =
    default;

DhcpPacFileFetcherFactoryChromeos::~DhcpPacFileFetcherFactoryChromeos() =
    default;

std::unique_ptr<net::DhcpPacFileFetcher>
DhcpPacFileFetcherFactoryChromeos::Create(
    net::URLRequestContext* url_request_context) {
  return std::make_unique<DhcpPacFileFetcherChromeos>(url_request_context);
}

}  // namespace chromeos
