// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Responsible for showing following banners in the file list.
 *  - WelcomeBanner
 *  - AuthFailBanner
 * @param {DirectoryModel} directoryModel The model.
 * @param {VolumeManagerWrapper} volumeManager The manager.
 * @param {Document} document HTML document.
 * @param {boolean} showWelcome True if the welcome banner can be shown.
 * @constructor
 * @extends {cr.EventTarget}
 */
function Banners(
    directoryModel, volumeManager, document, showWelcome) {
  this.directoryModel_ = directoryModel;
  this.volumeManager_ = volumeManager;
  this.document_ = assert(document);
  this.showOffers_ = false;
  this.showWelcome_ = showWelcome;
  this.driveEnabled_ = false;

  this.usePromoWelcomeBanner_ = true;
  this.privateOnDirectoryChangedBound_ =
      this.privateOnDirectoryChanged_.bind(this);

  var handler = this.checkSpaceAndMaybeShowWelcomeBanner_.bind(this);
  this.directoryModel_.addEventListener('scan-completed', handler);
  this.directoryModel_.addEventListener('rescan-completed', handler);
  this.directoryModel_.addEventListener('directory-changed',
      this.onDirectoryChanged_.bind(this));

  this.unmountedPanel_ = this.document_.querySelector('#unmounted-panel');
  this.volumeManager_.volumeInfoList.addEventListener(
      'splice', this.onVolumeInfoListSplice_.bind(this));
  this.volumeManager_.addEventListener('drive-connection-changed',
      this.onDriveConnectionChanged_.bind(this));

  chrome.storage.onChanged.addListener(this.onStorageChange_.bind(this));
  this.welcomeHeaderCounter_ = WELCOME_HEADER_COUNTER_LIMIT;
  this.warningDismissedCounter_ = 0;
  this.downloadsWarningDismissedTime_ = 0;

  this.ready_ = Promise.all([
    new Promise(function(resolve, reject) {
      chrome.storage.local.get(
          [
            WELCOME_HEADER_COUNTER_KEY,
            DRIVE_WARNING_DISMISSED_KEY,
            DOWNLOADS_WARNING_DISMISSED_KEY
          ],
          function(values) {
            if (chrome.runtime.lastError) {
              reject('Failed to load banner data from chrome.storage: ' +
                  chrome.runtime.lastError.message);
              return;
            }
            this.welcomeHeaderCounter_ =
                parseInt(values[WELCOME_HEADER_COUNTER_KEY], 10) || 0;
            this.warningDismissedCounter_ =
                parseInt(values[DRIVE_WARNING_DISMISSED_KEY], 10) || 0;
            this.downloadsWarningDismissedTime_ =
                parseInt(values[DOWNLOADS_WARNING_DISMISSED_KEY], 10) || 0;

            // If it's in test, override the counter to show the header by
            // force.
            if (chrome.test) {
              this.welcomeHeaderCounter_ = 0;
              this.warningDismissedCounter_ = 0;
            }
            resolve();
          }.bind(this));
    }.bind(this)),
    new Promise(function(resolve) {
      // Get the 'allowRedeemOffers' preference for banners.
      chrome.fileManagerPrivate.getPreferences(function(pref) {
        this.showOffers_ = pref.allowRedeemOffers;
        resolve();
      }.bind(this));
    }.bind(this))
  ]).catch(function(error) {
    console.error(error);
  });

  // Authentication failed banner.
  this.authFailedBanner_ =
      this.document_.querySelector('#drive-auth-failed-warning');
  var authFailedText = this.authFailedBanner_.querySelector('.drive-text');
  authFailedText.innerHTML = util.htmlUnescape(str('DRIVE_NOT_REACHED'));
  authFailedText.querySelector('a').addEventListener('click', function(e) {
    chrome.fileManagerPrivate.logoutUserForReauthentication();
    e.preventDefault();
  });
  this.maybeShowAuthFailBanner_();
}

/**
 * Banners extends cr.EventTarget.
 */
Banners.prototype.__proto__ = cr.EventTarget.prototype;

/**
 * Key in localStorage to keep number of times the Drive Welcome
 * banner has shown.
 */
