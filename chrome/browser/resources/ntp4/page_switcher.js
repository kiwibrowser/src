// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Page switcher
 * This is the class for the left and right navigation arrows that switch
 * between pages.
 */
cr.define('ntp', function() {

  /**
   * @constructor
   * @extends {HTMLButtonElement}
   */
  function PageSwitcher() {}

  PageSwitcher.prototype = {
    __proto__: HTMLButtonElement.prototype,

    decorate: function(el) {
      el.__proto__ = PageSwitcher.prototype;

      el.addEventListener('click', el.activate_);

      el.direction_ = el.id == 'page-switcher-start' ? -1 : 1;

      el.dragWrapper_ = new cr.ui.DragWrapper(el, el);
    },

    /**
     * Activate the switcher (go to the next card).
     * @private
     */
    activate_: function() {
      ntp.getCardSlider().selectCard(this.nextCardIndex_(), true);
    },

    /**
     * Calculate the index of the card that this button will switch to.
     * @private
     */
    nextCardIndex_: function() {
      var cardSlider = ntp.getCardSlider();
      var index = cardSlider.currentCard + this.direction_;
      var numCards = cardSlider.cardCount - 1;
      return Math.max(0, Math.min(index, numCards));
    },

    /**
     * Update the accessible label attribute of this button, based on the
     * current position in the card slider and the names of the cards.
     * @param {NodeList} dots The dot elements which display the names of the
     *     cards.
     */
    updateButtonAccessibleLabel: function(dots) {
      var currentIndex = ntp.getCardSlider().currentCard;
      var nextCardIndex = this.nextCardIndex_();
      if (nextCardIndex == currentIndex) {
        this.setAttribute('aria-label', '');  // No next card.
        return;
      }

      var currentDot = dots[currentIndex];
      var nextDot = dots[nextCardIndex];
      if (!currentDot || !nextDot) {
        this.setAttribute('aria-label', '');  // Dots not initialised yet.
        return;
      }

      var currentPageTitle = currentDot.displayTitle;
      var nextPageTitle = nextDot.displayTitle;
      var msgName = (currentPageTitle == nextPageTitle) ?
          'page_switcher_same_title' :
          'page_switcher_change_title';
      var ariaLabel = loadTimeData.getStringF(msgName, nextPageTitle);
      this.setAttribute('aria-label', ariaLabel);
    },

    shouldAcceptDrag: function(e) {
      // Only allow page switching when a drop could happen on the page being
      // switched to.
      var nextPage = ntp.getCardSlider().getCardAtIndex(this.nextCardIndex_());
      return nextPage.shouldAcceptDrag(e);
    },

    doDragEnter: function(e) {
      this.scheduleDelayedSwitch_(e);
      this.doDragOver(e);
    },

    doDragLeave: function(e) {
      this.cancelDelayedSwitch_();
    },

    doDragOver: function(e) {
      e.preventDefault();
      var targetPage = ntp.getCardSlider().currentCardValue;
      if (targetPage.shouldAcceptDrag(e))
        targetPage.setDropEffect(e.dataTransfer);
    },

    doDrop: function(e) {
      e.stopPropagation();
      this.cancelDelayedSwitch_();

      var tile = ntp.getCurrentlyDraggingTile();
      if (!tile)
        return;

      var sourcePage = tile.tilePage;
      var targetPage = ntp.getCardSlider().currentCardValue;
      if (targetPage == sourcePage || !targetPage.shouldAcceptDrag(e))
        return;

      targetPage.appendDraggingTile();
    },

    /**
     * Starts a timer to activate the switcher. The timer repeats until
     * cancelled by cancelDelayedSwitch_.
     * @private
     */
    scheduleDelayedSwitch_: function(e) {
      // Stop switching when the next page can't be dropped onto.
      var nextPage = ntp.getCardSlider().getCardAtIndex(this.nextCardIndex_());
      if (!nextPage.shouldAcceptDrag(e))
        return;

      var self = this;
      function navPageClearTimeout() {
        self.activate_();
        self.dragNavTimeout_ = null;
        self.scheduleDelayedSwitch_(e);
      }
      this.dragNavTimeout_ = window.setTimeout(navPageClearTimeout, 500);
    },

    /**
     * Cancels the timer that activates the switcher while dragging.
     * @private
     */
    cancelDelayedSwitch_: function() {
      if (this.dragNavTimeout_) {
        window.clearTimeout(this.dragNavTimeout_);
        this.dragNavTimeout_ = null;
      }
    },

  };

  /** @const */
  var initializePageSwitcher = PageSwitcher.prototype.decorate;

  return {
    initializePageSwitcher: initializePageSwitcher,
    PageSwitcher: PageSwitcher
  };
});
