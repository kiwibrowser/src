// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_MODELS_SIMPLE_MENU_MODEL_H_
#define UI_BASE_MODELS_SIMPLE_MENU_MODEL_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/models/menu_model.h"
#include "ui/gfx/image/image.h"

namespace ui {

class ButtonMenuItemModel;

// A simple MenuModel implementation with an imperative API for adding menu
// items. This makes it easy to construct fixed menus. Menus populated by
// dynamic data sources may be better off implementing MenuModel directly.
// The breadth of MenuModel is not exposed through this API.
class UI_BASE_EXPORT SimpleMenuModel : public MenuModel {
 public:
  class UI_BASE_EXPORT Delegate : public AcceleratorProvider {
   public:
    ~Delegate() override {}

    // Methods for determining the state of specific command ids.
    virtual bool IsCommandIdChecked(int command_id) const = 0;
    virtual bool IsCommandIdEnabled(int command_id) const = 0;
    virtual bool IsCommandIdVisible(int command_id) const;

    // Some command ids have labels, sublabels, minor text and icons that change
    // over time.
    virtual bool IsItemForCommandIdDynamic(int command_id) const;
    virtual base::string16 GetLabelForCommandId(int command_id) const;
    virtual base::string16 GetSublabelForCommandId(int command_id) const;
    virtual base::string16 GetMinorTextForCommandId(int command_id) const;
    // Gets the icon for the item with the specified id, returning true if there
    // is an icon, false otherwise.
    virtual bool GetIconForCommandId(int command_id,
                                     gfx::Image* icon) const;

    // Notifies the delegate that the item with the specified command id was
    // visually highlighted within the menu.
    virtual void CommandIdHighlighted(int command_id);

    // Performs the action associates with the specified command id.
    // The passed |event_flags| are the flags from the event which issued this
    // command and they can be examined to find modifier keys.
    virtual void ExecuteCommand(int command_id, int event_flags) = 0;

    // Notifies the delegate that the menu is about to show.
    virtual void MenuWillShow(SimpleMenuModel* source);

    // Notifies the delegate that the menu has closed.
    virtual void MenuClosed(SimpleMenuModel* source);

    // AcceleratorProvider overrides:
    // By default, returns false for all commands. Can be further overridden.
    bool GetAcceleratorForCommandId(
        int command_id,
        ui::Accelerator* accelerator) const override;
  };

  // The Delegate can be NULL, though if it is items can't be checked or
  // disabled.
  explicit SimpleMenuModel(Delegate* delegate);
  ~SimpleMenuModel() override;

  // Methods for adding items to the model.
  void AddItem(int command_id, const base::string16& label);
  void AddItemWithStringId(int command_id, int string_id);
  void AddItemWithIcon(int command_id,
                       const base::string16& label,
                       const gfx::ImageSkia& icon);
  void AddItemWithStringIdAndIcon(int command_id,
                                  int string_id,
                                  const gfx::ImageSkia& icon);
  void AddCheckItem(int command_id, const base::string16& label);
  void AddCheckItemWithStringId(int command_id, int string_id);
  void AddRadioItem(int command_id, const base::string16& label, int group_id);
  void AddRadioItemWithStringId(int command_id, int string_id, int group_id);

  // Adds a separator of the specified type to the model.
  // - Adding a separator after another separator is always invalid if they
  //   differ in type, but silently ignored if they are both NORMAL.
  // - Adding a separator to an empty model is invalid, unless they are NORMAL
  //   or SPACING. NORMAL separators are silently ignored if the model is empty.
  void AddSeparator(MenuSeparatorType separator_type);

  // These methods take pointers to various sub-models. These models should be
  // owned by the same owner of this SimpleMenuModel.
  void AddButtonItem(int command_id, ButtonMenuItemModel* model);
  void AddSubMenu(int command_id,
                  const base::string16& label,
                  MenuModel* model);
  void AddSubMenuWithStringId(int command_id, int string_id, MenuModel* model);
  void AddActionableSubMenu(int command_id,
                            const base::string16& label,
                            MenuModel* model);
  void AddActionableSubmenuWithStringIdAndIcon(int command_id,
                                               int string_id,
                                               MenuModel* model,
                                               const gfx::ImageSkia& icon);