var WELCOME_HEADER_COUNTER_KEY = 'driveWelcomeHeaderCounter';

// If the warning was dismissed before, this key stores the quota value
// (as of the moment of dismissal).
// If the warning was never dismissed or was reset this key stores 0.
var DRIVE_WARNING_DISMISSED_KEY = 'driveSpaceWarningDismissed';

/**
 * If the warning was dismissed before, this key stores the moment of dismissal
 * in milliseconds since the UNIX epoch (i.e. the value of Date.now()).
 * @const {string}
 */
var DOWNLOADS_WARNING_DISMISSED_KEY = 'downloadsSpaceWarningDismissed';

/**
 * Maximum times Drive Welcome banner could have shown.
 */
var WELCOME_HEADER_COUNTER_LIMIT = 25;

/**
 * If the remaining space in Drive is less than 10%, we'll show a warning.
 * @const {number}
 */
var DRIVE_SPACE_WARNING_THRESHOLD_RATIO = 0.1;

/**
 * If the remaining space in Downloads is less than 1GB, we'll show a warning.
 * This value is in bytes.
 * @const {number}
 */
var DOWNLOADS_SPACE_WARNING_THRESHOLD_SIZE = 1 * 1024 * 1024 * 1024;

/**
 * After the warning banner is dismissed, we won't show it for 36 hours.
 * This value is in milliseconds.
 * @const {number}
 */
var DOWNLOADS_SPACE_WARNING_DISMISS_DURATION = 36 * 60 * 60 * 1000;

/**
 * @param {number} value How many times the Drive Welcome header banner
 * has shown.
 * @private
 */
Banners.prototype.setWelcomeHeaderCounter_ = function(value) {
  var values = {};
  values[WELCOME_HEADER_COUNTER_KEY] = value;
  chrome.storage.local.set(values);
};

/**
 * @param {number} value How many times the low space warning has dismissed.
 * @private
 */
Banners.prototype.setWarningDismissedCounter_ = function(value) {
  var values = {};
  values[DRIVE_WARNING_DISMISSED_KEY] = value;
  chrome.storage.local.set(values);
};

/**
 * chrome.storage.onChanged event handler.
 * @param {Object<Object>} changes Changes values.
 * @param {string} areaName "local" or "sync".
 * @private
 */
Banners.prototype.onStorageChange_ = function(changes, areaName) {
  if (areaName == 'local' && WELCOME_HEADER_COUNTER_KEY in changes) {
    this.welcomeHeaderCounter_ = changes[WELCOME_HEADER_COUNTER_KEY].newValue;
  }
  if (areaName == 'local' && DRIVE_WARNING_DISMISSED_KEY in changes) {
    this.warningDismissedCounter_ =
        changes[DRIVE_WARNING_DISMISSED_KEY].newValue;
  }
  if (areaName == 'local' && DOWNLOADS_WARNING_DISMISSED_KEY in changes) {
    this.downloadsWarningDismissedTime_ =
        changes[DOWNLOADS_WARNING_DISMISSED_KEY].newValue;
  }
};

/**
 * Invoked when the drive connection status is change in the volume manager.
 * @private
 */
Banners.prototype.onDriveConnectionChanged_ = function() {
  this.maybeShowAuthFailBanner_();
};

/**
 * @param {string} type 'none'|'page'|'header'.
 * @param {string} messageId Resource ID of the message.
 * @private
 */
