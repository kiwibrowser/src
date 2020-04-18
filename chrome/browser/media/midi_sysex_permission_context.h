// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIDI_SYSEX_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_MEDIA_MIDI_SYSEX_PERMISSION_CONTEXT_H_

#include "base/macros.h"
#include "chrome/browser/permissions/permission_context_base.h"

class GURL;
class PermissionRequestID;

class MidiSysexPermissionContext : public PermissionContextBase {
 public:
  explicit MidiSysexPermissionContext(Profile* profile);
  ~MidiSysexPermissionContext() override;

 private:
  // PermissionContextBase:
  void UpdateTabContext(const PermissionRequestID& id,
                        const GURL& requesting_frame,
                        bool allowed) override;
  bool IsRestrictedToSecureOrigins() const override;

  DISALLOW_COPY_AND_ASSIGN(MidiSysexPermissionContext);
};

#endif  // CHROME_BROWSER_MEDIA_MIDI_SYSEX_PERMISSION_CONTEXT_H_
