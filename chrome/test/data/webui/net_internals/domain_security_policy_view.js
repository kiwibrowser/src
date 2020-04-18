// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['net_internals_test.js']);

// Anonymous namespace
(function() {

/*
 * A valid hash that can be set for a domain.
 * @type {string}
 */
var VALID_HASH = 'sha256/7HIpactkIAq2Y49orFOOQKurWxmmSFZhBCoQYcRhJ3Y=';

/*
 * An invalid hash that can't be set for a domain.
 * @type {string}
 */
var INVALID_HASH = 'invalid';

/*
 * Possible results of an HSTS query.
 * @enum {number}
 */
var QueryResultType = {SUCCESS: 0, NOT_FOUND: 1, ERROR: 2};

/**
 * A Task that waits for the results of a lookup query. Once the results are
 * received, checks them before completing.  Does not initiate the query.
 * @param {string} domain The domain that was looked up.
 * @param {string} inputId The ID of the input element for the lookup domain.
 * @param {string} outputId The ID of the element where the results are
       presented.
 * @param {QueryResultType} queryResultType The expected result type of the
 *     results of the query.
 * @extends {NetInternalsTest.Task}
 */
function CheckQueryResultTask(domain, inputId, outputId, queryResultType) {
  this.domain_ = domain;
  this.inputId_ = inputId;
  this.outputId_ = outputId;
  this.queryResultType_ = queryResultType;
  NetInternalsTest.Task.call(this);
}

CheckQueryResultTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  /**
   * Validates |result| and completes the task.
   * @param {object} result Results from the query.
   */
  onQueryResult_: function(result) {
    // Ignore results after |this| is finished.
    if (this.isDone())
      return;

    expectEquals(this.domain_, $(this.inputId_).value);

    // Each case has its own validation function because of the design of the
    // test reporting infrastructure.
    if (result.error != undefined) {
      this.checkError_(result);
    } else if (!result.result) {
      this.checkNotFound_(result);
    } else {
      this.checkSuccess_(result);
    }
    this.running_ = false;

    // Start the next task asynchronously, so it can add another observer
    // without getting the current result.
    window.setTimeout(this.onTaskDone.bind(this), 1);
  },

  /**
   * On errors, checks the result.
   * @param {object} result Results from the query.
   */
  checkError_: function(result) {
    expectEquals(QueryResultType.ERROR, this.queryResultType_);
    expectEquals(result.error, $(this.outputId_).innerText);
  },

  /**
   * Checks the result when the entry was not found.
   * @param {object} result Results from the query.
   */
  checkNotFound_: function(result) {
    expectEquals(QueryResultType.NOT_FOUND, this.queryResultType_);
    expectEquals('Not found', $(this.outputId_).innerText);
  },

  /**
   * Checks successful results.
   * @param {object} result Results from the query.
   */
  checkSuccess_: function(result) {
    expectEquals(QueryResultType.SUCCESS, this.queryResultType_);
    // Verify that the domain appears somewhere in the displayed text.
    var outputText = $(this.outputId_).innerText;
    expectLE(0, outputText.search(this.domain_));
  }
};

/**
 * A Task that waits for the results of an HSTS/PKP query.  Once the results are
 * received, checks them before completing.  Does not initiate the query.
 * @param {string} domain The domain expected in the returned results.
 * @param {bool} stsSubdomains Whether or not the stsSubdomains flag is expected
 *     to be set in the returned results.  Ignored on error and not found
 *     results.
 * @param {bool} pkpSubdomains Whether or not the pkpSubdomains flag is expected
 *     to be set in the returned results.  Ignored on error and not found
 *     results.
 * @param {number} stsObserved The time the STS policy was observed.
 * @param {number} pkpObserved The time the PKP policy was observed.
 * @param {string} publicKeyHashes Expected public key hashes.  Ignored on error
 *     error and not found results.
 * @param {QueryResultType} queryResultType The expected result type of the
 *     results of the query.
 * @extends {CheckQueryResultTask}
 */
function CheckHSTSPKPQueryResultTask(
    domain, stsSubdomains, pkpSubdomains, stsObserved, pkpObserved,
    publicKeyHashes, queryResultType) {
  this.stsSubdomains_ = stsSubdomains;
  this.pkpSubdomains_ = pkpSubdomains;
  this.stsObserved_ = stsObserved;
  this.pkpObserved_ = pkpObserved;
  this.publicKeyHashes_ = publicKeyHashes;
  CheckQueryResultTask.call(
      this, domain, DomainSecurityPolicyView.QUERY_HSTS_PKP_INPUT_ID,
      DomainSecurityPolicyView.QUERY_HSTS_PKP_OUTPUT_DIV_ID, queryResultType);
}

CheckHSTSPKPQueryResultTask.prototype = {
  __proto__: CheckQueryResultTask.prototype,

  /**
   * Starts watching for the query results.
   */
  start: function() {
    g_browser.addHSTSObserver(this);
  },

  /**
   * Callback from the BrowserBridge.  Validates |result| and completes the
   * task.
   * @param {object} result Results from the query.
   */
  onHSTSQueryResult: function(result) {
    this.onQueryResult_(result);
  },

  /**
   * Checks successful results.
   * @param {object} result Results from the query.
   */
  checkSuccess_: function(result) {
    expectEquals(this.stsSubdomains_, result.dynamic_sts_include_subdomains);
    expectEquals(this.pkpSubdomains_, result.dynamic_pkp_include_subdomains);
    // Disabled because of http://crbug.com/397639
    // expectLE(this.stsObserved_, result.dynamic_sts_observed);
    // expectLE(this.pkpObserved_, result.dynamic_pkp_observed);

    // |public_key_hashes| is an old synonym for what is now
    // |preloaded_spki_hashes|, which in turn is a legacy synonym for
    // |static_spki_hashes|. Look for all three, and also for
    // |dynamic_spki_hashes|.
    if (typeof result.public_key_hashes === 'undefined')
      result.public_key_hashes = '';
    if (typeof result.preloaded_spki_hashes === 'undefined')
      result.preloaded_spki_hashes = '';
    if (typeof result.static_spki_hashes === 'undefined')
      result.static_spki_hashes = '';
    if (typeof result.dynamic_spki_hashes === 'undefined')
      result.dynamic_spki_hashes = '';

    var hashes = [];
    if (result.public_key_hashes)
      hashes.push(result.public_key_hashes);
    if (result.preloaded_spki_hashes)
      hashes.push(result.preloaded_spki_hashes);
    if (result.static_spki_hashes)
      hashes.push(result.static_spki_hashes);
    if (result.dynamic_spki_hashes)
      hashes.push(result.dynamic_spki_hashes);

    expectEquals(this.publicKeyHashes_, hashes.join(','));
    CheckQueryResultTask.prototype.checkSuccess_.call(this, result);
  }
};

/**
 * A Task to try and add an HSTS/PKP domain via the HTML form. The task will
 * wait until the results from the automatically sent query have been received,
 * and then checks them against the expected values.
 * @param {string} domain The domain to send and expected to be returned.
 * @param {bool} stsSubdomains Whether the HSTS subdomain checkbox should be
 *     selected. Also the corresponding expected return value, in the success
 *     case.
 * @param {bool} pkpSubdomains Whether the pinning subdomain checkbox should be
 *     selected. Also the corresponding expected return value, in the success
 *     case. When publicKeyHashes is INVALID_HASH, the corresponding key will
 *     not be present in the result.
 * @param {number} stsObserved The time the STS policy was observed.
 * @param {number} pkpObserved The time the PKP policy was observed.
 * @param {string} publicKeyHashes Public key hash to send.  Also the
 *     corresponding expected return value, on success.  When this is the string
 *     INVALID_HASH, an empty string is expected to be received instead.
 * @param {QueryResultType} queryResultType Expected result type.
 * @extends {CheckHSTSPKPQueryResultTask}
 */
function AddHSTSPKPTask(
    domain, stsSubdomains, pkpSubdomains, publicKeyHashes, stsObserved,
    pkpObserved, queryResultType) {
  this.requestedPublicKeyHashes_ = publicKeyHashes;
  this.requestedPkpSubdomains_ = pkpSubdomains;
  if (publicKeyHashes == INVALID_HASH || publicKeyHashes === '') {
    // Although this tests with the pinning subdomain checkbox set, the
    // pin itself is invalid, so no PKP entry will be stored. When queried,
    // the key will not be present.
    pkpSubdomains = undefined;
    publicKeyHashes = '';
  }
  CheckHSTSPKPQueryResultTask.call(
      this, domain, stsSubdomains, pkpSubdomains, stsObserved, pkpObserved,
      publicKeyHashes, queryResultType);
}

AddHSTSPKPTask.prototype = {
  __proto__: CheckHSTSPKPQueryResultTask.prototype,

  /**
   * Fills out the add form, simulates a click to submit it, and starts
   * listening for the results of the query that is automatically submitted.
   */
  start: function() {
    $(DomainSecurityPolicyView.ADD_HSTS_PKP_INPUT_ID).value = this.domain_;
    $(DomainSecurityPolicyView.ADD_STS_CHECK_ID).checked = this.stsSubdomains_;
    $(DomainSecurityPolicyView.ADD_PKP_CHECK_ID).checked =
        this.requestedPkpSubdomains_;
    $(DomainSecurityPolicyView.ADD_PINS_ID).value =
        this.requestedPublicKeyHashes_;
    $(DomainSecurityPolicyView.ADD_HSTS_PKP_SUBMIT_ID).click();
    CheckHSTSPKPQueryResultTask.prototype.start.call(this);
  }
};

/**
 * A Task to query a domain and wait for the results.  Parameters mirror those
 * of CheckHSTSPKPQueryResultTask, except |domain| is also the name of the
 * domain to query.
 * @extends {CheckHSTSPKPQueryResultTask}
 */
function QueryHSTSPKPTask(
    domain, stsSubdomains, pkpSubdomains, stsObserved, pkpObserved,
    publicKeyHashes, queryResultType) {
  CheckHSTSPKPQueryResultTask.call(
      this, domain, stsSubdomains, pkpSubdomains, stsObserved, pkpObserved,
      publicKeyHashes, queryResultType);
}

QueryHSTSPKPTask.prototype = {
  __proto__: CheckHSTSPKPQueryResultTask.prototype,

  /**
   * Fills out the query form, simulates a click to submit it, and starts
   * listening for the results.
   */
  start: function() {
    CheckHSTSPKPQueryResultTask.prototype.start.call(this);
    $(DomainSecurityPolicyView.QUERY_HSTS_PKP_INPUT_ID).value = this.domain_;
    $(DomainSecurityPolicyView.QUERY_HSTS_PKP_SUBMIT_ID).click();
  }
};

/**
 * Task that deletes a single domain, then queries the deleted domain to make
 * sure it's gone.
 * @param {string} domain The domain to delete.
 * @param {QueryResultType} queryResultType The result of the query.  Can be
 *     QueryResultType.ERROR or QueryResultType.NOT_FOUND.
 * @extends {QueryTask}
 */
function DeleteTask(domain, queryResultType) {
  expectNotEquals(queryResultType, QueryResultType.SUCCESS);
  this.domain_ = domain;
  QueryHSTSPKPTask.call(this, domain, false, false, '', 0, 0, queryResultType);
}

DeleteTask.prototype = {
  __proto__: QueryHSTSPKPTask.prototype,

  /**
   * Fills out the delete form and simulates a click to submit it.  Then sends
   * a query.
   */
  start: function() {
    $(DomainSecurityPolicyView.DELETE_INPUT_ID).value = this.domain_;
    $(DomainSecurityPolicyView.DELETE_SUBMIT_ID).click();
    QueryHSTSPKPTask.prototype.start.call(this);
  }
};

/**
 * A Task that waits for the results of an Expect-CT query. Once the results are
 * received, checks them before completing.  Does not initiate the query.
 * @param {string} domain The domain expected in the returned results.
 * @param {bool} enforce Whether or not the 'enforce' flag is expected
 *     to be set in the returned results.  Ignored on error and not found
 *     results.
 * @param {string} reportUri Expected report URI for the policy. Ignored on
 *     error and not found results.
 * @param {QueryResultType} queryResultType The expected result type of the
 *     results of the query.
 * @extends {CheckQueryResultTask}
 */
function CheckExpectCTQueryResultTask(
    domain, enforce, reportUri, queryResultType) {
  this.enforce_ = enforce;
  this.reportUri_ = reportUri;
  CheckQueryResultTask.call(
      this, domain, DomainSecurityPolicyView.QUERY_EXPECT_CT_INPUT_ID,
      DomainSecurityPolicyView.QUERY_EXPECT_CT_OUTPUT_DIV_ID, queryResultType);
}

CheckExpectCTQueryResultTask.prototype = {
  __proto__: CheckQueryResultTask.prototype,

  /**
   * Starts watching for the query results.
   */
  start: function() {
    g_browser.addExpectCTObserver(this);
  },

  /**
   * Callback from the BrowserBridge.  Validates |result| and completes the
   * task.
   * @param {object} result Results from the query.
   */
  onExpectCTQueryResult: function(result) {
    this.onQueryResult_(result);
  },

  /**
   * Checks successful results.
   * @param {object} result Results from the query.
   */
  checkSuccess_: function(result) {
    expectEquals(this.enforce_, result.dynamic_expect_ct_enforce);
    expectEquals(this.reportUri_, result.dynamic_expect_ct_report_uri);
    CheckQueryResultTask.prototype.checkSuccess_.call(this, result);
  }
};

/**
 * A Task to try and add an Expect-CT domain via the HTML form. The task will
 * wait until the results from the automatically sent query have been received,
 * and then checks them against the expected values.
 * @param {string} domain The domain to send and expected to be returned.
 * @param {bool} enforce Whether the enforce checkbox should be
 *     selected. Also the corresponding expected return value, in the success
 *     case.
 * @param {string} reportUri The report URI for the Expect-CT policy. Also the
 *     corresponding expected return value, on success.
 * @param {QueryResultType} queryResultType Expected result type.
 * @extends {CheckExpectCTQueryResultTask}
 */
function AddExpectCTTask(domain, enforce, reportUri, queryResultType) {
  CheckExpectCTQueryResultTask.call(
      this, domain, enforce, reportUri, queryResultType);
}

AddExpectCTTask.prototype = {
  __proto__: CheckExpectCTQueryResultTask.prototype,

  /**
   * Fills out the add form, simulates a click to submit it, and starts
   * listening for the results of the query that is automatically submitted.
   */
  start: function() {
    $(DomainSecurityPolicyView.ADD_EXPECT_CT_INPUT_ID).value = this.domain_;
    $(DomainSecurityPolicyView.ADD_EXPECT_CT_ENFORCE_CHECK_ID).checked =
        this.enforce_;
    $(DomainSecurityPolicyView.ADD_EXPECT_CT_REPORT_URI_INPUT_ID).value =
        this.reportUri_;
    $(DomainSecurityPolicyView.ADD_EXPECT_CT_SUBMIT_ID).click();
    CheckExpectCTQueryResultTask.prototype.start.call(this);
  }
};

/**
 * A Task to query a domain and wait for the results.  Parameters mirror those
 * of CheckExpectCTQueryResultTask, except |domain| is also the name of the
 * domain to query.
 * @extends {CheckExpectCTQueryResultTask}
 */
function QueryExpectCTTask(domain, enforce, reportUri, queryResultType) {
  CheckExpectCTQueryResultTask.call(
      this, domain, enforce, reportUri, queryResultType);
}

QueryExpectCTTask.prototype = {
  __proto__: CheckExpectCTQueryResultTask.prototype,

  /**
   * Fills out the query form, simulates a click to submit it, and starts
   * listening for the results.
   */
  start: function() {
    CheckExpectCTQueryResultTask.prototype.start.call(this);
    $(DomainSecurityPolicyView.QUERY_EXPECT_CT_INPUT_ID).value = this.domain_;
    $(DomainSecurityPolicyView.QUERY_EXPECT_CT_SUBMIT_ID).click();
  }
};

/**
 * A Task to retrieve a test report-uri.
 * @extends {NetInternalsTest.Task}
 */
function GetTestReportURITask() {
  NetInternalsTest.Task.call(this);
}

GetTestReportURITask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  /**
   * Sets |NetInternals.callback|, and sends the request to the browser process.
   */
  start: function() {
    NetInternalsTest.setCallback(this.onReportURIReceived_.bind(this));
    chrome.send('setUpTestReportURI');
  },

  /**
   * Saves the received report-uri and completes the task.
   * @param {string} reportURI Report URI received from the browser process.
   */
  onReportURIReceived_: function(reportURI) {
    this.reportURI_ = reportURI;
    this.onTaskDone();
  },

  /**
   * Returns the saved report-uri received from the browser process.
   */
  reportURI: function() {
    return this.reportURI_;
  }
};