Banners.prototype.prepareAndShowWelcomeBanner_ = function(type, messageId) {
  if (!this.showWelcome_)
    return;

  this.showWelcomeBanner_(type);

  var container = queryRequiredElement(
      '.drive-welcome.' + type, this.document_);
  if (container.firstElementChild)
    return;  // Do not re-create.

  if (!this.document_.querySelector('link[drive-welcome-style]')) {
    var style = this.document_.createElement('link');
    style.rel = 'stylesheet';
    style.href = 'foreground/css/drive_welcome.css';
    style.setAttribute('drive-welcome-style', '');
    this.document_.head.appendChild(style);
  }

  var wrapper = util.createChild(container, 'drive-welcome-wrapper');
  util.createChild(wrapper, 'drive-welcome-icon');

  if (type === 'header') {
    util.createChild(wrapper, 'banner-cloud-bg');
    util.createChild(wrapper, 'banner-people');
  }

  var close = util.createChild(wrapper, 'banner-close');
  close.addEventListener('click', this.closeWelcomeBanner_.bind(this));

  var message = util.createChild(wrapper, 'drive-welcome-message');

  var title = util.createChild(message, 'drive-welcome-title');

  var text = util.createChild(message, 'drive-welcome-text');
  text.innerHTML = str(messageId);

  var links = util.createChild(message, 'drive-welcome-links');

  var more;
  if (this.usePromoWelcomeBanner_) {
    var welcomeTitle = str('DRIVE_WELCOME_TITLE_ALTERNATIVE');
    if (util.boardIs('link'))
      welcomeTitle = str('DRIVE_WELCOME_TITLE_ALTERNATIVE_1TB');
    title.textContent = welcomeTitle;
    more = util.createChild(links, '', 'a');
    more.href = str('GOOGLE_DRIVE_REDEEM_URL');
    var moreInnerButton = util.createChild(
        more, 'imitate-paper-button primary', 'button');
    moreInnerButton.tabIndex = -1;
    moreInnerButton.textContent = str('DRIVE_WELCOME_CHECK_ELIGIBILITY');
  } else {
    title.textContent = str('DRIVE_WELCOME_TITLE');
    more = util.createChild(links, 'plain-link', 'a');
    more.textContent = str('DRIVE_LEARN_MORE');
    more.href = str('GOOGLE_DRIVE_OVERVIEW_URL');
  }
  more.tabIndex = 21;  // See: go/filesapp-tabindex.
  more.id = 'drive-welcome-link';
  more.target = '_blank';

  var dismiss;
  if (this.usePromoWelcomeBanner_) {
    dismiss = util.createChild(
        links, 'imitate-paper-button secondary', 'button');
  } else {
    dismiss = util.createChild(links, 'plain-link');
  }

  dismiss.classList.add('drive-welcome-dismiss');
  dismiss.textContent = str('DRIVE_WELCOME_DISMISS');
  dismiss.addEventListener('click', this.closeWelcomeBanner_.bind(this));

  this.previousDirWasOnDrive_ = false;
};

/**
 * Show or hide the "Low Google Drive space" warning.
 * @param {boolean} show True if the box need to be shown.
 * @param {Object=} opt_sizeStats Size statistics. Should be defined when
 *     showing the warning.
 * @private
 */
Banners.prototype.showLowDriveSpaceWarning_ = function(show, opt_sizeStats) {
  var box = this.document_.querySelector('#volume-space-warning');

  // Avoid showing two banners.
  // TODO(kaznacheev): Unify the low space warning and the promo header.
  if (show)
    this.cleanupWelcomeBanner_();

  if (box.hidden == !show)
    return;

  if (this.warningDismissedCounter_) {
    if (opt_sizeStats &&
        // Quota had not changed
        this.warningDismissedCounter_ == opt_sizeStats.totalSize &&
        opt_sizeStats.remainingSize / opt_sizeStats.totalSize < 0.15) {
      // Since the last dismissal decision the quota has not changed AND
      // the user did not free up significant space. Obey the dismissal.
      show = false;
    } else {
      // Forget the dismissal. Warning will be shown again.
      this.setWarningDismissedCounter_(0);
    }
  }

  box.textContent = '';
  if (show && opt_sizeStats) {
    var icon = this.document_.createElement('div');
    icon.className = 'drive-icon';
    box.appendChild(icon);

    var text = this.document_.createElement('div');
    text.className = 'drive-text';
    text.textContent = strf('DRIVE_SPACE_AVAILABLE_LONG',
        util.bytesToString(opt_sizeStats.remainingSize));
    box.appendChild(text);

    var link = this.document_.createElement('a');
    link.href = str('GOOGLE_DRIVE_BUY_STORAGE_URL');
    link.target = '_blank';
    var button = this.document_.createElement('button');
    button.className = 'imitate-paper-button';
    button.textContent = str('DRIVE_BUY_MORE_SPACE_LINK');
    link.appendChild(button);
    box.appendChild(link);

    var close = this.document_.createElement('div');
    close.className = 'banner-close';
    box.appendChild(close);
    close.addEventListener('click', function(total) {
      var values = {};
      values[DRIVE_WARNING_DISMISSED_KEY] = total;
      chrome.storage.local.set(values);
      box.hidden = true;
      this.requestRelayout_(100);
    }.bind(this, opt_sizeStats.totalSize));
  }

  if (box.hidden != !show) {
    box.hidden = !show;
    this.requestRelayout_(100);
  }
};
/**
 * Closes the Drive Welcome banner.
 * @private
 */
