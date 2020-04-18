// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for media-router-search-highlighter. */
cr.define('media_router_search_highlighter', function() {
  function registerTests() {
    suite('MediaRouterSearchHighlighter', function() {
      /**
       * Media Router Search Highlighted created before each test.
       * @type {MediaRouterSearchHighlighter}
       */
      var searchHighlighter;

      // Checks whether the |textContent| of |searchHighlighter| and its |text|
      // property matches |expected|.
      var checkTextContent = function(expected) {
        assertEquals(expected, searchHighlighter.$['text'].textContent);
        assertEquals(expected, searchHighlighter.text);
      };

      // Computes the flat text string that should be displayed when the search
      // highlighter is given |data|.
      var computeAnswerText = function(data) {
        var answer = '';
        for (var i = 0; i < data.highlightedText.length; ++i) {
          if (data.plainText[i]) {
            answer += data.plainText[i];
          }
          if (data.highlightedText[i]) {
            answer += data.highlightedText[i];
          }
        }
        return answer;
      };

      // Import media_router_search_highlighter.html before running suite.
      suiteSetup(function() {
        return PolymerTest.importHtml(
            'chrome://media-router/elements/media_router_search_highlighter/' +
            'media_router_search_highlighter.html');
      });

      // Initialize a media-router-search-highlighter before each test.
      setup(function(done) {
        PolymerTest.clearBody();
        searchHighlighter =
            document.createElement('media-router-search-highlighter');
        document.body.appendChild(searchHighlighter);

        // Let the search highlighter be created and attached.
        setTimeout(done);
      });

      test('text content correct', function(done) {
        var testInputs = [];

        // Both null and '' should be acceptable in the arrays for producing no
        // text.
        var highlightedOnlyMultiple = {
          highlightedText: ['one', 'two', 'three'],
          plainText: ['', null, ''],
        };
        testInputs.push(highlightedOnlyMultiple);

        var highlightedOnlySingle = {
          highlightedText: ['onelongsection'],
          plainText: [null],
        };
        testInputs.push(highlightedOnlySingle);

        var htmlHighlightedSingle = {
          highlightedText: ['<b></b>'],
          plainText: ['one'],
        };
        testInputs.push(htmlHighlightedSingle);

        var htmlHighlightedSplit = {
          highlightedText: ['<b>', '</b>'],
          plainText: ['one', 'two'],
        };
        testInputs.push(htmlHighlightedSplit);

        var htmlMixedSingle = {
          highlightedText: ['&amp;'],
          plainText: ['<&lt;>'],
        };
        testInputs.push(htmlMixedSingle);

        var htmlMixedSplit = {
          highlightedText: ['/>'],
          plainText: ['<br'],
        };
        testInputs.push(htmlMixedSplit);

        var htmlPlainSingle = {
          highlightedText: [''],
          plainText: ['<br/>'],
        };
        testInputs.push(htmlPlainSingle);

        var htmlPlainSplit = {
          highlightedText: [null, null],
          plainText: ['<spa', 'n>'],
        };
        testInputs.push(htmlPlainSplit);

        var mixedHighlightedFirstMultiple = {
          highlightedText: ['first', 'last'],
          plainText: [null, 'middle'],
        };
        testInputs.push(mixedHighlightedFirstMultiple);

        var mixedHighlightedFirstSingle = {
          highlightedText: ['onlytext', null],
          plainText: ['', 'plain'],
        };
        testInputs.push(mixedHighlightedFirstSingle);

        var mixedPlainFirstMultiple = {
          highlightedText: ['second', null],
          plainText: ['first', 'third'],
        };
        testInputs.push(mixedPlainFirstMultiple);

        var mixedPlainFirstSingle = {
          highlightedText: ['', 'highlight'],
          plainText: ['plaintextonly', ''],
        };
        testInputs.push(mixedPlainFirstSingle);

        var plainTextOnlyMultiple = {
          highlightedText: [null, '', null],
          plainText: ['one', 'two', 'three'],
        };
        testInputs.push(plainTextOnlyMultiple);

        var plainTextOnlySingle = {
          highlightedText: [''],
          plainText: ['lonestring'],
        };
        testInputs.push(plainTextOnlySingle);

        testInputs.forEach(function(data) {
          searchHighlighter.data = data;
          checkTextContent(computeAnswerText(data));
        });
        done();
      });
    });
  }

  return {
    registerTests: registerTests,
  };
});
