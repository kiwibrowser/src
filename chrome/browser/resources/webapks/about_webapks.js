// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   name: string,
 *   shortName: string,
 *   packageName: string,
 *   shellApkVersion: number,
 *   versionCode: number,
 *   uri: string,
 *   scope: string,
 *   manifestUrl: string,
 *   manifestStartUrl: string,
 *   displayMode: string,
 *   orientation: string,
 *   themeColor: string,
 *   backgroundColor: string,
 *   lastUpdateCheckTimeMs: number,
 *   relaxUpdates: boolean,
 * }}
 */
var WebApkInfo;

/**
 * Creates and returns an element (with |text| as content) assigning it the
 * |className| class.
 *
 * @param {string} text Text to be shown in the span.
 * @param {string} type Type of element to be added such as 'div'.
 * @param {string} className Class to be assigned to the new element.
 * @return {Element} The created element.
 */
function createElementWithTextAndClass(text, type, className) {
  var element = createElementWithClassName(type, className);
  element.textContent = text;
  return element;
}

/**
 * Callback from the backend with the information of a WebAPK to display.
 * This will be called once for each WebAPK available on the device and each
 * one will be appended at the end of the other.
 *
 * @param {!Array<WebApkInfo>} webApkList List of objects with information about
 * WebAPKs installed.
 */
function returnWebApksInfo(webApkList) {
  for (let webApkInfo of webApkList) {
    addWebApk(webApkInfo);
  }
}

/**
 * @param {HTMLElement} webApkList List of elements which contain WebAPK
 * attributes.
 * @param {string} label Text that identifies the new element.
 * @param {string} value Text to set in the new element.
 */
function addWebApkField(webApkList, label, value) {
  var divElement =
      createElementWithTextAndClass(label, 'div', 'app-property-label');
  divElement.appendChild(
      createElementWithTextAndClass(value, 'span', 'app-property-value'));
  webApkList.appendChild(divElement);
}

/**
 * Adds a new entry to the page with the information of a WebAPK.
 *
 * @param {WebApkInfo} webApkInfo Information about an installed WebAPK.
 */
function addWebApk(webApkInfo) {
  /** @type {HTMLElement} */ var webApkList = $('webapk-list');

  webApkList.appendChild(
      createElementWithTextAndClass(webApkInfo.name, 'span', 'app-name'));

  webApkList.appendChild(createElementWithTextAndClass(
      'Short name: ', 'span', 'app-property-label'));
  webApkList.appendChild(document.createTextNode(webApkInfo.shortName));

  addWebApkField(webApkList, 'Package name: ', webApkInfo.packageName);
  addWebApkField(
      webApkList, 'Shell APK version: ', '' + webApkInfo.shellApkVersion);
  addWebApkField(webApkList, 'Version code: ', '' + webApkInfo.versionCode);
  addWebApkField(webApkList, 'URI: ', webApkInfo.uri);
  addWebApkField(webApkList, 'Scope: ', webApkInfo.scope);
  addWebApkField(webApkList, 'Manifest URL: ', webApkInfo.manifestUrl);
  addWebApkField(
      webApkList, 'Manifest Start URL: ', webApkInfo.manifestStartUrl);
  addWebApkField(webApkList, 'Display Mode: ', webApkInfo.displayMode);
  addWebApkField(webApkList, 'Orientation: ', webApkInfo.orientation);
  addWebApkField(webApkList, 'Theme color: ', webApkInfo.themeColor);
  addWebApkField(webApkList, 'Background color: ', webApkInfo.backgroundColor);
  addWebApkField(
      webApkList, 'Last Update Check Time: ',
      new Date(webApkInfo.lastUpdateCheckTimeMs).toString());
  addWebApkField(
      webApkList, 'Check for Updates Less Frequently: ',
      webApkInfo.relaxUpdates.toString());
}

document.addEventListener('DOMContentLoaded', function() {
  chrome.send('requestWebApksInfo');
});
