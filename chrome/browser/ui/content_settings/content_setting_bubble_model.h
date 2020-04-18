// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_MODEL_H_
#define CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_MODEL_H_

#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/ui/blocked_content/framebust_block_tab_helper.h"
#include "chrome/common/custom_handlers/protocol_handler.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/common/media_stream_request.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

class ContentSettingBubbleModelDelegate;
class Profile;
class ProtocolHandlerRegistry;

namespace content {
class WebContents;
}

namespace rappor {
class RapporServiceImpl;
}

// The hierarchy of bubble models:
//
// ContentSettingBubbleModel                  - base class
//   ContentSettingSimpleBubbleModel             - single content setting
//     ContentSettingMixedScriptBubbleModel        - mixed script
//     ContentSettingRPHBubbleModel                - protocol handlers
//     ContentSettingMidiSysExBubbleModel          - midi sysex
//     ContentSettingDomainListBubbleModel         - domain list (geolocation)
//     ContentSettingPluginBubbleModel             - plugins
//     ContentSettingSingleRadioGroup              - radio group
//       ContentSettingCookiesBubbleModel            - cookies
//       ContentSettingPopupBubbleModel              - popups
//       ContentSettingFramebustBlockBubbleModel     - blocked frame busting
//   ContentSettingMediaStreamBubbleModel        - media (camera and mic)
//   ContentSettingSubresourceFilterBubbleModel  - filtered subresources
//   ContentSettingDownloadsBubbleModel          - automatic downloads

// Forward declaration necessary for downcasts.
class ContentSettingSimpleBubbleModel;
class ContentSettingMediaStreamBubbleModel;
class ContentSettingSubresourceFilterBubbleModel;
class ContentSettingDownloadsBubbleModel;
class ContentSettingFramebustBlockBubbleModel;

// This model provides data for ContentSettingBubble, and also controls
// the action triggered when the allow / block radio buttons are triggered.
class ContentSettingBubbleModel : public content::NotificationObserver {
 public:
  typedef ContentSettingBubbleModelDelegate Delegate;

  struct ListItem {
    ListItem(const gfx::Image& image,
             const base::string16& title,
             bool has_link,
             int32_t item_id)
        : image(image), title(title), has_link(has_link), item_id(item_id) {}

    gfx::Image image;
    base::string16 title;
    bool has_link;
    int32_t item_id;
  };
  typedef std::vector<ListItem> ListItems;

  class Owner {
   public:
    virtual void OnListItemAdded(const ListItem& item) {}
    virtual void OnListItemRemovedAt(int index) {}

   protected:
    virtual ~Owner() = default;
  };

  typedef std::vector<base::string16> RadioItems;
  struct RadioGroup {
    RadioGroup();
    ~RadioGroup();

    GURL url;
    base::string16 title;
    RadioItems radio_items;
    int default_item;
  };

  struct DomainList {
    DomainList();
    DomainList(const DomainList& other);
    ~DomainList();

    base::string16 title;
    std::set<std::string> hosts;
  };

  struct MediaMenu {
    MediaMenu();
    MediaMenu(const MediaMenu& other);
    ~MediaMenu();

    base::string16 label;
    content::MediaStreamDevice default_device;
    content::MediaStreamDevice selected_device;
    bool disabled;
  };
  typedef std::map<content::MediaStreamType, MediaMenu> MediaMenuMap;

  enum class ManageTextStyle {
    // No Manage button or checkbox is displayed.
    kNone,
    // Manage text is displayed as a non-prominent button.
    kButton,
    // Manage text is used as a checkbox title.
    kCheckbox,
  };

  struct BubbleContent {
    BubbleContent();
    ~BubbleContent();

    base::string16 title;
    base::string16 message;
    ListItems list_items;
    RadioGroup radio_group;
    bool radio_group_enabled = false;
    std::vector<DomainList> domain_lists;
    base::string16 custom_link;
    bool custom_link_enabled = false;
    base::string16 manage_text;
    ManageTextStyle manage_text_style = ManageTextStyle::kButton;
    MediaMenuMap media_menus;
    bool show_learn_more = false;
    base::string16 done_button_text;

