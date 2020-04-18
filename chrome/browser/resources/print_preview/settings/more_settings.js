// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Toggles visibility of the specified printing options sections.
   * @param {!print_preview.DestinationStore} destinationStore To listen for
   *     destination changes.
   * @param {!Array<!print_preview.SettingsSection>} settingsSections Sections
   *     to toggle by this component.
   * @constructor
   * @extends {print_preview.Component}
   */
  function MoreSettings(destinationStore, settingsSections) {
    print_preview.Component.call(this);

    /** @private {!print_preview.DestinationStore} */
    this.destinationStore_ = destinationStore;

    /** @private {!Array<!print_preview.SettingsSection>} */
    this.settingsSections_ = settingsSections;

    /** @private {boolean} */
    this.showAll_ = false;

    /** @private {boolean} */
    this.capabilitiesReady_ = false;

    /** @private {boolean} */
    this.firstDestinationReady_ = false;

    /**
     * Used to record usage statistics.
     * @private {!print_preview.PrintSettingsUiMetricsContext}
     */
    this.metrics_ = new print_preview.PrintSettingsUiMetricsContext();
  }

  MoreSettings.prototype = {
    __proto__: print_preview.Component.prototype,

    /** @return {boolean} Whether the settings are expanded. */
    get isExpanded() {
      return this.showAll_;
    },

    /** @override */
    enterDocument: function() {
      print_preview.Component.prototype.enterDocument.call(this);

      this.tracker.add(this.getElement(), 'click', this.onClick_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SELECT,
          this.onDestinationChanged_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .SELECTED_DESTINATION_CAPABILITIES_READY,
          this.onDestinationCapabilitiesReady_.bind(this));
      this.settingsSections_.forEach(section => {
        this.tracker.add(
            section,
            print_preview.SettingsSection.EventType.COLLAPSIBLE_CONTENT_CHANGED,
            this.updateState_.bind(this));
      });

      this.updateState_(true);
    },

    /**
     * Toggles "more/fewer options" state and notifies all the options sections
     *     to reflect the new state.
     * @private
     */
    onClick_: function() {
      this.showAll_ = !this.showAll_;
      this.updateState_(false);
      this.metrics_.record(
          this.isExpanded ? print_preview.Metrics.PrintSettingsUiBucket
                                .MORE_SETTINGS_CLICKED :
                            print_preview.Metrics.PrintSettingsUiBucket
                                .LESS_SETTINGS_CLICKED);
    },

    /**
     * Called when the destination selection has changed. Updates UI elements.
     * @private
     */
    onDestinationChanged_: function() {
      this.firstDestinationReady_ = true;
      this.capabilitiesReady_ = false;
      this.updateState_(false);
    },

    /**
     * Called when the destination selection has changed. Updates UI elements.
     * @private
     */
    onDestinationCapabilitiesReady_: function() {
      this.capabilitiesReady_ = true;
      this.updateState_(false);
    },

    /**
     * Updates the component appearance according to the current state.
     * @param {boolean} noAnimation Whether section visibility transitions
     *     should not be animated.
     * @private
     */
    updateState_: function(noAnimation) {
      if (!this.firstDestinationReady_) {
        fadeOutElement(this.getElement());
        return;
      }
      // When capabilities are not known yet, don't change the state to avoid
      // unnecessary fade in/out cycles.
      if (!this.capabilitiesReady_)
        return;

      this.getChildElement('.more-settings-label').textContent =
          loadTimeData.getString(
              this.isExpanded ? 'lessOptionsLabel' : 'moreOptionsLabel');
      const iconEl = this.getChildElement('.more-settings-icon');
      iconEl.classList.toggle('more-settings-icon-plus', !this.isExpanded);
      iconEl.classList.toggle('more-settings-icon-minus', this.isExpanded);

      const availableSections =
          this.settingsSections_.reduce(function(count, section) {
            return count + (section.isAvailable() ? 1 : 0);
          }, 0);

      // Magic 6 is chosen as the number of sections when it still feels like
      // manageable and not too crowded.
      const hasSectionsToToggle = availableSections > 6 &&
          this.settingsSections_.some(function(section) {
            return section.hasCollapsibleContent();
          });

      if (hasSectionsToToggle)
        fadeInElement(this.getElement(), noAnimation);
      else
        fadeOutElement(this.getElement());

      const collapseContent = !this.isExpanded && hasSectionsToToggle;
      this.settingsSections_.forEach(function(section) {
        section.setCollapseContent(collapseContent, noAnimation);
      });
    }
  };

  // Export
  return {MoreSettings: MoreSettings};
});
