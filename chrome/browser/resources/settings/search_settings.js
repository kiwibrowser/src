// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('settings');

/**
 * A data structure used by callers to combine the results of multiple search
 * requests.
 *
 * @typedef {{
 *   canceled: Boolean,
 *   didFindMatches: Boolean,
 *   wasClearSearch: Boolean,
 * }}
 */
settings.SearchResult;

cr.define('settings', function() {
  /**
   * A CSS attribute indicating that a node should be ignored during searching.
   * @type {string}
   */
  const SKIP_SEARCH_CSS_ATTRIBUTE = 'no-search';

  /**
   * List of elements types that should not be searched at all.
   * The only DOM-MODULE node is in <body> which is not searched, therefore
   * DOM-MODULE is not needed in this set.
   * @type {!Set<string>}
   */
  const IGNORED_ELEMENTS = new Set([
    'CONTENT',
    'CR-EVENTS',
    'DIALOG',
    'IMG',
    'IRON-ICON',
    'IRON-LIST',
    'PAPER-ICON-BUTTON',
    'PAPER-RIPPLE',
    'PAPER-SLIDER',
    'PAPER-SPINNER-LITE',
    'STYLE',
    'TEMPLATE',
  ]);

  /**
   * Finds all previous highlighted nodes under |node| (both within self and
   * children's Shadow DOM) and replaces the highlights (yellow rectangle and
   * search bubbles) with the original text node.
   * TODO(dpapad): Consider making this a private method of TopLevelSearchTask.
   * @param {!Node} node
   * @private
   */
  function findAndRemoveHighlights_(node) {
    cr.search_highlight_utils.findAndRemoveHighlights(node);
    cr.search_highlight_utils.findAndRemoveBubbles(node);
  }

  /**
   * Traverses the entire DOM (including Shadow DOM), finds text nodes that
   * match the given regular expression and applies the highlight UI. It also
   * ensures that <settings-section> instances become visible if any matches
   * occurred under their subtree.
   *
   * @param {!settings.SearchRequest} request
   * @param {!Node} root The root of the sub-tree to be searched
   * @private
   */
  function findAndHighlightMatches_(request, root) {
    let foundMatches = false;
    function doSearch(node) {
      if (node.nodeName == 'TEMPLATE' && node.hasAttribute('route-path') &&
          !node.if && !node.hasAttribute(SKIP_SEARCH_CSS_ATTRIBUTE)) {
        request.queue_.addRenderTask(new RenderTask(request, node));
        return;
      }

      if (IGNORED_ELEMENTS.has(node.nodeName))
        return;

      if (node instanceof HTMLElement) {
        const element = /** @type {HTMLElement} */ (node);
        if (element.hasAttribute(SKIP_SEARCH_CSS_ATTRIBUTE) ||
            element.hasAttribute('hidden') || element.style.display == 'none') {
          return;
        }
      }

      if (node.nodeType == Node.TEXT_NODE) {
        const textContent = node.nodeValue.trim();
        if (textContent.length == 0)
          return;

        if (request.regExp.test(textContent)) {
          foundMatches = true;
          revealParentSection_(node, request.rawQuery_);

          // Don't highlight <select> nodes, yellow rectangles can't be
          // displayed within an <option>.
          // TODO(dpapad): highlight <select> controls with a search bubble
          // instead.
          if (node.parentNode.nodeName != 'OPTION') {
            request.addTextObserver(node);
            cr.search_highlight_utils.highlight(
                node, textContent.split(request.regExp));
          }
        }
        // Returning early since TEXT_NODE nodes never have children.
        return;
      }

      let child = node.firstChild;
      while (child !== null) {
        // Getting a reference to the |nextSibling| before calling doSearch()
        // because |child| could be removed from the DOM within doSearch().
        const nextSibling = child.nextSibling;
        doSearch(child);
        child = nextSibling;
      }

      const shadowRoot = node.shadowRoot;
      if (shadowRoot)
        doSearch(shadowRoot);
    }

    doSearch(root);
    return foundMatches;
  }

  /**
   * Finds and makes visible the <settings-section> parent of |node|.
   * @param {!Node} node
   * @param {string} rawQuery
   * @private
   */
  function revealParentSection_(node, rawQuery) {
    let associatedControl = null;
    // Find corresponding SETTINGS-SECTION parent and make it visible.
    let parent = node;
    while (parent && parent.nodeName !== 'SETTINGS-SECTION') {
      parent = parent.nodeType == Node.DOCUMENT_FRAGMENT_NODE ?
          parent.host :
          parent.parentNode;
      if (parent.nodeName == 'SETTINGS-SUBPAGE') {
        // TODO(dpapad): Cast to SettingsSubpageElement here.
        associatedControl = assert(
            parent.associatedControl,
            'An associated control was expected for SETTINGS-SUBPAGE ' +
                parent.pageTitle + ', but was not found.');
      }
    }
    if (parent)
      parent.hiddenBySearch = false;

    // Need to add the search bubble after the parent SETTINGS-SECTION has
    // become visible, otherwise |offsetWidth| returns zero.
    if (associatedControl) {
      cr.search_highlight_utils.highlightControlWithBubble(
          associatedControl, rawQuery);
    }
  }

  /** @abstract */
  class Task {
    /**
     * @param {!settings.SearchRequest} request
     * @param {!Node} node
     */
    constructor(request, node) {
      /** @protected {!settings.SearchRequest} */
      this.request = request;

      /** @protected {!Node} */
      this.node = node;
    }

    /**
     * @abstract
     * @return {!Promise}
     */
    exec() {}
  }

  class RenderTask extends Task {
    /**
     * A task that takes a <template is="dom-if">...</template> node
     * corresponding to a setting subpage and renders it. A
     * SearchAndHighlightTask is posted for the newly rendered subtree, once
     * rendering is done.
     *
     * @param {!settings.SearchRequest} request
     * @param {!Node} node
     */
    constructor(request, node) {
      super(request, node);
    }

    /** @override */
    exec() {
      const routePath = this.node.getAttribute('route-path');
      const subpageTemplate =
          this.node['_content'].querySelector('settings-subpage');
      subpageTemplate.setAttribute('route-path', routePath);
      assert(!this.node.if);
      this.node.if = true;

      return new Promise((resolve, reject) => {
        const parent = this.node.parentNode;
        parent.async(() => {
          const renderedNode =
              parent.querySelector('[route-path="' + routePath + '"]');
          // Register a SearchAndHighlightTask for the part of the DOM that was
          // just rendered.
          this.request.queue_.addSearchAndHighlightTask(
              new SearchAndHighlightTask(this.request, assert(renderedNode)));
          resolve();
        });
      });
    }
  }

  class SearchAndHighlightTask extends Task {
    /**
     * @param {!settings.SearchRequest} request
     * @param {!Node} node
     */
    constructor(request, node) {
      super(request, node);
    }

    /** @override */
    exec() {
      const foundMatches = findAndHighlightMatches_(this.request, this.node);
      this.request.updateMatches(foundMatches);
      return Promise.resolve();
    }
  }

  class TopLevelSearchTask extends Task {
    /**
     * @param {!settings.SearchRequest} request
     * @param {!Node} page
     */
    constructor(request, page) {
      super(request, page);
    }

    /** @override */
    exec() {
      findAndRemoveHighlights_(this.node);

      const shouldSearch = this.request.regExp !== null;
      this.setSectionsVisibility_(!shouldSearch);
      if (shouldSearch) {
        const foundMatches = findAndHighlightMatches_(this.request, this.node);
        this.request.updateMatches(foundMatches);
      }

      return Promise.resolve();
    }

    /**
     * @param {boolean} visible
     * @private
     */
    setSectionsVisibility_(visible) {
      const sections = this.node.querySelectorAll('settings-section');

      for (let i = 0; i < sections.length; i++)
        sections[i].hiddenBySearch = !visible;
    }
  }

  class TaskQueue {
    /** @param {!settings.SearchRequest} request */
    constructor(request) {
      /** @private {!settings.SearchRequest} */
      this.request_ = request;

      /**
       * @private {{
       *   high: !Array<!Task>,
       *   middle: !Array<!Task>,
       *   low: !Array<!Task>
       * }}
       */
      this.queues_;
      this.reset();

      /** @private {?Function} */
      this.onEmptyCallback_ = null;

      /**
       * Whether a task is currently running.
       * @private {boolean}
       */
      this.running_ = false;
    }

    /** Drops all tasks. */
    reset() {
      this.queues_ = {high: [], middle: [], low: []};
    }

    /** @param {!TopLevelSearchTask} task */
    addTopLevelSearchTask(task) {
      this.queues_.high.push(task);
      this.consumePending_();
    }

    /** @param {!SearchAndHighlightTask} task */
    addSearchAndHighlightTask(task) {
      this.queues_.middle.push(task);
      this.consumePending_();
    }

    /** @param {!RenderTask} task */
    addRenderTask(task) {
      this.queues_.low.push(task);
      this.consumePending_();
    }

    /**
     * Registers a callback to be called every time the queue becomes empty.
     * @param {function():void} onEmptyCallback
     */
    onEmpty(onEmptyCallback) {
      this.onEmptyCallback_ = onEmptyCallback;
    }

    /**
     * @return {!Task|undefined}
     * @private
     */
    popNextTask_() {
      return this.queues_.high.shift() || this.queues_.middle.shift() ||
          this.queues_.low.shift();
    }

    /** @private */
    consumePending_() {
      if (this.running_)
        return;

      const task = this.popNextTask_();
      if (!task) {
        this.running_ = false;
        if (this.onEmptyCallback_)
          this.onEmptyCallback_();
        return;
      }

      this.running_ = true;
      window.requestIdleCallback(() => {
        if (!this.request_.canceled) {
          task.exec().then(() => {
            this.running_ = false;
            this.consumePending_();
          });
        }
        // Nothing to do otherwise. Since the request corresponding to this
        // queue was canceled, the queue is disposed along with the request.
      });
    }
  }

  class SearchRequest {
    /**
     * @param {string} rawQuery
     * @param {!HTMLElement} root
     */
    constructor(rawQuery, root) {
      /** @private {string} */
      this.rawQuery_ = rawQuery;

      /** @private {!HTMLElement} */
      this.root_ = root;

      /** @type {?RegExp} */
      this.regExp = this.generateRegExp_();

      /**
       * Whether this request was canceled before completing.
       * @type {boolean}
       */
      this.canceled = false;

      /** @private {boolean} */
      this.foundMatches_ = false;

      /** @type {!PromiseResolver} */
      this.resolver = new PromiseResolver();

      /** @private {!TaskQueue} */
      this.queue_ = new TaskQueue(this);
      this.queue_.onEmpty(() => {
        this.resolver.resolve(this);
      });

      /** @private {!Set<!MutationObserver>} */
      this.textObservers_ = new Set();
    }

    removeAllTextObservers() {
      this.textObservers_.forEach(observer => {
        observer.disconnect();
      });
      this.textObservers_.clear();
    }

    /** @param {!Node} textNode */
    addTextObserver(textNode) {
      const originalParentNode = /** @type {!Node} */ (textNode.parentNode);
      const observer = new MutationObserver(mutations => {
        const oldValue = mutations[0].oldValue.trim();
        const newValue = textNode.nodeValue.trim();
        if (oldValue != newValue) {
          observer.disconnect();
          this.textObservers_.delete(observer);
          cr.search_highlight_utils.findAndRemoveHighlights(originalParentNode);
        }
      });
      observer.observe(
          textNode, {characterData: true, characterDataOldValue: true});
      this.textObservers_.add(observer);
    }

    /**
     * Fires this search request.
     */
    start() {
      this.queue_.addTopLevelSearchTask(
          new TopLevelSearchTask(this, this.root_));
    }

    /**
     * @return {?RegExp}
     * @private
     */
    generateRegExp_() {
      let regExp = null;

      // Generate search text by escaping any characters that would be
      // problematic for regular expressions.
      const searchText = this.rawQuery_.trim().replace(SANITIZE_REGEX, '\\$&');
      if (searchText.length > 0)
        regExp = new RegExp(`(${searchText})`, 'i');

      return regExp;
    }

    /**
     * @param {string} rawQuery
     * @return {boolean} Whether this SearchRequest refers to an identical
     *     query.
     */
    isSame(rawQuery) {
      return this.rawQuery_ == rawQuery;
    }

    /**
     * Updates the result for this search request.
     * @param {boolean} found
     */
    updateMatches(found) {
      this.foundMatches_ = this.foundMatches_ || found;
    }

    /** @return {boolean} Whether any matches were found. */
    didFindMatches() {
      return this.foundMatches_;
    }
  }

  /** @type {!RegExp} */
  const SANITIZE_REGEX = /[-[\]{}()*+?.,\\^$|#\s]/g;

  /** @interface */
  class SearchManager {
    /**
     * @param {string} text The text to search for.
     * @param {!Node} page
     * @return {!Promise<!settings.SearchRequest>} A signal indicating that
     *     searching finished.
     */
    search(text, page) {}
  }

  /** @implements {SearchManager} */
  class SearchManagerImpl {
    constructor() {
      /** @private {!Set<!settings.SearchRequest>} */
      this.activeRequests_ = new Set();

      /** @private {!Set<!settings.SearchRequest>} */
      this.completedRequests_ = new Set();

      /** @private {?string} */
      this.lastSearchedText_ = null;
    }

    /** @override */
    search(text, page) {
      // Cancel any pending requests if a request with different text is
      // submitted.
      if (text != this.lastSearchedText_) {
        this.activeRequests_.forEach(function(request) {
          request.removeAllTextObservers();
          request.canceled = true;
          request.resolver.resolve(request);
        });
        this.activeRequests_.clear();
        this.completedRequests_.forEach(request => {
          request.removeAllTextObservers();
        });
        this.completedRequests_.clear();
      }

      this.lastSearchedText_ = text;
      const request = new SearchRequest(text, page);
      this.activeRequests_.add(request);
      request.start();
      return request.resolver.promise.then(() => {
        this.activeRequests_.delete(request);
        this.completedRequests_.add(request);
        return request;
      });
    }
  }
  cr.addSingletonGetter(SearchManagerImpl);

  /** @return {!SearchManager} */
  function getSearchManager() {
    return SearchManagerImpl.getInstance();
  }

  /**
   * Sets the SearchManager singleton instance, useful for testing.
   * @param {!SearchManager} searchManager
   */
  function setSearchManagerForTesting(searchManager) {
    SearchManagerImpl.instance_ = searchManager;
  }

  return {
    getSearchManager: getSearchManager,
    setSearchManagerForTesting: setSearchManagerForTesting,
    SearchRequest: SearchRequest,
  };
});