   private:
    DISALLOW_COPY_AND_ASSIGN(BubbleContent);
  };

  static const int kAllowButtonIndex;

  // Creates a bubble model for a particular |content_type|. Note that not all
  // bubbles fit this description.
  // TODO(msramek): Move this to ContentSettingSimpleBubbleModel or remove
  // entirely.
  static ContentSettingBubbleModel* CreateContentSettingBubbleModel(
      Delegate* delegate,
      content::WebContents* web_contents,
      Profile* profile,
      ContentSettingsType content_type);

  ~ContentSettingBubbleModel() override;

  const BubbleContent& bubble_content() const { return bubble_content_; }

  void set_owner(Owner* owner) { owner_ = owner; }

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  virtual void OnRadioClicked(int radio_index) {}
  virtual void OnListItemClicked(int index, int event_flags) {}
  virtual void OnCustomLinkClicked() {}
  virtual void OnManageButtonClicked() {}
  virtual void OnManageCheckboxChecked(bool is_checked) {}
  virtual void OnLearnMoreClicked() {}
  virtual void OnMediaMenuClicked(content::MediaStreamType type,
                                  const std::string& selected_device_id) {}

  // Called by the view code when the bubble is closed by the user using the
  // Done button.
  virtual void OnDoneClicked() {}

  // TODO(msramek): The casting methods below are only necessary because
  // ContentSettingBubbleController in the Cocoa UI needs to know the type of
  // the bubble it wraps. Find a solution that does not require reflection nor
  // recreating the entire hierarchy for Cocoa UI.
  // Cast this bubble into ContentSettingSimpleBubbleModel if possible.
  virtual ContentSettingSimpleBubbleModel* AsSimpleBubbleModel();

  // Cast this bubble into ContentSettingMediaStreamBubbleModel if possible.
  virtual ContentSettingMediaStreamBubbleModel* AsMediaStreamBubbleModel();

  // Cast this bubble into ContentSettingSubresourceFilterBubbleModel
  // if possible.
  virtual ContentSettingSubresourceFilterBubbleModel*
  AsSubresourceFilterBubbleModel();

  // Cast this bubble into ContentSettingDownloadsBubbleModel if possible.
  virtual ContentSettingDownloadsBubbleModel* AsDownloadsBubbleModel();

  // Cast this bubble into ContentSettingFramebustBlockBubbleModel if possible.
  virtual ContentSettingFramebustBlockBubbleModel*
  AsFramebustBlockBubbleModel();

  // Sets the Rappor service used for testing.
  void SetRapporServiceImplForTesting(
      rappor::RapporServiceImpl* rappor_service) {
    rappor_service_ = rappor_service;
  }

 protected:
  ContentSettingBubbleModel(
      Delegate* delegate,
      content::WebContents* web_contents,
      Profile* profile);

  content::WebContents* web_contents() const { return web_contents_; }
  Profile* profile() const { return profile_; }
  Delegate* delegate() const { return delegate_; }

  void set_title(const base::string16& title) { bubble_content_.title = title; }
  void set_message(const base::string16& message) {
    bubble_content_.message = message;
  }
  void AddListItem(const ListItem& item);
  void RemoveListItem(int index);
  void set_radio_group(const RadioGroup& radio_group) {
    bubble_content_.radio_group = radio_group;
  }
  void set_radio_group_enabled(bool enabled) {
    bubble_content_.radio_group_enabled = enabled;
  }
  void add_domain_list(const DomainList& domain_list) {
    bubble_content_.domain_lists.push_back(domain_list);
  }
  void set_custom_link(const base::string16& link) {
    bubble_content_.custom_link = link;
  }
  void set_custom_link_enabled(bool enabled) {
    bubble_content_.custom_link_enabled = enabled;
  }
  void set_manage_text(const base::string16& text) {
    bubble_content_.manage_text = text;
  }
  void set_manage_text_style(ManageTextStyle manage_text_style) {
    bubble_content_.manage_text_style = manage_text_style;
  }
  void add_media_menu(content::MediaStreamType type, const MediaMenu& menu) {
    bubble_content_.media_menus[type] = menu;
  }
  void set_selected_device(const content::MediaStreamDevice& device) {
    bubble_content_.media_menus[device.type].selected_device = device;
  }
  void set_show_learn_more(bool show_learn_more) {
    bubble_content_.show_learn_more = show_learn_more;
  }
  void set_done_button_text(const base::string16& done_button_text) {
    bubble_content_.done_button_text = done_button_text;
  }
  rappor::RapporServiceImpl* rappor_service() const { return rappor_service_; }

