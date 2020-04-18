// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

// Allow a function to be provided by tests, which will be called when
// the page has been populated with site engagement details.
var whenPageIsPopulatedForTest;

/** @type {function()} */
var disableAutoupdateForTests;

/** @type {mojom.SiteEngagementDetailsProviderPtr} */
var uiHandler;

(function() {
var resolvePageIsPopulated = null;
var pageIsPopulatedPromise = new Promise((resolve, reject) => {
  resolvePageIsPopulated = resolve;
});

whenPageIsPopulatedForTest = function() {
  return pageIsPopulatedPromise;
};

function initialize() {
  uiHandler = new mojom.SiteEngagementDetailsProviderPtr;
  Mojo.bindInterface(
      mojom.SiteEngagementDetailsProvider.name,
      mojo.makeRequest(uiHandler).handle);

  /** @type {?HTMLElement} */
  var engagementTableBody = $('engagement-table-body');
  /** @type {?number} */
  var updateInterval = null;
  /** @type {?Array<!mojom.SiteEngagementDetails>} */
  var info = null;
  /** @type {string} */
  var sortKey = 'totalScore';
  /** @type {boolean} */
  var sortReverse = true;

  // Set table header sort handlers.
  var engagementTableHeader = $('engagement-table-header');
  var headers = engagementTableHeader.children;
  for (var i = 0; i < headers.length; i++) {
    headers[i].addEventListener('click', (e) => {
      var newSortKey = e.target.getAttribute('sort-key');
      if (sortKey == newSortKey) {
        sortReverse = !sortReverse;
      } else {
        sortKey = newSortKey;
        sortReverse = false;
      }
      var oldSortColumn = document.querySelector('.sort-column');
      oldSortColumn.classList.remove('sort-column');
      e.target.classList.add('sort-column');
      if (sortReverse)
        e.target.setAttribute('sort-reverse', '');
      else
        e.target.removeAttribute('sort-reverse');
      renderTable();
    });
  }

  /**
   * Creates a single row in the engagement table.
   * @param {mojom.SiteEngagementDetails} info The info to create the row from.
   * @return {HTMLElement}
   */
  function createRow(info) {
    var originCell = createElementWithClassName('td', 'origin-cell');
    originCell.textContent = info.origin.url;

    var baseScoreInput =
        createElementWithClassName('input', 'base-score-input');
    baseScoreInput.addEventListener(
        'change', handleBaseScoreChange.bind(undefined, info.origin));
    baseScoreInput.addEventListener('focus', disableAutoupdate);
    baseScoreInput.addEventListener('blur', enableAutoupdate);
    baseScoreInput.value = info.baseScore;

    var baseScoreCell = createElementWithClassName('td', 'base-score-cell');
    baseScoreCell.appendChild(baseScoreInput);

    var bonusScoreCell = createElementWithClassName('td', 'bonus-score-cell');
    bonusScoreCell.textContent = info.installedBonus;

    var totalScoreCell = createElementWithClassName('td', 'total-score-cell');
    totalScoreCell.textContent = info.totalScore;

    var engagementBar = createElementWithClassName('div', 'engagement-bar');
    engagementBar.style.width = (info.totalScore * 4) + 'px';

    var engagementBarCell =
        createElementWithClassName('td', 'engagement-bar-cell');
    engagementBarCell.appendChild(engagementBar);

    var row = /** @type {HTMLElement} */ (document.createElement('tr'));
    row.appendChild(originCell);
    row.appendChild(baseScoreCell);
    row.appendChild(bonusScoreCell);
    row.appendChild(totalScoreCell);
    row.appendChild(engagementBarCell);

    // Stores correspondent engagementBarCell to change it's length on
    // scoreChange event.
    baseScoreInput.barCellRef = engagementBar;
    return row;
  }

  function disableAutoupdate() {
    if (updateInterval)
      clearInterval(updateInterval);
    updateInterval = null;
  }
  disableAutoupdateForTests = disableAutoupdate;

  function enableAutoupdate() {
    if (updateInterval)
      clearInterval(updateInterval);
    updateInterval = setInterval(updateEngagementTable, 5000);
  }

  /**
   * Sets the base engagement score when a score input is changed.
   * Resets the length of engagement-bar-cell to match the new score.
   * Also resets the update interval.
   * @param {!url.mojom.Url} origin The origin of the engagement score to set.
   * @param {Event} e
   */
  function handleBaseScoreChange(origin, e) {
    var baseScoreInput = e.target;
    uiHandler.setSiteEngagementBaseScoreForUrl(origin, baseScoreInput.value);
    baseScoreInput.barCellRef.style.width = (baseScoreInput.value * 4) + 'px';
    baseScoreInput.blur();
    enableAutoupdate();
  }

  /**
   * Remove all rows from the engagement table.
   */
  function clearTable() {
    engagementTableBody.innerHTML = '';
  }

  /**
   * Sort the engagement info based on |sortKey| and |sortReverse|.
   */
  function sortInfo() {
    info.sort((a, b) => {
      return (sortReverse ? -1 : 1) * compareTableItem(sortKey, a, b);
    });
  }

  /**
   * Compares two SiteEngagementDetails objects based on |sortKey|.
   * @param {string} sortKey The name of the property to sort by.
   * @return {number} A negative number if |a| should be ordered before |b|, a
   * positive number otherwise.
   */
  function compareTableItem(sortKey, a, b) {
    var val1 = a[sortKey];
    var val2 = b[sortKey];

    // Compare the hosts of the origin ignoring schemes.
    if (sortKey == 'origin')
      return new URL(val1.url).host > new URL(val2.url).host ? 1 : -1;

    if (sortKey == 'baseScore' || sortKey == 'bonusScore' ||
        sortKey == 'totalScore') {
      return val1 - val2;
    }

    assertNotReached('Unsupported sort key: ' + sortKey);
    return 0;
  }

  /**
   * Rounds the supplied value to two decimal places of accuracy.
   * @param {number} score
   * @return {number}
   */
  function roundScore(score) {
    return Number(Math.round(score * 100) / 100);
  }

  /**
   * Regenerates the engagement table from |info|.
   */
  function renderTable() {
    clearTable();
    sortInfo();

    info.forEach((info) => {
      // Round all scores to 2 decimal places.
      info.baseScore = roundScore(info.baseScore);
      info.installedBonus = roundScore(info.installedBonus);
      info.totalScore = roundScore(info.totalScore);

      engagementTableBody.appendChild(createRow(info));
    });
  }

  /**
   * Retrieve site engagement info and render the engagement table.
   */
  function updateEngagementTable() {
    // Populate engagement table.
    uiHandler.getSiteEngagementDetails().then((response) => {
      info = response.info;
      renderTable();
      resolvePageIsPopulated();
    });
  }

  updateEngagementTable();
  enableAutoupdate();
}

document.addEventListener('DOMContentLoaded', initialize);
})();
