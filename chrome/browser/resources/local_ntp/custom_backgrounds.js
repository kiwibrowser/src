// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


'use strict';

let customBackgrounds = {};

/**
 * Enum for key codes.
 * @enum {int}
 * @const
 */
customBackgrounds.KEYCODES = {
  BACKSPACE: 8,
  ENTER: 13,
  ESC: 27,
  TAB: 9,
};

/**
 * Enum for HTML element ids.
 * @enum {string}
 * @const
 */
customBackgrounds.IDS = {
  BACK: 'bg-sel-back',
  CONNECT_GOOGLE_PHOTOS: 'edit-bg-google-photos',
  CONNECT_GOOGLE_PHOTOS_TEXT: 'edit-bg-google-photos-text',
  DEFAULT_WALLPAPERS: 'edit-bg-default-wallpapers',
  DEFAULT_WALLPAPERS_TEXT: 'edit-bg-default-wallpapers-text',
  DONE: 'bg-sel-footer-done',
  EDIT_BG: 'edit-bg',
  EDIT_BG_DIALOG: 'edit-bg-dialog',
  EDIT_BG_GEAR: 'edit-bg-gear',
  EDIT_BG_OVERLAY: 'edit-bg-overlay',
  MENU: 'bg-sel-menu',
  OPTIONS_TITLE: 'edit-bg-title',
  OVERLAY: 'bg-sel-menu-overlay',
  RESTORE_DEFAULT: 'edit-bg-restore-default',
  RESTORE_DEFAULT_TEXT: 'edit-bg-restore-default-text',
  REFRESH_TEXT: 'bg-sel-refresh-text',
  REFRESH_TOGGLE: 'bg-daily-refresh',
  UPLOAD_IMAGE: 'edit-bg-upload-image',
  UPLOAD_IMAGE_TEXT: 'edit-bg-upload-image-text',
  TILES: 'bg-sel-tiles',
  TITLE: 'bg-sel-title',
};

/**
 * Enum for classnames.
 * @enum {string}
 * @const
 */
customBackgrounds.CLASSES = {
  COLLECTION_DIALOG: 'is-col-sel',
  COLLECTION_SELECTED: 'bg-selected',  // Highlight selected tile
  COLLECTION_TILE: 'bg-sel-tile',  // Preview tile for background customization
  COLLECTION_TITLE: 'bg-sel-tile-title',  // Title of a background image
  DONE_AVAILABLE: 'done-available',
  IMAGE_DIALOG: 'is-img-sel',
  SHOW_OVERLAY: 'show-overlay',
};

/**
 * Alias for document.getElementById.
 * @param {string} id The ID of the element to find.
 * @return {HTMLElement} The found element or null if not found.
 */
function $(id) {
  // eslint-disable-next-line no-restricted-properties
  return document.getElementById(id);
}

/**
 * Remove all collection tiles from the container when the dialog
 * is closed.
 */
customBackgrounds.resetSelectionDialog = function() {
  $(customBackgrounds.IDS.TILES).scrollTop = 0;
  var tileContainer = $(customBackgrounds.IDS.TILES);
  while (tileContainer.firstChild) {
    tileContainer.removeChild(tileContainer.firstChild);
  }
  $(customBackgrounds.IDS.DONE)
      .classList.remove(customBackgrounds.CLASSES.DONE_AVAILABLE);
};

/**
 * Show dialog for selecting a collection. Populates the dialog
 * with data from |coll|.
 * @param {string} dialogTitle The title to be displayed at the top of the
 * dialog.
 */
