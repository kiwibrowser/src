// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(rltoscano): Move data/* into print_preview.data namespace

// <include src="component.js">
// <include src="print_preview_focus_manager.js">
//

cr.exportPath('print_preview');

/**
 * States of the print preview.
 * @enum {string}
 * @private
 */
print_preview.PrintPreviewUiState_ = {
  INITIALIZING: 'initializing',
  READY: 'ready',
  OPENING_PDF_PREVIEW: 'opening-pdf-preview',
  OPENING_NATIVE_PRINT_DIALOG: 'opening-native-print-dialog',
  PRINTING: 'printing',
  FILE_SELECTION: 'file-selection',
  CLOSING: 'closing',
  ERROR: 'error'
};

/**
 * What can happen when print preview tries to print.
 * @enum {string}
 * @private
 */
print_preview.PrintAttemptResult_ = {
  NOT_READY: 'not-ready',
  PRINTED: 'printed',
  READY_WAITING_FOR_PREVIEW: 'ready-waiting-for-preview'
};

cr.define('print_preview', function() {
  'use strict';

  const PrintPreviewUiState_ = print_preview.PrintPreviewUiState_;

  /**
   * Container class for Chromium's print preview.
   * @constructor
   * @extends {print_preview.Component}
   */
  function PrintPreview() {
    print_preview.Component.call(this);

    /**
     * Used to communicate with Chromium's print system.
     * @type {!print_preview.NativeLayer}
     * @private
     */
    this.nativeLayer_ = print_preview.NativeLayer.getInstance();

    /**
     * Event target that contains information about the logged in user.
     * @type {!print_preview.UserInfo}
     * @private
     */
    this.userInfo_ = new print_preview.UserInfo();

    /**
     * Data store which holds print destinations.
     * @type {!print_preview.DestinationStore}
     * @private
     */
    this.destinationStore_ = new print_preview.DestinationStore(
        this.userInfo_, this.listenerTracker);

    /**
     * Application state.
     * @type {!print_preview.AppState}
     * @private
     */
    this.appState_ = new print_preview.AppState(this.destinationStore_);

    /**
     * Data model that holds information about the document to print.
     * @type {!print_preview.DocumentInfo}
     * @private
     */
    this.documentInfo_ = new print_preview.DocumentInfo();

    /**
     * Data store which holds printer sharing invitations.
     * @type {!print_preview.InvitationStore}
     * @private
     */
    this.invitationStore_ = new print_preview.InvitationStore(this.userInfo_);

    /**
     * Storage of the print ticket used to create the print job.
     * @type {!print_preview.PrintTicketStore}
     * @private
     */
    this.printTicketStore_ = new print_preview.PrintTicketStore(
        this.destinationStore_, this.appState_, this.documentInfo_);

    /**
     * Holds the print and cancel buttons and renders some document statistics.
     * @type {!print_preview.PrintHeader}
     * @private
     */
    this.printHeader_ = new print_preview.PrintHeader(
        this.printTicketStore_, this.destinationStore_);
    this.addChild(this.printHeader_);

    /**
     * Component used to search for print destinations.
     * @type {!print_preview.DestinationSearch}
     * @private
     */
    this.destinationSearch_ = new print_preview.DestinationSearch(
        this.destinationStore_, this.invitationStore_, this.userInfo_,
        this.appState_);
    this.addChild(this.destinationSearch_);

    /**
     * Component that renders the print destination.
     * @type {!print_preview.DestinationSettings}
     * @private
     */
    this.destinationSettings_ =
        new print_preview.DestinationSettings(this.destinationStore_);
    this.addChild(this.destinationSettings_);

    /**
     * Component that renders UI for entering in page range.
     * @type {!print_preview.PageSettings}
     * @private
     */
    this.pageSettings_ =
        new print_preview.PageSettings(this.printTicketStore_.pageRange);
    this.addChild(this.pageSettings_);

    /**
     * Component that renders the copies settings.
     * @type {!print_preview.CopiesSettings}
     * @private
     */
    this.copiesSettings_ = new print_preview.CopiesSettings(
        this.printTicketStore_.copies, this.printTicketStore_.collate);
    this.addChild(this.copiesSettings_);

    /**
     * Component that renders the layout settings.
     * @type {!print_preview.LayoutSettings}
     * @private
     */
    this.layoutSettings_ =
        new print_preview.LayoutSettings(this.printTicketStore_.landscape);
    this.addChild(this.layoutSettings_);

    /**
     * Component that renders the color options.
     * @type {!print_preview.ColorSettings}
     * @private
     */
    this.colorSettings_ =
        new print_preview.ColorSettings(this.printTicketStore_.color);
    this.addChild(this.colorSettings_);

    /**
     * Component that renders the media size settings.
     * @type {!print_preview.MediaSizeSettings}
     * @private
     */
    this.mediaSizeSettings_ =
        new print_preview.MediaSizeSettings(this.printTicketStore_.mediaSize);
    this.addChild(this.mediaSizeSettings_);

    /**
     * Component that renders a select box for choosing margin settings.
     * @type {!print_preview.MarginSettings}
     * @private
     */
    this.marginSettings_ =
        new print_preview.MarginSettings(this.printTicketStore_.marginsType);
    this.addChild(this.marginSettings_);

    /**
     * Component that renders the DPI settings.
     * @type {!print_preview.DpiSettings}
     * @private
     */
    this.dpiSettings_ =
        new print_preview.DpiSettings(this.printTicketStore_.dpi);
    this.addChild(this.dpiSettings_);

    /**
     * Component that renders the scaling settings.
     * @type {!print_preview.ScalingSettings}
     * @private
     */
    this.scalingSettings_ = new print_preview.ScalingSettings(
        this.printTicketStore_.scaling, this.printTicketStore_.fitToPage);
    this.addChild(this.scalingSettings_);

    /**
     * Component that renders miscellaneous print options.
     * @type {!print_preview.OtherOptionsSettings}
     * @private
     */
    this.otherOptionsSettings_ = new print_preview.OtherOptionsSettings(
        this.printTicketStore_.duplex, this.printTicketStore_.cssBackground,
        this.printTicketStore_.selectionOnly,
        this.printTicketStore_.headerFooter, this.printTicketStore_.rasterize);
    this.addChild(this.otherOptionsSettings_);

    /**
     * Component that renders the advanced options button.
     * @type {!print_preview.AdvancedOptionsSettings}
     * @private
     */
    this.advancedOptionsSettings_ = new print_preview.AdvancedOptionsSettings(
        this.printTicketStore_.vendorItems, this.destinationStore_);
    this.addChild(this.advancedOptionsSettings_);

    /**
     * Component used to search for print destinations.
     * @type {!print_preview.AdvancedSettings}
     * @private
     */
    this.advancedSettings_ =
        new print_preview.AdvancedSettings(this.printTicketStore_);
    this.addChild(this.advancedSettings_);

    const settingsSections = [
      this.destinationSettings_, this.pageSettings_, this.copiesSettings_,
      this.mediaSizeSettings_, this.layoutSettings_, this.marginSettings_,
      this.colorSettings_, this.dpiSettings_, this.scalingSettings_,
      this.otherOptionsSettings_, this.advancedOptionsSettings_
    ];

    /**
     * Component representing more/less settings button.
     * @type {!print_preview.MoreSettings}
     * @private
     */
    this.moreSettings_ = new print_preview.MoreSettings(
        this.destinationStore_, settingsSections);
    this.addChild(this.moreSettings_);

    /**
     * Area of the UI that holds the print preview.
     * @type {!print_preview.PreviewArea}
     * @private
     */
    this.previewArea_ = new print_preview.PreviewArea(
        this.destinationStore_, this.printTicketStore_, this.documentInfo_);
    this.addChild(this.previewArea_);

    /**
     * Interface to the Google Cloud Print API. Null if Google Cloud Print
     * integration is disabled.
     * @type {cloudprint.CloudPrintInterface}
     * @private
     */
    this.cloudPrintInterface_ = null;

    /**
     * Whether in kiosk mode where print preview can print automatically without
     * user intervention. See http://crbug.com/31395. Print will start when
     * both the print ticket has been initialized, and an initial printer has
     * been selected.
     * @type {boolean}
     * @private
     */
    this.isInKioskAutoPrintMode_ = false;

    /**
     * Whether Print Preview is in App Kiosk mode, basically, use only printers
     * available for the device.
     * @type {boolean}
     * @private
     */
    this.isInAppKioskMode_ = false;

    /**
     * Whether Print with System Dialog link should be hidden. Overrides the
     * default rules for System dialog link visibility.
     * @type {boolean}
     * @private
     */
    this.hideSystemDialogLink_ = true;

    /**
     * State of the print preview UI.
     * @type {print_preview.PrintPreviewUiState_}
     * @private
     */
    this.uiState_ = PrintPreviewUiState_.INITIALIZING;

    /**
     * Whether document preview generation is in progress.
     * @type {boolean}
     * @private
     */
    this.isPreviewGenerationInProgress_ = true;

    /**
     * Whether to show system dialog before next printing.
     * @type {boolean}
     * @private
     */
    this.showSystemDialogBeforeNextPrint_ = false;

    /**
     * Whether the preview is listening for the manipulate-settings-for-test
     * UI event.
     * @private {boolean}
     */
    this.isListeningForManipulateSettings_ = false;
  }

  PrintPreview.prototype = {
    __proto__: print_preview.Component.prototype,
    /**
     * @return {!print_preview.PreviewArea} The preview area. Used for tests.
     */
    getPreviewArea: function() {
      return this.previewArea_;
    },

    /** Sets up the page and print preview by getting the printer list. */
    initialize: function() {
      this.decorate($('print-preview'));
      if (!this.previewArea_.hasCompatiblePlugin) {
        this.setIsEnabled_(false);
      }
      this.nativeLayer_.getInitialSettings().then(
          this.onInitialSettingsSet_.bind(this));
      print_preview.PrintPreviewFocusManager.getInstance().initialize();
      cr.ui.FocusOutlineManager.forDocument(document);
      this.listenerTracker.add('print-failed', this.onPrintFailed_.bind(this));
      this.listenerTracker.add(
          'use-cloud-print', this.onCloudPrintEnable_.bind(this));
      this.listenerTracker.add(
          'print-preset-options',
          this.onPrintPresetOptionsFromDocument_.bind(this));
      this.listenerTracker.add(
          'page-count-ready', this.onPageCountReady_.bind(this));
      this.listenerTracker.add(
          'enable-manipulate-settings-for-test',
          this.onEnableManipulateSettingsForTest_.bind(this));
    },

    /** @override */
    enterDocument: function() {
      if ($('system-dialog-link')) {
        this.tracker.add(
            getRequiredElement('system-dialog-link'), 'click',
            this.openSystemPrintDialog_.bind(this));
      }
      if ($('open-pdf-in-preview-link')) {
        this.tracker.add(
            getRequiredElement('open-pdf-in-preview-link'), 'click',
            this.onOpenPdfInPreviewLinkClick_.bind(this));
      }

      this.tracker.add(
          this.previewArea_,
          print_preview.PreviewArea.EventType.PREVIEW_GENERATION_IN_PROGRESS,
          this.onPreviewGenerationInProgress_.bind(this));
      this.tracker.add(
          this.previewArea_,
          print_preview.PreviewArea.EventType.PREVIEW_GENERATION_DONE,
          this.onPreviewGenerationDone_.bind(this));
      this.tracker.add(
          this.previewArea_,
          print_preview.PreviewArea.EventType.PREVIEW_GENERATION_FAIL,
          this.onPreviewGenerationFail_.bind(this));
      this.tracker.add(
          this.previewArea_,
          print_preview.PreviewArea.EventType.OPEN_SYSTEM_DIALOG_CLICK,
          this.openSystemPrintDialog_.bind(this));
      this.tracker.add(
          this.previewArea_,
          print_preview.PreviewArea.EventType.SETTINGS_INVALID,
          this.onSettingsInvalid_.bind(this));

      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .SELECTED_DESTINATION_CAPABILITIES_READY,
          this.printIfReady_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.SELECTED_DESTINATION_INVALID,
          this.onSelectedDestinationInvalid_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .SELECTED_DESTINATION_UNSUPPORTED,
          this.onSelectedDestinationUnsupported_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SELECT,
          this.onDestinationSelect_.bind(this));

      this.tracker.add(
          this.printHeader_,
          print_preview.PrintHeader.EventType.PRINT_BUTTON_CLICK,
          this.onPrintButtonClick_.bind(this));
      this.tracker.add(
          this.printHeader_,
          print_preview.PrintHeader.EventType.CANCEL_BUTTON_CLICK,
          this.onCancelButtonClick_.bind(this));

      this.tracker.add(window, 'keydown', this.onKeyDown_.bind(this));
      this.previewArea_.setPluginKeyEventCallback(this.onKeyDown_.bind(this));

      this.tracker.add(
          this.destinationSettings_,
          print_preview.DestinationSettings.EventType.CHANGE_BUTTON_ACTIVATE,
          this.onDestinationChangeButtonActivate_.bind(this));

      this.tracker.add(
          this.destinationSearch_,
          print_preview.DestinationSearch.EventType.MANAGE_PRINT_DESTINATIONS,
          this.onManagePrintDestinationsActivated_.bind(this));
      this.tracker.add(
          this.destinationSearch_,
          print_preview.DestinationSearch.EventType.ADD_ACCOUNT,
          this.onCloudPrintSignInActivated_.bind(this, true /*addAccount*/));
      this.tracker.add(
          this.destinationSearch_,
          print_preview.DestinationSearch.EventType.SIGN_IN,
          this.onCloudPrintSignInActivated_.bind(this, false /*addAccount*/));
      this.tracker.add(
          this.destinationSearch_,
          print_preview.DestinationListItem.EventType.REGISTER_PROMO_CLICKED,
          this.onCloudPrintRegisterPromoClick_.bind(this));

      this.tracker.add(
          this.advancedOptionsSettings_,
          print_preview.AdvancedOptionsSettings.EventType.BUTTON_ACTIVATED,
          this.onAdvancedOptionsButtonActivated_.bind(this));

      /* Ticket items that may be invalid. */
      [this.printTicketStore_.copies, this.printTicketStore_.pageRange,
       this.printTicketStore_.scaling,
      ].forEach((item) => {
        this.tracker.add(
            item, print_preview.ticket_items.TicketItem.EventType.CHANGE,
            this.onTicketChange_.bind(this));
      });
    },

    /** @override */
    decorateInternal: function() {
      this.printHeader_.decorate($('print-header'));
      this.destinationSearch_.decorate($('destination-search'));
      this.destinationSettings_.decorate($('destination-settings'));
      this.pageSettings_.decorate($('page-settings'));
      this.copiesSettings_.decorate($('copies-settings'));
      this.layoutSettings_.decorate($('layout-settings'));
      this.colorSettings_.decorate($('color-settings'));
      this.mediaSizeSettings_.decorate($('media-size-settings'));
      this.marginSettings_.decorate($('margin-settings'));
      this.dpiSettings_.decorate($('dpi-settings'));
      this.scalingSettings_.decorate($('scaling-settings'));
      this.otherOptionsSettings_.decorate($('other-options-settings'));
      this.advancedOptionsSettings_.decorate($('advanced-options-settings'));
      this.advancedSettings_.decorate($('advanced-settings'));
      this.moreSettings_.decorate($('more-settings'));
      this.previewArea_.decorate($('preview-area'));
    },

    /**
     * Sets whether the controls in the print preview are enabled.
     * @param {boolean} isEnabled Whether the controls in the print preview are
     *     enabled.
     * @private
     */
    setIsEnabled_: function(isEnabled) {
      if ($('system-dialog-link'))
        $('system-dialog-link').disabled = !isEnabled;
      if ($('open-pdf-in-preview-link'))
        $('open-pdf-in-preview-link').disabled = !isEnabled;
      this.printHeader_.isEnabled = isEnabled;
      this.destinationSettings_.isEnabled = isEnabled;
      this.pageSettings_.isEnabled = isEnabled;
      this.copiesSettings_.isEnabled = isEnabled;
      this.layoutSettings_.isEnabled = isEnabled;
      this.colorSettings_.isEnabled = isEnabled;
      this.mediaSizeSettings_.isEnabled = isEnabled;
      this.marginSettings_.isEnabled = isEnabled;
      this.dpiSettings_.isEnabled = isEnabled;
      this.scalingSettings_.isEnabled = isEnabled;
      this.otherOptionsSettings_.isEnabled = isEnabled;
      this.advancedOptionsSettings_.isEnabled = isEnabled;
    },

    /**
     * Prints the document or launches a pdf preview on the local system.
     * @param {boolean} isPdfPreview Whether to launch the pdf preview.
     * @private
     */
    printDocumentOrOpenPdfPreview_: function(isPdfPreview) {
      assert(
          this.uiState_ == PrintPreviewUiState_.READY,
          'Print document request received when not in ready state: ' +
              this.uiState_);
      if (isPdfPreview) {
        this.uiState_ = PrintPreviewUiState_.OPENING_PDF_PREVIEW;
      } else if (
          this.destinationStore_.selectedDestination.id ==
          print_preview.Destination.GooglePromotedId.SAVE_AS_PDF) {
        this.uiState_ = PrintPreviewUiState_.FILE_SELECTION;
      } else {
        this.uiState_ = PrintPreviewUiState_.PRINTING;
      }
      this.setIsEnabled_(false);
      this.printHeader_.isCancelButtonEnabled = true;
      const printAttemptResult = this.printIfReady_();
      if (printAttemptResult ==
          print_preview.PrintAttemptResult_.READY_WAITING_FOR_PREVIEW) {
        if ((this.destinationStore_.selectedDestination.isLocal &&
             !this.destinationStore_.selectedDestination.isPrivet &&
             !this.destinationStore_.selectedDestination.isExtension &&
             this.destinationStore_.selectedDestination.id !=
                 print_preview.Destination.GooglePromotedId.SAVE_AS_PDF) ||
            this.uiState_ == PrintPreviewUiState_.OPENING_PDF_PREVIEW) {
          // Hide the dialog for now. The actual print command will be issued
          // when the preview generation is done.
          this.nativeLayer_.hidePreview();
        }
      }
    },

    /**
     * Attempts to print if needed and if ready.
     * @return {print_preview.PrintAttemptResult_} Attempt result.
     * @private
     */
    printIfReady_: function() {
      const okToPrint =
          (this.uiState_ == PrintPreviewUiState_.PRINTING ||
           this.uiState_ == PrintPreviewUiState_.OPENING_PDF_PREVIEW ||
           this.uiState_ == PrintPreviewUiState_.FILE_SELECTION ||
           this.isInKioskAutoPrintMode_) &&
          this.destinationStore_.selectedDestination &&
          this.destinationStore_.selectedDestination.capabilities;
      if (!okToPrint) {
        return print_preview.PrintAttemptResult_.NOT_READY;
      }
      if (this.isPreviewGenerationInProgress_) {
        return print_preview.PrintAttemptResult_.READY_WAITING_FOR_PREVIEW;
      }
      assert(
          this.printTicketStore_.isTicketValid(),
          'Trying to print with invalid ticket');
      if (getIsVisible(this.moreSettings_.getElement())) {
        new print_preview.PrintSettingsUiMetricsContext().record(
            this.moreSettings_.isExpanded ?
                print_preview.Metrics.PrintSettingsUiBucket
                    .PRINT_WITH_SETTINGS_EXPANDED :
                print_preview.Metrics.PrintSettingsUiBucket
                    .PRINT_WITH_SETTINGS_COLLAPSED);
      }
      const destination = assert(this.destinationStore_.selectedDestination);
      const whenPrintDone = this.sendPrintRequest_(destination);
      if (destination.isLocal ||
          this.uiState_ == PrintPreviewUiState_.OPENING_PDF_PREVIEW) {
        const onError = destination.id ==
                print_preview.Destination.GooglePromotedId.SAVE_AS_PDF ?
            this.onFileSelectionCancel_.bind(this) :
            this.onPrintFailed_.bind(this);
        whenPrintDone.then(this.close_.bind(this), onError);
      } else {
        // Cloud print resolves when print data is returned to submit to cloud
        // print, or if print ticket cannot be read, no PDF data is found, or
        // PDF is oversized.
        whenPrintDone.then(
            this.onPrintToCloud_.bind(this), this.onPrintFailed_.bind(this));
      }
      this.showSystemDialogBeforeNextPrint_ = false;
      return print_preview.PrintAttemptResult_.PRINTED;
    },

    /**
     * @param {!print_preview.Destination} destination Destination to print to.
     * @return {!Promise} Promise that resolves when print request is resolved
     *     or rejected.
     * @private
     */
    sendPrintRequest_: function(destination) {
      const printTicketStore = this.printTicketStore_;
      const documentInfo = this.documentInfo_;
      assert(
          printTicketStore.isTicketValid(),
          'Trying to print when ticket is not valid');

      assert(
          !this.showSystemDialogBeforeNextPrint_ ||
              (cr.isWindows && destination.isLocal),
          'Implemented for Windows only');

      // Note: update
      // chrome/browser/ui/webui/print_preview/print_preview_handler_unittest.cc
      // with any changes to ticket creation.
      const ticket = {
        mediaSize: printTicketStore.mediaSize.getValue(),
        pageCount: printTicketStore.pageRange.getPageNumberSet().size,
        landscape: printTicketStore.landscape.getValue(),
        color:
            destination.getNativeColorModel(printTicketStore.color.getValue()),
        headerFooterEnabled: false,  // Only used in print preview
        marginsType: printTicketStore.marginsType.getValue(),
        duplex: printTicketStore.duplex.getValue() ?
            print_preview.PreviewGenerator.DuplexMode.LONG_EDGE :
            print_preview.PreviewGenerator.DuplexMode.SIMPLEX,
        copies: printTicketStore.copies.getValueAsNumber(),
        collate: printTicketStore.collate.getValue(),
        shouldPrintBackgrounds: printTicketStore.cssBackground.getValue(),
        shouldPrintSelectionOnly: false,  // Only used in print preview
        previewModifiable: documentInfo.isModifiable,
        printToPDF: destination.id ==
            print_preview.Destination.GooglePromotedId.SAVE_AS_PDF,
        printWithCloudPrint: !destination.isLocal,
        printWithPrivet: destination.isPrivet,
        printWithExtension: destination.isExtension,
        rasterizePDF: printTicketStore.rasterize.getValue(),
        scaleFactor: printTicketStore.scaling.getValueAsNumber(),
        dpiHorizontal: 'horizontal_dpi' in printTicketStore.dpi.getValue() ?
            printTicketStore.dpi.getValue().horizontal_dpi :
            0,
        dpiVertical: 'vertical_dpi' in printTicketStore.dpi.getValue() ?
            printTicketStore.dpi.getValue().vertical_dpi :
            0,
        deviceName: destination.id,
        fitToPageEnabled: printTicketStore.fitToPage.getValue(),
        pageWidth: documentInfo.pageSize.width,
        pageHeight: documentInfo.pageSize.height,
        showSystemDialog: this.showSystemDialogBeforeNextPrint_
      };

      if (!destination.isLocal) {
        // We can't set cloudPrintID if the destination is "Print with Cloud
        // Print" because the native system will try to print to Google Cloud
        // Print with this ID instead of opening a Google Cloud Print dialog.
        ticket.cloudPrintID = destination.id;
      }

      if (printTicketStore.marginsType.isCapabilityAvailable() &&
          printTicketStore.marginsType.isValueEqual(
              print_preview.ticket_items.MarginsTypeValue.CUSTOM)) {
        const customMargins = printTicketStore.customMargins.getValue();
        const orientationEnum =
            print_preview.ticket_items.CustomMarginsOrientation;
        ticket.marginsCustom = {
          marginTop: customMargins.get(orientationEnum.TOP),
          marginRight: customMargins.get(orientationEnum.RIGHT),
          marginBottom: customMargins.get(orientationEnum.BOTTOM),
          marginLeft: customMargins.get(orientationEnum.LEFT)
        };
      }

      if (destination.isPrivet || destination.isExtension) {
        ticket.ticket = printTicketStore.createPrintTicket(destination);
        ticket.capabilities = JSON.stringify(destination.capabilities);
      }

      if (this.uiState_ == PrintPreviewUiState_.OPENING_PDF_PREVIEW) {
        ticket.OpenPDFInPreview = true;
      }

      return this.nativeLayer_.print(JSON.stringify(ticket));
    },

    /**
     * Closes the print preview.
     * @param {boolean} isCancel Whether this was called due to the user
     *     closing the dialog without printing.
     * @private
     */
    close_: function(isCancel) {
      this.exitDocument();
      this.uiState_ = PrintPreviewUiState_.CLOSING;
      this.nativeLayer_.dialogClose(isCancel);
    },

    /**
     * Opens the native system print dialog after disabling all controls.
     * @private
     */
    openSystemPrintDialog_: function() {
      if (!this.shouldShowSystemDialogLink_())
        return;
      if ($('system-dialog-link').classList.contains('disabled'))
        return;
      if (cr.isWindows) {
        this.showSystemDialogBeforeNextPrint_ = true;
        this.printDocumentOrOpenPdfPreview_(false /*isPdfPreview*/);
        return;
      }
      setIsVisible(getRequiredElement('system-dialog-throbber'), true);
      this.setIsEnabled_(false);
      this.uiState_ = PrintPreviewUiState_.OPENING_NATIVE_PRINT_DIALOG;
      this.nativeLayer_.showSystemDialog();
    },

    /**
     * Called when the native layer has initial settings to set. Sets the
     * initial settings of the print preview and begins fetching print
     * destinations.
     * @param {!print_preview.NativeInitialSettings} settings The initial print
     *     preview settings persisted through the session.
     * @private
     */
    onInitialSettingsSet_: function(settings) {
      assert(
          this.uiState_ == PrintPreviewUiState_.INITIALIZING,
          'Updating initial settings when not in initializing state: ' +
              this.uiState_);
      this.uiState_ = PrintPreviewUiState_.READY;

      this.isInKioskAutoPrintMode_ = settings.isInKioskAutoPrintMode;
      this.isInAppKioskMode_ = settings.isInAppKioskMode;

      // The following components must be initialized in this order.
      this.appState_.init(settings.serializedAppStateStr);
      this.documentInfo_.init(
          settings.previewModifiable, settings.documentTitle,
          settings.documentHasSelection);
      this.printTicketStore_.init(
          settings.thousandsDelimeter, settings.decimalDelimeter,
          settings.unitType, settings.shouldPrintSelectionOnly);
      this.destinationStore_.init(
          settings.isInAppKioskMode, settings.printerName,
          settings.serializedDefaultDestinationSelectionRulesStr,
          this.appState_.recentDestinations || []);
      this.appState_.setInitialized();

      // This is only visible in the task manager.
      $('document-title').innerText = settings.documentTitle;
      this.hideSystemDialogLink_ = settings.isInAppKioskMode;
      if ($('system-dialog-link')) {
        setIsVisible(
            getRequiredElement('system-dialog-link'),
            this.shouldShowSystemDialogLink_());
      }
    },

    /**
     * Called when Google Cloud Print integration is enabled by the
     * PrintPreviewHandler.
     * Fetches the user's cloud printers.
     * @param {string} cloudPrintUrl The URL to use for cloud print servers.
     * @param {boolean} appKioskMode Whether to print automatically for kiosk
     *     mode.
     * @private
     */
    onCloudPrintEnable_: function(cloudPrintUrl, appKioskMode) {
      this.cloudPrintInterface_ = new cloudprint.CloudPrintInterface(
          cloudPrintUrl, this.nativeLayer_, this.userInfo_, appKioskMode);
      this.tracker.add(
          this.cloudPrintInterface_,
          cloudprint.CloudPrintInterfaceEventType.SUBMIT_DONE,
          this.onCloudPrintSubmitDone_.bind(this));
      this.tracker.add(
          this.cloudPrintInterface_,
          cloudprint.CloudPrintInterfaceEventType.SEARCH_FAILED,
          this.onCloudPrintError_.bind(this));
      this.tracker.add(
          this.cloudPrintInterface_,
          cloudprint.CloudPrintInterfaceEventType.SUBMIT_FAILED,
          this.onCloudPrintError_.bind(this));
      this.tracker.add(
          this.cloudPrintInterface_,
          cloudprint.CloudPrintInterfaceEventType.PRINTER_FAILED,
          this.onCloudPrintError_.bind(this));

      this.destinationStore_.setCloudPrintInterface(this.cloudPrintInterface_);
      this.invitationStore_.setCloudPrintInterface(this.cloudPrintInterface_);
      if (this.destinationSearch_.getIsVisible()) {
        this.destinationStore_.startLoadCloudDestinations();
        this.invitationStore_.startLoadingInvitations();
      }
    },

    /**
     * Called from the native layer when ready to print to Google Cloud Print.
     * @param {string} data The body to send in the HTTP request.
     * @private
     */
    onPrintToCloud_: function(data) {
      assert(
          this.uiState_ == PrintPreviewUiState_.PRINTING,
          'Document ready to be sent to the cloud when not in printing ' +
              'state: ' + this.uiState_);
      assert(
          this.cloudPrintInterface_ != null,
          'Google Cloud Print is not enabled');
      const destination = this.destinationStore_.selectedDestination;
      assert(destination != null);
      this.cloudPrintInterface_.submit(
          destination, this.printTicketStore_.createPrintTicket(destination),
          this.documentInfo_, data);
    },

    /**
     * Called from the native layer when the user cancels the save-to-pdf file
     * selection dialog.
     * @private
     */
    onFileSelectionCancel_: function() {
      assert(
          this.uiState_ == PrintPreviewUiState_.FILE_SELECTION,
          'File selection cancelled when not in file-selection state: ' +
              this.uiState_);
      this.setIsEnabled_(true);
      this.uiState_ = PrintPreviewUiState_.READY;
    },

    /**
     * Called after successfully submitting a job to Google Cloud Print.
     * @param {!Event} event Contains the ID of the submitted print job.
     * @private
     */
    onCloudPrintSubmitDone_: function(event) {
      assert(
          this.uiState_ == PrintPreviewUiState_.PRINTING,
          'Submited job to Google Cloud Print but not in printing state ' +
              this.uiState_);
      this.close_(false);
    },

    /**
     * Called when there was an error communicating with Google Cloud print.
     * Displays an error message in the print header.
     * @param {!Event} event Contains the error message.
     * @private
     */
    onCloudPrintError_: function(event) {
      if (event.status == 0) {
        return;  // Ignore, the system does not have internet connectivity.
      }
      if (event.status == 403) {
        if (!this.isInAppKioskMode_) {
          this.destinationSearch_.showCloudPrintPromo();
        }
      } else {
        this.printHeader_.setErrorMessage(event.message);
      }
      if (event.status == 200) {
        console.error(
            'Google Cloud Print Error: (' + event.errorCode + ') ' +
            event.message);
      } else {
        console.error('Google Cloud Print Error: HTTP status ' + event.status);
      }
    },

    /**
     * Called when the preview area's preview generation is in progress.
     * @private
     */
    onPreviewGenerationInProgress_: function() {
      this.isPreviewGenerationInProgress_ = true;
    },

    /**
     * Called when the preview area's preview generation is complete.
     * @private
     */
    onPreviewGenerationDone_: function() {
      this.isPreviewGenerationInProgress_ = false;
      this.printHeader_.isPrintButtonEnabled = true;
      if (this.isListeningForManipulateSettings_)
        this.nativeLayer_.uiLoadedForTest();
      this.printIfReady_();
    },

    /**
     * Called when the preview area's preview failed to load.
     * @private
     */
    onPreviewGenerationFail_: function() {
      this.isPreviewGenerationInProgress_ = false;
      this.printHeader_.isPrintButtonEnabled = false;
      if (this.uiState_ == PrintPreviewUiState_.PRINTING)
        this.nativeLayer_.cancelPendingPrintRequest();
    },

    /**
     * Called when the 'Open pdf in preview' link is clicked. Launches the pdf
     * preview app.
     * @private
     */
    onOpenPdfInPreviewLinkClick_: function() {
      if ($('open-pdf-in-preview-link').classList.contains('disabled'))
        return;
      assert(
          this.uiState_ == PrintPreviewUiState_.READY,
          'Trying to open pdf in preview when not in ready state: ' +
              this.uiState_);
      setIsVisible(getRequiredElement('open-preview-app-throbber'), true);
      this.previewArea_.showCustomMessage(
          loadTimeData.getString('openingPDFInPreview'));
      this.printDocumentOrOpenPdfPreview_(true /*isPdfPreview*/);
    },

    /**
     * Called when the print header's print button is clicked. Prints the
     * document.
     * @private
     */
    onPrintButtonClick_: function() {
      assert(
          this.uiState_ == PrintPreviewUiState_.READY,
          'Trying to print when not in ready state: ' + this.uiState_);
      this.printDocumentOrOpenPdfPreview_(false /*isPdfPreview*/);
    },

    /**
     * Called when the print header's cancel button is clicked. Closes the
     * print dialog.
     * @private
     */
    onCancelButtonClick_: function() {
      this.close_(true);
    },

    /**
     * Called when the register promo for Cloud Print is clicked.
     * @private
     */
    onCloudPrintRegisterPromoClick_: function(e) {
      const devicesUrl = 'chrome://devices/register?id=' + e.destination.id;
      this.nativeLayer_.forceOpenNewTab(devicesUrl);
      this.destinationStore_.waitForRegister(e.destination.id);
    },

    /**
     * Consume escape key presses and ctrl + shift + p. Delegate everything else
     * to the preview area.
     * @param {KeyboardEvent} e The keyboard event.
     * @private
     * @suppress {uselessCode}
     * Current compiler preprocessor leaves all the code inside all the <if>s,
     * so the compiler claims that code after first return is unreachable.
     */
    onKeyDown_: function(e) {
      // Escape key closes the dialog.
      if (e.keyCode == 27 && !hasKeyModifiers(e)) {
        // On non-mac with toolkit-views, ESC key is handled by C++-side instead
        // of JS-side.
        if (cr.isMac) {
          this.close_(true);
          e.preventDefault();
        }
        return;
      }

      // On Mac, Cmd-. should close the print dialog.
      if (cr.isMac && e.keyCode == 190 && e.metaKey) {
        this.close_(true);
        e.preventDefault();
        return;
      }

      // Ctrl + Shift + p / Mac equivalent.
      if (e.keyCode == 80) {
        if ((cr.isMac && e.metaKey && e.altKey && !e.shiftKey && !e.ctrlKey) ||
            (!cr.isMac && e.shiftKey && e.ctrlKey && !e.altKey && !e.metaKey)) {
          this.openSystemPrintDialog_();
          e.preventDefault();
          return;
        }
      }

      if (e.keyCode == 13 /*enter*/ &&
          !document.querySelector('.overlay:not([hidden])') &&
          this.destinationStore_.selectedDestination &&
          this.printTicketStore_.isTicketValid() &&
          this.printHeader_.isPrintButtonEnabled) {
        assert(
            this.uiState_ == PrintPreviewUiState_.READY,
            'Trying to print when not in ready state: ' + this.uiState_);
        const activeElementTag = document.activeElement.tagName.toUpperCase();
        if (activeElementTag != 'BUTTON' && activeElementTag != 'SELECT' &&
            activeElementTag != 'A') {
          this.printDocumentOrOpenPdfPreview_(false /*isPdfPreview*/);
          e.preventDefault();
        }
        return;
      }

      // Pass certain directional keyboard events to the PDF viewer.
      this.previewArea_.handleDirectionalKeyEvent(e);
    },

    /**
     * Called when the destination store has selected an unsupported cloud
     * printer.
     * @private
     */
    onSelectedDestinationUnsupported_: function() {
      this.previewArea_.showUnsupportedCloudPrinterMessage();
      this.onSettingsInvalid_();
    },

    /**
     * Called when the destination store fails to fetch capabilities for the
     * selected printer.
     * @private
     */
    onSelectedDestinationInvalid_: function() {
      this.previewArea_.showCustomMessage(
          loadTimeData.getString('invalidPrinterSettings'));
      this.onSettingsInvalid_();
    },

    /**
     * Called when native layer receives invalid settings for a preview request.
     * @private
     */
    onSettingsInvalid_: function() {
      this.uiState_ = PrintPreviewUiState_.ERROR;
      this.isPreviewGenerationInProgress_ = false;
      this.printHeader_.isPrintButtonEnabled = false;
      this.previewArea_.setDestinationValid(false);
      this.updateLinks_();
    },

    /**
     * Called when a ticket item that can be invalid is updated. Updates the
     * enabled state of the system dialog link on Windows and the open pdf in
     * preview link on Mac.
     * @private
     */
    onTicketChange_: function() {
      this.printHeader_.onTicketChange();
      this.updateLinks_();
    },

    /**
     * Called to update the state of the system dialog and open in preview
     * links to reflect invalid print tickets or printers.
     */
    updateLinks_: function() {
      const disable = !this.printTicketStore_.isTicketValid() ||
          this.uiState_ == print_preview.PrintPreviewUiState_.ERROR;
      if (cr.isWindows && $('system-dialog-link'))
        $('system-dialog-link').disabled = disable;
      if ($('open-pdf-in-preview-link'))
        $('open-pdf-in-preview-link').disabled = disable;
    },

    /**
     * Called when the destination settings' change button is activated.
     * Displays the destination search component.
     * @private
     */
    onDestinationChangeButtonActivate_: function() {
      this.destinationSearch_.setIsVisible(true);
    },

    /**
     * Called when the destination settings' change button is activated.
     * Displays the destination search component.
     * @private
     */
    onAdvancedOptionsButtonActivated_: function() {
      this.advancedSettings_.showForDestination(
          assert(this.destinationStore_.selectedDestination));
    },

    /**
     * Called when the destination search dispatches manage all print
     * destinations event. Calls corresponding native layer method.
     * @private
     */
    onManagePrintDestinationsActivated_: function() {
      this.nativeLayer_.managePrinters();
    },

    /**
     * Called when the user wants to sign in to Google Cloud Print. Calls the
     * corresponding native layer event.
     * @param {boolean} addAccount Whether to open an 'add a new account' or
     *     default sign in page.
     * @private
     */
    onCloudPrintSignInActivated_: function(addAccount) {
      this.nativeLayer_.signIn(addAccount)
          .then(this.destinationStore_.onDestinationsReload.bind(
              this.destinationStore_));
    },

    /**
     * Updates printing options according to source document presets.
     * @param {boolean} disableScaling Whether the document disables scaling.
     * @param {number} copies The default number of copies from the document.
     * @param {number} duplex The default duplex setting from the document.
     * @private
     */
    onPrintPresetOptionsFromDocument_: function(
        disableScaling, copies, duplex) {
      if (disableScaling)
        this.documentInfo_.updateIsScalingDisabled(true);

      if (copies > 0 && this.printTicketStore_.copies.isCapabilityAvailable()) {
        this.printTicketStore_.copies.updateValue(copies);
      }

      if (duplex >= 0 & this.printTicketStore_.duplex.isCapabilityAvailable()) {
        this.printTicketStore_.duplex.updateValue(duplex);
      }
    },

    /**
     * Called when the Page Count Ready message is received to update the fit to
     * page scaling value in the scaling settings.
     * @param {number} pageCount The document's page count (unused).
     * @param {number} previewResponseId The request ID that corresponds to this
     *     page count (unused).
     * @param {number} fitToPageScaling The scaling required to fit the document
     *     to page.
     * @private
     */
    onPageCountReady_: function(
        pageCount, previewResponseId, fitToPageScaling) {
      if (fitToPageScaling >= 0) {
        this.scalingSettings_.updateFitToPageScaling(fitToPageScaling);
      }
    },

    /**
     * Called when printing to a privet, cloud, or extension printer fails.
     * @param {*} httpError The HTTP error code, or -1 or a string describing
     *     the error, if not an HTTP error.
     * @private
     */
    onPrintFailed_: function(httpError) {
      console.error('Printing failed with error code ' + httpError);
      this.printHeader_.setErrorMessage(
          loadTimeData.getString('couldNotPrint'));
    },

    /**
     * Called to start listening for the manipulate-settings-for-test WebUI
     * event so that settings can be modified by this event.
     * @private
     */
    onEnableManipulateSettingsForTest_: function() {
      this.listenerTracker.add(
          'manipulate-settings-for-test',
          this.onManipulateSettingsForTest_.bind(this));
      this.isListeningForManipulateSettings_ = true;
    },

    /**
     * Called when the print preview settings need to be changed for testing.
     * @param {!print_preview.PreviewSettings} settings Contains print preview
     *     settings to change and the values to change them to.
     * @private
     */
    onManipulateSettingsForTest_: function(settings) {
      if ('selectSaveAsPdfDestination' in settings) {
        this.saveAsPdfForTest_();  // No parameters.
      } else if ('layoutSettings' in settings) {
        this.setLayoutSettingsForTest_(settings.layoutSettings.portrait);
      } else if ('pageRange' in settings) {
        this.setPageRangeForTest_(settings.pageRange);
      } else if ('headersAndFooters' in settings) {
        this.setHeadersAndFootersForTest_(settings.headersAndFooters);
      } else if ('backgroundColorsAndImages' in settings) {
        this.setBackgroundColorsAndImagesForTest_(
            settings.backgroundColorsAndImages);
      } else if ('margins' in settings) {
        this.setMarginsForTest_(settings.margins);
      }
    },

    /**
     * Called by onManipulateSettingsForTest_(). Sets the print destination
     * as a pdf.
     * @private
     */
    saveAsPdfForTest_: function() {
      if (this.destinationStore_.selectedDestination &&
          print_preview.Destination.GooglePromotedId.SAVE_AS_PDF ==
              this.destinationStore_.selectedDestination.id) {
        this.nativeLayer_.uiLoadedForTest();
        return;
      }

      const destinations = this.destinationStore_.destinations();
      let pdfDestination = null;
      for (let i = 0; i < destinations.length; i++) {
        if (destinations[i].id ==
            print_preview.Destination.GooglePromotedId.SAVE_AS_PDF) {
          pdfDestination = destinations[i];
          break;
        }
      }

      if (pdfDestination)
        this.destinationStore_.selectDestination(pdfDestination);
      else
        this.nativeLayer_.uiFailedLoadingForTest();
    },

    /**
     * Called by onManipulateSettingsForTest_(). Sets the layout settings to
     * either portrait or landscape.
     * @param {boolean} portrait Whether to use portrait page layout;
     *     if false: landscape.
     * @private
     */
    setLayoutSettingsForTest_: function(portrait) {
      const combobox = document.querySelector('.layout-settings-select');
      if (combobox.value == 'portrait') {
        this.nativeLayer_.uiLoadedForTest();
      } else {
        combobox.value = 'landscape';
        this.layoutSettings_.onSelectChange();
      }
    },

    /**
     * Called by onManipulateSettingsForTest_(). Sets the page range for
     * for the print preview settings.
     * @param {string} pageRange Sets the page range to the desired value(s).
     *     Ex: "1-5,9" means pages 1 through 5 and page 9 will be printed.
     * @private
     */
    setPageRangeForTest_: function(pageRange) {
      const textbox = document.querySelector('.page-settings-custom-input');
      if (textbox.value == pageRange) {
        this.nativeLayer_.uiLoadedForTest();
      } else {
        textbox.value = pageRange;
        document.querySelector('.page-settings-custom-radio').click();
      }
    },

    /**
     * Called by onManipulateSettings_(). Checks or unchecks the headers and
     * footers option on print preview.
     * @param {boolean} headersAndFooters Whether the "Headers and Footers"
     *     checkbox should be checked.
     * @private
     */
    setHeadersAndFootersForTest_: function(headersAndFooters) {
      const checkbox = document.querySelector('.header-footer-checkbox');
      if (headersAndFooters == checkbox.checked)
        this.nativeLayer_.uiLoadedForTest();
      else
        checkbox.click();
    },

    /**
     * Called by onManipulateSettings_(). Checks or unchecks the background
     * colors and images option on print preview.
     * @param {boolean} backgroundColorsAndImages If true, the checkbox should
     *     be checked. Otherwise it should be unchecked.
     * @private
     */
    setBackgroundColorsAndImagesForTest_: function(backgroundColorsAndImages) {
      const checkbox = document.querySelector('.css-background-checkbox');
      if (backgroundColorsAndImages == checkbox.checked)
        this.nativeLayer_.uiLoadedForTest();
      else
        checkbox.click();
    },

    /**
     * Called by onManipulateSettings_(). Sets the margin settings
     * that are desired. Custom margin settings aren't currently supported.
     * @param {number} margins The desired margins combobox index. Must be
     *     a valid index or else the test fails.
     * @private
     */
    setMarginsForTest_: function(margins) {
      const combobox = document.querySelector('.margin-settings-select');
      if (margins == combobox.selectedIndex) {
        this.nativeLayer_.uiLoadedForTest();
      } else if (margins >= 0 && margins < combobox.length) {
        combobox.selectedIndex = margins;
        this.marginSettings_.onSelectChange();
      } else {
        this.nativeLayer_.uiFailedLoadingForTest();
      }
    },

    /**
     * Returns true if "Print using system dialog" link should be shown for
     * current destination.
     * @return {boolean} Returns true if link should be shown.
     */
    shouldShowSystemDialogLink_: function() {
      if (cr.isChromeOS || this.hideSystemDialogLink_)
        return false;
      if (!cr.isWindows)
        return true;
      const selectedDest = this.destinationStore_.selectedDestination;
      return !!selectedDest &&
          selectedDest.origin == print_preview.DestinationOrigin.LOCAL &&
          selectedDest.id !=
          print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
    },

    /**
     * Called when a print destination is selected. Shows/hides the "Print with
     * Cloud Print" link in the navbar.
     * @private
     */
    onDestinationSelect_: function() {
      if ($('system-dialog-link')) {
        setIsVisible(
            getRequiredElement('system-dialog-link'),
            this.shouldShowSystemDialogLink_());
      }
      // Reset if we had a bad settings fetch since the user selected a new
      // printer.
      if (this.uiState_ == PrintPreviewUiState_.ERROR) {
        this.uiState_ = PrintPreviewUiState_.READY;
        this.updateLinks_();
        this.previewArea_.setDestinationValid(true);
      }
      if (this.destinationStore_.selectedDestination &&
          this.isInKioskAutoPrintMode_) {
        this.onPrintButtonClick_();
      }
    },
  };

  // Export
  return {PrintPreview: PrintPreview};
});

