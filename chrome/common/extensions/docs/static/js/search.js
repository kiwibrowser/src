// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Activate the search box:
(function() {
  var form = document.getElementById('chrome-docs-cse-search-form');
  var searchInput = document.getElementById('chrome-docs-cse-input');

  var cx = '010997258251033819707:7owyldxmpkc';

  var gcse = document.createElement('script');
  gcse.type = 'text/javascript';
  gcse.async = true;
  gcse.src = (document.location.protocol == 'https:' ? 'https:' : 'http:') +
      '//www.google.com/cse/cse.js?cx=' + cx;
  var s = document.getElementsByTagName('script')[0];
  s.parentNode.insertBefore(gcse, s);

  var executeQuery = function(e) {
    var element = google.search.cse.element.getElement('results');
    if (searchInput.value == '') {
      element.clearAllResults();
    } else {
      element.execute(searchInput.value);
    }
    e.preventDefault();
    return true;
  }

  form.addEventListener('submit', executeQuery);

  // Attach autocomplete to the search box
  var enableAutoComplete = function() {
    google.search.CustomSearchControl.attachAutoCompletionWithOptions(
      cx, searchInput, form,
      // set to true to prevent the search box form from being submitted, since
      // the search control displaying the results is on the same page.
      {'preferOnSubmitToSubmit': true}
     );
  };

  var myAutocompleteCallback = function() {
    // Search module is loaded.
    if (document.readyState == 'complete') {
      enableAutoComplete();
    } else {
      google.setOnLoadCallback(enableAutoComplete, true);
    }
  };

  window.__gcse = {
    callback: myAutocompleteCallback
  };

})();
