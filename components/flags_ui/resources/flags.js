// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This variable structure is here to document the structure that the template
 * expects to correctly populate the page.
 */

/**
 * Takes the |experimentalFeaturesData| input argument which represents data
 * about all the current feature entries and populates the html jstemplate with
 * that data. It expects an object structure like the above.
 * @param {Object} experimentalFeaturesData Information about all experiments.
 *     See returnFlagsExperiments() for the structure of this object.
 */
function renderTemplate(experimentalFeaturesData) {
  // This is the javascript code that processes the template:
  jstProcess(new JsEvalContext(experimentalFeaturesData), $('flagsTemplate'));

  // Add handlers to dynamically created HTML elements.
  var elements = document.getElementsByClassName('experiment-select');
  for (var i = 0; i < elements.length; ++i) {
    elements[i].onchange = function() {
      handleSelectExperimentalFeatureChoice(this, this.selectedIndex);
      return false;
    };
  }

  elements = document.getElementsByClassName('experiment-enable-disable');
  for (var i = 0; i < elements.length; ++i) {
    elements[i].onchange = function() {
      handleEnableExperimentalFeature(this,
          this.options[this.selectedIndex].value == 'enabled');
      return false;
    };
  }

  elements = document.getElementsByClassName('experiment-origin-list-value');
  for (var i = 0; i < elements.length; ++i) {
    elements[i].onchange = function() {
      handleSetOriginListFlag(this, this.value);
      return false;
    };
  }

  elements = document.getElementsByClassName('experiment-restart-button');
  for (var i = 0; i < elements.length; ++i) {
    elements[i].onclick = restartBrowser;
  }

  // Tab panel selection.
  var tabEls = document.getElementsByClassName('tab');
  for (var i = 0; i < tabEls.length; ++i) {
    tabEls[i].addEventListener('click', function(e) {
      e.preventDefault();
      for (var j= 0; j < tabEls.length; ++j) {
        tabEls[j].parentNode.classList.toggle('selected', tabEls[j] == this);
      }
    });
  }

  var smallScreenCheck = window.matchMedia('(max-width: 480px)');
  // Toggling of experiment description overflow content on smaller screens.
  elements = document.querySelectorAll('.experiment .flex:first-child');
  for (var i = 0; i < elements.length; ++i) {
    elements[i].onclick = function(e) {
      if (smallScreenCheck.matches) {
        this.classList.toggle('expand');
      }
    };
  }

  $('experiment-reset-all').onclick = resetAllFlags;

  highlightReferencedFlag();
  var search = FlagSearch.getInstance();
  search.init();
}

/**
 * Highlight an element associated with the page's location's hash. We need to
 * fake fragment navigation with '.scrollIntoView()', since the fragment IDs
 * don't actually exist until after the template code runs; normal navigation
 * therefore doesn't work.
 */
function highlightReferencedFlag() {
  if (window.location.hash) {
    var el = document.querySelector(window.location.hash);
    if (el && !el.classList.contains('referenced')) {
      // Unhighlight whatever's highlighted.
      if (document.querySelector('.referenced'))
        document.querySelector('.referenced').classList.remove('referenced');
      // Highlight the referenced element.
      el.classList.add('referenced');

      // Switch to unavailable tab if the flag is in this section.
      if ($('tab-content-unavailable').contains(el)) {
        $('tab-available').parentNode.classList.remove('selected');
        $('tab-unavailable').parentNode.classList.add('selected');
      }
      el.scrollIntoView();
    }
  }
}

/**
 * Asks the C++ FlagsDOMHandler to get details about the available experimental
 * features and return detailed data about the configuration. The
 * FlagsDOMHandler should reply to returnFlagsExperiments() (below).
 */
function requestExperimentalFeaturesData() {
  chrome.send('requestExperimentalFeatures');
}

/**
 * Asks the C++ FlagsDOMHandler to restart the browser (restoring tabs).
 */
function restartBrowser() {
  chrome.send('restartBrowser');
}

/**
 * Reset all flags to their default values and refresh the UI.
 */
function resetAllFlags() {
  // Asks the C++ FlagsDOMHandler to reset all flags to default values.
  chrome.send('resetAllFlags');
  requestExperimentalFeaturesData();
}

/**
 * Called by the WebUI to re-populate the page with data representing the
 * current state of all experimental features.
 * @param {Object} experimentalFeaturesData Information about all experimental
 *    features in the following format:
 *   {
 *     supportedFeatures: [
 *       {
 *         internal_name: 'Feature ID string',
 *         name: 'Feature name',
 *         description: 'Description',
 *         // enabled and default are only set if the feature is single valued.
 *         // enabled is true if the feature is currently enabled.
 *         // is_default is true if the feature is in its default state.
 *         enabled: true,
 *         is_default: false,
 *         // choices is only set if the entry has multiple values.
 *         choices: [
 *           {
 *             internal_name: 'Experimental feature ID string',
 *             description: 'description',
 *             selected: true
 *           }
 *         ],
 *         supported_platforms: [
 *           'Mac',
 *           'Linux'
 *         ],
 *       }
 *     ],
 *     unsupportedFeatures: [
 *       // Mirrors the format of |supportedFeatures| above.
 *     ],
 *     needsRestart: false,
 *     showBetaChannelPromotion: false,
 *     showDevChannelPromotion: false,
 *     showOwnerWarning: false
 *   }
 */