  // Methods for inserting items into the model.
  void InsertItemAt(int index, int command_id, const base::string16& label);
  void InsertItemWithStringIdAt(int index, int command_id, int string_id);
  void InsertSeparatorAt(int index, MenuSeparatorType separator_type);
  void InsertCheckItemAt(int index,
                         int command_id,
                         const base::string16& label);
  void InsertCheckItemWithStringIdAt(int index, int command_id, int string_id);
  void InsertRadioItemAt(int index,
                         int command_id,
                         const base::string16& label,
                         int group_id);
  void InsertRadioItemWithStringIdAt(
      int index, int command_id, int string_id, int group_id);
  void InsertSubMenuAt(int index,
                       int command_id,
                       const base::string16& label,
                       MenuModel* model);
  void InsertSubMenuWithStringIdAt(
      int index, int command_id, int string_id, MenuModel* model);

  // Remove item at specified index from the model.
  void RemoveItemAt(int index);

  // Sets the icon for the item at |index|.
  void SetIcon(int index, const gfx::Image& icon);

  // Sets the sublabel for the item at |index|.
  void SetSublabel(int index, const base::string16& sublabel);

  // Sets the minor text for the item at |index|.
  void SetMinorText(int index, const base::string16& minor_text);

  // Sets the minor icon for the item at |index|.
  void SetMinorIcon(int index, const gfx::VectorIcon& minor_icon);

  // Clears all items. Note that it does not free MenuModel of submenu.
  void Clear();

  // Returns the index of the item that has the given |command_id|. Returns
  // -1 if not found.
  int GetIndexOfCommandId(int command_id) const;

  // Overridden from MenuModel:
  bool HasIcons() const override;
  int GetItemCount() const override;
  ItemType GetTypeAt(int index) const override;
  ui::MenuSeparatorType GetSeparatorTypeAt(int index) const override;
  int GetCommandIdAt(int index) const override;
  base::string16 GetLabelAt(int index) const override;
  base::string16 GetSublabelAt(int index) const override;
  base::string16 GetMinorTextAt(int index) const override;
  const gfx::VectorIcon* GetMinorIconAt(int index) const override;
  bool IsItemDynamicAt(int index) const override;
  bool GetAcceleratorAt(int index, ui::Accelerator* accelerator) const override;
  bool IsItemCheckedAt(int index) const override;
  int GetGroupIdAt(int index) const override;
  bool GetIconAt(int index, gfx::Image* icon) override;
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(int index) const override;
  bool IsEnabledAt(int index) const override;
  bool IsVisibleAt(int index) const override;
  void HighlightChangedTo(int index) override;
  void ActivatedAt(int index) override;
  void ActivatedAt(int index, int event_flags) override;
  MenuModel* GetSubmenuModelAt(int index) const override;
  void MenuWillShow() override;
  void MenuWillClose() override;
  void SetMenuModelDelegate(
      ui::MenuModelDelegate* menu_model_delegate) override;
  MenuModelDelegate* GetMenuModelDelegate() const override;

  // Sets |histogram_name_|.
  void set_histogram_name(const std::string& histogram_name) {
    histogram_name_ = histogram_name;
  }

 protected:
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }
  Delegate* delegate() { return delegate_; }

  // One or more of the menu menu items associated with the model has changed.
  // Do any handling if necessary.
  virtual void MenuItemsChanged();

 private:
  struct Item {
    Item(Item&&);
    Item(int command_id, ItemType type, base::string16 label);
    Item& operator=(Item&&);
    ~Item();

    int command_id = 0;
    ItemType type = TYPE_COMMAND;
    base::string16 label;
    base::string16 sublabel;
    base::string16 minor_text;
    const gfx::VectorIcon* minor_icon = nullptr;
    gfx::Image icon;
    int group_id = -1;
    MenuModel* submenu = nullptr;
    ButtonMenuItemModel* button_model = nullptr;
    MenuSeparatorType separator_type = NORMAL_SEPARATOR;
  };

  typedef std::vector<Item> ItemVector;

  // Records the command for UMA.
  void RecordHistogram(int command_id) const;

  // Returns |index|.
  int ValidateItemIndex(int index) const;

  // Functions for inserting items into |items_|.
  void AppendItem(Item item);
  void InsertItemAtIndex(Item item, int index);
  void ValidateItem(const Item& item);

  // Notify the delegate that the menu is closed.
  void OnMenuClosed();

  ItemVector items_;

  Delegate* delegate_;

  MenuModelDelegate* menu_model_delegate_;

  // The UMA histogram name that is be used to log command ids.
  std::string histogram_name_;

  base::WeakPtrFactory<SimpleMenuModel> method_factory_;

  DISALLOW_COPY_AND_ASSIGN(SimpleMenuModel);
};

}  // namespace ui

#endif  // UI_BASE_MODELS_SIMPLE_MENU_MODEL_H_
