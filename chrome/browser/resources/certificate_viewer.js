// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cert_viewer', function() {
  'use strict';

  /**
   * Initialize the certificate viewer dialog by wiring up the close button,
   * substituting in translated strings and requesting certificate details.
   */
  function initialize() {
    cr.ui.decorate('tabbox', cr.ui.TabBox);

    var args = JSON.parse(chrome.getVariableValue('dialogArguments'));
    getCertificateInfo(args);

    /**
     * Initialize the second tab's contents.
     * This is a 'oneShot' function, meaning it will only be invoked once,
     * no matter how many times it is called.  This is done for unit-testing
     * purposes in case a test needs to initialize the tab before the timer
     * fires.
     */
    var initializeDetailTab = oneShot(function() {
      initializeTree($('hierarchy'), showCertificateFields);
      initializeTree($('cert-fields'), showCertificateFieldValue);
      createCertificateHierarchy(args.hierarchy);
    });

    // The second tab's contents aren't visible on startup, so we can
    // shorten startup by not populating those controls until after we
    // have had a chance to draw the visible controls the first time.
    // The value of 200ms is quick enough that the user couldn't open the
    // tab in that time but long enough to allow the first tab to draw on
    // even the slowest machine.
    setTimeout(initializeDetailTab, 200);

    $('tabbox').addEventListener('selectedChange', function f(e) {
      $('tabbox').removeEventListener('selectedChange', f);
      initializeDetailTab();
    }, true);

    stripGtkAccessorKeys();

    $('export').onclick = exportCertificate;
  }

  /**
   * Decorate a function so that it can only be invoked once.
   */
  function oneShot(fn) {
    var fired = false;
    return function() {
      if (fired)
        return;
      fired = true;
      fn();
    };
  }

  /**
   * Initialize a cr.ui.Tree object from a given element using the specified
   * change handler.
   * @param {HTMLElement} tree The HTMLElement to style as a tree.
   * @param {function()} handler Function to call when a node is selected.
   */
  function initializeTree(tree, handler) {
    cr.ui.decorate(tree, cr.ui.Tree);
    tree.detail = {payload: {}, children: {}};
    tree.addEventListener('change', handler);
  }

  /**
   * The tab name strings in the languages file have accessor keys indicated
   * by a preceding & sign. Strip these out for now.
   * TODO(flackr) These accessor keys could be implemented with Javascript or
   *     translated strings could be added / modified to remove the & sign.
   */
  function stripGtkAccessorKeys() {
    // Copy all the tab labels into an array.
    var nodes = Array.prototype.slice.call($('tabs').childNodes, 0);
    nodes.push($('export'));
    for (var i = 0; i < nodes.length; i++)
      nodes[i].textContent = nodes[i].textContent.replace('&', '');
  }

  /**
   * Expand all nodes of the given tree object.
   * @param {cr.ui.Tree} tree The tree object to expand all nodes on.
   */
  function revealTree(tree) {
    tree.expanded = true;
    for (var key in tree.detail.children) {
      revealTree(tree.detail.children[key]);
    }
  }

  /**
   * This function is called from certificate_viewer_ui.cc with the certificate
   * information. Display all returned information to the user.
   * @param {Object} certInfo Certificate information in named fields.
   */
  function getCertificateInfo(certInfo) {
    for (var key in certInfo.general) {
      $(key).textContent = certInfo.general[key];
    }
  }

  /**
   * This function populates the certificate hierarchy.
   * @param {Object} hierarchy A dictionary containing the hierarchy.
   */
  function createCertificateHierarchy(hierarchy) {
    var treeItem = $('hierarchy');
    var root = constructTree(hierarchy[0]);
    treeItem.detail.children['root'] = root;
    treeItem.add(root);

    // Select the last item in the hierarchy (really we have a list here - each
    // node has at most one child).  This will reveal the parent nodes and
    // populate the fields view.
    var last = root;
    while (last.detail.children && last.detail.children[0])
      last = last.detail.children[0];
    last.selected = true;
  }

  /**
   * Constructs a cr.ui.TreeItem corresponding to the passed in tree
   * @param {Object} tree Dictionary describing the tree structure.
   * @return {cr.ui.TreeItem} Tree node corresponding to the input dictionary.
   */
  function constructTree(tree) {
    var treeItem = new cr.ui.TreeItem({
      label: tree.label,
      detail: {payload: tree.payload ? tree.payload : {}, children: {}}
    });
    if (tree.children) {
      for (var i = 0; i < tree.children.length; i++) {
        treeItem.add(
            treeItem.detail.children[i] = constructTree(tree.children[i]));
      }
    }
    return treeItem;
  }

  /**
   * Clear any previous certificate fields in the tree.
   */
  function clearCertificateFields() {
    var treeItem = $('cert-fields');
    for (var key in treeItem.detail.children) {
      treeItem.remove(treeItem.detail.children[key]);
      delete treeItem.detail.children[key];
    }
  }

  /**
   * Request certificate fields for the selected certificate in the hierarchy.
   */
  function showCertificateFields() {
    clearCertificateFields();
    var item = $('hierarchy').selectedItem;
    if (item && item.detail.payload.index !== undefined)
      chrome.send('requestCertificateFields', [item.detail.payload.index]);
  }

  /**
   * Show the returned certificate fields for the selected certificate.
   * @param {Object} certFields A dictionary containing the fields tree
   *     structure.
   */
  function getCertificateFields(certFields) {
    clearCertificateFields();
    var treeItem = $('cert-fields');
    treeItem.add(
        treeItem.detail.children['root'] = constructTree(certFields[0]));
    revealTree(treeItem);
    // Ensure the list is scrolled to the top by selecting the first item.
    treeItem.children[0].selected = true;
  }

  /**
   * Show certificate field value for a selected certificate field.
   */
  function showCertificateFieldValue() {
    var item = $('cert-fields').selectedItem;
    if (item && item.detail.payload.val)
      $('cert-field-value').textContent = item.detail.payload.val;
    else
      $('cert-field-value').textContent = '';
  }

  /**
   * Export the selected certificate.
   */
  function exportCertificate() {
    var item = $('hierarchy').selectedItem;
    if (item && item.detail.payload.index !== undefined)
      chrome.send('exportCertificate', [item.detail.payload.index]);
  }

  return {
    initialize: initialize,
    getCertificateFields: getCertificateFields,
  };
});

document.addEventListener('DOMContentLoaded', cert_viewer.initialize);