function returnExperimentalFeatures(experimentalFeaturesData) {
  var bodyContainer = $('body-container');
  renderTemplate(experimentalFeaturesData);

  if (experimentalFeaturesData.showBetaChannelPromotion)
    $('channel-promo-beta').hidden = false;
  else if (experimentalFeaturesData.showDevChannelPromotion)
    $('channel-promo-dev').hidden = false;

  bodyContainer.style.visibility = 'visible';
  var ownerWarningDiv = $('owner-warning');
  if (ownerWarningDiv)
    ownerWarningDiv.hidden = !experimentalFeaturesData.showOwnerWarning;
}

/**
 * Handles updating the UI after experiment selections have been made.
 * Adds or removes experiment highlighting depending on whether the experiment
 * is set to the default option then shows the restart button.
 * @param {HTMLElement} node The select node for the experiment being changed.
 * @param {number} index The selected option index.
 */
function experimentChangesUiUpdates(node, index) {
  var selected = node.options[index];
  var experimentContainerEl = $(node.internal_name).firstElementChild;
  var isDefault =
      ("default" in selected.dataset && selected.dataset.default == "1") ||
      (!("default" in selected.dataset) && index === 0);
  experimentContainerEl.classList.toggle('experiment-default', isDefault);
  experimentContainerEl.classList.toggle('experiment-switched', !isDefault);

  $('needs-restart').classList.add('show');
}

/**
 * Handles a 'enable' or 'disable' button getting clicked.
 * @param {HTMLElement} node The node for the experiment being changed.
 * @param {boolean} enable Whether to enable or disable the experiment.
 */
function handleEnableExperimentalFeature(node, enable) {
  // Tell the C++ FlagsDOMHandler to enable/disable the experiment.
  chrome.send('enableExperimentalFeature', [String(node.internal_name),
                                            String(enable)]);
  experimentChangesUiUpdates(node, enable ? 1 : 0);
}

function handleSetOriginListFlag(node, value) {
  chrome.send('setOriginListFlag', [String(node.internal_name), String(value)]);
  $('needs-restart').classList.add('show');
}

/**
 * Invoked when the selection of a multi-value choice is changed to the
 * specified index.
 * @param {HTMLElement} node The node for the experiment being changed.
 * @param {number} index The index of the option that was selected.
 */
function handleSelectExperimentalFeatureChoice(node, index) {
  // Tell the C++ FlagsDOMHandler to enable the selected choice.
  chrome.send('enableExperimentalFeature',
              [String(node.internal_name) + '@' + index, 'true']);
  experimentChangesUiUpdates(node, index);
}

/**
 * Handles in page searching. Matches against the experiment flag name.
 */
var FlagSearch = function() {
  FlagSearch.instance_ = this;

  this.experiments_ = Object.assign({}, FlagSearch.SearchContent);
  this.unavailableExperiments_ = Object.assign({}, FlagSearch.SearchContent);

  this.searchBox_ = $('search');
  this.noMatchMsg_ = document.querySelectorAll('.no-match');

  this.searchIntervalId_ = null;
  this.initialized = false;
};

// Delay in ms following a keypress, before a search is made.
FlagSearch.SEARCH_DEBOUNCE_TIME_MS = 150;

/**
 * Object definition for storing the elements which are searched on.
 * @typedef {Object<string, HTMLElement[]>}
 */
FlagSearch.SearchContent = {
  link: [],
  title: [],
  description: []
};

/**
 * Get the singleton instance of FlagSearch.
 * @return {Object} Instance of FlagSearch.
 */
FlagSearch.getInstance = function() {
  if (FlagSearch.instance_) {
    return FlagSearch.instance_;
  } else {
    return new FlagSearch();
  }
};

