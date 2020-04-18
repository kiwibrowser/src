// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_icon_loader.h"

AppIconLoader::AppIconLoader() {}

AppIconLoader::AppIconLoader(Profile* profile,
                             int icon_size,
                             AppIconLoaderDelegate* delegate)
    : profile_(profile), icon_size_(icon_size), delegate_(delegate) {}

AppIconLoader::~AppIconLoader() {}
