// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CATALOG_PUBLIC_CPP_MANIFEST_PARSING_UTIL_H_
#define SERVICES_CATALOG_PUBLIC_CPP_MANIFEST_PARSING_UTIL_H_

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/optional.h"

namespace base {
class Value;
}

// TODO(jcivelli): http://crbug.com/687250 Remove this file and inline
//                 PopulateRequiredFiles() in Entry::Deserialize.
namespace catalog {

using RequiredFileMap = std::map<std::string, base::FilePath>;

base::Optional<RequiredFileMap> RetrieveRequiredFiles(
    const base::Value& manifest);

}  // namespace content

#endif  // SERVICES_CATALOG_PUBLIC_CPP_MANIFEST_PARSING_UTIL_H_