FlagSearch.prototype = {
  /**
   * Initialises the in page search. Adding searchbox listeners and
   * collates the text elements used for string matching.
   */
  init: function() {
    this.experiments_.link =
        document.querySelectorAll('#tab-content-available .permalink');
    this.experiments_.title =
        document.querySelectorAll('#tab-content-available .experiment-name');
    this.experiments_.description =
        document.querySelectorAll('#tab-content-available p');

    this.unavailableExperiments_.link =
        document.querySelectorAll('#tab-content-unavailable .permalink');
    this.unavailableExperiments_.title =
        document.querySelectorAll('#tab-content-unavailable .experiment-name');
    this.unavailableExperiments_.description =
        document.querySelectorAll('#tab-content-unavailable p');

    if (!this.initialized) {
      this.searchBox_.addEventListener('keyup', this.debounceSearch.bind(this));
      document.querySelector('.clear-search').addEventListener('click',
          this.clearSearch.bind(this));

      window.addEventListener('keyup', function(e) {
          if (document.activeElement.nodeName == "TEXTAREA") {
            return;
          }
          switch(e.key) {
            case '/':
              this.searchBox_.focus();
              break;
            case 'Escape':
            case 'Enter':
              this.searchBox_.blur();
              break;
          }
      }.bind(this));
      this.searchBox_.focus();
      this.initialized = true;
    }
  },

  /**
   * Clears a search showing all experiments.
   */
  clearSearch: function() {
    this.searchBox_.value = '';
    this.doSearch();
  },

  /**
   * Reset existing highlights on an element.
   * @param {HTMLElement} el The element to remove all highlighted mark up on.
   * @param {string} text Text to reset the element's textContent to.
   */
  resetHighlights: function(el, text) {
    if (el.children) {
      el.textContent = text;
    }
  },

  /**
   * Highlights the search term within a given element.
   * @param {string} searchTerm Search term user entered.
   * @param {HTMLElement} el The node containing the text to match against.
   * @return {boolean} Whether there was a match.
   */
  highlightMatchInElement: function(searchTerm, el) {
    // Experiment container.
    var parentEl = el.parentNode.parentNode.parentNode;
    var text = el.textContent;
    var match = text.toLowerCase().indexOf(searchTerm);

    parentEl.classList.toggle('hidden', match == -1);

    if (match == -1) {
      this.resetHighlights(el, text);
      return false;
    }

    if (searchTerm != '') {
      // Clear all nodes.
      el.textContent = '';

      if (match > 0) {
        var textNodePrefix =
            document.createTextNode(text.substring(0, match));
        el.appendChild(textNodePrefix);
      }

      var matchEl = document.createElement('mark');
      matchEl.textContent = text.substr(match, searchTerm.length);
      el.appendChild(matchEl);

      var matchSuffix = text.substring(match + searchTerm.length);
      if (matchSuffix) {
        var textNodeSuffix = document.createTextNode(matchSuffix);
        el.appendChild(textNodeSuffix);
      }
    } else {
      this.resetHighlights(el, text);
    }
    return true;
  },

  /**
   * Goes through all experiment text and highlights the relevant matches.
   * Only the first instance of a match in each experiment text block is
   * highlighted. This prevents the sea of yellow that happens using the global
   * find in page search.
   * @param {FlagSearch.SearchContent} searchContent Object containing the
   *     experiment text elements to search against.
   * @param {string} searchTerm
   * @return {number} The number of matches found.
   */
  highlightAllMatches: function(searchContent, searchTerm) {
    var matches = 0;
    for (var i = 0, j = searchContent.link.length; i < j; i++) {
      if (this.highlightMatchInElement(searchTerm, searchContent.title[i])) {
        this.resetHighlights(searchContent.description[i],
            searchContent.description[i].textContent);
        this.resetHighlights(searchContent.link[i],
            searchContent.link[i].textContent);
        matches++;
        continue;
      }
      if (this.highlightMatchInElement(searchTerm,
          searchContent.description[i])) {
        this.resetHighlights(searchContent.title[i],
            searchContent.title[i].textContent);
        this.resetHighlights(searchContent.link[i],
            searchContent.link[i].textContent);
        matches++;
        continue;
      }
      // Match links, replace spaces with hyphens as flag names don't
      // have spaces.
      if (this.highlightMatchInElement(searchTerm.replace(/\s/, '-'),
          searchContent.link[i])) {
        this.resetHighlights(searchContent.title[i],
            searchContent.title[i].textContent);
        this.resetHighlights(searchContent.description[i],
            searchContent.description[i].textContent);
        matches++;
      }
    }
    return matches;
  },

  /**
   * Performs a search against the experiment title, description, permalink.
   * @param {Event} e
   */
  doSearch: function(e) {
    var searchTerm =
        this.searchBox_.value.trim().toLowerCase();

    if (searchTerm || searchTerm == '') {
      document.body.classList.toggle('searching', searchTerm);
      // Available experiments
      this.noMatchMsg_[0].classList.toggle('hidden',
          this.highlightAllMatches(this.experiments_, searchTerm));
      // Unavailable experiments
      this.noMatchMsg_[1].classList.toggle('hidden',
          this.highlightAllMatches(this.unavailableExperiments_, searchTerm));
    }

    this.searchIntervalId_ = null;
  },

  /**
   * Debounces the search to improve performance and prevent too many searches
   * from being initiated.
   * @param {Event} e
   */
  debounceSearch: function(e) {
    // Don't search if the search term did not change.
    if (this.searchValue_ == this.searchBox_.value) {
      return;
    }

    if (this.searchIntervalId_) {
      clearTimeout(this.searchIntervalId_);
    }
    this.searchIntervalId_ = setTimeout(this.doSearch.bind(this),
        FlagSearch.SEARCH_DEBOUNCE_TIME_MS);
  }
};

// Get and display the data upon loading.
document.addEventListener('DOMContentLoaded', requestExperimentalFeaturesData);

// Update the highlighted flag when the hash changes.
window.addEventListener('hashchange', highlightReferencedFlag);