customBackgrounds.showCollectionSelectionDialog = function() {
  var tileContainer = $(customBackgrounds.IDS.TILES);
  var overlay = $(customBackgrounds.IDS.OVERLAY);

  if (!overlay.open)
    overlay.showModal();

  // Create dialog header
  $(customBackgrounds.IDS.TITLE).textContent =
      configData.translatedStrings.selectChromeWallpaper;
  $(customBackgrounds.IDS.MENU)
      .classList.add(customBackgrounds.CLASSES.COLLECTION_DIALOG);
  $(customBackgrounds.IDS.MENU)
      .classList.remove(customBackgrounds.CLASSES.IMAGE_DIALOG);

  // Create dialog tiles
  for (var i = 0; i < coll.length; ++i) {
    var tile = document.createElement('div');
    tile.classList.add(customBackgrounds.CLASSES.COLLECTION_TILE);
    tile.style.backgroundImage = 'url(' + coll[i].previewImageUrl + ')';
    tile.id = 'coll_tile_' + i;
    tile.dataset.id = coll[i].collectionId;
    tile.dataset.name = coll[i].collectionName;
    tile.tabIndex = 0;

    var title = document.createElement('div');
    title.classList.add(customBackgrounds.CLASSES.COLLECTION_TITLE);
    title.textContent = tile.dataset.name;

    var tileInteraction = function(event) {
      var tile = event.target;
      if (tile.classList.contains(customBackgrounds.CLASSES.COLLECTION_TITLE))
        tile = tile.parentNode;

      // Load images for selected collection
      var imgElement = $('ntp-images-loader');
      if (imgElement) {
        imgElement.parentNode.removeChild(imgElement);
      }
      var imgScript = document.createElement('script');
      imgScript.id = 'ntp-images-loader';
      imgScript.src = 'chrome-search://local-ntp/ntp-background-images.js?' +
          'collection_id=' + tile.dataset.id;
      document.body.appendChild(imgScript);

      imgScript.onload = function() {
        customBackgrounds.resetSelectionDialog();
        if (coll_img.length > 0 &&
            coll_img[0].collectionId == tile.dataset.id) {
          customBackgrounds.showImageSelectionDialog(tile.dataset.name);
        } else {
          overlay.close();
          customBackgrounds.resetSelectionDialog();
        }
      };
    };

    tile.onclick = tileInteraction;
    tile.onkeyup = function(event) {
      if (event.keyCode === customBackgrounds.KEYCODES.ENTER) {
        tileInteraction(event);
      }
    };

    tile.appendChild(title);
    tileContainer.appendChild(tile);
  }

  $(customBackgrounds.IDS.DONE).tabIndex = -1;

  // Create dialog footer
  $(customBackgrounds.IDS.DONE).textContent =
      configData.translatedStrings.selectionDone;

  overlay.onclick = function(event) {
    if (event.target == overlay) {
      overlay.close();
      customBackgrounds.resetSelectionDialog();
    }
  };
};


/**
 * Show dialog for selecting an image or toggling on daily refresh. Image
 * data should previous have been loaded into coll_img via
 * chrome-search://local-ntp/ntp-background-images.js?collection_id=<collection_id>
 * @param {string} dialogTitle The title to be displayed at the top of the
 */
