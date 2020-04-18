// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <include src="assert.js">

/**
 * Alias for document.getElementById. Found elements must be HTMLElements.
 * @param {string} id The ID of the element to find.
 * @return {HTMLElement} The found element or null if not found.
 */
function $(id) {
  var el = document.getElementById(id);
  return el ? assertInstanceof(el, HTMLElement) : null;
}

// TODO(devlin): This should return SVGElement, but closure compiler is missing
// those externs.
/**
 * Alias for document.getElementById. Found elements must be SVGElements.
 * @param {string} id The ID of the element to find.
 * @return {Element} The found element or null if not found.
 */
function getSVGElement(id) {
  var el = document.getElementById(id);
  return el ? assertInstanceof(el, Element) : null;
}

/**
 * Add an accessible message to the page that will be announced to
 * users who have spoken feedback on, but will be invisible to all
 * other users. It's removed right away so it doesn't clutter the DOM.
 * @param {string} msg The text to be pronounced.
 */
function announceAccessibleMessage(msg) {
  var element = document.createElement('div');
  element.setAttribute('aria-live', 'polite');
  element.style.position = 'relative';
  element.style.left = '-9999px';
  element.style.height = '0px';
  element.innerText = msg;
  document.body.appendChild(element);
  window.setTimeout(function() {
    document.body.removeChild(element);
  }, 0);
}

/**
 * Generates a CSS url string.
 * @param {string} s The URL to generate the CSS url for.
 * @return {string} The CSS url string.
 */
