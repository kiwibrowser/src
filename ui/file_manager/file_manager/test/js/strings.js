// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// loadTimeData contains localized content.
// Auto convert strings where possible using the id.
// e.g. NEW_FILE_BUTTON_LABEL => 'New file'
// Use a Proxy class to intercept get calls.

loadTimeData.data = new Proxy(
    {
      AUDIO_FILE_TYPE: '$1 audio',
      CANCEL_LABEL: 'Cancel',
      CHROMEOS_RELEASE_BOARD: 'unknown',
      COPY_FILE_NAME: 'Copying $1...',
      COPY_ITEMS_REMAINING: 'Copying $1 items...',
      DEFAULT_NEW_FOLDER_NAME: 'New Folder',
      DEFAULT_TASK_LABEL: '(default)',
      DELETE_FILE_NAME: 'Deleting "$1"...',
      DIRECTORY_ALREADY_EXISTS: 'The folder named "$1" already exists. ' +
          'Please choose a different name.',
      DOWNLOADS_DIRECTORY_LABEL: 'Downloads',
      DATE_COLUMN_LABEL: 'Date modified',
      DELETE_ITEMS_REMAINING: 'Deleting $1 items...',
      DRIVE_BUY_MORE_SPACE: 'Buy more storage...',
      DRIVE_DIRECTORY_LABEL: 'Google Drive',
      DRIVE_MENU_HELP: 'Help',
      DRIVE_MOBILE_CONNECTION_OPTION: 'Do not use mobile data for sync',
      DRIVE_MY_DRIVE_LABEL: 'My Drive',
      DRIVE_NOT_REACHED: 'Google Drive could not be reached.  ' +
          'Please <a href="javascript://">log out</a> and log back in.',
      DRIVE_OFFLINE_COLLECTION_LABEL: 'Offline',
      DRIVE_SHARED_WITH_ME_COLLECTION_LABEL: 'Shared with me',
      DRIVE_SHOW_HOSTED_FILES_OPTION: 'Show Google Docs files',
      DRIVE_VISIT_DRIVE_GOOGLE_COM: 'Go to drive.google.com...',
      DRIVE_WELCOME_CHECK_ELIGIBILITY: 'Check eligibility',
      DRIVE_WELCOME_DISMISS: 'Dismiss',
      DRIVE_WELCOME_TEXT_LONG: '<p><strong>Access files from everywhere, ' +
          'even offline.</strong> Files in Google Drive are up to date and ' +
          'available from any device.</p><p><strong>Keep your files safe.' +
          '</strong> No matter what happens to your device, your files are ' +
          'safely stored in Google Drive.</p><p><strong>Share, create and ' +
          'collaborate</strong> on files with others all in one place.</p>',
      DRIVE_WELCOME_TEXT_SHORT:
          'All files saved in this folder are backed up online automatically',
      DRIVE_WELCOME_TITLE_ALTERNATIVE: 'Get 100 GB free with Google Drive',
      EMPTY_FOLDER: 'Nothing to see here...',
      ERROR_LINUX_FILES_CONNECTION: 'Unable to view Linux Files',
      FILENAME_LABEL: 'File name',
      GALLERY_CONFIRM_DELETE_ONE: 'Are you sure you want to delete "$1"?',
      GALLERY_CONFIRM_DELETE_SOME: 'Are you sure you want to delete $1 items?',
      GDOC_DOCUMENT_FILE_TYPE: 'Google document',
      GENERIC_FILE_TYPE: '$1 file',
      GOOGLE_DRIVE_REDEEM_URL: 'http://www.google.com/intl/en/chrome/' +
          'devices/goodies.html?utm_source=filesapp&utm_medium=banner&' +
          'utm_campaign=gsg',
      IMAGE_FILE_TYPE: '$1 image',
      INSTALL_NEW_EXTENSION_LABEL: 'Install new from the webstore',
      LINUX_FILES_ROOT_LABEL: 'Linux Files',
      MANY_ENTRIES_SELECTED: '$1 items selected',
      MANY_FILES_SELECTED: '$1 files selected',
      METADATA_BOX_ALBUM_TITLE: 'Album',
      METADATA_BOX_MEDIA_MIME_TYPE: 'Type',
      METADATA_BOX_MODIFICATION_TIME: 'Modified time',
      MOVE_FILE_NAME: 'Moving $1...',
      MOVE_ITEMS_REMAINING: 'Moving $1 items...',
      NAME_COLUMN_LABEL: 'Name',
      OFFLINE_COLUMN_LABEL: 'Available offline',
      OK_LABEL: 'OK',
      ONE_DIRECTORY_SELECTED: '1 folder selected',
      ONE_FILE_SELECTED: '1 file selected',
      OPEN_LABEL: 'Open',
      PLAIN_TEXT_FILE_TYPE: 'Plain text',
      PREPARING_LABEL: 'Preparing',
      SEE_MENU_FOR_ACTIONS: 'More options available on the action bar. ' +
          'Press Alt + A to focus the action bar.',
      SIZE_COLUMN_LABEL: 'Size',
      SPACE_AVAILABLE: '$1 available',
      STATUS_COLUMN_LABEL: 'Status',
      TIME_TODAY: 'Today $1',
      TYPE_COLUMN_LABEL: 'Type',
      UI_LOCALE: 'en_US',
      RECENT_ROOT_LABEL: 'Recent',
      SEARCH_TEXT_LABEL: 'Search',
      SELECT_ALL_COMMAND_LABEL: 'Select all',
      SIZE_BYTES: '$1 bytes',
      SIZE_GB: '$1 GB',
      SIZE_KB: '$1 KB',
      SIZE_PB: '$1 PB',
      SUGGEST_DIALOG_INSTALLATION_FAILED: 'Installation failed.',
      SUGGEST_DIALOG_INSTALLING_SPINNER_ALT: 'Installing',
      SUGGEST_DIALOG_LINK_TO_WEBSTORE: 'See more...',
      SUGGEST_DIALOG_LOADING_SPINNER_ALT: 'Loading',
      SUGGEST_DIALOG_TITLE: 'Select an app to open this file',
      TASK_OPEN: 'Open',
      TOGGLE_HIDDEN_FILES_COMMAND_LABEL: 'Show hidden files',
      VIDEO_FILE_TYPE: '$1 video',
      WAITING_FOR_SPACE_INFO: 'Waiting for space info...',
      language: 'en',
      textdirection: 'ltr',
    },
    {
      get: (target, prop) => {
        // Return any specific result from above.
        if (prop in target)
          return target[prop];
        // Single word as-is.
        if (!prop.includes('_')) {
          return prop.substring(0, 1) + prop.substring(1).toLowerCase();
        }
        // List of regexps to match against for auto-convert.
        var autoConvert = [
          /^CLOUD_IMPORT_/,
          /^DRIVE_SHARE_TYPE_/,
          /^METADATA_BOX_EXIF_/,
          /^METADATA_BOX_FILE_/,
          /^METADATA_BOX_MEDIA_/,
          /^METADATA_BOX_/,
          /^QUICK_VIEW_/,
          /^SHORTCUT_/,
          /_BUTTON_LABEL$/,
          /_BUTTON_TOOLTIP$/,
        ];
        for (var i = 0; i < autoConvert.length; i++) {
          if (prop.match(autoConvert[i])) {
            // Keep first capital, lower case the rest, convert '_' to ' '.
            var r = prop.replace(autoConvert[i], '');
            return r.substring(0, 1) +
                r.substring(1).toLowerCase().replace(/_/g, ' ');
          }
        }
        console.error('Unknown loadTimeData [' + prop + ']');
        return prop;
      }
    });

// Overwrite LoadTimeData.prototype.data setter as nop.
// Default implementation throws errors when both background and
// foreground re-set loadTimeData.data.
Object.defineProperty(LoadTimeData.prototype, 'data', {set: () => {}});