// Pull in all other scripts in a single shot.
// <include src="common/overlay.js">
// <include src="common/search_box.js">
// <include src="common/search_bubble.js">

// <include src="data/page_number_set.js">
// <include src="data/destination.js">
// <include src="data/destination_match.js">
// <include src="data/local_parsers.js">
// <include src="data/cloud_parsers.js">
// <include src="data/destination_store.js">
// <include src="data/invitation.js">
// <include src="data/invitation_store.js">
// <include src="data/margins.js">
// <include src="data/document_info.js">
// <include src="data/printable_area.js">
// <include src="data/measurement_system.js">
// <include src="data/print_ticket_store.js">
// <include src="data/coordinate2d.js">
// <include src="data/size.js">
// <include src="data/capabilities_holder.js">
// <include src="data/user_info.js">
// <include src="data/app_state.js">

// <include src="data/ticket_items/ticket_item.js">

// <include src="data/ticket_items/custom_margins.js">
// <include src="data/ticket_items/collate.js">
// <include src="data/ticket_items/color.js">
// <include src="data/ticket_items/copies.js">
// <include src="data/ticket_items/dpi.js">
// <include src="data/ticket_items/duplex.js">
// <include src="data/ticket_items/header_footer.js">
// <include src="data/ticket_items/media_size.js">
// <include src="data/ticket_items/scaling.js">
// <include src="data/ticket_items/landscape.js">
// <include src="data/ticket_items/margins_type.js">
// <include src="data/ticket_items/page_range.js">
// <include src="data/ticket_items/fit_to_page.js">
// <include src="data/ticket_items/css_background.js">
// <include src="data/ticket_items/selection_only.js">
// <include src="data/ticket_items/rasterize.js">
// <include src="data/ticket_items/vendor_items.js">

