// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the "Manage Profile" subpage of
 * the People section to interact with the browser. Chrome Browser only.
 */

/**
 * Contains the possible profile shortcut statuses. These strings must be kept
 * in sync with the C++ Manage Profile handler.
 * @enum {string}
 */
const ProfileShortcutStatus = {
  PROFILE_SHORTCUT_SETTING_HIDDEN: 'profileShortcutSettingHidden',
  PROFILE_SHORTCUT_NOT_FOUND: 'profileShortcutNotFound',
  PROFILE_SHORTCUT_FOUND: 'profileShortcutFound',
};

cr.define('settings', function() {
  /** @interface */
  class ManageProfileBrowserProxy {
    /**
     * Gets the available profile icons to choose from.
     * @return {!Promise<!Array<!AvatarIcon>>}
     */
    getAvailableIcons() {}

    /**
     * Sets the profile's icon to the GAIA avatar.
     */
    setProfileIconToGaiaAvatar() {}

    /**
     * Sets the profile's icon to one of the default avatars.
     * @param {string} iconUrl The new profile URL.
     */
    setProfileIconToDefaultAvatar(iconUrl) {}

    /**
     * Sets the profile's name.
     * @param {string} name The new profile name.
     */
    setProfileName(name) {}

    /**
     * Returns whether the current profile has a shortcut.
     * @return {!Promise<ProfileShortcutStatus>}
     */
    getProfileShortcutStatus() {}

    /**
     * Adds a shortcut for the current profile.
     */
    addProfileShortcut() {}

    /**
     * Removes the shortcut of the current profile.
     */
    removeProfileShortcut() {}
  }

  /**
   * @implements {settings.ManageProfileBrowserProxy}
   */
  class ManageProfileBrowserProxyImpl {
    /** @override */
    getAvailableIcons() {
      return cr.sendWithPromise('getAvailableIcons');
    }

    /** @override */
    setProfileIconToGaiaAvatar() {
      chrome.send('setProfileIconToGaiaAvatar');
    }

    /** @override */
    setProfileIconToDefaultAvatar(iconUrl) {
      chrome.send('setProfileIconToDefaultAvatar', [iconUrl]);
    }

    /** @override */
    setProfileName(name) {
      chrome.send('setProfileName', [name]);
    }

    /** @override */
    getProfileShortcutStatus() {
      return cr.sendWithPromise('requestProfileShortcutStatus');
    }

    /** @override */
    addProfileShortcut() {
      chrome.send('addProfileShortcut');
    }

    /** @override */
    removeProfileShortcut() {
      chrome.send('removeProfileShortcut');
    }
  }

  // The singleton instance_ is replaced with a test version of this wrapper
  // during testing.
  cr.addSingletonGetter(ManageProfileBrowserProxyImpl);

  return {
    ManageProfileBrowserProxy: ManageProfileBrowserProxy,
    ManageProfileBrowserProxyImpl: ManageProfileBrowserProxyImpl,
  };
});
