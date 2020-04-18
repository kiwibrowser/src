// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Component used for searching for a print destination.
   * This is a modal dialog that allows the user to search and select a
   * destination to print to. When a destination is selected, it is written to
   * the destination store.
   * @param {!print_preview.DestinationStore} destinationStore Data store
   *     containing the destinations to search through.
   * @param {!print_preview.InvitationStore} invitationStore Data store
   *     holding printer sharing invitations.
   * @param {!print_preview.UserInfo} userInfo Event target that contains
   *     information about the logged in user.
   * @param {!print_preview.AppState} appState Contains recent destination list.
   * @constructor
   * @extends {print_preview.Overlay}
   */
  function DestinationSearch(
      destinationStore, invitationStore, userInfo, appState) {
    print_preview.Overlay.call(this);

    /**
     * Data store containing the destinations to search through.
     * @private {!print_preview.DestinationStore}
     */
    this.destinationStore_ = destinationStore;

    /**
     * Data store holding printer sharing invitations.
     * @private {!print_preview.InvitationStore}
     */
    this.invitationStore_ = invitationStore;

    /**
     * Event target that contains information about the logged in user.
     * @private {!print_preview.UserInfo}
     */
    this.userInfo_ = userInfo;

    /**
     * Contains recent destinations that are currently set to be persisted into
     * the sticky settings.
     * @private {!print_preview.AppState}
     */
    this.appState_ = appState;

    /**
     * Currently displayed printer sharing invitation.
     * @private {print_preview.Invitation}
     */
    this.invitation_ = null;

    /**
     * Used to record usage statistics.
     * @private {!print_preview.DestinationSearchMetricsContext}
     */
    this.metrics_ = new print_preview.DestinationSearchMetricsContext();

    /**
     * Whether or not a UMA histogram for the register promo being shown was
     * already recorded.
     * @private {boolean}
     */
    this.registerPromoShownMetricRecorded_ = false;

    /**
     * Child overlay used for resolving a provisional destination. The overlay
     * is shown when the user attempts to select a provisional destination.
     * Set only when a destination is being resolved.
     * @private {?print_preview.ProvisionalDestinationResolver}
     */
    this.provisionalDestinationResolver_ = null;

    /**
     * The destination that is currently in configuration.
     * @private {?print_preview.Destination}
     */
    this.destinationInConfiguring_ = null;

    /**
     * Search box used to search through the destination lists.
     * @private {!print_preview.SearchBox}
     */
    this.searchBox_ = new print_preview.SearchBox(
        loadTimeData.getString('searchBoxPlaceholder'));
    this.addChild(this.searchBox_);

    /**
     * Destination list containing recent destinations.
     * @private {!print_preview.DestinationList}
     */
    this.recentList_ = new print_preview.RecentDestinationList(this);
    this.addChild(this.recentList_);

    /**
     * Destination list containing all print destinations.
     * @private {!print_preview.DestinationList}
     */
    this.printList_ = new print_preview.DestinationList(
        this, loadTimeData.getString('printDestinationsTitle'),
        loadTimeData.getString('manage'));
    this.addChild(this.printList_);
  }

  /**
   * Event types dispatched by the component.
   * @enum {string}
   */
  DestinationSearch.EventType = {
    // Dispatched when user requests to sign-in into another Google account.
    ADD_ACCOUNT: 'print_preview.DestinationSearch.ADD_ACCOUNT',

    // Dispatched when the user requests to manage their print destinations.
    MANAGE_PRINT_DESTINATIONS:
        'print_preview.DestinationSearch.MANAGE_PRINT_DESTINATIONS',

    // Dispatched when the user requests to sign-in to their Google account.
    SIGN_IN: 'print_preview.DestinationSearch.SIGN_IN'
  };

  /**
   * Number of unregistered destinations that may be promoted to the top.
   * @type {number}
   * @const
   * @private
   */
  DestinationSearch.MAX_PROMOTED_UNREGISTERED_PRINTERS_ = 2;

  DestinationSearch.prototype = {
    __proto__: print_preview.Overlay.prototype,

    /** @override */
    onSetVisibleInternal: function(isVisible) {
      if (isVisible) {
        this.searchBox_.focus();
        if (getIsVisible(this.getChildElement('.cloudprint-promo'))) {
          this.metrics_.record(
              print_preview.Metrics.DestinationSearchBucket.SIGNIN_PROMPT);
        }
        if (this.userInfo_.initialized)
          this.onUsersChanged_();
        this.reflowLists_();
        this.metrics_.record(
            print_preview.Metrics.DestinationSearchBucket.DESTINATION_SHOWN);

        this.destinationStore_.startLoadAllDestinations();
        this.invitationStore_.startLoadingInvitations();
      } else {
        // Collapse all destination lists
        this.printList_.setIsShowAll(false);
        if (this.provisionalDestinationResolver_)
          this.provisionalDestinationResolver_.cancel();
        this.resetSearch_();
      }
    },

    /** @override */
    onCancelInternal: function() {
      this.metrics_.record(print_preview.Metrics.DestinationSearchBucket
                               .DESTINATION_CLOSED_UNCHANGED);
    },

    /** Shows the Google Cloud Print promotion banner. */
    showCloudPrintPromo: function() {
      const cloudPrintPromoElement = this.getChildElement('.cloudprint-promo');
      if (getIsVisible(cloudPrintPromoElement))
        return;

      setIsVisible(cloudPrintPromoElement, true);
      if (this.getIsVisible()) {
        this.metrics_.record(
            print_preview.Metrics.DestinationSearchBucket.SIGNIN_PROMPT);
      }
      this.reflowLists_();
    },

    /** @override */
    enterDocument: function() {
      print_preview.Overlay.prototype.enterDocument.call(this);

      this.tracker.add(
          this.getChildElement('.account-select'), 'change',
          this.onAccountChange_.bind(this));

      this.tracker.add(
          this.getChildElement('.sign-in'), 'click',
          this.onSignInActivated_.bind(this));

      this.tracker.add(
          this.getChildElement('.invitation-accept-button'), 'click',
          this.onInvitationProcessButtonClick_.bind(this, true /*accept*/));
      this.tracker.add(
          this.getChildElement('.invitation-reject-button'), 'click',
          this.onInvitationProcessButtonClick_.bind(this, false /*accept*/));

      this.tracker.add(
          this.getChildElement('.cloudprint-promo > .close-button'), 'click',
          this.onCloudprintPromoCloseButtonClick_.bind(this));
      this.tracker.add(
          this.searchBox_, print_preview.SearchBox.EventType.SEARCH,
          this.onSearch_.bind(this));
      this.tracker.add(
          this, print_preview.DestinationListItem.EventType.CONFIGURE_REQUEST,
          this.onDestinationConfigureRequest_.bind(this));
      this.tracker.add(
          this, print_preview.DestinationListItem.EventType.SELECT,
          this.onDestinationSelect_.bind(this));
      this.tracker.add(
          this,
          print_preview.DestinationListItem.EventType.REGISTER_PROMO_CLICKED,
          () => {
            this.metrics_.record(print_preview.Metrics.DestinationSearchBucket
                                     .REGISTER_PROMO_SELECTED);
          });

      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATIONS_INSERTED,
          this.onDestinationsInserted_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SELECT,
          this.onDestinationStoreSelect_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SEARCH_STARTED,
          this.updateThrobbers_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SEARCH_DONE,
          this.onDestinationSearchDone_.bind(this));
      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .PROVISIONAL_DESTINATION_RESOLVED,
          this.onDestinationsInserted_.bind(this));

      this.tracker.add(
          this.invitationStore_,
          print_preview.InvitationStore.EventType.INVITATION_SEARCH_DONE,
          this.updateInvitations_.bind(this));
      this.tracker.add(
          this.invitationStore_,
          print_preview.InvitationStore.EventType.INVITATION_PROCESSED,
          this.updateInvitations_.bind(this));

      this.tracker.add(
          this.printList_,
          print_preview.DestinationList.EventType.ACTION_LINK_ACTIVATED,
          this.onManagePrintDestinationsActivated_.bind(this));

      this.tracker.add(
          this.userInfo_, print_preview.UserInfo.EventType.USERS_CHANGED,
          this.onUsersChanged_.bind(this));

      this.tracker.add(
          this.getChildElement('.button-strip .cancel-button'), 'click',
          this.cancel.bind(this));

      this.tracker.add(window, 'resize', this.onWindowResize_.bind(this));

      this.updateThrobbers_();

      // Render any destinations already in the store.
      this.renderDestinations_();
    },

    /** @override */
    decorateInternal: function() {
      this.searchBox_.render(this.getChildElement('.search-box-container'));
      this.recentList_.render(this.getChildElement('.recent-list'));
      this.printList_.render(this.getChildElement('.print-list'));
      this.getChildElement('.promo-text').innerHTML = loadTimeData.getStringF(
          'cloudPrintPromotion', '<a is="action-link" class="sign-in">',
          '</a>');
      this.getChildElement('.account-select-label').textContent =
          loadTimeData.getString('accountSelectTitle');
    },

    /**
     * @return {number} Height available for destination lists, in pixels.
     * @private
     */
    getAvailableListsHeight_: function() {
      const elStyle = window.getComputedStyle(this.getElement());
      return this.getElement().offsetHeight -
          parseInt(elStyle.getPropertyValue('padding-top'), 10) -
          parseInt(elStyle.getPropertyValue('padding-bottom'), 10) -
          this.getChildElement('.lists').offsetTop -
          this.getChildElement('.invitation-container').offsetHeight -
          this.getChildElement('.cloudprint-promo').offsetHeight -
          this.getChildElement('.action-area').offsetHeight;
    },

    /**
     * Filters all destination lists with the given query.
     * @param {RegExp} query Query to filter destination lists by.
     * @private
     */
    filterLists_: function(query) {
      this.recentList_.updateSearchQuery(query);
      this.printList_.updateSearchQuery(query);
    },

    /**
     * Resets the search query.
     * @private
     */
    resetSearch_: function() {
      this.searchBox_.setQuery(null);
      this.filterLists_(null);
    },

    /**
     * @param {?string} filterAccount Account to filter recent destinations by.
     * @return {!Array<!print_preview.Destination>} List of recent destinations
     * @private
     */
    getRecentDestinations_(filterAccount) {
      let recentDestinations = [];
      this.appState_.recentDestinations.forEach((recentDestination) => {
        const origin = recentDestination.origin;
        const id = recentDestination.id;
        const account = recentDestination.account || '';
        const destination =
            this.destinationStore_.getDestination(origin, id, account);
        if (destination &&
            (!destination.account || destination.account == filterAccount)) {
          recentDestinations.push(destination);
        }
      });
      return recentDestinations;
    },

    /**
     * Renders all of the destinations in the destination store.
     * @private
     */
    renderDestinations_: function() {
      const recentDestinations =
          this.getRecentDestinations_(this.userInfo_.activeUser);
      const localDestinations = [];
      const cloudDestinations = [];
      const unregisteredCloudDestinations = [];

      const destinations =
          this.destinationStore_.destinations(this.userInfo_.activeUser);
      destinations.forEach(function(destination) {
        if (destination.isLocal ||
            destination.origin == print_preview.DestinationOrigin.DEVICE) {
          localDestinations.push(destination);
        } else {
          if (destination.connectionStatus ==
              print_preview.DestinationConnectionStatus.UNREGISTERED) {
            unregisteredCloudDestinations.push(destination);
          } else {
            cloudDestinations.push(destination);
          }
        }
      });

      if (unregisteredCloudDestinations.length != 0 &&
          !this.registerPromoShownMetricRecorded_) {
        this.metrics_.record(
            print_preview.Metrics.DestinationSearchBucket.REGISTER_PROMO_SHOWN);
        this.registerPromoShownMetricRecorded_ = true;
      }

      const finalCloudDestinations =
          unregisteredCloudDestinations
              .slice(0, DestinationSearch.MAX_PROMOTED_UNREGISTERED_PRINTERS_)
              .concat(
                  cloudDestinations,
                  unregisteredCloudDestinations.slice(
                      DestinationSearch.MAX_PROMOTED_UNREGISTERED_PRINTERS_));
      const finalPrintDestinations =
          localDestinations.concat(finalCloudDestinations);

      this.recentList_.updateDestinations(recentDestinations);
      this.printList_.updateDestinations(finalPrintDestinations);
    },

    /**
     * Reflows the destination lists according to the available height.
     * @private
     */
    reflowLists_: function() {
      if (!this.getIsVisible()) {
        return;
      }

      const lists = [this.recentList_, this.printList_];
      const getListsTotalHeight = function(lists, counts) {
        return lists.reduce(function(sum, list, index) {
          const container = list.getContainerElement();
          return sum + list.getEstimatedHeightInPixels(counts[index]) +
              parseInt(window.getComputedStyle(container).paddingBottom, 10);
        }, 0);
      };
      const getCounts = function(lists, count) {
        return lists.map(function(list) {
          return count;
        });
      };

      const availableHeight = this.getAvailableListsHeight_();
      const listsEl = this.getChildElement('.lists');
      listsEl.style.maxHeight = availableHeight + 'px';

      const maxListLength = lists.reduce(function(prevCount, list) {
        return Math.max(prevCount, list.getDestinationsCount());
      }, 0);

      let i = 1;
      for (; i <= maxListLength; i++) {
        if (getListsTotalHeight(lists, getCounts(lists, i)) > availableHeight) {
          i--;
          break;
        }
      }
      const counts = getCounts(lists, i);
      // Fill up the possible n-1 free slots left by the previous loop.
      if (getListsTotalHeight(lists, counts) < availableHeight) {
        for (let countIndex = 0; countIndex < counts.length; countIndex++) {
          counts[countIndex]++;
          if (getListsTotalHeight(lists, counts) > availableHeight) {
            counts[countIndex]--;
            break;
          }
        }
      }

      lists.forEach(function(list, index) {
        list.updateShortListSize(counts[index]);
      });

      // Set height of the list manually so that search filter doesn't change
      // lists height.
      const listsHeight = getListsTotalHeight(lists, counts) + 'px';
      if (listsHeight != listsEl.style.height) {
        // Try to close account select if there's a possibility it's open now.
        const accountSelectEl = this.getChildElement('.account-select');
        if (!accountSelectEl.disabled) {
          accountSelectEl.disabled = true;
          accountSelectEl.disabled = false;
        }
        listsEl.style.height = listsHeight;
      }
    },

    /**
     * Updates whether the throbbers for the various destination lists should be
     * shown or hidden.
     * @private
     */
    updateThrobbers_: function() {
      this.printList_.setIsThrobberVisible(
          this.destinationStore_.isPrintDestinationSearchInProgress);
      this.recentList_.setIsThrobberVisible(
          this.destinationStore_.isPrintDestinationSearchInProgress);
      this.reflowLists_();
    },

    /**
     * Updates printer sharing invitations UI.
     * @private
     */
    updateInvitations_: function() {
      const invitations = this.userInfo_.activeUser ?
          this.invitationStore_.invitations(this.userInfo_.activeUser) :
          [];
      if (invitations.length > 0) {
        if (this.invitation_ != invitations[0]) {
          this.metrics_.record(print_preview.Metrics.DestinationSearchBucket
                                   .INVITATION_AVAILABLE);
        }
        this.invitation_ = invitations[0];
        this.showInvitation_(this.invitation_);
      } else {
        this.invitation_ = null;
      }
      setIsVisible(
          this.getChildElement('.invitation-container'), !!this.invitation_);
      this.reflowLists_();
    },

    /**
     * @param {!print_preview.Invitation} invitation Invitation to show.
     * @private
     */
    showInvitation_: function(invitation) {
      let invitationText = '';
      if (invitation.asGroupManager) {
        invitationText = loadTimeData.getStringF(
            'groupPrinterSharingInviteText', HTMLEscape(invitation.sender),
            HTMLEscape(invitation.destination.displayName),
            HTMLEscape(invitation.receiver));
      } else {
        invitationText = loadTimeData.getStringF(
            'printerSharingInviteText', HTMLEscape(invitation.sender),
            HTMLEscape(invitation.destination.displayName));
      }
      this.getChildElement('.invitation-text').innerHTML = invitationText;

      const acceptButton = this.getChildElement('.invitation-accept-button');
      acceptButton.textContent = loadTimeData.getString(
          invitation.asGroupManager ? 'acceptForGroup' : 'accept');
      acceptButton.disabled = !!this.invitationStore_.invitationInProgress;
      this.getChildElement('.invitation-reject-button').disabled =
          !!this.invitationStore_.invitationInProgress;
      setIsVisible(
          this.getChildElement('#invitation-process-throbber'),
          !!this.invitationStore_.invitationInProgress);
    },

    /**
     * Called when user's logged in accounts change. Updates the UI.
     * @private
     */
    onUsersChanged_: function() {
      const loggedIn = this.userInfo_.loggedIn;
      if (loggedIn) {
        const accountSelectEl = this.getChildElement('.account-select');
        accountSelectEl.innerHTML = '';
        this.userInfo_.users.forEach(function(account) {
          const option = document.createElement('option');
          option.text = account;
          option.value = account;
          accountSelectEl.add(option);
        });
        const option = document.createElement('option');
        option.text = loadTimeData.getString('addAccountTitle');
        option.value = '';
        accountSelectEl.add(option);

        accountSelectEl.selectedIndex = this.userInfo_.activeUser ?
            this.userInfo_.users.indexOf(this.userInfo_.activeUser) :
            -1;
      }

      setIsVisible(this.getChildElement('.user-info'), loggedIn);
      setIsVisible(this.getChildElement('.cloudprint-promo'), !loggedIn);
      this.updateInvitations_();
    },

    /**
     * Called when a destination search should be executed. Filters the
     * destination lists with the given query.
     * @param {!Event} event Contains the search query.
     * @private
     */
    onSearch_: function(event) {
      this.filterLists_(event.queryRegExp);
    },

    /**
     * Handler for {@code print_preview.DestinationListItem.EventType.
     * CONFIGURE_REQUEST} event, which is called to check a destination list
     * item needs to be setup on Chrome OS before being selected. Note we do not
     * allow configuring more than one destination at the same time.
     * @param {!CustomEvent} event Contains the destination needs to be setup.
     * @private
     */
    onDestinationConfigureRequest_: function(event) {
      const destination = event.detail.destination;
      const destinationItem =
          this.printList_.getDestinationItem(destination.id);
      assert(
          destinationItem != null,
          'User does not select a valid destination item.');

      // Another printer setup is in process or the printer doesn't need to be
      // set up. Reject the setup request directly.
      if (this.destinationInConfiguring_ != null ||
          destination.origin != print_preview.DestinationOrigin.CROS ||
          destination.capabilities != null) {
        destinationItem.onConfigureRequestRejected(
            this.destinationInConfiguring_ != null);
      } else {
        destinationItem.onConfigureRequestAccepted();
        this.handleConfigureDestination_(destination);
      }
    },

    /**
     * Called When a destination needs to be setup.
     * @param {!print_preview.Destination} destination The destination needs to
     *     be setup.
     * @private
     */
    handleConfigureDestination_: function(destination) {
      assert(
          destination.origin == print_preview.DestinationOrigin.CROS,
          'Only local printer on Chrome OS requires setup.');
      this.destinationInConfiguring_ = destination;
      this.destinationStore_.resolveCrosDestination(destination)
          .then(
              response => {
                this.destinationInConfiguring_ = null;
                this.printList_.getDestinationItem(destination.id)
                    .onConfigureResolved(response);
              },
              () => {
                this.destinationInConfiguring_ = null;
                this.printList_.getDestinationItem(destination.id)
                    .onConfigureResolved(
                        {printerId: destination.id, success: false});
              });
    },

    /**
     * Handler for {@code print_preview.DestinationListItem.EventType.SELECT}
     * event, which is called when a destination list item is selected.
     * @param {Event} event Contains the selected destination.
     * @private
     */
    onDestinationSelect_: function(event) {
      this.handleOnDestinationSelect_(event.destination);
    },

    /**
     * Called when a destination is selected. Clears the search and hides the
     * widget. If The destination is provisional, it runs provisional
     * destination resolver first.
     * @param {!print_preview.Destination} destination The selected destination.
     * @private
     */
    handleOnDestinationSelect_: function(destination) {
      if (destination.isProvisional) {
        assert(
            !this.provisionalDestinationResolver_,
            'Provisional destination resolver already exists.');
        this.provisionalDestinationResolver_ =
            print_preview.ProvisionalDestinationResolver.create(
                this.destinationStore_, destination);
        assert(
            !!this.provisionalDestinationResolver_,
            'Unable to create provisional destination resolver');

        const lastFocusedElement = document.activeElement;
        this.addChild(this.provisionalDestinationResolver_);
        this.provisionalDestinationResolver_.run(this.getElement())
            .then(resolvedDestination => {
              this.handleOnDestinationSelect_(resolvedDestination);
            })
            .catch(function() {
              console.error(
                  'Failed to resolve provisional destination: ' +
                  destination.id);
            })
            .then(() => {
              this.removeChild(assert(this.provisionalDestinationResolver_));
              this.provisionalDestinationResolver_ = null;

              // Restore focus to the previosly focused element if it's
              // still shown in the search.
              if (lastFocusedElement && this.getIsVisible() &&
                  getIsVisible(lastFocusedElement) &&
                  this.getElement().contains(lastFocusedElement)) {
                lastFocusedElement.focus();
              }
            });
        return;
      }

      this.setIsVisible(false);
      this.destinationStore_.selectDestination(destination);
      this.metrics_.record(print_preview.Metrics.DestinationSearchBucket
                               .DESTINATION_CLOSED_CHANGED);
    },

    /**
     * Called when a destination is selected. Selected destination are marked as
     * recent, so we have to update our recent destinations list.
     * @private
     */
    onDestinationStoreSelect_: function() {
      const recentDestinations =
          this.getRecentDestinations_(this.userInfo_.activeUser);
      this.recentList_.updateDestinations(recentDestinations);
      this.reflowLists_();
    },

    /**
     * Called when destinations are inserted into the store. Rerenders
     * destinations.
     * @private
     */
    onDestinationsInserted_: function() {
      this.renderDestinations_();
      this.reflowLists_();
    },

    /**
     * Called when destinations are inserted into the store. Rerenders
     * destinations.
     * @private
     */
    onDestinationSearchDone_: function() {
      this.updateThrobbers_();
      this.renderDestinations_();
      this.reflowLists_();
      // In case user account information was retrieved with this search
      // (knowing current user account is required to fetch invitations).
      this.invitationStore_.startLoadingInvitations();
    },

    /**
     * Called when the manage all printers action is activated.
     * @private
     */
    onManagePrintDestinationsActivated_: function() {
      cr.dispatchSimpleEvent(
          this,
          print_preview.DestinationSearch.EventType.MANAGE_PRINT_DESTINATIONS);
    },

    /**
     * Called when the "Sign in" link on the Google Cloud Print promo is
     * activated.
     * @private
     */
    onSignInActivated_: function() {
      cr.dispatchSimpleEvent(this, DestinationSearch.EventType.SIGN_IN);
      this.metrics_.record(
          print_preview.Metrics.DestinationSearchBucket.SIGNIN_TRIGGERED);
    },

    /**
     * Called when item in the Accounts list is selected. Initiates active user
     * switch or, for 'Add account...' item, opens Google sign-in page.
     * @private
     */
    onAccountChange_: function() {
      const accountSelectEl = this.getChildElement('.account-select');
      const account =
          accountSelectEl.options[accountSelectEl.selectedIndex].value;
      if (account) {
        this.userInfo_.activeUser = account;
        this.destinationStore_.reloadUserCookieBasedDestinations();
        this.invitationStore_.startLoadingInvitations();
        this.metrics_.record(
            print_preview.Metrics.DestinationSearchBucket.ACCOUNT_CHANGED);
      } else {
        cr.dispatchSimpleEvent(this, DestinationSearch.EventType.ADD_ACCOUNT);
        // Set selection back to the active user.
        for (let i = 0; i < accountSelectEl.options.length; i++) {
          if (accountSelectEl.options[i].value == this.userInfo_.activeUser) {
            accountSelectEl.selectedIndex = i;
            break;
          }
        }
        this.metrics_.record(
            print_preview.Metrics.DestinationSearchBucket.ADD_ACCOUNT_SELECTED);
      }
    },

    /**
     * Called when the printer sharing invitation Accept/Reject button is
     * clicked.
     * @private
     */
    onInvitationProcessButtonClick_: function(accept) {
      this.metrics_.record(
          accept ? print_preview.Metrics.DestinationSearchBucket
                       .INVITATION_ACCEPTED :
                   print_preview.Metrics.DestinationSearchBucket
                       .INVITATION_REJECTED);
      this.invitationStore_.processInvitation(assert(this.invitation_), accept);
      this.updateInvitations_();
    },

    /**
     * Called when the close button on the cloud print promo is clicked. Hides
     * the promo.
     * @private
     */
    onCloudprintPromoCloseButtonClick_: function() {
      setIsVisible(this.getChildElement('.cloudprint-promo'), false);
      this.reflowLists_();
    },

    /**
     * Called when the window is resized. Reflows layout of destination lists.
     * @private
     */
    onWindowResize_: function() {
      this.reflowLists_();
    },
  };

  // Export
  return {DestinationSearch: DestinationSearch};
});