 private:
  content::WebContents* web_contents_;
  Profile* profile_;
  Owner* owner_;
  Delegate* delegate_;
  BubbleContent bubble_content_;
  // A registrar for listening for WEB_CONTENTS_DESTROYED notifications.
  content::NotificationRegistrar registrar_;
  // The service used to record Rappor metrics. Can be set for testing.
  rappor::RapporServiceImpl* rappor_service_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingBubbleModel);
};

// A generic bubble used for a single content setting.
class ContentSettingSimpleBubbleModel : public ContentSettingBubbleModel {
 public:
  ContentSettingSimpleBubbleModel(Delegate* delegate,
                                  content::WebContents* web_contents,
                                  Profile* profile,
                                  ContentSettingsType content_type);

  ContentSettingsType content_type() { return content_type_; }

  // ContentSettingBubbleModel implementation.
  ContentSettingSimpleBubbleModel* AsSimpleBubbleModel() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(FramebustBlockBrowserTest, ManageButtonClicked);

  // ContentSettingBubbleModel implementation.
  void SetTitle();
  void SetMessage();
  void SetManageText();
  void OnManageButtonClicked() override;
  void SetCustomLink();
  void OnCustomLinkClicked() override;

  ContentSettingsType content_type_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingSimpleBubbleModel);
};

// RPH stands for Register Protocol Handler.
class ContentSettingRPHBubbleModel : public ContentSettingSimpleBubbleModel {
 public:
  ContentSettingRPHBubbleModel(Delegate* delegate,
                               content::WebContents* web_contents,
                               Profile* profile,
                               ProtocolHandlerRegistry* registry);
  ~ContentSettingRPHBubbleModel() override;

  void OnRadioClicked(int radio_index) override;
  void OnDoneClicked() override;

 private:
  void RegisterProtocolHandler();
  void UnregisterProtocolHandler();
  void IgnoreProtocolHandler();
  void ClearOrSetPreviousHandler();
  void PerformActionForSelectedItem();

  int selected_item_;
  // Initially false, set to true if the user explicitly interacts with the
  // bubble.
  bool interacted_;
  ProtocolHandlerRegistry* registry_;
  ProtocolHandler pending_handler_;
  ProtocolHandler previous_handler_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingRPHBubbleModel);
};

// The model of the content settings bubble for media settings.
class ContentSettingMediaStreamBubbleModel : public ContentSettingBubbleModel {
 public:
  ContentSettingMediaStreamBubbleModel(Delegate* delegate,
                                       content::WebContents* web_contents,
                                       Profile* profile);

  ~ContentSettingMediaStreamBubbleModel() override;

  // ContentSettingBubbleModel:
  ContentSettingMediaStreamBubbleModel* AsMediaStreamBubbleModel() override;
  void OnManageButtonClicked() override;

 private:
  // Helper functions to check if this bubble was invoked for microphone,
  // camera, or both devices.
  bool MicrophoneAccessed() const;
  bool CameraAccessed() const;

  bool MicrophoneBlocked() const;
  bool CameraBlocked() const;

  void SetTitle();
  void SetMessage();
  void SetManageText();

  // Sets the data for the radio buttons of the bubble.
  void SetRadioGroup();

  // Sets the data for the media menus of the bubble.
  void SetMediaMenus();

  // Sets the string that suggests reloading after the settings were changed.
  void SetCustomLink();

  // Updates the camera and microphone setting with the passed |setting|.
  void UpdateSettings(ContentSetting setting);

  // Updates the camera and microphone default device with the passed |type|
  // and device.
  void UpdateDefaultDeviceForType(content::MediaStreamType type,
                                  const std::string& device);

