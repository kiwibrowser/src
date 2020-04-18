// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Test fixture for generated tests.
 * @extends {testing.Test}
 */
function CertificateViewerUITest() {}

CertificateViewerUITest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Define the C++ fixture class and include it.
   * @type {?string}
   * @override
   */
  typedefCppFixture: 'CertificateViewerUITest',

  /**
   * Show the certificate viewer dialog.
   */
  testGenPreamble: function() {
    GEN('ShowCertificateViewer();');
  },

  /**
   * Tests that the dialog opened to the correct URL.
   */
  testDialogUrl: function() {
    assertEquals(chrome.getVariableValue('expectedUrl'), window.location.href);
  },

  /**
   * Tests for the correct common name in the test certificate.
   */
  testCN: function() {
    assertEquals('www.google.com', $('issued-cn').textContent);
  },

  testDetails: function() {
    var certHierarchy = $('hierarchy');
    var certFields = $('cert-fields');
    var certFieldVal = $('cert-field-value');

    // Select the second tab, causing its data to be loaded if needed.
    $('tabbox').selectedIndex = 1;

    // There must be at least one certificate in the hierarchy.
    assertLT(0, certHierarchy.childNodes.length);

    // Select the first certificate on the chain and ensure the details show up.
    // Override the receive certificate function to catch when fields are
    // loaded.
    var getCertificateFields = cert_viewer.getCertificateFields;
    cert_viewer.getCertificateFields =
        this.continueTest(WhenTestDone.ALWAYS, function(certFieldDetails) {
          getCertificateFields(certFieldDetails);
          cert_viewer.getCertificateFields = getCertificateFields;
          assertLT(0, certFields.childNodes.length);

          // Test that a field can be selected to see the details for that
          // field.
          var item = getElementWithValue(certFields);
          assertNotEquals(null, item);
          certFields.selectedItem = item;
          assertEquals(item.detail.payload.val, certFieldVal.textContent);

          // Test that selecting an item without a value empties the field.
          certFields.selectedItem = certFields.childNodes[0];
          assertEquals('', certFieldVal.textContent);
        });
    certHierarchy.selectedItem = certHierarchy.childNodes[0];
  }
};

/**
 * Test fixture for ChromeOS modal dialog version of certificate viewer.
 * @extends {CertificateViewerUITest}
 */
function CertificateViewerModalUITest() {}

CertificateViewerModalUITest.prototype = {
  __proto__: CertificateViewerUITest.prototype,

  /**
   * Show the certificate viewer dialog.
   */
  testGenPreamble: function() {
    GEN('#if defined(OS_CHROMEOS)');
    GEN('ShowModalCertificateViewer();');
    GEN('#endif');
  },
};

/**
 * Test fixture for asynchronous tests.
 * @extends {CertificateViewerUITest}
 */
function CertificateViewerUITestAsync() {}

CertificateViewerUITestAsync.prototype = {
  __proto__: CertificateViewerUITest.prototype,

  /** @inheritDoc */
  isAsync: true,
};


/**
 * Test fixture for ChromeOS modal dialog version for asynchronous tests.
 * @extends {CertificateViewerUITest}
 */
function CertificateViewerModalUITestAsync() {}

CertificateViewerModalUITestAsync.prototype = {
  __proto__: CertificateViewerModalUITest.prototype,

  /** @inheritDoc */
  isAsync: true,
};

// Include the bulk of c++ code.
// Certificate viewer UI tests are disabled on platforms with native certificate
// viewers.
GEN('#include "chrome/test/data/webui/certificate_viewer_ui_test-inl.h"');
GEN('');

// Constructors and destructors must be provided in .cc to prevent clang errors.
GEN('CertificateViewerUITest::CertificateViewerUITest() {}');
GEN('CertificateViewerUITest::~CertificateViewerUITest() {}');

/**
 * Tests that the dialog opened to the correct URL.
 */
TEST_F('CertificateViewerUITest', 'testDialogURL', function() {
  this.testDialogUrl();
});

/**
 * Tests for the correct common name in the test certificate.
 */
TEST_F('CertificateViewerUITest', 'testCN', function() {
  this.testCN();
});

/**
 * Test the details pane of the certificate viewer. This verifies that a
 * certificate in the chain can be selected to view the fields. And that fields
 * can be selected to view their values.
 */
TEST_F('CertificateViewerUITestAsync', 'testDetails', function() {
  this.testDetails();
});

// Same tests as above but running within modal (instead of constrained) dialog
// version of certificate viewer UI.
GEN('#if defined(OS_CHROMEOS)');

TEST_F('CertificateViewerModalUITest', 'testDialogURL', function() {
  this.testDialogUrl();
});

TEST_F('CertificateViewerModalUITest', 'testCN', function() {
  this.testCN();
});

/**
 * Disabled due to flakiness on Linux ChromiumOS bot: http://crbug/669597
 */
TEST_F('CertificateViewerModalUITestAsync', 'DISABLED_testDetails', function() {
  this.testDetails();
});

GEN('#endif');

////////////////////////////////////////////////////////////////////////////////
// Support functions

/**
 * Find the first tree item (in the certificate fields tree) with a value.
 * @param {!Element} tree Certificate fields subtree to search.
 * @return {?Element} The first found element with a value, null if not found.
 */
function getElementWithValue(tree) {
  for (var i = 0; i < tree.childNodes.length; i++) {
    var element = tree.childNodes[i];
    if (element.detail && element.detail.payload && element.detail.payload.val)
      return element;
    if (element = getElementWithValue(element))
      return element;
  }
  return null;
}