/**
 * A Task to send a test Expect-CT report and wait for the result.
 * @param {getTestReportURITask} GetTestReportURITask The task that retrieved a
                                 test report-uri.
 * @extends {NetInternalsTest.Task}
 */
function SendTestReportTask(getTestReportURITask) {
  this.reportURITask_ = getTestReportURITask;
  NetInternalsTest.Task.call(this);
}

SendTestReportTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  /**
   * Sends the test report and starts watching for the result.
   */
  start: function() {
    g_browser.addExpectCTObserver(this);
    $(DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_INPUT_ID).value =
        this.reportURITask_.reportURI();
    $(DomainSecurityPolicyView.TEST_REPORT_EXPECT_CT_SUBMIT_ID).click();
  },

  /**
   * Callback from the BrowserBridge.  Checks that |result| indicates success
   * and completes the task.
   * @param {object} result Results from the query.
   */
  onExpectCTTestReportResult: function(result) {
    expectEquals('success', result);
    // Start the next task asynchronously, so it can add another observer
    // without getting the current result.
    window.setTimeout(this.onTaskDone.bind(this), 1);
  },
};

/**
 * Checks that querying a domain that was never added fails.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewQueryNotFound',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new QueryHSTSPKPTask(
          'somewhere.com', false, false, now, now, '',
          QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Checks that querying a domain with an invalid name returns an error.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewQueryError',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new QueryHSTSPKPTask(
          '\u3024', false, false, now, now, '', QueryResultType.ERROR));
      taskQueue.run();
    });

/**
 * Deletes a domain that was never added.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewDeleteNotFound',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Deletes a domain that returns an error on lookup.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewDeleteError',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new DeleteTask('\u3024', QueryResultType.ERROR));
      taskQueue.run();
    });

/**
 * Adds a domain and then deletes it.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewAddDelete',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new AddHSTSPKPTask(
          'somewhere.com', false, false, VALID_HASH, now, now,
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Tries to add a domain with an invalid name.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewAddFail',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new AddHSTSPKPTask(
          '0123456789012345678901234567890' +
              '012345678901234567890123456789012345',
          false, false, '', now, now, QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Tries to add a domain with a name that errors out on lookup due to having
 * non-ASCII characters in it.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewAddError',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new AddHSTSPKPTask(
          '\u3024', false, false, '', now, now, QueryResultType.ERROR));
      taskQueue.run();
    });

/**
 * Adds a domain with an invalid hash.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewAddInvalidHash',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new AddHSTSPKPTask(
          'somewhere.com', true, true, INVALID_HASH, now, now,
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Adds the same domain twice in a row, modifying some values the second time.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewAddOverwrite',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new AddHSTSPKPTask(
          'somewhere.com', true, true, VALID_HASH, now, now,
          QueryResultType.SUCCESS));
      taskQueue.addTask(new AddHSTSPKPTask(
          'somewhere.com', false, false, '', now, now,
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Adds two different domains and then deletes them.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewAddTwice',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(new AddHSTSPKPTask(
          'somewhere.com', false, false, VALID_HASH, now, now,
          QueryResultType.SUCCESS));
      taskQueue.addTask(new QueryHSTSPKPTask(
          'somewhereelse.com', false, false, now, now, '',
          QueryResultType.NOT_FOUND));
      taskQueue.addTask(new AddHSTSPKPTask(
          'somewhereelse.com', true, false, '', now, now,
          QueryResultType.SUCCESS));
      taskQueue.addTask(new QueryHSTSPKPTask(
          'somewhere.com', false, false, now, now, VALID_HASH,
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.addTask(new QueryHSTSPKPTask(
          'somewhereelse.com', true, undefined, now, now, '',
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhereelse.com', QueryResultType.NOT_FOUND));
      taskQueue.run(true);
    });

/**
 * Checks that querying an Expect-CT domain that was never added fails.
 */