Banners.prototype.closeWelcomeBanner_ = function() {
  this.cleanupWelcomeBanner_();
  // Stop showing the welcome banner.
  this.setWelcomeHeaderCounter_(WELCOME_HEADER_COUNTER_LIMIT);
};

/**
 * Shows or hides the welcome banner for drive.
 * @private
 */
Banners.prototype.checkSpaceAndMaybeShowWelcomeBanner_ = function() {
  this.ready_.then(function() {
    if (!this.isOnCurrentProfileDrive()) {
      // We are not on the drive file system. Do not show (close) the welcome
      // banner.
      this.cleanupWelcomeBanner_();
      this.previousDirWasOnDrive_ = false;
      return;
    }

    var driveVolume = this.volumeManager_.getCurrentProfileVolumeInfo(
        VolumeManagerCommon.VolumeType.DRIVE);
    if (this.welcomeHeaderCounter_ >= WELCOME_HEADER_COUNTER_LIMIT ||
        !driveVolume || driveVolume.error) {
      // The banner is already shown enough times or the drive FS is not
      // mounted. So, do nothing here.
      return;
    }

    if (!this.showOffers_ || !this.showWelcome_) {
      // Because it is not necessary to show the offer, set
      // |usePromoWelcomeBanner_| false here. Note that it probably should be
      // able to do this in the constructor, but there remains non-trivial path,
      // which may be causes |usePromoWelcomeBanner_| == true's behavior even
      // if |showOffers_| is false.
      // TODO(hidehiko): Make sure if it is expected or not, and simplify
      // |showOffers_| if possible.
      this.usePromoWelcomeBanner_ = false;
    }

    // Choose the offer basing on the board name. The default one is 100 GB.
    var offerSize = 100;  // In GB.
    var offerServiceId = 'drive.cros.echo.1';

    if (util.boardIs('link')) {
      offerSize = 1024;  // 1 TB.
      offerServiceId = 'drive.cros.echo.2';
    }

    // Perform asynchronous tasks in parallel.
    var group = new AsyncUtil.Group();

    // If the offer has been checked, then do not show the promo anymore.
    group.add(function(onCompleted) {
      chrome.echoPrivate.getOfferInfo(offerServiceId, function(offerInfo) {
        // If the offer has not been checked, then an error is raised.
        if (!chrome.runtime.lastError)
          this.usePromoWelcomeBanner_ = false;
        onCompleted();
      }.bind(this));
    }.bind(this));

    group.add(function(onCompleted) {
      if (this.usePromoWelcomeBanner_) {
        // getSizeStats for Drive file system accesses to the server, so we
        // should minimize the invocation.

        // Current directory must be set, since this code is called after
        // scanning is completed. However, the volumeInfo may be gone.
        chrome.fileManagerPrivate.getSizeStats(
            driveVolume.volumeId,
            function(result) {
              if (result && result.totalSize >= offerSize * 1024 * 1024 * 1024)
                this.usePromoWelcomeBanner_ = false;
              onCompleted();
            }.bind(this));
      }
    }.bind(this));

    group.run(this.maybeShowWelcomeBanner_.bind(this));
  }.bind(this));
};

/**
 * Decides which banner should be shown, and show it. This method is designed
 * to be called only from checkSpaceAndMaybeShowWelcomeBanner_.
 * @private
 */
