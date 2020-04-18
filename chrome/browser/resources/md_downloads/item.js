// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('downloads', function() {
  const Item = Polymer({
    is: 'downloads-item',

    properties: {
      data: {
        type: Object,
      },

      completelyOnDisk_: {
        computed: 'computeCompletelyOnDisk_(' +
            'data.state, data.file_externally_removed)',
        type: Boolean,
        value: true,
      },

      controlledBy_: {
        computed: 'computeControlledBy_(data.by_ext_id, data.by_ext_name)',
        type: String,
        value: '',
      },

      isActive_: {
        computed: 'computeIsActive_(' +
            'data.state, data.file_externally_removed)',
        type: Boolean,
        value: true,
      },

      isDangerous_: {
        computed: 'computeIsDangerous_(data.state)',
        type: Boolean,
        value: false,
      },

      isMalware_: {
        computed: 'computeIsMalware_(isDangerous_, data.danger_type)',
        type: Boolean,
        value: false,
      },

      isInProgress_: {
        computed: 'computeIsInProgress_(data.state)',
        type: Boolean,
        value: false,
      },

      pauseOrResumeText_: {
        computed: 'computePauseOrResumeText_(isInProgress_, data.resume)',
        type: String,
      },

      showCancel_: {
        computed: 'computeShowCancel_(data.state)',
        type: Boolean,
        value: false,
      },

      showProgress_: {
        computed: 'computeShowProgress_(showCancel_, data.percent)',
        type: Boolean,
        value: false,
      },
    },

    observers: [
      // TODO(dbeam): this gets called way more when I observe data.by_ext_id
      // and data.by_ext_name directly. Why?
      'observeControlledBy_(controlledBy_)',
      'observeIsDangerous_(isDangerous_, data)',
    ],

    /** @private {?downloads.BrowserProxy} */
    browserProxy_: null,

    /** @override */
    ready: function() {
      this.browserProxy_ = downloads.BrowserProxy.getInstance();
      this.content = this.$.content;
    },

    /**
     * @param {string} url
     * @return {string} A reasonably long URL.
     * @private
     */
    chopUrl_: function(url) {
      return url.slice(0, 300);
    },

    /** @private */
    computeClass_: function() {
      const classes = [];

      if (this.isActive_)
        classes.push('is-active');

      if (this.isDangerous_)
        classes.push('dangerous');

      if (this.showProgress_)
        classes.push('show-progress');

      return classes.join(' ');
    },

    /** @private */
    computeCompletelyOnDisk_: function() {
      return this.data.state == downloads.States.COMPLETE &&
          !this.data.file_externally_removed;
    },

    /** @private */
    computeControlledBy_: function() {
      if (!this.data.by_ext_id || !this.data.by_ext_name)
        return '';

      const url = `chrome://extensions#${this.data.by_ext_id}`;
      const name = this.data.by_ext_name;
      return loadTimeData.getStringF('controlledByUrl', url, HTMLEscape(name));
    },

    /** @private */
    computeDangerIcon_: function() {
      return this.isDangerous_ ? 'cr:warning' : '';
    },

    /** @private */
    computeDate_: function() {
      assert(typeof this.data.hideDate == 'boolean');
      if (this.data.hideDate)
        return '';
      return assert(this.data.since_string || this.data.date_string);
    },

    /** @private */
    computeDescription_: function() {
      const data = this.data;

      switch (data.state) {
        case downloads.States.DANGEROUS:
          const fileName = data.file_name;
          switch (data.danger_type) {
            case downloads.DangerType.DANGEROUS_FILE:
              return loadTimeData.getString('dangerFileDesc');

            case downloads.DangerType.DANGEROUS_URL:
            case downloads.DangerType.DANGEROUS_CONTENT:
            case downloads.DangerType.DANGEROUS_HOST:
              return loadTimeData.getString('dangerDownloadDesc');

            case downloads.DangerType.UNCOMMON_CONTENT:
              return loadTimeData.getString('dangerUncommonDesc');

            case downloads.DangerType.POTENTIALLY_UNWANTED:
              return loadTimeData.getString('dangerSettingsDesc');
          }
          break;

        case downloads.States.IN_PROGRESS:
        case downloads.States.PAUSED:  // Fallthrough.
          return data.progress_status_text;
      }

      return '';
    },

    /** @private */
    computeIsActive_: function() {
      return this.data.state != downloads.States.CANCELLED &&
          this.data.state != downloads.States.INTERRUPTED &&
          !this.data.file_externally_removed;
    },

    /** @private */
    computeIsDangerous_: function() {
      return this.data.state == downloads.States.DANGEROUS;
    },

    /** @private */
    computeIsInProgress_: function() {
      return this.data.state == downloads.States.IN_PROGRESS;
    },

    /** @private */
    computeIsMalware_: function() {
      return this.isDangerous_ &&
          (this.data.danger_type == downloads.DangerType.DANGEROUS_CONTENT ||
           this.data.danger_type == downloads.DangerType.DANGEROUS_HOST ||
           this.data.danger_type == downloads.DangerType.DANGEROUS_URL ||
           this.data.danger_type == downloads.DangerType.POTENTIALLY_UNWANTED);
    },

    /** @private */
    computePauseOrResumeText_: function() {
      if (this.isInProgress_)
        return loadTimeData.getString('controlPause');
      if (this.data.resume)
        return loadTimeData.getString('controlResume');
      return '';
    },

    /** @private */
    computeRemoveStyle_: function() {
      const canDelete = loadTimeData.getBoolean('allowDeletingHistory');
      const hideRemove = this.isDangerous_ || this.showCancel_ || !canDelete;
      return hideRemove ? 'visibility: hidden' : '';
    },

    /** @private */
    computeShowCancel_: function() {
      return this.data.state == downloads.States.IN_PROGRESS ||
          this.data.state == downloads.States.PAUSED;
    },

    /** @private */
    computeShowProgress_: function() {
      return this.showCancel_ && this.data.percent >= -1;
    },

    /** @private */
    computeTag_: function() {
      switch (this.data.state) {
        case downloads.States.CANCELLED:
          return loadTimeData.getString('statusCancelled');

        case downloads.States.INTERRUPTED:
          return this.data.last_reason_text;

        case downloads.States.COMPLETE:
          return this.data.file_externally_removed ?
              loadTimeData.getString('statusRemoved') :
              '';
      }

      return '';
    },

    /** @private */
    isIndeterminate_: function() {
      return this.data.percent == -1;
    },

    /** @private */
    observeControlledBy_: function() {
      this.$['controlled-by'].innerHTML = this.controlledBy_;
    },

    /** @private */
    observeIsDangerous_: function() {
      if (!this.data)
        return;

      if (this.isDangerous_) {
        this.$.url.removeAttribute('href');
      } else {
        this.$.url.href = assert(this.data.url);
        const filePath = encodeURIComponent(this.data.file_path);
        const scaleFactor = `?scale=${window.devicePixelRatio}x`;
        this.$['file-icon'].src = `chrome://fileicon/${filePath}${scaleFactor}`;
      }
    },

    /** @private */
    onCancelTap_: function() {
      this.browserProxy_.cancel(this.data.id);
    },

    /** @private */
    onDiscardDangerousTap_: function() {
      this.browserProxy_.discardDangerous(this.data.id);
    },

    /**
     * @private
     * @param {Event} e
     */
    onDragStart_: function(e) {
      e.preventDefault();
      this.browserProxy_.drag(this.data.id);
    },

    /**
     * @param {Event} e
     * @private
     */
    onFileLinkTap_: function(e) {
      e.preventDefault();
      this.browserProxy_.openFile(this.data.id);
    },

    /** @private */
    onPauseOrResumeTap_: function() {
      if (this.isInProgress_)
        this.browserProxy_.pause(this.data.id);
      else
        this.browserProxy_.resume(this.data.id);
    },

    /** @private */
    onRemoveTap_: function() {
      this.browserProxy_.remove(this.data.id);
    },

    /** @private */
    onRetryTap_: function() {
      this.browserProxy_.download(this.data.url);
    },

    /** @private */
    onSaveDangerousTap_: function() {
      this.browserProxy_.saveDangerous(this.data.id);
    },

    /** @private */
    onShowTap_: function() {
      this.browserProxy_.show(this.data.id);
    },
  });

  return {Item: Item};
});
