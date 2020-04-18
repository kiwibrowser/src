// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_PUBLIC_RAPPOR_UTILS_H_
#define COMPONENTS_RAPPOR_PUBLIC_RAPPOR_UTILS_H_

#include <string>

#include "components/rappor/public/rappor_service.h"

class GURL;

namespace rappor {

// Records a string to a Rappor metric.
// If |rappor_service| is NULL, this call does nothing.
void SampleString(RapporService* rappor_service,
                  const std::string& metric,
                  RapporType type,
                  const std::string& sample);

// Extract the domain and registry for a sample from a GURL.
// For file:// urls this will just return "file://" and for other special
// schemes like chrome-extension will return the scheme and host.
std::string GetDomainAndRegistrySampleFromGURL(const GURL& gurl);

// Records the domain and registry of a url to a Rappor metric.
// If |rappor_service| is NULL, this call does nothing.
void SampleDomainAndRegistryFromGURL(RapporService* rappor_service,
                                     const std::string& metric,
                                     const GURL& gurl);

// Returns NULL if there is no default service.
RapporService* GetDefaultService();

void SetDefaultServiceAccessor(RapporService* (*getDefaultService)());

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_PUBLIC_RAPPOR_UTILS_H_
