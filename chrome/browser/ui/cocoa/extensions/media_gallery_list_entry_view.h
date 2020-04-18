// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_MEDIA_GALLERY_LIST_ENTRY_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_MEDIA_GALLERY_LIST_ENTRY_VIEW_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/media_galleries/media_galleries_preferences.h"
#import "ui/base/models/menu_model.h"

@class MediaGalleryButton;

class MediaGalleryListEntryController {
 public:
  virtual void OnCheckboxToggled(MediaGalleryPrefId pref_id, bool checked) {}
  virtual void OnFolderViewerClicked(MediaGalleryPrefId pref_id) {}
  virtual ui::MenuModel* GetContextMenu(MediaGalleryPrefId pref_id);

 protected:
  virtual ~MediaGalleryListEntryController() {}
};

@interface MediaGalleryListEntry : NSView {
 @private
  MediaGalleryListEntryController* controller_;  // |controller_| owns |this|.
  MediaGalleryPrefId prefId_;

  base::scoped_nsobject<MediaGalleryButton> checkbox_;
  base::scoped_nsobject<NSTextField> details_;
}

// Does size to fit if frameRect is empty.
- (id)initWithFrame:(NSRect)frameRect
         controller:(MediaGalleryListEntryController*)controller_
           prefInfo:(const MediaGalleryPrefInfo&)prefInfo;

- (void)setState:(bool)selected;

@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_MEDIA_GALLERY_LIST_ENTRY_VIEW_H_
