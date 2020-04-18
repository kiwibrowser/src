// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_MENU_CONTROLLER_H_
#define UI_BASE_COCOA_MENU_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"

namespace ui {
class MenuModel;
}

UI_BASE_EXPORT extern NSString* const kMenuControllerMenuWillOpenNotification;
UI_BASE_EXPORT extern NSString* const kMenuControllerMenuDidCloseNotification;

// A controller for the cross-platform menu model. The menu that's created
// has the tag and represented object set for each menu item. The object is a
// NSValue holding a pointer to the model for that level of the menu (to
// allow for hierarchical menus). The tag is the index into that model for
// that particular item. It is important that the model outlives this object
// as it only maintains weak references.
UI_BASE_EXPORT
@interface MenuControllerCocoa : NSObject<NSMenuDelegate> {
 @protected
  ui::MenuModel* model_;  // Weak.
  base::scoped_nsobject<NSMenu> menu_;
}

@property(nonatomic, assign) ui::MenuModel* model;

// Whether to activate selected menu items via a posted task. This may allow the
// selection to be handled earlier, whilst the menu is fading out. If the posted
// task wasn't processed by the time the action is normally sent, it will be
// sent synchronously at that stage.
@property(nonatomic, assign) BOOL postItemSelectedAsTask;

// Note that changing this will have no effect if you use
// |-initWithModel:useWithPopUpButtonCell:| or after the first call to |-menu|.
@property(nonatomic) BOOL useWithPopUpButtonCell;

+ (base::string16)elideMenuTitle:(const base::string16&)title
                         toWidth:(int)width;

// NIB-based initializer. This does not create a menu. Clients can set the
// properties of the object and the menu will be created upon the first call to
// |-menu|. Note that the menu will be immutable after creation.
- (id)init;

// Builds a NSMenu from the pre-built model (must not be nil). Changes made
// to the contents of the model after calling this will not be noticed. If
// the menu will be displayed by a NSPopUpButtonCell, it needs to be of a
// slightly different form (0th item is empty). Note this attribute of the menu
// cannot be changed after it has been created.
- (id)initWithModel:(ui::MenuModel*)model
    useWithPopUpButtonCell:(BOOL)useWithCell;

// Programmatically close the constructed menu.
- (void)cancel;

// Access to the constructed menu if the complex initializer was used. If the
// default initializer was used, then this will create the menu on first call.
- (NSMenu*)menu;

// Whether the menu is currently open.
- (BOOL)isMenuOpen;

// NSMenuDelegate methods this class implements. Subclasses should call super
// if extending the behavior.
- (void)menuWillOpen:(NSMenu*)menu;
- (void)menuDidClose:(NSMenu*)menu;

@end

// Protected methods that subclassers can override and/or invoke.
@interface MenuControllerCocoa (Protected)

// Called before the menu is to be displayed to update the state (enabled,
// radio, etc) of each item in the menu. Also will update the title if the item
// is marked as "dynamic".
- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item;

// Adds the item at |index| in |model| as an NSMenuItem at |index| of |menu|.
// Associates a submenu if the MenuModel::ItemType is TYPE_SUBMENU.
- (void)addItemToMenu:(NSMenu*)menu
              atIndex:(NSInteger)index
            fromModel:(ui::MenuModel*)model;

// Creates a NSMenu from the given model. If the model has submenus, this can
// be invoked recursively.
- (NSMenu*)menuFromModel:(ui::MenuModel*)model;

// Returns the maximum width for the menu item. Returns -1 to indicate that
// there's no maximum width.
- (int)maxWidthForMenuModel:(ui::MenuModel*)model
                 modelIndex:(int)modelIndex;
@end

#endif  // UI_BASE_COCOA_MENU_CONTROLLER_H_