Banners.prototype.maybeShowWelcomeBanner_ = function() {
  this.ready_.then(function() {
    if (this.directoryModel_.getFileList().length == 0 &&
        this.welcomeHeaderCounter_ == 0) {
      // Only show the full page banner if the header banner was never shown.
      // Do not increment the counter.
      // The timeout below is required because sometimes another
      // 'rescan-completed' event arrives shortly with non-empty file list.
      setTimeout(function() {
        if (this.isOnCurrentProfileDrive() && this.welcomeHeaderCounter_ == 0) {
          this.prepareAndShowWelcomeBanner_('page', 'DRIVE_WELCOME_TEXT_LONG');
        }
      }.bind(this), 2000);
    } else {
      // We do not want to increment the counter when the user navigates
      // between different directories on Drive, but we increment the counter
      // once anyway to prevent the full page banner from showing.
      if (!this.previousDirWasOnDrive_ || this.welcomeHeaderCounter_ == 0) {
        this.setWelcomeHeaderCounter_(this.welcomeHeaderCounter_ + 1);
        this.prepareAndShowWelcomeBanner_('header', 'DRIVE_WELCOME_TEXT_SHORT');
      }
    }
    this.previousDirWasOnDrive_ = true;
  }.bind(this));
};

/**
 * @return {boolean} True if current directory is on Drive root of current
 * profile.
 */
Banners.prototype.isOnCurrentProfileDrive = function() {
  var entry = this.directoryModel_.getCurrentDirEntry();
  if (!entry || util.isFakeEntry(entry))
    return false;
  var locationInfo = this.volumeManager_.getLocationInfo(entry);
  if (!locationInfo)
    return false;
  return locationInfo.rootType === VolumeManagerCommon.RootType.DRIVE &&
         locationInfo.volumeInfo.profile.isCurrentProfile;
};

/**
 * Shows the Drive Welcome banner.
 * @param {string} type 'page'|'head'|'none'.
 * @private
 */
Banners.prototype.showWelcomeBanner_ = function(type) {
  var container = this.document_.querySelector('.dialog-container');
  if (container.getAttribute('drive-welcome') != type) {
    container.setAttribute('drive-welcome', type);
    this.requestRelayout_(200);  // Resize only after the animation is done.
  }
};

/**
 * Update the UI when the current directory changes.
 *
 * @param {Event} event The directory-changed event.
 * @private
 */
Banners.prototype.onDirectoryChanged_ = function(event) {
  var rootVolume = this.volumeManager_.getVolumeInfo(event.newDirEntry);
  var previousRootVolume = event.previousDirEntry ?
      this.volumeManager_.getVolumeInfo(event.previousDirEntry) : null;
  if (!rootVolume)
    return;

  // Show (or hide) the low space warning.
  this.maybeShowLowSpaceWarning_(rootVolume);

  // Add or remove listener to show low space warning, if necessary.
  var isLowSpaceWarningTarget = this.isLowSpaceWarningTarget_(rootVolume);
  if (isLowSpaceWarningTarget !==
      this.isLowSpaceWarningTarget_(previousRootVolume)) {
    if (isLowSpaceWarningTarget) {
      chrome.fileManagerPrivate.onDirectoryChanged.addListener(
          this.privateOnDirectoryChangedBound_);
    } else {
      chrome.fileManagerPrivate.onDirectoryChanged.removeListener(
          this.privateOnDirectoryChangedBound_);
    }
  }

  if (!this.isOnCurrentProfileDrive()) {
    this.cleanupWelcomeBanner_();
    this.authFailedBanner_.hidden = true;
  }

  this.updateDriveUnmountedPanel_();
  if (this.isOnCurrentProfileDrive()) {
    this.unmountedPanel_.classList.remove('retry-enabled');
    this.maybeShowAuthFailBanner_();
  }
};

/**
 * @param {VolumeInfo} volumeInfo Volume info to be checked.
 * @return {boolean} true if the file system specified by |root| is a target
 *     to show low space warning. Otherwise false.
 * @private
 */
Banners.prototype.isLowSpaceWarningTarget_ = function(volumeInfo) {
  if (!volumeInfo)
    return false;
  return volumeInfo.profile.isCurrentProfile &&
         (volumeInfo.volumeType === VolumeManagerCommon.VolumeType.DOWNLOADS ||
          volumeInfo.volumeType === VolumeManagerCommon.VolumeType.DRIVE);
};

