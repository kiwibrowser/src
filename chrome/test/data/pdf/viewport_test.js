// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  function testDocumentNeedsScrollbars() {
    var viewport =
        new Viewport(new MockWindow(100, 100), new MockSizer(), function() {},
                     function() {}, function() {}, function() {}, 10, 1, 0);
    var scrollbars;

    viewport.setDocumentDimensions(new MockDocumentDimensions(90, 90));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertFalse(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(100.49, 100.49));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertFalse(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(100.5, 100.5));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertTrue(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(110, 110));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertTrue(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(90, 101));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(101, 90));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertFalse(scrollbars.vertical);
    chrome.test.assertTrue(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(91, 101));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertTrue(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(101, 91));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertTrue(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(40, 51));
    scrollbars = viewport.documentNeedsScrollbars_(2);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(101, 202));
    scrollbars = viewport.documentNeedsScrollbars_(0.5);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);
    chrome.test.succeed();

    // Test the case when there is a toolbar at the top.
    var toolbarHeight = 10;
    var viewport =
        new Viewport(new MockWindow(100, 100), new MockSizer(), function() {},
                     function() {}, function() {}, function() {}, 10, 1,
                     toolbarHeight);
    var scrollbars;

    viewport.setDocumentDimensions(new MockDocumentDimensions(90, 90));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertFalse(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(91, 91));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(100, 100));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(101, 101));
    scrollbars = viewport.documentNeedsScrollbars_(1);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertTrue(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(45, 45));
    scrollbars = viewport.documentNeedsScrollbars_(2);
    chrome.test.assertFalse(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);

    viewport.setDocumentDimensions(new MockDocumentDimensions(46, 46));
    scrollbars = viewport.documentNeedsScrollbars_(2);
    chrome.test.assertTrue(scrollbars.vertical);
    chrome.test.assertFalse(scrollbars.horizontal);
  },

  function testSetZoom() {
    var mockSizer = new MockSizer();
    var mockWindow = new MockWindow(100, 100, mockSizer);
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);

    // Test setting the zoom without the document dimensions set. The sizer
    // shouldn't change size.
    mockCallback.reset();
    viewport.setZoom(0.5);
    chrome.test.assertEq(0.5, viewport.zoom);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq('0px', mockSizer.style.width);
    chrome.test.assertEq('0px', mockSizer.style.height);
    chrome.test.assertEq(0, mockWindow.pageXOffset);
    chrome.test.assertEq(0, mockWindow.pageYOffset);

    viewport.setZoom(1);
    viewport.setDocumentDimensions(new MockDocumentDimensions(200, 200));

    // Test zooming out.
    mockCallback.reset();
    viewport.setZoom(0.5);
    chrome.test.assertEq(0.5, viewport.zoom);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq('100px', mockSizer.style.width);
    chrome.test.assertEq('100px', mockSizer.style.height);

    // Test zooming in.
    mockCallback.reset();
    viewport.setZoom(2);
    chrome.test.assertEq(2, viewport.zoom);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq('400px', mockSizer.style.width);
    chrome.test.assertEq('400px', mockSizer.style.height);

    // Test that the scroll position scales correctly. It scales relative to the
    // top-left of the page.
    viewport.setZoom(1);
    mockWindow.pageXOffset = 50;
    mockWindow.pageYOffset = 50;
    viewport.setZoom(2);
    chrome.test.assertEq('400px', mockSizer.style.width);
    chrome.test.assertEq('400px', mockSizer.style.height);
    chrome.test.assertEq(100, mockWindow.pageXOffset);
    chrome.test.assertEq(100, mockWindow.pageYOffset);
    mockWindow.scrollTo(250, 250);
    viewport.setZoom(1);
    chrome.test.assertEq('200px', mockSizer.style.width);
    chrome.test.assertEq('200px', mockSizer.style.height);
    chrome.test.assertEq(100, mockWindow.pageXOffset);
    chrome.test.assertEq(100, mockWindow.pageYOffset);

    var documentDimensions = new MockDocumentDimensions(0, 0);
    documentDimensions.addPage(200, 200);
    viewport.setDocumentDimensions(documentDimensions);
    mockWindow.scrollTo(0, 0);
    viewport.fitToPage();
    viewport.setZoom(1);
    chrome.test.assertEq(FittingType.NONE, viewport.fittingType);
    chrome.test.assertEq('200px', mockSizer.style.width);
    chrome.test.assertEq('200px', mockSizer.style.height);
    chrome.test.assertEq(0, mockWindow.pageXOffset);
    chrome.test.assertEq(0, mockWindow.pageYOffset);

    viewport.fitToWidth();
    viewport.setZoom(1);
    chrome.test.assertEq(FittingType.NONE, viewport.fittingType);
    chrome.test.assertEq('200px', mockSizer.style.width);
    chrome.test.assertEq('200px', mockSizer.style.height);
    chrome.test.assertEq(0, mockWindow.pageXOffset);
    chrome.test.assertEq(0, mockWindow.pageYOffset);

    chrome.test.succeed();
  },

  function testGetMostVisiblePage() {
    var mockWindow = new MockWindow(100, 100);
    var viewport = new Viewport(mockWindow, new MockSizer(), function() {},
                                function() {}, function() {}, function() {},
                                0, 1, 0);

    var documentDimensions = new MockDocumentDimensions(100, 100);
    documentDimensions.addPage(50, 100);
    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(100, 200);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    // Scrolled to the start of the first page.
    mockWindow.scrollTo(0, 0);
    chrome.test.assertEq(0, viewport.getMostVisiblePage());

    // Scrolled to the start of the second page.
    mockWindow.scrollTo(0, 100);
    chrome.test.assertEq(1, viewport.getMostVisiblePage());

    // Scrolled half way through the first page.
    mockWindow.scrollTo(0, 50);
    chrome.test.assertEq(0, viewport.getMostVisiblePage());

    // Scrolled just over half way through the first page.
    mockWindow.scrollTo(0, 51);
    chrome.test.assertEq(1, viewport.getMostVisiblePage());

    // Scrolled most of the way through the second page.
    mockWindow.scrollTo(0, 180);
    chrome.test.assertEq(2, viewport.getMostVisiblePage());

    // Scrolled just past half way through the second page.
    mockWindow.scrollTo(0, 160);
    chrome.test.assertEq(1, viewport.getMostVisiblePage());

    // Scrolled just over half way through the first page with 2x zoom.
    viewport.setZoom(2);
    mockWindow.scrollTo(0, 151);
    chrome.test.assertEq(1, viewport.getMostVisiblePage());

    // Zoomed out with the entire document visible.
    viewport.setZoom(0.25);
    mockWindow.scrollTo(0, 0);
    chrome.test.assertEq(0, viewport.getMostVisiblePage());
    chrome.test.succeed();
  },

  function testFitToWidth() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    function assertZoomed(expectedMockWidth, expectedMockHeight, expectedZoom) {
      chrome.test.assertEq(FittingType.FIT_TO_WIDTH, viewport.fittingType);
      chrome.test.assertTrue(mockCallback.wasCalled);
      chrome.test.assertEq(`${expectedMockWidth}px`, mockSizer.style.width);
      chrome.test.assertEq(`${expectedMockHeight}px`, mockSizer.style.height);
      chrome.test.assertEq(expectedZoom, viewport.zoom);
    };

    function testForSize(
        pageWidth, pageHeight, expectedMockWidth, expectedMockHeight,
        expectedZoom) {
      documentDimensions.reset();
      documentDimensions.addPage(pageWidth, pageHeight);
      viewport.setDocumentDimensions(documentDimensions);
      viewport.setZoom(0.1);
      mockCallback.reset();
      viewport.fitToWidth();
      assertZoomed(expectedMockWidth, expectedMockHeight, expectedZoom);
    };

    // Document width which matches the window width.
    testForSize(100, 100, 100, 100, 1);

    // Document width which matches the window width, but taller.
    testForSize(100, 200, 100, 200, 1);

    // Document width which matches the window width, but shorter.
    testForSize(100, 50, 100, 50, 1);

    // Document width which is twice the size of the window width.
    testForSize(200, 100, 100, 50, 0.5);

    // Document width which is half the size of the window width.
    testForSize(50, 100, 100, 200, 2);

    // Test that the scroll position stays the same relative to the page after
    // fit to page is called.
    documentDimensions.reset();
    documentDimensions.addPage(50, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 100);
    mockCallback.reset();
    viewport.fitToWidth();
    chrome.test.assertEq(FittingType.FIT_TO_WIDTH, viewport.fittingType);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(2, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(200, viewport.position.y);

    // Test fitting works with scrollbars. The page will need to be zoomed to
    // fit to width, which will cause the page height to span outside of the
    // viewport, triggering 15px scrollbars to be shown.
    viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                            function() {}, function() {}, function() {},
                            15, 1, 0);
    documentDimensions.reset();
    documentDimensions.addPage(50, 100);
    viewport.setDocumentDimensions(documentDimensions);
    mockCallback.reset();
    viewport.fitToWidth();
    chrome.test.assertEq(FittingType.FIT_TO_WIDTH, viewport.fittingType);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq('85px', mockSizer.style.width);
    chrome.test.assertEq(1.7, viewport.zoom);
    chrome.test.succeed();
  },

  function testFitToPage() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    function assertZoomed(expectedMockWidth, expectedMockHeight, expectedZoom) {
      chrome.test.assertEq(FittingType.FIT_TO_PAGE, viewport.fittingType);
      chrome.test.assertTrue(mockCallback.wasCalled);
      chrome.test.assertEq(`${expectedMockWidth}px`, mockSizer.style.width);
      chrome.test.assertEq(`${expectedMockHeight}px`, mockSizer.style.height);
      chrome.test.assertEq(expectedZoom, viewport.zoom);
    };

    function testForSize(
        pageWidth, pageHeight, expectedMockWidth, expectedMockHeight,
        expectedZoom) {
      documentDimensions.reset();
      documentDimensions.addPage(pageWidth, pageHeight);
      viewport.setDocumentDimensions(documentDimensions);
      viewport.setZoom(0.1);
      mockCallback.reset();
      viewport.fitToPage();
      assertZoomed(expectedMockWidth, expectedMockHeight, expectedZoom);
    };

    // Page size which matches the window size.
    testForSize(100, 100, 100, 100, 1);

    // Page size whose width is larger than its height.
    testForSize(200, 100, 100, 50, 0.5);

    // Page size whose height is larger than its width.
    testForSize(100, 200, 50, 100, 0.5);

    // Page size smaller than the window size in width but not height.
    testForSize(50, 100, 50, 100, 1);

    // Page size smaller than the window size in height but not width.
    testForSize(100, 50, 100, 50, 1);

    // Page size smaller than the window size in both width and height.
    testForSize(25, 50, 50, 100, 2);

    // Page size smaller in one dimension and bigger in another.
    testForSize(50, 200, 25, 100, 0.5);

    // Test that when there are multiple pages the height of the most visible
    // page and the width of the widest page are sized to.
    documentDimensions.reset();
    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(200, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 0);
    mockCallback.reset();
    viewport.fitToPage();
    assertZoomed(100, 250, 0.5);

    viewport.setZoom(1);
    mockWindow.scrollTo(0, 100);
    mockCallback.reset();
    viewport.fitToPage();
    assertZoomed(50, 125, 0.25);

    // Test that the top of the most visible page is scrolled to.
    documentDimensions.reset();
    documentDimensions.addPage(200, 200);
    documentDimensions.addPage(100, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 0);
    viewport.fitToPage();
    chrome.test.assertEq(FittingType.FIT_TO_PAGE, viewport.fittingType);
    chrome.test.assertEq(0.5, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 175);
    viewport.fitToPage();
    chrome.test.assertEq(0.25, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(50, viewport.position.y);

    // Test that when the window size changes, fit-to-page occurs but does not
    // scroll to the top of the page (it should stay at the scaled scroll
    // position).
    mockWindow.scrollTo(0, 0);
    viewport.fitToPage();
    chrome.test.assertEq(FittingType.FIT_TO_PAGE, viewport.fittingType);
    chrome.test.assertEq(0.5, viewport.zoom);
    mockWindow.scrollTo(0, 10);
    mockWindow.setSize(50, 50);
    chrome.test.assertEq(0.25, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(5, viewport.position.y);

    chrome.test.succeed();
  },

  function testFitToHeight() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(
        mockWindow, mockSizer, mockCallback.callback, function() {},
        function() {}, function() {}, 0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    function assertZoomed(expectedMockWidth, expectedMockHeight, expectedZoom) {
      chrome.test.assertEq(FittingType.FIT_TO_HEIGHT, viewport.fittingType);
      chrome.test.assertTrue(mockCallback.wasCalled);
      chrome.test.assertEq(`${expectedMockWidth}px`, mockSizer.style.width);
      chrome.test.assertEq(`${expectedMockHeight}px`, mockSizer.style.height);
      chrome.test.assertEq(expectedZoom, viewport.zoom);
    };

    function testForSize(
        pageWidth, pageHeight, expectedMockWidth, expectedMockHeight,
        expectedZoom) {
      documentDimensions.reset();
      documentDimensions.addPage(pageWidth, pageHeight);
      viewport.setDocumentDimensions(documentDimensions);
      viewport.setZoom(0.1);
      mockCallback.reset();
      viewport.fitToHeight();
      assertZoomed(expectedMockWidth, expectedMockHeight, expectedZoom);
    };

    // Page size which matches the window size.
    testForSize(100, 100, 100, 100, 1);

    // Page size wider than window but same height.
    testForSize(200, 100, 200, 100, 1);

    // Page size narrower than window but same height.
    testForSize(50, 100, 50, 100, 1);

    // Page size shorter than window.
    testForSize(100, 50, 200, 100, 2);

    // Page size taller than window.
    testForSize(100, 200, 50, 100, 0.5);

    // Test that when there are multiple pages the height of the most visible
    // page and the width of the widest page are sized to.
    documentDimensions.reset();
    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(200, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 0);
    chrome.test.assertEq(0, viewport.getMostVisiblePage());
    mockCallback.reset();
    viewport.fitToHeight();
    assertZoomed(200, 500, 1);

    viewport.setZoom(1);
    mockWindow.scrollTo(0, 100);
    chrome.test.assertEq(1, viewport.getMostVisiblePage());
    mockCallback.reset();
    viewport.fitToHeight();
    assertZoomed(50, 125, 0.25);

    // Test that the top of the most visible page is scrolled to.
    documentDimensions.reset();
    documentDimensions.addPage(200, 200);
    documentDimensions.addPage(100, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 0);
    chrome.test.assertEq(0, viewport.getMostVisiblePage());
    viewport.fitToHeight();
    chrome.test.assertEq(0, viewport.getMostVisiblePage());
    chrome.test.assertEq(FittingType.FIT_TO_HEIGHT, viewport.fittingType);
    chrome.test.assertEq(0.5, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);
    viewport.setZoom(1);
    mockWindow.scrollTo(0, 175);
    chrome.test.assertEq(1, viewport.getMostVisiblePage());
    viewport.fitToHeight();
    chrome.test.assertEq(1, viewport.getMostVisiblePage());
    chrome.test.assertEq(0.25, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(50, viewport.position.y);

    // Test that when the window size changes, fit-to-height occurs but does not
    // scroll to the top of the page (it should stay at the scaled scroll
    // position).
    mockWindow.scrollTo(0, 0);
    viewport.fitToHeight();
    chrome.test.assertEq(FittingType.FIT_TO_HEIGHT, viewport.fittingType);
    chrome.test.assertEq(0.5, viewport.zoom);
    mockWindow.scrollTo(0, 10);
    mockWindow.setSize(50, 50);
    chrome.test.assertEq(0.25, viewport.zoom);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(5, viewport.position.y);

    chrome.test.succeed();
  },

  function testGoToPage() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(200, 200);
    documentDimensions.addPage(100, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    mockCallback.reset();
    viewport.goToPage(0);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);

    mockCallback.reset();
    viewport.goToPage(1);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(100, viewport.position.y);

    mockCallback.reset();
    viewport.goToPage(2);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(300, viewport.position.y);

    viewport.setZoom(0.5);
    mockCallback.reset();
    viewport.goToPage(2);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(150, viewport.position.y);
    chrome.test.succeed();
  },

  function testGoToPageAndXY() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(200, 200);
    documentDimensions.addPage(100, 400);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    mockCallback.reset();
    viewport.goToPageAndXY(0, 0, 0);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);

    mockCallback.reset();
    viewport.goToPageAndXY(1, 0, 0);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(100, viewport.position.y);

    mockCallback.reset();
    viewport.goToPageAndXY(2, 42, 46);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0 + 42, viewport.position.x);
    chrome.test.assertEq(300 + 46, viewport.position.y);

    mockCallback.reset();
    viewport.goToPageAndXY(2, 42, 0);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0 + 42, viewport.position.x);
    chrome.test.assertEq(300, viewport.position.y);

    mockCallback.reset();
    viewport.goToPageAndXY(2, 0, 46);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(300 + 46, viewport.position.y);

    viewport.setZoom(0.5);
    mockCallback.reset();
    viewport.goToPageAndXY(2, 42, 46);
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(0 + 21, viewport.position.x);
    chrome.test.assertEq(150 + 23, viewport.position.y);
    chrome.test.succeed();
  },

  function testScrollTo() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    documentDimensions.addPage(200, 200);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);

    mockCallback.reset();
    viewport.scrollTo({x: 0, y: 0});
    chrome.test.assertFalse(mockCallback.wasCalled);
    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);

    mockCallback.reset();
    viewport.scrollTo({x: 10, y: 20});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(10, viewport.position.x);
    chrome.test.assertEq(20, viewport.position.y);

    mockCallback.reset();
    viewport.scrollTo({y: 30});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(10, viewport.position.x);
    chrome.test.assertEq(30, viewport.position.y);

    mockCallback.reset();
    viewport.scrollTo({y: 30});
    chrome.test.assertFalse(mockCallback.wasCalled);
    chrome.test.assertEq(10, viewport.position.x);
    chrome.test.assertEq(30, viewport.position.y);

    mockCallback.reset();
    viewport.scrollTo({x: 40});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(40, viewport.position.x);
    chrome.test.assertEq(30, viewport.position.y);

    mockCallback.reset();
    viewport.scrollTo({});
    chrome.test.assertFalse(mockCallback.wasCalled);
    chrome.test.assertEq(40, viewport.position.x);
    chrome.test.assertEq(30, viewport.position.y);

    chrome.test.succeed();
  },

  function testScrollBy() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();

    documentDimensions.addPage(200, 200);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    chrome.test.assertEq(0, viewport.position.x);
    chrome.test.assertEq(0, viewport.position.y);

    mockCallback.reset();
    viewport.scrollBy({x: 10, y: 20});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(10, viewport.position.x);
    chrome.test.assertEq(20, viewport.position.y);

    mockCallback.reset();
    viewport.scrollBy({x: 10, y: 20});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(20, viewport.position.x);
    chrome.test.assertEq(40, viewport.position.y);

    mockCallback.reset();
    viewport.scrollBy({x: -5, y: 0});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(15, viewport.position.x);
    chrome.test.assertEq(40, viewport.position.y);

    mockCallback.reset();
    viewport.scrollBy({x: 0, y: 60});
    chrome.test.assertTrue(mockCallback.wasCalled);
    chrome.test.assertEq(15, viewport.position.x);
    chrome.test.assertEq(100, viewport.position.y);

    mockCallback.reset();
    viewport.scrollBy({x: 0, y: 0});
    chrome.test.assertFalse(mockCallback.wasCalled);
    chrome.test.assertEq(15, viewport.position.x);
    chrome.test.assertEq(100, viewport.position.y);

    chrome.test.succeed();
  },

  function testGetPageScreenRect() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var mockCallback = new MockViewportChangedCallback();
    var viewport = new Viewport(mockWindow, mockSizer, mockCallback.callback,
                                function() {}, function() {}, function() {},
                                0, 1, 0);
    var documentDimensions = new MockDocumentDimensions();
    documentDimensions.addPage(100, 100);
    documentDimensions.addPage(200, 200);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    // Test that the rect of the first page is positioned/sized correctly.
    mockWindow.scrollTo(0, 0);
    var rect1 = viewport.getPageScreenRect(0);
    chrome.test.assertEq(Viewport.PAGE_SHADOW.left + 100 / 2, rect1.x);
    chrome.test.assertEq(Viewport.PAGE_SHADOW.top, rect1.y);
    chrome.test.assertEq(100 - Viewport.PAGE_SHADOW.right -
        Viewport.PAGE_SHADOW.left, rect1.width);
    chrome.test.assertEq(100 - Viewport.PAGE_SHADOW.bottom -
        Viewport.PAGE_SHADOW.top, rect1.height);

    // Check that when we scroll, the rect of the first page is updated
    // correctly.
    mockWindow.scrollTo(100, 10);
    var rect2 = viewport.getPageScreenRect(0);
    chrome.test.assertEq(rect1.x - 100, rect2.x);
    chrome.test.assertEq(rect1.y - 10, rect2.y);
    chrome.test.assertEq(rect1.width, rect2.width);
    chrome.test.assertEq(rect1.height, rect2.height);

    // Check the rect of the second page is positioned/sized correctly.
    mockWindow.scrollTo(0, 100);
    rect1 = viewport.getPageScreenRect(1);
    chrome.test.assertEq(Viewport.PAGE_SHADOW.left, rect1.x);
    chrome.test.assertEq(Viewport.PAGE_SHADOW.top, rect1.y);
    chrome.test.assertEq(200 - Viewport.PAGE_SHADOW.right -
        Viewport.PAGE_SHADOW.left, rect1.width);
    chrome.test.assertEq(200 - Viewport.PAGE_SHADOW.bottom -
        Viewport.PAGE_SHADOW.top, rect1.height);
    chrome.test.succeed();
  },

  function testBeforeZoomAfterZoom() {
    var mockWindow = new MockWindow(100, 100);
    var mockSizer = new MockSizer();
    var viewport;
    var afterZoomCalled = false;
    var beforeZoomCalled = false;
    var afterZoom = function() {
        afterZoomCalled = true;
        chrome.test.assertTrue(beforeZoomCalled);
        chrome.test.assertEq(0.5, viewport.zoom);
    };
    var beforeZoom = function() {
        beforeZoomCalled = true;
        chrome.test.assertFalse(afterZoomCalled);
        chrome.test.assertEq(1, viewport.zoom);
    };
    viewport = new Viewport(mockWindow, mockSizer, function() {},
                            beforeZoom, afterZoom, function() {}, 0, 1, 0);
    viewport.setZoom(0.5);
    chrome.test.succeed();
  },

  function testInitialSetDocumentDimensionsZoomConstrained() {
    var viewport =
        new Viewport(new MockWindow(100, 100), new MockSizer(), function() {},
                     function() {}, function() {}, function() {}, 0, 1.2, 0);
    viewport.setDocumentDimensions(new MockDocumentDimensions(50, 50));
    chrome.test.assertEq(1.2, viewport.zoom);
    chrome.test.succeed();
  },

  function testInitialSetDocumentDimensionsZoomUnconstrained() {
    var viewport = new Viewport(
        new MockWindow(100, 100),
        new MockSizer(), function() {}, function() {}, function() {},
        function() {}, 0, 3, 0);
    viewport.setDocumentDimensions(new MockDocumentDimensions(50, 50));
    chrome.test.assertEq(2, viewport.zoom);
    chrome.test.succeed();
  },

  function testToolbarHeightOffset() {
    var mockSizer = new MockSizer();
    var mockWindow = new MockWindow(100, 100);
    var viewport = new Viewport(mockWindow,
        mockSizer, function() {}, function() {}, function() {}, function() {},
        0, 1, 50);
    var documentDimensions = new MockDocumentDimensions(0, 0);
    documentDimensions.addPage(50, 500);
    viewport.setDocumentDimensions(documentDimensions);
    viewport.setZoom(1);

    // Check that the sizer incorporates the toolbar height.
    chrome.test.assertEq('550px', mockSizer.style.height);
    chrome.test.assertEq('50px', mockSizer.style.width);
    chrome.test.assertEq(0, viewport.position.x);

    // Check the sizer incorporates the toolbar height correctly even if zoomed.
    viewport.setZoom(2);
    chrome.test.assertEq('1050px', mockSizer.style.height);
    chrome.test.assertEq('100px', mockSizer.style.width);

    // Test that the viewport scrolls to the correct offset when fit-to-page is
    // enabled. The top of the viewport should be at the start of the document.
    viewport.fitToPage();
    chrome.test.assertEq(0, viewport.position.y);

    // Check that going to a page scrolls to the correct offset when fit-to-page
    // is enabled. The top of the viewport should be at the start of the
    // document.
    mockWindow.scrollTo(0, 100);
    viewport.goToPage(0);
    chrome.test.assertEq(0, viewport.position.y);

    // Check that going to a page scrolls to the correct offset when fit-to-page
    // is not enabled. The top of the viewport should be before start of the
    // document.
    viewport.setZoom(1);
    viewport.goToPage(0);
    chrome.test.assertEq(-50, viewport.position.y);
    chrome.test.succeed();
  }
];

chrome.test.runTests(tests);
