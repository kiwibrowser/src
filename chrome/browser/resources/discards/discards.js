// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('discards', function() {
  'use strict';

  // The following variables are initialized by 'initialize'.
  // Points to the Mojo WebUI handler.
  let uiHandler;
  // After initialization this points to the discard info table body.
  let tabDiscardsInfoTableBody;
  // This holds the sorted tab discard infos as retrieved from the uiHandler.
  let infos;
  // Holds information about the current sorting of the table.
  let sortKey;
  let sortReverse;
  // Points to the timer that refreshes the table content.
  let updateTimer;

  // Specifies the update interval of the page, in ms.
  const UPDATE_INTERVAL_MS = 1000;

  /**
   * Ensures the discards info table has the appropriate length. Decorates
   * newly created rows with a 'row-index' attribute to enable event listeners
   * to quickly determine the index of the row.
   */
  function ensureTabDiscardsInfoTableLength() {
    let rows = tabDiscardsInfoTableBody.querySelectorAll('tr');
    if (rows.length < infos.length) {
      for (let i = rows.length; i < infos.length; ++i) {
        let row = createEmptyTabDiscardsInfoTableRow();
        row.setAttribute('data-row-index', i.toString());
        tabDiscardsInfoTableBody.appendChild(row);
      }
    } else if (rows.length > infos.length) {
      for (let i = infos.length; i < rows.length; ++i) {
        tabDiscardsInfoTableBody.removeChild(rows[i]);
      }
    }
  }

  /**
   * Compares two TabDiscardsInfos based on the data in the provided sort-key.
   * @param {string} sortKey The key of the sort. See the "data-sort-key"
   *     attribute of the table headers for valid sort-keys.
   * @param {boolean|number|string} a The first value being compared.
   * @param {boolean|number|string} b The second value being compared.
   * @return {number} A negative number if a < b, 0 if a == b, and a positive
   *     number if a > b.
   */
  function compareTabDiscardsInfos(sortKey, a, b) {
    let val1 = a[sortKey];
    let val2 = b[sortKey];

    // Compares strings.
    if (sortKey == 'title' || sortKey == 'tabUrl') {
      val1 = val1.toLowerCase();
      val2 = val2.toLowerCase();
      if (val1 == val2)
        return 0;
      return val1 > val2 ? 1 : -1;
    }

    // Compares boolean fields.
    if (['isMedia', 'isFrozen', 'isDiscarded', 'isAutoDiscardable'].includes(
            sortKey)) {
      if (val1 == val2)
        return 0;
      return val1 ? 1 : -1;
    }

    // Compares numeric fields.
    // Note: Visibility is represented as a numeric value.
    if ([
          'visibility',
          'discardCount',
          'utilityRank',
          'reactivationScore',
          'lastActiveSeconds',
        ].includes(sortKey)) {
      return val1 - val2;
    }

    assertNotReached('Unsupported sort key: ' + sortKey);
    return 0;
  }

  /**
   * Sorts the tab discards info data in |infos| according to the current
   * |sortKey|.
   */
  function sortTabDiscardsInfoTable() {
    infos = infos.sort((a, b) => {
      return (sortReverse ? -1 : 1) * compareTabDiscardsInfos(sortKey, a, b);
    });
  }

  /**
   * Pluralizes a string according to the given count. Assumes that appending an
   * 's' is sufficient to make a string plural.
   * @param {string} s The string to be made plural if necessary.
   * @param {number} n The count of the number of ojects.
   * @return {string} The plural version of |s| if n != 1, otherwise |s|.
   */
  function maybeMakePlural(s, n) {
    return n == 1 ? s : s + 's';
  }

  /**
   * Converts a |secondsAgo| last-active time to a user friendly string.
   * @param {number} secondsAgo The amount of time since the tab was active.
   * @return {string} An English string representing the last active time.
   */
  function lastActiveToString(secondsAgo) {
    // These constants aren't perfect, but close enough.
    const SECONDS_PER_MINUTE = 60;
    const MINUTES_PER_HOUR = 60;
    const SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;
    const HOURS_PER_DAY = 24;
    const SECONDS_PER_DAY = SECONDS_PER_HOUR * HOURS_PER_DAY;
    const DAYS_PER_WEEK = 7;
    const SECONDS_PER_WEEK = SECONDS_PER_DAY * DAYS_PER_WEEK;
    const SECONDS_PER_MONTH = SECONDS_PER_DAY * 30.5;
    const SECONDS_PER_YEAR = SECONDS_PER_DAY * 365;

    // Seconds ago.
    if (secondsAgo < SECONDS_PER_MINUTE)
      return 'just now';

    // Minutes ago.
    let minutesAgo = Math.floor(secondsAgo / SECONDS_PER_MINUTE);
    if (minutesAgo < MINUTES_PER_HOUR) {
      return minutesAgo.toString() + maybeMakePlural(' minute', minutesAgo) +
          ' ago';
    }

    // Hours and minutes and ago.
    let hoursAgo = Math.floor(secondsAgo / SECONDS_PER_HOUR);
    minutesAgo = minutesAgo % MINUTES_PER_HOUR;
    if (hoursAgo < HOURS_PER_DAY) {
      let s = hoursAgo.toString() + maybeMakePlural(' hour', hoursAgo);
      if (minutesAgo > 0) {
        s += ' and ' + minutesAgo.toString() +
            maybeMakePlural(' minute', minutesAgo);
      }
      s += ' ago';
      return s;
    }

    // Days ago.
    let daysAgo = Math.floor(secondsAgo / SECONDS_PER_DAY);
    if (daysAgo < DAYS_PER_WEEK) {
      return daysAgo.toString() + maybeMakePlural(' day', daysAgo) + ' ago';
    }

    // Weeks ago. There's an awkward gap to bridge where 4 weeks can have
    // elapsed but not quite 1 month. Be sure to use weeks to report that.
    let weeksAgo = Math.floor(secondsAgo / SECONDS_PER_WEEK);
    let monthsAgo = Math.floor(secondsAgo / SECONDS_PER_MONTH);
    if (monthsAgo < 1) {
      return 'over ' + weeksAgo.toString() +
          maybeMakePlural(' week', weeksAgo) + ' ago';
    }

    // Months ago.
    let yearsAgo = Math.floor(secondsAgo / SECONDS_PER_YEAR);
    if (yearsAgo < 1) {
      return 'over ' + monthsAgo.toString() +
          maybeMakePlural(' month', monthsAgo) + ' ago';
    }

    // Years ago.
    return 'over ' + yearsAgo.toString() + maybeMakePlural(' year', yearsAgo) +
        ' ago';
  }

  /**
   * Returns a string representation of a boolean value for display in a table.
   * @param {boolean} bool A boolean value.
   * @return {string} A string representing the bool.
   */
  function boolToString(bool) {
    return bool ? '✔' : '\xa0';
  }

  /**
   * Returns a string representation of a visibility enum value for display in
   * a table.
   * @param {int} visibility A value in LifecycleUnitVisibility.
   * @return {string} A string representation of the visibility.
   */
  function visibilityToString(visibility) {
    switch (visibility) {
      case 0:
        return 'hidden';
      case 1:
        return 'occluded';
      case 2:
        return 'visible';
    }
    assertNotReached('Unsupported visibility: ' + visibility);
  }

  /**
   * Returns the index of the row in the table that houses the given |element|.
   * @param {HTMLElement} element Any element in the DOM.
   */
  function getRowIndex(element) {
    let row = element.closest('tr');
    return parseInt(row.getAttribute('data-row-index'), 10);
  }

  /**
   * Creates an empty tab discards table row with action-link listeners, etc.
   * By default the links are inactive.
   */
  function createEmptyTabDiscardsInfoTableRow() {
    let template = $('tab-discard-info-row');
    let content = document.importNode(template.content, true);
    let row = content.querySelector('tr');

    // Set up the listener for the auto-discardable toggle action.
    let isAutoDiscardable = row.querySelector('.is-auto-discardable-link');
    isAutoDiscardable.setAttribute('disabled', '');
    isAutoDiscardable.addEventListener('click', (e) => {
      // Get the info backing this row.
      let info = infos[getRowIndex(e.target)];
      // Disable the action. The update function is responsible for
      // re-enabling actions if necessary.
      e.target.setAttribute('disabled', '');
      // Perform the action.
      uiHandler.setAutoDiscardable(info.id, !info.isAutoDiscardable)
          .then(stableUpdateTabDiscardsInfoTable());
    });

    // Set up the listeners for discard links.
    let discardListener = function(e) {
      // Get the info backing this row.
      let info = infos[getRowIndex(e.target)];
      // Determine whether this is urgent or not.
      let urgent = e.target.classList.contains('discard-urgent-link');
      // Disable the action. The update function is responsible for
      // re-enabling actions if necessary.
      e.target.setAttribute('disabled', '');
      // Perform the action.
      uiHandler.discardById(info.id, urgent).then((response) => {
        stableUpdateTabDiscardsInfoTable();
      });
    };
    let discardLink = row.querySelector('.discard-link');
    let discardUrgentLink = row.querySelector('.discard-urgent-link');
    discardLink.addEventListener('click', discardListener);
    discardUrgentLink.addEventListener('click', discardListener);


    // Set up the listeners for freeze links.
    let lifecycleListener = function(e) {
      // Get the info backing this row.
      let info = infos[getRowIndex(e.target)];
      // Perform the action.
      uiHandler.freezeById(info.id);
    };

    let freezeLink = row.querySelector('.freeze-link');
    freezeLink.addEventListener('click', lifecycleListener);

    return row;
  }

  /**
   * Updates a tab discards info table row in place. Sets/unsets 'disabled'
   * attributes on action-links as necessary, and populates all contents.
   */
  function updateTabDiscardsInfoTableRow(row, info) {
    // Update the content.
    row.querySelector('.utility-rank-cell').textContent =
        info.utilityRank.toString();
    row.querySelector('.reactivation-score-cell').textContent =
        info.hasReactivationScore ? info.reactivationScore.toFixed(4) : 'N/A';
    row.querySelector('.favicon-div').style.backgroundImage =
        cr.icon.getFavicon(info.tabUrl);
    row.querySelector('.title-div').textContent = info.title;
    row.querySelector('.tab-url-cell').textContent = info.tabUrl;
    row.querySelector('.visibility-cell').textContent =
        visibilityToString(info.visibility);
    row.querySelector('.is-media-cell').textContent =
        boolToString(info.isMedia);
    row.querySelector('.is-frozen-cell').textContent =
        boolToString(info.isFrozen);
    row.querySelector('.is-discarded-cell').textContent =
        boolToString(info.isDiscarded);
    row.querySelector('.discard-count-cell').textContent =
        info.discardCount.toString();
    row.querySelector('.is-auto-discardable-div').textContent =
        boolToString(info.isAutoDiscardable);
    row.querySelector('.last-active-cell').textContent =
        lastActiveToString(info.lastActiveSeconds);

    // Enable/disable action links as appropriate.
    row.querySelector('.is-auto-discardable-link').removeAttribute('disabled');
    let discardLink = row.querySelector('.discard-link');
    let discardUrgentLink = row.querySelector('.discard-urgent-link');
    if (info.isDiscarded) {
      discardLink.setAttribute('disabled', '');
      discardUrgentLink.setAttribute('disabled', '');
    } else {
      discardLink.removeAttribute('disabled');
      discardUrgentLink.removeAttribute('disabled');
    }

    let freezeLink = row.querySelector('.freeze-link');
    if (info.isFrozen)
      freezeLink.setAttribute('disabled', '');
    else
      freezeLink.removeAttribute('disabled', '');
  }

  /**
   * Causes the discards info table to be rendered. Reuses existing table rows
   * in place to minimize disruption to the page.
   */
  function renderTabDiscardsInfoTable() {
    ensureTabDiscardsInfoTableLength();
    let rows = tabDiscardsInfoTableBody.querySelectorAll('tr');
    for (let i = 0; i < infos.length; ++i)
      updateTabDiscardsInfoTableRow(rows[i], infos[i]);
  }

  /**
   * Causes the discard info table to be updated in as stable a manner as
   * possible. That is, rows will stay in their relative positions, even if the
   * current sort order is violated. Only the addition or removal of rows (tabs)
   * can cause the layout to change.
   */
  function stableUpdateTabDiscardsInfoTableImpl() {
    uiHandler.getTabDiscardsInfo().then((response) => {
      let newInfos = response.infos;
      let stableInfos = [];

      // Update existing infos in place, remove old ones, and append new ones.
      // This tries to keep the existing ordering stable so that clicking links
      // is minimally disruptive.
      for (let i = 0; i < infos.length; ++i) {
        let oldInfo = infos[i];
        let newInfo = null;
        for (let j = 0; j < newInfos.length; ++j) {
          if (newInfos[j].id == oldInfo.id) {
            newInfo = newInfos[j];
            break;
          }
        }

        // Old infos that have corresponding new infos are pushed first, in the
        // current order of the old infos.
        if (newInfo != null)
          stableInfos.push(newInfo);
      }

      // Make sure info about new tabs is appended to the end, in the order they
      // were originally returned.
      for (let i = 0; i < newInfos.length; ++i) {
        let newInfo = newInfos[i];
        let oldInfo = null;
        for (let j = 0; j < infos.length; ++j) {
          if (infos[j].id == newInfo.id) {
            oldInfo = infos[j];
            break;
          }
        }

        // Entirely new information (has no corresponding old info) is appended
        // to the end.
        if (oldInfo == null)
          stableInfos.push(newInfo);
      }

      // Swap out the current info with the new stably sorted information.
      infos = stableInfos;

      // Render the content in place.
      renderTabDiscardsInfoTable();
    });
  }

  /**
   * A wrapper to stableUpdateTabDiscardsInfoTableImpl that is called due to
   * user action and not due to the automatic timer. Cancels the existing timer
   * and reschedules it after rendering instantaneously.
   */
  function stableUpdateTabDiscardsInfoTable() {
    if (updateTimer)
      clearInterval(updateTimer);
    stableUpdateTabDiscardsInfoTableImpl();
    updateTimer =
        setInterval(stableUpdateTabDiscardsInfoTableImpl, UPDATE_INTERVAL_MS);
  }

  /**
   * Initializes this page. Invoked by the DOMContentLoaded event.
   */
  function initialize() {
    uiHandler = new mojom.DiscardsDetailsProviderPtr;
    Mojo.bindInterface(
        mojom.DiscardsDetailsProvider.name, mojo.makeRequest(uiHandler).handle);

    tabDiscardsInfoTableBody = $('tab-discards-info-table-body');
    infos = [];
    sortKey = 'utilityRank';
    sortReverse = false;
    updateTimer = null;

    // Set the column sort handlers.
    let tabDiscardsInfoTableHeader = $('tab-discards-info-table-header');
    let headers = tabDiscardsInfoTableHeader.children;
    for (let header of headers) {
      header.addEventListener('click', (e) => {
        let newSortKey = e.target.dataset.sortKey;

        // Skip columns that aren't explicitly labeled with a sort-key
        // attribute.
        if (newSortKey == null)
          return;

        // Reverse the sort key if the key itself hasn't changed.
        if (sortKey == newSortKey) {
          sortReverse = !sortReverse;
        } else {
          sortKey = newSortKey;
          sortReverse = false;
        }

        // Undecorate the old sort column, and decorate the new one.
        let oldSortColumn = document.querySelector('.sort-column');
        oldSortColumn.classList.remove('sort-column');
        e.target.classList.add('sort-column');
        if (sortReverse)
          e.target.setAttribute('data-sort-reverse', '');
        else
          e.target.removeAttribute('data-sort-reverse');

        sortTabDiscardsInfoTable();
        renderTabDiscardsInfoTable();
      });
    }

    // Setup the "Discard a tab now" links.
    let discardNow = $('discard-now-link');
    let discardNowUrgent = $('discard-now-urgent-link');
    let discardListener = function(e) {
      e.target.setAttribute('disabled', '');
      let urgent = e.target.id.includes('urgent');
      uiHandler.discard(urgent).then(() => {
        stableUpdateTabDiscardsInfoTable();
        e.target.removeAttribute('disabled');
      });
    };
    discardNow.addEventListener('click', discardListener);
    discardNowUrgent.addEventListener('click', discardListener);

    stableUpdateTabDiscardsInfoTable();
  }

  document.addEventListener('DOMContentLoaded', initialize);

  // These functions are exposed on the 'discards' object created by
  // cr.define. This allows unittesting of these functions.
  return {
    compareTabDiscardsInfos: compareTabDiscardsInfos,
    lastActiveToString: lastActiveToString,
    maybeMakePlural: maybeMakePlural
  };
});