/**
 * Callback which is invoked when the file system has been changed.
 * @param {Object} event chrome.fileManagerPrivate.onDirectoryChanged event.
 * @private
 */
Banners.prototype.privateOnDirectoryChanged_ = function(event) {
  if (!this.directoryModel_.getCurrentDirEntry())
    return;

  var currentDirEntry = assert(this.directoryModel_.getCurrentDirEntry());
  var currentVolume = currentDirEntry &&
      this.volumeManager_.getVolumeInfo(currentDirEntry);
  var eventVolume = this.volumeManager_.getVolumeInfo(event.entry);
  if (currentVolume === eventVolume) {
    // The file system we are currently on is changed.
    // So, check the free space.
    this.maybeShowLowSpaceWarning_(currentVolume);
  }
};

/**
 * Shows or hides the low space warning.
 * @param {VolumeInfo} volume Type of volume, which we are interested in.
 * @private
 */
Banners.prototype.maybeShowLowSpaceWarning_ = function(volume) {
  // TODO(kaznacheev): Unify the two low space warning.
  switch (volume.volumeType) {
    case VolumeManagerCommon.VolumeType.DOWNLOADS:
      this.showLowDriveSpaceWarning_(false);
      break;
    case VolumeManagerCommon.VolumeType.DRIVE:
      this.showLowDownloadsSpaceWarning_(false);
      break;
    default:
      // If the current file system is neither the DOWNLOAD nor the DRIVE,
      // just hide the warning.
      this.showLowDownloadsSpaceWarning_(false);
      this.showLowDriveSpaceWarning_(false);
      return;
  }

  // If not mounted correctly, then do not continue.
  if (!volume.fileSystem)
    return;

  chrome.fileManagerPrivate.getSizeStats(
      volume.volumeId, function(sizeStats) {
        var currentVolume = this.volumeManager_.getVolumeInfo(
            assert(this.directoryModel_.getCurrentDirEntry()));
        if (volume !== currentVolume) {
          // This happens when the current directory is moved during requesting
          // the file system size. Just ignore it.
          return;
        }
        // sizeStats is undefined, if some error occurs.
        if (!sizeStats || sizeStats.totalSize == 0)
          return;

        if (volume.volumeType === VolumeManagerCommon.VolumeType.DOWNLOADS) {
          // Show the warning banner when the available space is less than 1GB.
          this.showLowDownloadsSpaceWarning_(
              sizeStats.remainingSize < DOWNLOADS_SPACE_WARNING_THRESHOLD_SIZE);
        } else {
          // Show the warning banner when the available space ls less than 10%.
          var remainingRatio = sizeStats.remainingSize / sizeStats.totalSize;
          this.showLowDriveSpaceWarning_(
              remainingRatio < DRIVE_SPACE_WARNING_THRESHOLD_RATIO, sizeStats);
        }
      }.bind(this));
};

/**
 * removes the Drive Welcome banner.
 * @private
 */
Banners.prototype.cleanupWelcomeBanner_ = function() {
  this.showWelcomeBanner_('none');
};

/**
 * Notifies the file manager what layout must be recalculated.
 * @param {number} delay In milliseconds.
 * @private
 */
Banners.prototype.requestRelayout_ = function(delay) {
  var self = this;
  setTimeout(function() {
    cr.dispatchSimpleEvent(self, 'relayout');
  }, delay);
};

/**
 * Show or hide the "Low disk space" warning.
 * @param {boolean} show True if the box need to be shown.
 * @private
 */
