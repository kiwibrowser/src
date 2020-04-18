// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Footer shown when the Files app is opened as a file/folder selecting dialog.
 * @param {DialogType} dialogType Dialog type.
 * @param {!Element} container Container of the dialog footer.
 * @param {!Element} filenameInput Filename input element.
 * @constructor
 */
function DialogFooter(dialogType, container, filenameInput) {
  /**
   * Root element of the footer.
   * @type {!Element}
   * @const
   */
  this.element = container;

  /**
   * Dialog type.
   * @type {DialogType}
   * @const
   * @private
   */
  this.dialogType_ = dialogType;

  /**
   * OK button in the footer.
   * @const
   * @type {!HTMLButtonElement}
   */
  this.okButton = /** @type {!HTMLButtonElement} */
      (container.querySelector('.ok'));

  /**
   * OK button's label in the footer.
   * @const
   * @type {!HTMLSpanElement}
   */
  this.okButtonLabel = /** @type {!HTMLSpanElement} */
      (this.okButton.querySelector('span'));

  /**
   * Cancel button in the footer.
   * @const
   * @type {!HTMLButtonElement}
   */
  this.cancelButton = /** @type {!HTMLButtonElement} */
      (container.querySelector('.cancel'));

  /**
   * New folder button in the footer.
   * @const
   * @type {!HTMLButtonElement}
   */
  this.newFolderButton = /** @type {!HTMLButtonElement} */
      (container.querySelector('#new-folder-button'));

  /**
   * File type selector in the footer.
   * @const
   * @type {!HTMLSelectElement}
   */
  this.fileTypeSelector = /** @type {!HTMLSelectElement} */
      (container.querySelector('.file-type'));

  /**
   * @const
   * @type {!Element}
   */
  this.filenameInput = filenameInput;

  // Initialize the element styles.
  container.classList.add('button-panel');

  // Set initial label for OK button. The label can be updated dynamically
  // depending on dialog types.
  this.okButtonLabel.textContent = DialogFooter.getOKButtonLabel_(dialogType);

  // Register event handlers.
  this.filenameInput.addEventListener(
      'keydown', this.onFilenameInputKeyDown_.bind(this));
  this.filenameInput.addEventListener(
      'focus', this.onFilenameInputFocus_.bind(this));
}

DialogFooter.prototype = {
  /**
   * @return {number} Selected filter index. The index is 1 based and 0 means
   *     'any file types'. Keep the meaniing consistent with the index passed to
   *     chrome.fileManagerPrivate.selectFile.
   */
  get selectedFilterIndex() {
    return ~~this.fileTypeSelector.value;
  }
};

/**
 * Finds the dialog footer element for the dialog type.
 * @param {DialogType} dialogType Dialog type.
 * @param {!Document} document Document.
 * @return {!DialogFooter} Dialog footer created with the found element.
 */
DialogFooter.findDialogFooter = function(dialogType, document) {
  return new DialogFooter(
      dialogType,
      queryRequiredElement('.dialog-footer'),
      queryRequiredElement('#filename-input-box input'));
};

/**
 * Obtains the label of OK button for the dialog type.
 * @param {DialogType} dialogType Dialog type.
 * @return {string} OK button label.
 * @private
 */
DialogFooter.getOKButtonLabel_ = function(dialogType) {
  switch (dialogType) {
    case DialogType.SELECT_UPLOAD_FOLDER:
      return str('UPLOAD_LABEL');

    case DialogType.SELECT_SAVEAS_FILE:
      return str('SAVE_LABEL');

    case DialogType.SELECT_FOLDER:
    case DialogType.SELECT_OPEN_FILE:
    case DialogType.SELECT_OPEN_MULTI_FILE:
    case DialogType.FULL_PAGE:
      return str('OPEN_LABEL');

    default:
      throw new Error('Unknown dialog type: ' + dialogType);
  }
};

/**
 * Fills the file type list or hides it.
 * @param {!Array<{extensions: Array<string>, description: string}>} fileTypes
 *     List of file type.
 * @param {boolean} includeAllFiles Whether the filter includes the 'all files'
 *     item or not.
 */
DialogFooter.prototype.initFileTypeFilter = function(
    fileTypes, includeAllFiles) {
  for (var i = 0; i < fileTypes.length; i++) {
    var fileType = fileTypes[i];
    var option = document.createElement('option');
    var description = fileType.description;
    if (!description) {
      // See if all the extensions in the group have the same description.
      for (var j = 0; j !== fileType.extensions.length; j++) {
        var currentDescription = FileListModel.getFileTypeString(
            FileType.getTypeForName('.' + fileType.extensions[j]));
        if (!description)  {
          // Set the first time.
          description = currentDescription;
        } else if (description != currentDescription) {
          // No single description, fall through to the extension list.
          description = null;
          break;
        }
      }

      if (!description) {
        // Convert ['jpg', 'png'] to '*.jpg, *.png'.
        description = fileType.extensions.map(function(s) {
          return '*.' + s;
        }).join(', ');
      }
    }
    option.innerText = description;
    option.value = i + 1;

    if (fileType.selected)
      option.selected = true;

    this.fileTypeSelector.appendChild(option);
  }

  if (includeAllFiles) {
    var option = document.createElement('option');
    option.innerText = str('ALL_FILES_FILTER');
    option.value = 0;
    this.fileTypeSelector.appendChild(option);
  }

  var options = this.fileTypeSelector.querySelectorAll('option');
  if (options.length >= 2) {
    // There is in fact no choice, show the selector.
    this.fileTypeSelector.hidden = false;
  }
};

/**
 * @param {Event} event Focus event.
 * @private
 */
DialogFooter.prototype.onFilenameInputFocus_ = function(event) {
  // On focus we want to select everything but the extension, but
  // Chrome will select-all after the focus event completes.  We
  // schedule a timeout to alter the focus after that happens.
  setTimeout(function() {
    this.selectTargetNameInFilenameInput();
  }.bind(this), 0);
};

/**
 * @param {Event} event Key event.
 * @private
 */
DialogFooter.prototype.onFilenameInputKeyDown_ = function(event) {
  if ((util.getKeyModifiers(event) + event.keyCode) === '13' /* Enter */)
    this.okButton.click();
};

DialogFooter.prototype.selectTargetNameInFilenameInput = function() {
  this.filenameInput.focus();
  var selectionEnd = this.filenameInput.value.lastIndexOf('.');
  if (selectionEnd == -1) {
    this.filenameInput.select();
  } else {
    this.filenameInput.selectionStart = 0;
    this.filenameInput.selectionEnd = selectionEnd;
  }
};
