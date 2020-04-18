// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {string} toTest The string to be tested.
 * @return {boolean} True if |toTest| contains only digits. Leading and trailing
 *     whitespace is allowed.
 */
function isInteger(toTest) {
  const numericExp = /^\s*[0-9]+\s*$/;
  return numericExp.test(toTest);
}

/**
 * Returns true if |value| is a valid non zero positive integer.
 * @param {string} value The string to be tested.
 * @return {boolean} true if the |value| is valid non zero positive integer.
 */
function isPositiveInteger(value) {
  return isInteger(value) && parseInt(value, 10) > 0;
}

/**
 * Returns true if the contents of the two arrays are equal.
 * @param {Array<{from: number, to: number}>} array1 The first array.
 * @param {Array<{from: number, to: number}>} array2 The second array.
 * @return {boolean} true if the arrays are equal.
 */
function areArraysEqual(array1, array2) {
  if (array1.length != array2.length)
    return false;
  for (let i = 0; i < array1.length; i++)
    if (array1[i] !== array2[i])
      return false;
  return true;
}

/**
 * Returns true if the contents of the two page ranges are equal.
 * @param {Array} array1 The first array.
 * @param {Array} array2 The second array.
 * @return {boolean} true if the arrays are equal.
 */
function areRangesEqual(array1, array2) {
  if (array1.length != array2.length)
    return false;
  for (let i = 0; i < array1.length; i++)
    if (array1[i].from != array2[i].from || array1[i].to != array2[i].to) {
      return false;
    }
  return true;
}

/**
 * Removes duplicate elements from |inArray| and returns a new array.
 * |inArray| is not affected. It assumes that |inArray| is already sorted.
 * @param {!Array<number>} inArray The array to be processed.
 * @return {!Array<number>} The array after processing.
 */
function removeDuplicates(inArray) {
  const out = [];

  if (inArray.length == 0)
    return out;

  out.push(inArray[0]);
  for (let i = 1; i < inArray.length; ++i)
    if (inArray[i] != inArray[i - 1])
      out.push(inArray[i]);
  return out;
}

/** @enum {number} */
const PageRangeStatus = {
  NO_ERROR: 0,
  SYNTAX_ERROR: -1,
  LIMIT_ERROR: -2
};

/**
 * Returns a list of ranges in |pageRangeText|. The ranges are
 * listed in the order they appear in |pageRangeText| and duplicates are not
 * eliminated. If |pageRangeText| is not valid, PageRangeStatus.SYNTAX_ERROR
 * is returned.
 * A valid selection has a parsable format and every page identifier is
 * greater than 0 unless wildcards are used(see examples).
 * If a page is greater than |totalPageCount|, PageRangeStatus.LIMIT_ERROR
 * is returned.
 * If |totalPageCount| is 0 or undefined function uses impossibly large number
 * instead.
 * Wildcard the first number must be larger than 0 and less or equal then
 * |totalPageCount|. If it's missed then 1 is used as the first number.
 * Wildcard the second number must be larger then the first number. If it's
 * missed then |totalPageCount| is used as the second number.
 * Example: "1-4, 9, 3-6, 10, 11" is valid, assuming |totalPageCount| >= 11.
 * Example: "1-4, -6" is valid, assuming |totalPageCount| >= 6.
 * Example: "2-" is valid, assuming |totalPageCount| >= 2, means from 2 to the
 *          end.
 * Example: "4-2, 11, -6" is invalid.
 * Example: "-" is valid, assuming |totalPageCount| >= 1.
 * Example: "1-4dsf, 11" is invalid regardless of |totalPageCount|.
 * @param {string} pageRangeText The text to be checked.
 * @param {number=} opt_totalPageCount The total number of pages.
 * @return {!PageRangeStatus|!Array<{from: number, to: number}>}
 */