  // ContentSettingBubbleModel implementation.
  void OnRadioClicked(int radio_index) override;
  void OnMediaMenuClicked(content::MediaStreamType type,
                          const std::string& selected_device) override;

  // The index of the selected radio item.
  int selected_item_;
  // The content settings that are associated with the individual radio
  // buttons.
  ContentSetting radio_item_setting_[2];
  // The state of the microphone and camera access.
  TabSpecificContentSettings::MicrophoneCameraState state_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingMediaStreamBubbleModel);
};

// The model for the deceptive content bubble.
class ContentSettingSubresourceFilterBubbleModel
    : public ContentSettingBubbleModel {
 public:
  ContentSettingSubresourceFilterBubbleModel(Delegate* delegate,
                                             content::WebContents* web_contents,
                                             Profile* profile);

  ~ContentSettingSubresourceFilterBubbleModel() override;

 private:
  void SetMessage();
  void SetTitle();
  void SetManageText();

  // ContentSettingBubbleModel:
  void OnManageCheckboxChecked(bool is_checked) override;
  ContentSettingSubresourceFilterBubbleModel* AsSubresourceFilterBubbleModel()
      override;
  void OnLearnMoreClicked() override;
  void OnDoneClicked() override;

  bool is_checked_ = false;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingSubresourceFilterBubbleModel);
};

// The model for automatic downloads setting.
class ContentSettingDownloadsBubbleModel : public ContentSettingBubbleModel {
 public:
  ContentSettingDownloadsBubbleModel(Delegate* delegate,
                                     content::WebContents* web_contents,
                                     Profile* profile);
  ~ContentSettingDownloadsBubbleModel() override;

  // ContentSettingBubbleModel overrides:
  ContentSettingDownloadsBubbleModel* AsDownloadsBubbleModel() override;

 private:
  void SetRadioGroup();
  void SetTitle();
  void SetManageText();

  // ContentSettingBubbleModel overrides:
  void OnRadioClicked(int radio_index) override;
  void OnManageButtonClicked() override;

  int selected_item_ = 0;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingDownloadsBubbleModel);
};

class ContentSettingSingleRadioGroup : public ContentSettingSimpleBubbleModel {
 public:
  ContentSettingSingleRadioGroup(Delegate* delegate,
                                 content::WebContents* web_contents,
                                 Profile* profile,
                                 ContentSettingsType content_type);
  ~ContentSettingSingleRadioGroup() override;

 protected:
  bool settings_changed() const;
  int selected_item() const { return selected_item_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(FramebustBlockBrowserTest, AllowRadioButtonSelected);
  FRIEND_TEST_ALL_PREFIXES(FramebustBlockBrowserTest,
                           DisallowRadioButtonSelected);

  void SetRadioGroup();
  void SetNarrowestContentSetting(ContentSetting setting);
  void OnRadioClicked(int radio_index) override;

  ContentSetting block_setting_;
  int selected_item_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingSingleRadioGroup);
};

#if !defined(OS_ANDROID)
// The model for the blocked Framebust bubble.
class ContentSettingFramebustBlockBubbleModel
    : public ContentSettingSingleRadioGroup,
      public FramebustBlockTabHelper::Observer {
 public:
  ContentSettingFramebustBlockBubbleModel(Delegate* delegate,
                                          content::WebContents* web_contents,
                                          Profile* profile);

  ~ContentSettingFramebustBlockBubbleModel() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // ContentSettingBubbleModel:
  void OnListItemClicked(int index, int event_flags) override;
  ContentSettingFramebustBlockBubbleModel* AsFramebustBlockBubbleModel()
      override;

  // FramebustBlockTabHelper::Observer:
  void OnBlockedUrlAdded(const GURL& blocked_url) override;

 private:
  ListItem CreateListItem(const GURL& url);

  DISALLOW_COPY_AND_ASSIGN(ContentSettingFramebustBlockBubbleModel);
};
#endif  // !defined(OS_ANDROID)

#endif  // CHROME_BROWSER_UI_CONTENT_SETTINGS_CONTENT_SETTING_BUBBLE_MODEL_H_
