// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
  var searchBox = document.getElementById('search_input');
  var samples = document.getElementsByClassName('sample');
  var SEARCH_PREFIX = 'search:';

  function filterSamples() {
    var searchText = searchBox.value.toLowerCase();
    window.location.hash = SEARCH_PREFIX + encodeURIComponent(searchText);
    for (var i = 0; i < samples.length; ++i) {
      var sample = samples[i];
      var sampleTitle = '';
      if (sample.getElementsByTagName('h2').length > 0)
        sampleTitle = sample.getElementsByTagName('h2')[0].textContent;
      if (sample.getAttribute('tags').toLowerCase().indexOf(searchText) < 0 &&
          sampleTitle.toLowerCase().indexOf(searchText) < 0)
        sample.style.display = 'none';
      else
        sample.style.display = '';
    }
  }
  function updateSearchBox(value) {
    searchBox.value = value;
    filterSamples();
  }
  searchBox.addEventListener('search', filterSamples);
  searchBox.addEventListener('keyup', filterSamples);

  var apiFilterItems = document.getElementById('api_filter_items');
  apiFilterItems.addEventListener('click', function(event) {
    if (event.target instanceof HTMLAnchorElement) {
      updateSearchBox(event.target.innerText);
    }
  });

  // If we have a #fragment that corresponds to a search, prefill the search box
  // with it.
  var fragment = window.location.hash.substr(1);
  if (fragment.substr(0, SEARCH_PREFIX.length) == SEARCH_PREFIX) {
    updateSearchBox(decodeURIComponent(fragment.substr(SEARCH_PREFIX.length)));
  }
})();