function url(s) {
  // http://www.w3.org/TR/css3-values/#uris
  // Parentheses, commas, whitespace characters, single quotes (') and double
  // quotes (") appearing in a URI must be escaped with a backslash
  var s2 = s.replace(/(\(|\)|\,|\s|\'|\"|\\)/g, '\\$1');
  // WebKit has a bug when it comes to URLs that end with \
  // https://bugs.webkit.org/show_bug.cgi?id=28885
  if (/\\\\$/.test(s2)) {
    // Add a space to work around the WebKit bug.
    s2 += ' ';
  }
  return 'url("' + s2 + '")';
}

/**
 * Parses query parameters from Location.
 * @param {Location} location The URL to generate the CSS url for.
 * @return {Object} Dictionary containing name value pairs for URL
 */
function parseQueryParams(location) {
  var params = {};
  var query = unescape(location.search.substring(1));
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    params[pair[0]] = pair[1];
  }
  return params;
}

/**
 * Creates a new URL by appending or replacing the given query key and value.
 * Not supporting URL with username and password.
 * @param {Location} location The original URL.
 * @param {string} key The query parameter name.
 * @param {string} value The query parameter value.
 * @return {string} The constructed new URL.
 */
function setQueryParam(location, key, value) {
  var query = parseQueryParams(location);
  query[encodeURIComponent(key)] = encodeURIComponent(value);

  var newQuery = '';
  for (var q in query) {
    newQuery += (newQuery ? '&' : '?') + q + '=' + query[q];
  }

  return location.origin + location.pathname + newQuery + location.hash;
}

/**
 * @param {Node} el A node to search for ancestors with |className|.
 * @param {string} className A class to search for.
 * @return {Element} A node with class of |className| or null if none is found.
 */
function findAncestorByClass(el, className) {
  return /** @type {Element} */(findAncestor(el, function(el) {
    return el.classList && el.classList.contains(className);
  }));
}

/**
 * Return the first ancestor for which the {@code predicate} returns true.
 * @param {Node} node The node to check.
 * @param {function(Node):boolean} predicate The function that tests the
 *     nodes.
 * @return {Node} The found ancestor or null if not found.
 */
function findAncestor(node, predicate) {
  var last = false;
  while (node != null && !(last = predicate(node))) {
    node = node.parentNode;
  }
  return last ? node : null;
}

function swapDomNodes(a, b) {
  var afterA = a.nextSibling;
  if (afterA == b) {
    swapDomNodes(b, a);
    return;
  }
  var aParent = a.parentNode;
  b.parentNode.replaceChild(a, b);
  aParent.insertBefore(b, afterA);
}

/**
 * Disables text selection and dragging, with optional whitelist callbacks.
 * @param {function(Event):boolean=} opt_allowSelectStart Unless this function
 *    is defined and returns true, the onselectionstart event will be
 *    surpressed.
 * @param {function(Event):boolean=} opt_allowDragStart Unless this function
 *    is defined and returns true, the ondragstart event will be surpressed.
 */
function disableTextSelectAndDrag(opt_allowSelectStart, opt_allowDragStart) {
  // Disable text selection.
  document.onselectstart = function(e) {
    if (!(opt_allowSelectStart && opt_allowSelectStart.call(this, e)))
      e.preventDefault();
  };

  // Disable dragging.
  document.ondragstart = function(e) {
    if (!(opt_allowDragStart && opt_allowDragStart.call(this, e)))
      e.preventDefault();
  };
}

/**
 * TODO(dbeam): DO NOT USE. THIS IS DEPRECATED. Use an action-link instead.
 * Call this to stop clicks on <a href="#"> links from scrolling to the top of
 * the page (and possibly showing a # in the link).
 */
function preventDefaultOnPoundLinkClicks() {
  document.addEventListener('click', function(e) {
    var anchor = findAncestor(/** @type {Node} */(e.target), function(el) {
      return el.tagName == 'A';
    });
    // Use getAttribute() to prevent URL normalization.
    if (anchor && anchor.getAttribute('href') == '#')
      e.preventDefault();
  });
}

/**
 * Check the directionality of the page.
 * @return {boolean} True if Chrome is running an RTL UI.
 */
function isRTL() {
  return document.documentElement.dir == 'rtl';
}

/**
 * Get an element that's known to exist by its ID. We use this instead of just
 * calling getElementById and not checking the result because this lets us
 * satisfy the JSCompiler type system.
 * @param {string} id The identifier name.
 * @return {!HTMLElement} the Element.
 */
function getRequiredElement(id) {
  return assertInstanceof($(id), HTMLElement,
                          'Missing required element: ' + id);
}

/**
 * Query an element that's known to exist by a selector. We use this instead of
 * just calling querySelector and not checking the result because this lets us
 * satisfy the JSCompiler type system.
 * @param {string} selectors CSS selectors to query the element.
 * @param {(!Document|!DocumentFragment|!Element)=} opt_context An optional
 *     context object for querySelector.
 * @return {!HTMLElement} the Element.
 */
function queryRequiredElement(selectors, opt_context) {
  var element = (opt_context || document).querySelector(selectors);
  return assertInstanceof(element, HTMLElement,
                          'Missing required element: ' + selectors);
}

// Handle click on a link. If the link points to a chrome: or file: url, then
// call into the browser to do the navigation.
document.addEventListener('click', function(e) {
  if (e.defaultPrevented)
    return;

  var el = e.target;
  if (el.nodeType == Node.ELEMENT_NODE &&
      el.webkitMatchesSelector('A, A *')) {
    while (el.tagName != 'A') {
      el = el.parentElement;
    }

    if ((el.protocol == 'file:' || el.protocol == 'about:') &&
        (e.button == 0 || e.button == 1)) {
      chrome.send('navigateToUrl', [
        el.href,
        el.target,
        e.button,
        e.altKey,
        e.ctrlKey,
        e.metaKey,
        e.shiftKey
      ]);
      e.preventDefault();
    }
  }
});

/**
 * Creates a new URL which is the old URL with a GET param of key=value.
 * @param {string} url The base URL. There is not sanity checking on the URL so
 *     it must be passed in a proper format.
 * @param {string} key The key of the param.
 * @param {string} value The value of the param.
 * @return {string} The new URL.
 */
function appendParam(url, key, value) {
  var param = encodeURIComponent(key) + '=' + encodeURIComponent(value);

  if (url.indexOf('?') == -1)
    return url + '?' + param;
  return url + '&' + param;
}

/**
 * Creates an element of a specified type with a specified class name.
 * @param {string} type The node type.
 * @param {string} className The class name to use.
 * @return {Element} The created element.
 */
function createElementWithClassName(type, className) {
  var elm = document.createElement(type);
  elm.className = className;
  return elm;
}

/**
 * webkitTransitionEnd does not always fire (e.g. when animation is aborted
 * or when no paint happens during the animation). This function sets up
 * a timer and emulate the event if it is not fired when the timer expires.
 * @param {!HTMLElement} el The element to watch for webkitTransitionEnd.
 * @param {number=} opt_timeOut The maximum wait time in milliseconds for the
 *     webkitTransitionEnd to happen. If not specified, it is fetched from |el|
 *     using the transitionDuration style value.
 */
function ensureTransitionEndEvent(el, opt_timeOut) {
  if (opt_timeOut === undefined) {
    var style = getComputedStyle(el);
    opt_timeOut = parseFloat(style.transitionDuration) * 1000;

    // Give an additional 50ms buffer for the animation to complete.
    opt_timeOut += 50;
  }

  var fired = false;
  el.addEventListener('webkitTransitionEnd', function f(e) {
    el.removeEventListener('webkitTransitionEnd', f);
    fired = true;
  });
  window.setTimeout(function() {
    if (!fired)
      cr.dispatchSimpleEvent(el, 'webkitTransitionEnd', true);
  }, opt_timeOut);
}

/**
 * Alias for document.scrollTop getter.
 * @param {!HTMLDocument} doc The document node where information will be
 *     queried from.
 * @return {number} The Y document scroll offset.
 */
function scrollTopForDocument(doc) {
  return doc.documentElement.scrollTop || doc.body.scrollTop;
}

/**
 * Alias for document.scrollTop setter.
 * @param {!HTMLDocument} doc The document node where information will be
 *     queried from.
 * @param {number} value The target Y scroll offset.
 */
function setScrollTopForDocument(doc, value) {
  doc.documentElement.scrollTop = doc.body.scrollTop = value;
}

/**
 * Alias for document.scrollLeft getter.
 * @param {!HTMLDocument} doc The document node where information will be
 *     queried from.
 * @return {number} The X document scroll offset.
 */
function scrollLeftForDocument(doc) {
  return doc.documentElement.scrollLeft || doc.body.scrollLeft;
}

/**
 * Alias for document.scrollLeft setter.
 * @param {!HTMLDocument} doc The document node where information will be
 *     queried from.
 * @param {number} value The target X scroll offset.
 */
function setScrollLeftForDocument(doc, value) {
  doc.documentElement.scrollLeft = doc.body.scrollLeft = value;
}

/**
 * Replaces '&', '<', '>', '"', and ''' characters with their HTML encoding.
 * @param {string} original The original string.
 * @return {string} The string with all the characters mentioned above replaced.
 */
function HTMLEscape(original) {
  return original.replace(/&/g, '&amp;')
                 .replace(/</g, '&lt;')
                 .replace(/>/g, '&gt;')
                 .replace(/"/g, '&quot;')
                 .replace(/'/g, '&#39;');
}

/**
 * Shortens the provided string (if necessary) to a string of length at most
 * |maxLength|.
 * @param {string} original The original string.
 * @param {number} maxLength The maximum length allowed for the string.
 * @return {string} The original string if its length does not exceed
 *     |maxLength|. Otherwise the first |maxLength| - 1 characters with '...'
 *     appended.
 */
function elide(original, maxLength) {
  if (original.length <= maxLength)
    return original;
  return original.substring(0, maxLength - 1) + '\u2026';
}

/**
 * Quote a string so it can be used in a regular expression.
 * @param {string} str The source string.
 * @return {string} The escaped string.
 */
function quoteString(str) {
  return str.replace(/([\\\.\+\*\?\[\^\]\$\(\)\{\}\=\!\<\>\|\:])/g, '\\$1');
}