// <include src="native_layer.js">
// <include src="print_preview_animations.js">
// <include src="cloud_print_interface.js">
// <include src="print_preview_utils.js">
// <include src="print_header.js">
// <include src="metrics.js">

// <include src="settings/settings_section.js">
// <include src="settings/settings_section_select.js">
// <include src="settings/destination_settings.js">
// <include src="settings/page_settings.js">
// <include src="settings/copies_settings.js">
// <include src="settings/layout_settings.js">
// <include src="settings/color_settings.js">
// <include src="settings/media_size_settings.js">
// <include src="settings/margin_settings.js">
// <include src="settings/dpi_settings.js">
// <include src="settings/scaling_settings.js">
// <include src="settings/other_options_settings.js">
// <include src="settings/advanced_options_settings.js">
// <include src="settings/advanced_settings/advanced_settings.js">
// <include src="settings/advanced_settings/advanced_settings_item.js">
// <include src="settings/more_settings.js">

// <include src="previewarea/margin_control.js">
// <include src="previewarea/margin_control_container.js">
// <include src="../pdf/pdf_scripting_api.js">
// <include src="previewarea/preview_area.js">
// <include src="preview_generator.js">

// <include src="search/destination_list.js">
// <include src="search/recent_destination_list.js">
// <include src="search/destination_list_item.js">
// <include src="search/destination_search.js">
// <include src="search/provisional_destination_resolver.js">

window.addEventListener('DOMContentLoaded', function() {
  const previewWindow = /** @type {{isTest: boolean}} */ (window);
  if (!previewWindow.isTest) {
    const printPreview = new print_preview.PrintPreview();
    printPreview.initialize();
  }
});
