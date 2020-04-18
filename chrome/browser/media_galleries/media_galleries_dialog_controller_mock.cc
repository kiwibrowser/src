// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media_galleries/media_galleries_dialog_controller_mock.h"

#include "base/strings/utf_string_conversions.h"

using ::testing::_;
using ::testing::Return;

MediaGalleriesDialogControllerMock::MediaGalleriesDialogControllerMock() {
  ON_CALL(*this, GetHeader()).
      WillByDefault(Return(base::ASCIIToUTF16("Title")));
  ON_CALL(*this, GetSubtext()).
      WillByDefault(Return(base::ASCIIToUTF16("Desc")));
  ON_CALL(*this, GetAcceptButtonText()).
      WillByDefault(Return(base::ASCIIToUTF16("OK")));
  ON_CALL(*this, GetAuxiliaryButtonText()).
      WillByDefault(Return(base::ASCIIToUTF16("Button")));
  ON_CALL(*this, GetSectionEntries(_)).
      WillByDefault(Return(Entries()));
}

MediaGalleriesDialogControllerMock::~MediaGalleriesDialogControllerMock() {
}
