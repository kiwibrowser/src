// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This UI allows a user to query and update the browser's list of per-domain
 * security policies. These policies include:
 * - HSTS: HTTPS Strict Transport Security. A way for sites to elect to always
 *   use HTTPS. See http://dev.chromium.org/sts
 * - PKP: Public Key Pinning. A way for sites to pin themselves to particular
 *   public key fingerprints that must appear in their certificate chains. See
 *   https://tools.ietf.org/html/rfc7469
 * - Expect-CT. A way for sites to elect to always require valid Certificate
 *   Transparency information to be present. See
 *   https://tools.ietf.org/html/draft-ietf-httpbis-expect-ct-01
 */

var DomainSecurityPolicyView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function DomainSecurityPolicyView() {
    assertFirstConstructorCall(DomainSecurityPolicyView);

    // Call superclass's constructor.
    superClass.call(this, DomainSecurityPolicyView.MAIN_BOX_ID);

    this.deleteInput_ = $(DomainSecurityPolicyView.DELETE_INPUT_ID);
    this.addStsPkpInput_ = $(DomainSecurityPolicyView.ADD_HSTS_PKP_INPUT_ID);
    this.addStsCheck_ = $(DomainSecurityPolicyView.ADD_STS_CHECK_ID);
    this.addPkpCheck_ = $(DomainSecurityPolicyView.ADD_PKP_CHECK_ID);
    this.addPins_ = $(DomainSecurityPolicyView.ADD_PINS_ID);
    this.queryStsPkpInput_ =
        $(DomainSecurityPolicyView.QUERY_HSTS_PKP_INPUT_ID);
    this.queryStsPkpOutputDiv_ =
        $(DomainSecurityPolicyView.QUERY_HSTS_PKP_OUTPUT_DIV_ID);
    this.addExpectCTInput_ = $(DomainSecurityPolicyView.ADD_EXPECT_CT_INPUT_ID);
    this.addExpectCTReportUriInput_ =
        $(DomainSecurityPolicyView.ADD_EXPECT_CT_REPORT_URI_INPUT_ID);
    this.addExpectCTEnforceCheck_ =
        $(DomainSecurityPolicyView.ADD_EXPECT_CT_ENFORCE_CHECK_ID);
    this.queryExpectCTInput_ =
        $(DomainSecurityPolicyView.QUERY_EXPECT_CT_INPUT_ID);
    this.queryExpectCTOutputDiv_ =
        $(DomainSecurityPolicyView.QUERY_EXPECT_CT_OUTPUT_DIV_ID);
    this.testExpectCTReportInput_ =
        $(DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_INPUT_ID);
    this.testExpectCTOutputDiv_ =
        $(DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_OUTPUT_DIV_ID);

    var form = $(DomainSecurityPolicyView.DELETE_FORM_ID);
    form.addEventListener('submit', this.onSubmitDelete_.bind(this), false);

    form = $(DomainSecurityPolicyView.ADD_HSTS_PKP_FORM_ID);
    form.addEventListener('submit', this.onSubmitHSTSPKPAdd_.bind(this), false);

    form = $(DomainSecurityPolicyView.QUERY_HSTS_PKP_FORM_ID);
    form.addEventListener(
        'submit', this.onSubmitHSTSPKPQuery_.bind(this), false);

    form = $(DomainSecurityPolicyView.ADD_EXPECT_CT_FORM_ID);
    form.addEventListener(
        'submit', this.onSubmitExpectCTAdd_.bind(this), false);

    form = $(DomainSecurityPolicyView.QUERY_EXPECT_CT_FORM_ID);
    form.addEventListener(
        'submit', this.onSubmitExpectCTQuery_.bind(this), false);

    form = $(DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_FORM_ID);
    form.addEventListener(
        'submit', this.onSubmitExpectCTTestReport_.bind(this), false);

    g_browser.addHSTSObserver(this);
    g_browser.addExpectCTObserver(this);
  }

  DomainSecurityPolicyView.TAB_ID = 'tab-handle-domain-security-policy';
  DomainSecurityPolicyView.TAB_NAME = 'Domain Security Policy';
  // This tab was originally limited to HSTS. Even though it now encompasses
  // domain security policy more broadly, keep the hash as "#hsts" to preserve
  // links/documentation that directs users to chrome://net-internals#hsts.
  DomainSecurityPolicyView.TAB_HASH = '#hsts';

  // IDs for special HTML elements in domain_security_policy_view.html
  DomainSecurityPolicyView.MAIN_BOX_ID =
      'domain-security-policy-view-tab-content';
  DomainSecurityPolicyView.DELETE_INPUT_ID =
      'domain-security-policy-view-delete-input';
  DomainSecurityPolicyView.DELETE_FORM_ID =
      'domain-security-policy-view-delete-form';
  DomainSecurityPolicyView.DELETE_SUBMIT_ID =
      'domain-security-policy-view-delete-submit';
  // HSTS/PKP form elements
  DomainSecurityPolicyView.ADD_HSTS_PKP_INPUT_ID = 'hsts-view-add-input';
  DomainSecurityPolicyView.ADD_STS_CHECK_ID = 'hsts-view-check-sts-input';
  DomainSecurityPolicyView.ADD_PKP_CHECK_ID = 'hsts-view-check-pkp-input';
  DomainSecurityPolicyView.ADD_PINS_ID = 'hsts-view-add-pins';
  DomainSecurityPolicyView.ADD_HSTS_PKP_FORM_ID = 'hsts-view-add-form';
  DomainSecurityPolicyView.ADD_HSTS_PKP_SUBMIT_ID = 'hsts-view-add-submit';
  DomainSecurityPolicyView.QUERY_HSTS_PKP_INPUT_ID = 'hsts-view-query-input';
  DomainSecurityPolicyView.QUERY_HSTS_PKP_OUTPUT_DIV_ID =
      'hsts-view-query-output';
  DomainSecurityPolicyView.QUERY_HSTS_PKP_FORM_ID = 'hsts-view-query-form';
  DomainSecurityPolicyView.QUERY_HSTS_PKP_SUBMIT_ID = 'hsts-view-query-submit';
  // Expect-CT form elements
  DomainSecurityPolicyView.ADD_EXPECT_CT_INPUT_ID = 'expect-ct-view-add-input';
  DomainSecurityPolicyView.ADD_EXPECT_CT_REPORT_URI_INPUT_ID =
      'expect-ct-view-add-report-uri-input';
  DomainSecurityPolicyView.ADD_EXPECT_CT_ENFORCE_CHECK_ID =
      'expect-ct-view-check-enforce-input';
  DomainSecurityPolicyView.ADD_EXPECT_CT_FORM_ID = 'expect-ct-view-add-form';
  DomainSecurityPolicyView.ADD_EXPECT_CT_SUBMIT_ID =
      'expect-ct-view-add-submit';
  DomainSecurityPolicyView.QUERY_EXPECT_CT_INPUT_ID =
      'expect-ct-view-query-input';
  DomainSecurityPolicyView.QUERY_EXPECT_CT_FORM_ID =
      'expect-ct-view-query-form';
  DomainSecurityPolicyView.QUERY_EXPECT_CT_SUBMIT_ID =
      'expect-ct-view-query-submit';
  DomainSecurityPolicyView.QUERY_EXPECT_CT_OUTPUT_DIV_ID =
      'expect-ct-view-query-output';
  DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_INPUT_ID =
      'expect-ct-view-test-report-uri';
  DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_FORM_ID =
      'expect-ct-view-test-report-form';
  DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_SUBMIT_ID =
      'expect-ct-view-test-report-submit';
  DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_OUTPUT_DIV_ID =
      'expect-ct-view-test-report-output';

  cr.addSingletonGetter(DomainSecurityPolicyView);

  DomainSecurityPolicyView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    onSubmitHSTSPKPAdd_: function(event) {
      g_browser.sendHSTSAdd(
          this.addStsPkpInput_.value, this.addStsCheck_.checked,
          this.addPkpCheck_.checked, this.addPins_.value);
      g_browser.sendHSTSQuery(this.addStsPkpInput_.value);
      this.queryStsPkpInput_.value = this.addStsPkpInput_.value;
      this.addStsCheck_.checked = false;
      this.addPkpCheck_.checked = false;
      this.addStsPkpInput_.value = '';
      this.addPins_.value = '';
      event.preventDefault();
    },

    onSubmitDelete_: function(event) {
      g_browser.sendDomainSecurityPolicyDelete(this.deleteInput_.value);
      this.deleteInput_.value = '';
      event.preventDefault();
    },

    onSubmitHSTSPKPQuery_: function(event) {
      g_browser.sendHSTSQuery(this.queryStsPkpInput_.value);
      event.preventDefault();
    },

    onHSTSQueryResult: function(result) {
      if (result.error != undefined) {
        this.queryStsPkpOutputDiv_.innerHTML = '';
        var s = addNode(this.queryStsPkpOutputDiv_, 'span');
        s.textContent = result.error;
        s.style.color = '#e00';
        yellowFade(this.queryStsPkpOutputDiv_);
        return;
      }

      if (result.result == false) {
        this.queryStsPkpOutputDiv_.innerHTML = '<b>Not found</b>';
        yellowFade(this.queryStsPkpOutputDiv_);
        return;
      }

      this.queryStsPkpOutputDiv_.innerHTML = '';

      var s = addNode(this.queryStsPkpOutputDiv_, 'span');
      s.innerHTML = '<b>Found:</b><br/>';

      var keys = [
        'static_sts_domain',
        'static_upgrade_mode',
        'static_sts_include_subdomains',
        'static_sts_observed',
        'static_pkp_domain',
        'static_pkp_include_subdomains',
        'static_pkp_observed',
        'static_spki_hashes',
        'dynamic_sts_domain',
        'dynamic_upgrade_mode',
        'dynamic_sts_include_subdomains',
        'dynamic_sts_observed',
        'dynamic_sts_expiry',
        'dynamic_pkp_domain',
        'dynamic_pkp_include_subdomains',
        'dynamic_pkp_observed',
        'dynamic_pkp_expiry',
        'dynamic_spki_hashes',
      ];

      var kStaticHashKeys =
          ['public_key_hashes', 'preloaded_spki_hashes', 'static_spki_hashes'];

      var staticHashes = [];
      for (var i = 0; i < kStaticHashKeys.length; ++i) {
        var staticHashValue = result[kStaticHashKeys[i]];
        if (staticHashValue != undefined && staticHashValue != '')
          staticHashes.push(staticHashValue);
      }

      for (var i = 0; i < keys.length; ++i) {
        var key = keys[i];
        var value = result[key];
        addTextNode(this.queryStsPkpOutputDiv_, ' ' + key + ': ');

        // If there are no static_hashes, do not make it seem like there is a
        // static PKP policy in place.
        if (staticHashes.length == 0 && key.startsWith('static_pkp_')) {
          addNode(this.queryStsPkpOutputDiv_, 'br');
          continue;
        }

        if (key === 'static_spki_hashes') {
          addNodeWithText(
              this.queryStsPkpOutputDiv_, 'tt', staticHashes.join(','));
        } else if (key.indexOf('_upgrade_mode') >= 0) {
          addNodeWithText(
              this.queryStsPkpOutputDiv_, 'tt', modeToString(value));
        } else {
          addNodeWithText(
              this.queryStsPkpOutputDiv_, 'tt',
              value == undefined ? '' : value);
        }
        addNode(this.queryStsPkpOutputDiv_, 'br');
      }

      yellowFade(this.queryStsPkpOutputDiv_);
    },

    onSubmitExpectCTAdd_: function(event) {
      g_browser.sendExpectCTAdd(
          this.addExpectCTInput_.value, this.addExpectCTReportUriInput_.value,
          this.addExpectCTEnforceCheck_.checked);
      g_browser.sendExpectCTQuery(this.addExpectCTInput_.value);
      this.queryExpectCTInput_.value = this.addExpectCTInput_.value;
      this.addExpectCTInput_.value = '';
      this.addExpectCTReportUriInput_.value = '';
      this.addExpectCTEnforceCheck_.checked = false;
      event.preventDefault();
    },

    onSubmitExpectCTQuery_: function(event) {
      g_browser.sendExpectCTQuery(this.queryExpectCTInput_.value);
      event.preventDefault();
    },

    onExpectCTQueryResult: function(result) {
      if (result.error != undefined) {
        this.queryExpectCTOutputDiv_.innerHTML = '';
        var s = addNode(this.queryExpectCTOutputDiv_, 'span');
        s.textContent = result.error;
        s.style.color = '#e00';
        yellowFade(this.queryExpectCTOutputDiv_);
        return;
      }

      if (result.result == false) {
        this.queryExpectCTOutputDiv_.innerHTML = '<b>Not found</b>';
        yellowFade(this.queryExpectCTOutputDiv_);
        return;
      }

      this.queryExpectCTOutputDiv_.innerHTML = '';

      var s = addNode(this.queryExpectCTOutputDiv_, 'span');
      s.innerHTML = '<b>Found:</b><br/>';

      var keys = [
        'dynamic_expect_ct_domain',
        'dynamic_expect_ct_observed',
        'dynamic_expect_ct_expiry',
        'dynamic_expect_ct_enforce',
        'dynamic_expect_ct_report_uri',
      ];

      for (var i in keys) {
        var key = keys[i];
        var value = result[key];
        addTextNode(this.queryExpectCTOutputDiv_, ' ' + key + ': ');
        addNodeWithText(
            this.queryExpectCTOutputDiv_, 'tt',
            value == undefined ? '' : value);
        addNode(this.queryExpectCTOutputDiv_, 'br');
      }

      yellowFade(this.queryExpectCTOutputDiv_);
    },

    onSubmitExpectCTTestReport_: function(event) {
      g_browser.sendExpectCTTestReport(this.testExpectCTReportInput_.value);
      event.preventDefault();
    },

    onExpectCTTestReportResult: function(result) {
      if (result == 'success') {
        addTextNode(this.testExpectCTOutputDiv_, 'Test report succeeded');
      } else {
        addTextNode(this.testExpectCTOutputDiv_, 'Test report failed');
      }
      yellowFade(this.testExpectCTOutputDiv_);
    },

  };

  function modeToString(m) {
    // These numbers must match those in
    // TransportSecurityState::STSState::UpgradeMode.
    if (m == 0) {
      return 'FORCE_HTTPS';
    } else if (m == 1) {
      return 'DEFAULT';
    } else {
      return 'UNKNOWN';
    }
  }

  function yellowFade(element) {
    element.style.transitionProperty = 'background-color';
    element.style.transitionDuration = '0';
    element.style.backgroundColor = '#fffccf';
    setTimeout(function() {
      element.style.transitionDuration = '1000ms';
      element.style.backgroundColor = '#fff';
    }, 0);
  }

  return DomainSecurityPolicyView;
})();
