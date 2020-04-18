// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_MODEL_H_
#define IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_MODEL_H_

#include <vector>

#include "base/strings/string16.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

class PageInfoModelObserver;

namespace ios {
class ChromeBrowserState;
}

namespace web {
struct SSLStatus;
}

// TODO(crbug.com/227827) Merge 178763: PageInfoModel has been removed in
// upstream; check if we should use PageInfoModel.
// The model that provides the information that should be displayed in the page
// info dialog/bubble.
class PageInfoModel {
 public:
  enum SectionInfoType {
    SECTION_INFO_IDENTITY = 0,
    SECTION_INFO_CONNECTION,
    SECTION_INFO_FIRST_VISIT,
    SECTION_INFO_INTERNAL_PAGE,  // Used for chrome:// pages, etc.
  };

  // NOTE: ICON_STATE_OK ... ICON_STATE_ERROR must be listed in increasing
  // order of severity.  Code may depend on this order.
  enum SectionStateIcon {
    // No icon.
    ICON_NONE = -1,
    // State is OK.
    ICON_STATE_OK,
    // For example, unverified identity over HTTPS.
    ICON_STATE_ERROR,
    // An information icon.
    ICON_STATE_INFO,
    // Icon for offline pages.
    ICON_STATE_OFFLINE_PAGE,
    // Icon for internal pages.
    ICON_STATE_INTERNAL_PAGE,
  };

  // The button action that can be displayed in the Page Info. Only the button
  // of the first section that require one will be displayed.
  enum ButtonAction {
    // No button.
    BUTTON_NONE = -1,
    // Add a button to open help center on security related page.
    BUTTON_SHOW_SECURITY_HELP,
    // Add a button to reload the page.
    BUTTON_RELOAD,
  };

  struct SectionInfo {
    SectionInfo(SectionStateIcon icon_id,
                const base::string16& headline,
                const base::string16& description,
                SectionInfoType type,
                ButtonAction button)
        : icon_id(icon_id),
          headline(headline),
          description(description),
          type(type),
          button(button) {}

    // The overall state of the connection (error, warning, ok).
    SectionStateIcon icon_id;

    // A single line describing the section, optional.
    base::string16 headline;

    // The full description of what this section is.
    base::string16 description;

    // The type of SectionInfo we are dealing with, for example: Identity,
    // Connection, First Visit.
    SectionInfoType type;

    // The button at the bottom of the sheet that allows the user to do an extra
    // action on top of dismissing the sheet.
    ButtonAction button;
  };

  PageInfoModel(ios::ChromeBrowserState* browser_state,
                const GURL& url,
                const web::SSLStatus& ssl,
                PageInfoModelObserver* observer);
  ~PageInfoModel();

  int GetSectionCount();
  SectionInfo GetSectionInfo(int index);

  // Returns the native image type for an icon with the given id.
  gfx::Image* GetIconImage(SectionStateIcon icon_id);

  // Returns the label for the "Certificate Information", if needed.
  base::string16 GetCertificateLabel() const;

 protected:
  // Testing constructor. DO NOT USE.
  PageInfoModel();

  PageInfoModelObserver* observer_;

  std::vector<SectionInfo> sections_;

  // Label for "Certificate Information", if needed.
  base::string16 certificate_label_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PageInfoModel);
};

#endif  // IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_MODEL_H_