customBackgrounds.showImageSelectionDialog = function(dialogTitle) {
  var backButton = $(customBackgrounds.IDS.BACK);
  var dailyRefresh = $(customBackgrounds.IDS.REFRESH_TOGGLE);
  var doneButton = $(customBackgrounds.IDS.DONE);
  var overlay = $(customBackgrounds.IDS.OVERLAY);
  var selectedTile = null;
  var tileContainer = $(customBackgrounds.IDS.TILES);

  $(customBackgrounds.IDS.TITLE).textContent = dialogTitle;
  $(customBackgrounds.IDS.MENU)
      .classList.remove(customBackgrounds.CLASSES.COLLECTION_DIALOG);
  $(customBackgrounds.IDS.MENU)
      .classList.add(customBackgrounds.CLASSES.IMAGE_DIALOG);

  for (var i = 0; i < coll_img.length; ++i) {
    var tile = document.createElement('div');
    tile.classList.add(customBackgrounds.CLASSES.COLLECTION_TILE);
    tile.style.backgroundImage = 'url(' + coll_img[i].thumbnailImageUrl + ')';
    tile.id = 'img_tile_' + i;
    tile.dataset.url = coll_img[i].imageUrl;
    tile.tabIndex = 0;

    var tileInteraction = function(event) {
      var tile = event.target;
      if (selectedTile) {
        selectedTile.classList.remove(
            customBackgrounds.CLASSES.COLLECTION_SELECTED);
      }
      tile.classList.add(customBackgrounds.CLASSES.COLLECTION_SELECTED);
      selectedTile = tile;

      // Turn toggle off when an image is selected.
      $(customBackgrounds.IDS.REFRESH_TOGGLE).children[0].checked = false;
      $(customBackgrounds.IDS.DONE)
          .classList.add(customBackgrounds.CLASSES.DONE_AVAILABLE);
    };

    tile.onclick = tileInteraction;
    tile.onkeyup = function(event) {
      if (event.keyCode === customBackgrounds.KEYCODES.ENTER) {
        tileInteraction(event);
        doneButton.focus();
      }
    };

    tileContainer.appendChild(tile);
  }

  $('img_tile_0').focus();
  doneButton.tabIndex = 0;

  dailyRefresh.onclick = function(event) {
    if (selectedTile) {
      selectedTile.classList.remove(
          customBackgrounds.CLASSES.COLLECTION_SELECTED);
      selectedTile = null;
    }
    $(customBackgrounds.IDS.DONE)
        .classList.add(customBackgrounds.CLASSES.DONE_AVAILABLE);
  };

  var setBackground = function(event) {
    if (!selectedTile)
      return;

    overlay.close();
    window.chrome.embeddedSearch.newTabPage.setBackgroundURL(
        selectedTile.dataset.url);
    customBackgrounds.resetSelectionDialog();
  };

  doneButton.onclick = setBackground;
  doneButton.onkeyup = function(event) {
    if (event.keyCode === customBackgrounds.KEYCODES.ENTER) {
      setBackground(event);
      $(customBackgrounds.IDS.EDIT_BG).focus();
    }
  };

  backButton.onclick = function(event) {
    customBackgrounds.resetSelectionDialog();
    customBackgrounds.showCollectionSelectionDialog();
  };
};

/**
 * Load the NTPBackgroundCollections script. It'll create a global
 * variable name "coll" which is a dict of background collections data.
 * TODO(kmilka): add error UI as part of crbug.com/848981.
 * @private
 */
customBackgrounds.loadCollections = function() {
  var collElement = $('ntp-collection-loader');
  if (collElement) {
    collElement.parentNode.removeChild(collElement);
  }
  var collScript = document.createElement('script');
  collScript.id = 'ntp-collection-loader';
  collScript.src = 'chrome-search://local-ntp/ntp-background-collections.js';
  document.body.appendChild(collScript);
};

/**
 * Display dialog with various options for custom background source.
 */