Banners.prototype.showLowDownloadsSpaceWarning_ = function(show) {
  var box = this.document_.querySelector('.downloads-warning');

  if (box.hidden == !show)
    return;

  if (this.downloadsWarningDismissedTime_) {
    if (Date.now() - this.downloadsWarningDismissedTime_ <
        DOWNLOADS_SPACE_WARNING_DISMISS_DURATION) {
      show = false;
    }
  }

  box.textContent = '';
  if (show) {
    var icon = this.document_.createElement('div');
    icon.className = 'warning-icon';
    var message = this.document_.createElement('div');
    message.className = 'warning-message';
    message.innerHTML = util.htmlUnescape(str('DOWNLOADS_DIRECTORY_WARNING'));
    box.appendChild(icon);
    box.appendChild(message);
    box.querySelector('a').addEventListener('click', function(e) {
      util.visitURL(str('DOWNLOADS_LOW_SPACE_WARNING_HELP_URL'));
      e.preventDefault();
    });

    var close = this.document_.createElement('div');
    close.className = 'banner-close';
    box.appendChild(close);
    close.addEventListener('click', function() {
      var values = {};
      values[DOWNLOADS_WARNING_DISMISSED_KEY] = Date.now();
      chrome.storage.local.set(values);
      box.hidden = true;
      this.requestRelayout_(100);
    }.bind(this));
  }

  box.hidden = !show;
  this.requestRelayout_(100);
};

/**
 * Creates contents for the DRIVE unmounted panel.
 * @private
 */
Banners.prototype.ensureDriveUnmountedPanelInitialized_ = function() {
  var panel = this.unmountedPanel_;
  if (panel.firstElementChild)
    return;

  /**
   * Creates an element using given parameters.
   * @param {!Element} parent Parent element of the new element.
   * @param {string} tag Tag of the new element.
   * @param {string} className Class name of the new element.
   * @param {string=} opt_textContent Text content of the new element.
   * @return {!Element} The newly created element.
   */
  var create = function(parent, tag, className, opt_textContent) {
    var div = panel.ownerDocument.createElement(tag);
    div.className = className;
    div.textContent = opt_textContent || '';
    parent.appendChild(div);
    return div;
  };

  create(panel, 'div', 'error', str('DRIVE_CANNOT_REACH'));

  var learnMore = create(panel, 'a', 'learn-more plain-link',
                         str('DRIVE_LEARN_MORE'));
  learnMore.href = str('GOOGLE_DRIVE_ERROR_HELP_URL');
  learnMore.target = '_blank';
};

/**
 * Called when volume info list is updated.
 * @param {Event} event Splice event data on volume info list.
 * @private
 */
Banners.prototype.onVolumeInfoListSplice_ = function(event) {
  var isDriveVolume = function(volumeInfo) {
    return volumeInfo.volumeType === VolumeManagerCommon.VolumeType.DRIVE;
  };
  if (event.removed.some(isDriveVolume) || event.added.some(isDriveVolume))
    this.updateDriveUnmountedPanel_();
};

/**
 * Shows the panel when current directory is DRIVE and it's unmounted.
 * Hides it otherwise. The panel shows an error message if it failed.
 * @private
 */
Banners.prototype.updateDriveUnmountedPanel_ = function() {
  var node = this.document_.body;
  if (this.isOnCurrentProfileDrive()) {
    var driveVolume = this.volumeManager_.getCurrentProfileVolumeInfo(
        VolumeManagerCommon.VolumeType.DRIVE);
    if (driveVolume) {
      if (driveVolume.error) {
        this.ensureDriveUnmountedPanelInitialized_();
        this.unmountedPanel_.classList.add('retry-enabled');
        node.setAttribute('drive', 'error');
      } else {
        node.setAttribute('drive', 'mounted');
      }
    } else {
      this.unmountedPanel_.classList.remove('retry-enabled');
      node.setAttribute('drive', 'unmounted');
    }
  } else {
    node.removeAttribute('drive');
  }
};

/**
 * Updates the visibility of Drive Connection Warning banner, retrieving the
 * current connection information.
 * @private
 */
Banners.prototype.maybeShowAuthFailBanner_ = function() {
  var connection = this.volumeManager_.getDriveConnectionState();
  var showDriveNotReachedMessage =
      this.isOnCurrentProfileDrive() &&
      connection.type == VolumeManagerCommon.DriveConnectionType.OFFLINE &&
      connection.reason == VolumeManagerCommon.DriveConnectionReason.NOT_READY;
  this.authFailedBanner_.hidden = !showDriveNotReachedMessage;
};
