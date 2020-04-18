// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for issue-banner. */
cr.define('issue_banner', function() {
  function registerTests() {
    suite('IssueBanner', function() {
      /**
       * Issue Banner created before each test.
       * @type {IssueBanner}
       */
      var banner;

      /**
       * Fake blocking issue with an optional action created before
       * each test.
       * @type {media_router.Issue}
       */
      var fakeBlockingIssueOne;

      /**
       * Fake blocking issue without an optional action created before
       * each test.
       * @type {media_router.Issue}
       */
      var fakeBlockingIssueTwo;

      /**
       * Fake non-blocking issue with an optional action created before
       * each test.
       * @type {media_router.Issue}
       */
      var fakeNonBlockingIssueOne;

      /**
       * Fake non-blocking issue without an optional action created before
       * each test.
       * @type {media_router.Issue}
       */
      var fakeNonBlockingIssueTwo;

      // Checks whether the 'issue-action-click' event was fired with the
      // expected data.
      var checkDataFromEventFiring = function(issue, data, isDefault) {
        assertEquals(issue.id, data.detail.id);
        if (isDefault)
          assertEquals(issue.defaultActionType, data.detail.actionType);
        else
          assertEquals(issue.secondaryActionType, data.detail.actionType);
        assertEquals(issue.helpPageId, data.detail.helpPageId);
      };

      // Checks whether |expected| and the text in the |elementId| element
      // are equal.
      var checkElementText = function(expected, elementId) {
        assertEquals(expected.trim(), banner.$[elementId].textContent.trim());
      };

      // Checks whether |issue| title are equal with the title text in the UI.
      var checkIssueText = function(issue) {
        if (issue) {
          checkElementText(issue.title, 'title');

          checkElementText(
              loadTimeData.getString(banner.actionTypeToButtonTextResource_
                                         [issue.defaultActionType]),
              'default-button');

          if (issue.secondaryActionType) {
            checkElementText(
                loadTimeData.getString(banner.actionTypeToButtonTextResource_
                                           [issue.secondaryActionType]),
                'opt-button');
          }
        } else {
          checkElementText('', 'title');
          checkElementText('', 'default-button');
          checkElementText('', 'opt-button');
        }
      };

      // Checks whether parts of the UI is visible.
      var checkButtonVisibility = function(optAction) {
        assertEquals(
            !optAction,
            banner.$['buttons'].querySelector('paper-button').hidden);
      };

      // Import issue_banner.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/issue_banner/' +
            'issue_banner.html');
      });

      // Initialize an issue-banner before each test.
      setup(function(done) {
        PolymerTest.clearBody();
        banner = document.createElement('issue-banner');
        document.body.appendChild(banner);

        // Initialize issues.
        fakeBlockingIssueOne = new media_router.Issue(
            1, 'Issue Title 1', 'Issue Message 1', 0, 1, 'route id 1', true,
            1234);
        fakeBlockingIssueTwo = new media_router.Issue(
            2, 'Issue Title 2', 'Issue Message 2', 0, undefined, 'route id 2',
            true, 1234);
        fakeNonBlockingIssueOne = new media_router.Issue(
            3, 'Issue Title 3', 'Issue Message 3', 0, 1, 'route id 3', false,
            1234);
        fakeNonBlockingIssueTwo = new media_router.Issue(
            4, 'Issue Title 4', 'Issue Message 4', 0, undefined, 'route id 4',
            false, 1234);

        // Allow for the issue banner to be created and attached.
        setTimeout(done);
      });

      // Tests for 'issue-action-click' event firing when a blocking issue
      // default action is clicked.
      test('blocking issue default action click', function(done) {
        banner.issue = fakeBlockingIssueOne;
        banner.addEventListener('issue-action-click', function(data) {
          checkDataFromEventFiring(fakeBlockingIssueOne, data, true);
          done();
        });
        MockInteractions.tap(banner.$['default-button']);
      });

      // Tests for 'issue-action-click' event firing when a blocking issue
      // optional action is clicked.
      test('blocking issue optional action click', function(done) {
        banner.issue = fakeBlockingIssueOne;
        banner.addEventListener('issue-action-click', function(data) {
          checkDataFromEventFiring(fakeBlockingIssueOne, data, false);
          done();
        });
        MockInteractions.tap(banner.$['opt-button']);
      });

      // Tests for 'issue-action-click' event firing when a non-blocking issue
      // default action is clicked.
      test('non-blocking issue default action click', function(done) {
        banner.issue = fakeNonBlockingIssueOne;
        banner.addEventListener('issue-action-click', function(data) {
          checkDataFromEventFiring(fakeNonBlockingIssueOne, data, true);
          done();
        });
        MockInteractions.tap(banner.$['default-button']);
      });

      // Tests for 'issue-action-click' event firing when a non-blocking issue
      // optional action is clicked.
      test('non-blocking issue optional action click', function(done) {
        banner.issue = fakeNonBlockingIssueOne;
        banner.addEventListener('issue-action-click', function(data) {
          checkDataFromEventFiring(fakeNonBlockingIssueOne, data, false);
          done();
        });
        MockInteractions.tap(banner.$['opt-button']);
      });

      // Tests the issue text. While the UI will show only the blocking or
      // non-blocking interface, the issue's info will be set if specified.
      test('issue text', function() {
        // |issue| is initially undefined.
        assertEquals(undefined, banner.issue);

        // Set |issue| to be a blocking issue. Title text should be updated.
        banner.issue = fakeBlockingIssueOne;
        checkIssueText(banner.issue);

        // Set |issue| to be a non-blocking issue. Title text should be
        // updated.
        banner.issue = fakeNonBlockingIssueOne;
        checkIssueText(banner.issue);
      });

      // Tests whether parts of the issue-banner is hidden based on the
      // current state.
      test('hidden versus visible components', function() {
        // The blocking UI should be shown along with an optional action.
        banner.issue = fakeBlockingIssueOne;
        checkButtonVisibility(fakeBlockingIssueOne.secondaryActionType);

        // The blocking UI should be shown without an optional action.
        banner.issue = fakeBlockingIssueTwo;
        checkButtonVisibility(fakeBlockingIssueTwo.secondaryActionType);

        // The non-blocking UI should be shown along with an optional action.
        banner.issue = fakeNonBlockingIssueOne;
        checkButtonVisibility(fakeNonBlockingIssueOne.secondaryActionType);

        // The non-blocking UI should be shown without an optional action.
        banner.issue = fakeNonBlockingIssueTwo;
        checkButtonVisibility(fakeNonBlockingIssueTwo.secondaryActionType);
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