customBackgrounds.initCustomBackgrounds = function() {
  var editDialogOverlay = $(customBackgrounds.IDS.EDIT_BG_OVERLAY);
  var editDialog = $(customBackgrounds.IDS.EDIT_BG_DIALOG);

  $(customBackgrounds.IDS.CONNECT_GOOGLE_PHOTOS_TEXT).textContent =
      configData.translatedStrings.connectGooglePhotos;
  $(customBackgrounds.IDS.DEFAULT_WALLPAPERS_TEXT).textContent =
      configData.translatedStrings.defaultWallpapers;
  $(customBackgrounds.IDS.UPLOAD_IMAGE_TEXT).textContent =
      configData.translatedStrings.uploadImage;
  $(customBackgrounds.IDS.RESTORE_DEFAULT_TEXT).textContent =
      configData.translatedStrings.restoreDefaultBackground;
  $(customBackgrounds.IDS.OPTIONS_TITLE).textContent =
      configData.translatedStrings.customizeBackground;
  $(customBackgrounds.IDS.REFRESH_TEXT).textContent =
      configData.translatedStrings.dailyRefresh;

  // Control tab cycling through background options.
  $(customBackgrounds.IDS.DEFAULT_WALLPAPERS).onkeydown = function(event) {
    if (event.keyCode == customBackgrounds.KEYCODES.TAB) {
      event.preventDefault();
      $(customBackgrounds.IDS.RESTORE_DEFAULT).focus();
    }
  };

  $(customBackgrounds.IDS.RESTORE_DEFAULT).onkeydown = function(event) {
    if (event.keyCode == customBackgrounds.KEYCODES.TAB) {
      event.preventDefault();
      $(customBackgrounds.IDS.DEFAULT_WALLPAPERS).focus();
    }
  };

  $(customBackgrounds.IDS.RESTORE_DEFAULT).onfocus = function() {
    if (this.hidden) {
      $(customBackgrounds.IDS.DEFAULT_WALLPAPERS).focus();
    }
  };

  var editBackgroundInteraction = function(event) {
    customBackgrounds.loadCollections();
    $(customBackgrounds.IDS.EDIT_BG_OVERLAY)
        .classList.add(customBackgrounds.CLASSES.SHOW_OVERLAY);
    $(customBackgrounds.IDS.DEFAULT_WALLPAPERS).focus();
  };

  $(customBackgrounds.IDS.EDIT_BG).onclick = editBackgroundInteraction;
  $(customBackgrounds.IDS.EDIT_BG).onkeyup = function(event) {
    if (event.keyCode === customBackgrounds.KEYCODES.ENTER) {
      editBackgroundInteraction(event);
    }
  };

  var editDialogOverlayInteraction = function(event) {
    editDialogOverlay.classList.remove(customBackgrounds.CLASSES.SHOW_OVERLAY);
    $(customBackgrounds.IDS.EDIT_BG).focus();
  };

  editDialogOverlay.onclick = function(event) {
    if (event.target == editDialogOverlay)
      editDialogOverlayInteraction(event);
  };
  editDialogOverlay.onkeyup = function(event) {
    if (event.keyCode === customBackgrounds.KEYCODES.ESC ||
        event.keyCode === customBackgrounds.KEYCODES.BACKSPACE) {
      editDialogOverlayInteraction(event);
    }
  };

  var restoreDefaultInteraction = function(event) {
    $(customBackgrounds.IDS.EDIT_BG_OVERLAY)
        .classList.remove(customBackgrounds.CLASSES.SHOW_OVERLAY);
    window.chrome.embeddedSearch.newTabPage.setBackgroundURL('');
    $(customBackgrounds.IDS.EDIT_BG).focus();
  };
  $(customBackgrounds.IDS.RESTORE_DEFAULT).onclick = restoreDefaultInteraction;
  $(customBackgrounds.IDS.RESTORE_DEFAULT).onkeyup = function(event) {
    if (event.keyCode === customBackgrounds.KEYCODES.ENTER) {
      restoreDefaultInteraction(event);
    }
  };

  var defaultWallpapersInteraction = function(event) {
    $(customBackgrounds.IDS.EDIT_BG_OVERLAY)
        .classList.remove(customBackgrounds.CLASSES.SHOW_OVERLAY);
    if (typeof coll != 'undefined') {
      customBackgrounds.showCollectionSelectionDialog();
      $('coll_tile_0').focus();
    }
  };
  $(customBackgrounds.IDS.DEFAULT_WALLPAPERS).onclick =
      defaultWallpapersInteraction;
  $(customBackgrounds.IDS.DEFAULT_WALLPAPERS).onkeyup = function(event) {
    if (event.keyCode === customBackgrounds.KEYCODES.ENTER) {
      defaultWallpapersInteraction(event);
    }
  };

  $(customBackgrounds.IDS.MENU).onkeyup = function(event) {
    if (event.keyCode === customBackgrounds.KEYCODES.ESC ||
        event.keyCode === customBackgrounds.KEYCODES.BACKSPACE) {
      if ($(customBackgrounds.IDS.MENU)
              .classList.contains(
                  customBackgrounds.CLASSES.COLLECTION_DIALOG)) {
        $(customBackgrounds.IDS.OVERLAY).close();
        $(customBackgrounds.IDS.EDIT_BG).focus();
      } else {
        customBackgrounds.resetSelectionDialog();
        customBackgrounds.showCollectionSelectionDialog();
        $('coll_tile_0').focus();
      }
    }
  };
};