TEST_F(
    'NetInternalsTest',
    'netInternalsDomainSecurityPolicyViewExpectCTQueryNotFound', function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new QueryExpectCTTask(
          'somewhere.com', false, '', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Checks that querying an Expect-CT domain with an invalid name returns an
 * error.
 */
TEST_F(
    'NetInternalsTest',
    'netInternalsDomainSecurityPolicyViewExpectCTQueryError', function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var now = new Date().getTime() / 1000.0;
      taskQueue.addTask(
          new QueryExpectCTTask('\u3024', false, '', QueryResultType.ERROR));
      taskQueue.run();
    });

/**
 * Adds an Expect-CT domain and then deletes it.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewExpectCTAddDelete',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new AddExpectCTTask(
          'somewhere.com', true, '', QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Tries to add an Expect-CT domain with an invalid name.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewExpectCTAddFail',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new AddExpectCTTask(
          '0123456789012345678901234567890' +
              '012345678901234567890123456789012345',
          false, '', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Tries to add an Expect-CT domain with a name that errors out on lookup due to
 * having non-ASCII characters in it.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewExpectCTAddError',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(
          new AddExpectCTTask('\u3024', false, '', QueryResultType.ERROR));
      taskQueue.run();
    });

/**
 * Adds the same Expect-CT domain twice in a row, modifying some values the
 * second time.
 */
TEST_F(
    'NetInternalsTest',
    'netInternalsDomainSecurityPolicyViewExpectCTAddOverwrite', function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new AddExpectCTTask(
          'somewhere.com', true, 'https://reporting.test/',
          QueryResultType.SUCCESS));
      taskQueue.addTask(new AddExpectCTTask(
          'somewhere.com', false, 'https://other-reporting.test/',
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.run();
    });

/**
 * Adds two different Expect-CT domains and then deletes them.
 */
TEST_F(
    'NetInternalsTest', 'netInternalsDomainSecurityPolicyViewExpectCTAddTwice',
    function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      taskQueue.addTask(new AddExpectCTTask(
          'somewhere.com', true, '', QueryResultType.SUCCESS));
      taskQueue.addTask(new QueryExpectCTTask(
          'somewhereelse.com', false, '', QueryResultType.NOT_FOUND));
      taskQueue.addTask(new AddExpectCTTask(
          'somewhereelse.com', true, 'https://reporting.test/',
          QueryResultType.SUCCESS));
      taskQueue.addTask(new QueryExpectCTTask(
          'somewhere.com', true, '', QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhere.com', QueryResultType.NOT_FOUND));
      taskQueue.addTask(new QueryExpectCTTask(
          'somewhereelse.com', true, 'https://reporting.test/',
          QueryResultType.SUCCESS));
      taskQueue.addTask(
          new DeleteTask('somewhereelse.com', QueryResultType.NOT_FOUND));
      taskQueue.run(true);
    });


/**
 * Checks that sending an Expect-CT test report succeeds.
 */
TEST_F(
    'NetInternalsTest',
    'netInternalsDomainSecurityPolicyViewExpectCTTestReport', function() {
      NetInternalsTest.switchToView('hsts');
      taskQueue = new NetInternalsTest.TaskQueue(true);
      var getReportURITask = new GetTestReportURITask();
      taskQueue.addTask(getReportURITask);
      taskQueue.addTask(new SendTestReportTask(getReportURITask));
      taskQueue.run();
    });

})();  // Anonymous namespace