function pageRangeTextToPageRanges(pageRangeText, opt_totalPageCount) {
  if (pageRangeText == '') {
    return [];
  }

  const MAX_PAGE_NUMBER = 1000000000;
  const totalPageCount =
      opt_totalPageCount ? opt_totalPageCount : MAX_PAGE_NUMBER;

  const regex = /^\s*([0-9]*)\s*-\s*([0-9]*)\s*$/;
  const parts = pageRangeText.split(/,|\u3001/);

  const pageRanges = [];
  for (let i = 0; i < parts.length; ++i) {
    const match = parts[i].match(regex);
    if (match) {
      if (!isPositiveInteger(match[1]) && match[1] !== '')
        return PageRangeStatus.SYNTAX_ERROR;
      if (!isPositiveInteger(match[2]) && match[2] !== '')
        return PageRangeStatus.SYNTAX_ERROR;
      const from = match[1] ? parseInt(match[1], 10) : 1;
      const to = match[2] ? parseInt(match[2], 10) : totalPageCount;
      if (from > to)
        return PageRangeStatus.SYNTAX_ERROR;
      if (to > totalPageCount)
        return PageRangeStatus.LIMIT_ERROR;
      pageRanges.push({'from': from, 'to': to});
    } else {
      if (!isPositiveInteger(parts[i]))
        return PageRangeStatus.SYNTAX_ERROR;
      const singlePageNumber = parseInt(parts[i], 10);
      if (singlePageNumber > totalPageCount)
        return PageRangeStatus.LIMIT_ERROR;
      pageRanges.push({'from': singlePageNumber, 'to': singlePageNumber});
    }
  }
  return pageRanges;
}

/**
 * Returns a list of pages defined by |pagesRangeText|. The pages are
 * listed in the order they appear in |pageRangeText| and duplicates are not
 * eliminated. If |pageRangeText| is not valid according or
 * |totalPageCount| undefined [1,2,...,totalPageCount] is returned.
 * See pageRangeTextToPageRanges for details.
 * @param {string} pageRangeText The text to be checked.
 * @param {number} totalPageCount The total number of pages.
 * @return {!Array<number>} A list of all pages.
 */
function pageRangeTextToPageList(pageRangeText, totalPageCount) {
  const pageRanges = pageRangeTextToPageRanges(pageRangeText, totalPageCount);
  const pageList = [];
  if (Array.isArray(pageRanges)) {
    for (let i = 0; i < pageRanges.length; ++i) {
      for (let j = pageRanges[i].from;
           j <= Math.min(pageRanges[i].to, totalPageCount); ++j) {
        pageList.push(j);
      }
    }
  }
  if (pageList.length == 0) {
    for (let j = 1; j <= totalPageCount; ++j)
      pageList.push(j);
  }
  return pageList;
}

/**
 * @param {!Array<number>} pageList The list to be processed.
 * @return {!Array<number>} The contents of |pageList| in ascending order and
 *     without any duplicates. |pageList| is not affected.
 */
function pageListToPageSet(pageList) {
  let pageSet = [];
  if (pageList.length == 0)
    return pageSet;
  pageSet = pageList.slice(0);
  pageSet.sort(function(a, b) {
    return /** @type {number} */ (a) - /** @type {number} */ (b);
  });
  pageSet = removeDuplicates(pageSet);
  return pageSet;
}

/**
 * @param {!HTMLElement} element Element to check for visibility.
 * @return {boolean} Whether the given element is visible.
 */
function getIsVisible(element) {
  return !element.hidden;
}

/**
 * Shows or hides an element.
 * @param {!HTMLElement} element Element to show or hide.
 * @param {boolean} isVisible Whether the element should be visible or not.
 */
function setIsVisible(element, isVisible) {
  element.hidden = !isVisible;
}

/**
 * @param {!Array} array Array to check for item.
 * @param {*} item Item to look for in array.
 * @return {boolean} Whether the item is in the array.
 */
function arrayContains(array, item) {
  return array.indexOf(item) != -1;
}

/**
 * @param {!Array<!{locale: string, value: string}>} localizedStrings An array
 *     of strings with corresponding locales.
 * @param {string} locale Locale to look the string up for.
 * @return {string} A string for the requested {@code locale}. An empty string
 *     if there's no string for the specified locale found.
 */
function getStringForLocale(localizedStrings, locale) {
  locale = locale.toLowerCase();
  for (let i = 0; i < localizedStrings.length; i++) {
    if (localizedStrings[i].locale.toLowerCase() == locale)
      return localizedStrings[i].value;
  }
  return '';
}

/**
 * @param {!Array<!{locale: string, value: string}>} localizedStrings An array
 *     of strings with corresponding locales.
 * @return {string} A string for the current locale. An empty string if there's
 *     no string for the current locale found.
 */
function getStringForCurrentLocale(localizedStrings) {
  // First try to find an exact match and then look for the language only.
  return getStringForLocale(localizedStrings, navigator.language) ||
      getStringForLocale(localizedStrings, navigator.language.split('-')[0]);
}
