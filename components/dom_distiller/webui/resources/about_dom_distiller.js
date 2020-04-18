// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var domDistiller = {
  /**
   * Callback from the backend with the list of entries to display.
   * This call will build the entries section of the DOM distiller page, or hide
   * that section if there are none to display.
   * @param {!Array<string>} entries The entries.
   */
  onReceivedEntries: function(entries) {
    $('entries-list-loading').classList.add('hidden');
    if (!entries.length) $('entries-list').classList.add('hidden');

    var list = $('entries-list');
    domDistiller.removeAllChildren(list);
    for (var i = 0; i < entries.length; i++) {
      var listItem = document.createElement('li');
      var link = document.createElement('a');
      var entry_id = entries[i].entry_id;
      link.setAttribute('id', 'entry-' + entry_id);
      link.setAttribute('href', '#');
      link.innerText = entries[i].title;
      link.addEventListener('click', function(event) {
        domDistiller.onSelectArticle(event.target.id.substr("entry-".length));
      }, true);
      listItem.appendChild(link);
      list.appendChild(listItem);
    }
  },

  /**
   * Callback from the backend when adding an article failed.
   */
  onArticleAddFailed: function() {
    $('add-entry-error').classList.remove('hidden');
  },

  /**
   * Callback from the backend when viewing a URL failed.
   */
  onViewUrlFailed: function() {
    $('view-url-error').classList.remove('hidden');
  },

  removeAllChildren: function(root) {
    while(root.firstChild) {
      root.removeChild(root.firstChild);
    }
  },

  /**
   * Sends a request to the browser process to add the URL specified to the list
   * of articles.
   */
  onAddArticle: function() {
    $('add-entry-error').classList.add('hidden');
    var url = $('article_url').value;
    chrome.send('addArticle', [url]);
  },

  /**
   * Sends a request to the browser process to view a distilled version of the
   * URL specified.
   */
  onViewUrl: function() {
    $('view-url-error').classList.add('hidden');
    var url = $('article_url').value;
    chrome.send('viewUrl', [url]);
  },

  /**
   * Sends a request to the browser process to view a distilled version of the
   * selected article.
   */
  onSelectArticle: function(articleId) {
    chrome.send('selectArticle', [articleId]);
  },

  /* All the work we do on load. */
  onLoadWork: function() {
    $('list-section').classList.remove('hidden');
    $('entries-list-loading').classList.add('hidden');
    $('add-entry-error').classList.add('hidden');
    $('view-url-error').classList.add('hidden');

    $('refreshbutton').addEventListener('click', function(event) {
      domDistiller.onRequestEntries();
    }, false);
    $('addbutton').addEventListener('click', function(event) {
      domDistiller.onAddArticle();
    }, false);
    $('viewbutton').addEventListener('click', function(event) {
      domDistiller.onViewUrl();
    }, false);
    domDistiller.onRequestEntries();
  },

  onRequestEntries: function() {
    $('entries-list-loading').classList.remove('hidden');
    chrome.send('requestEntries');
  },
}

document.addEventListener('DOMContentLoaded', domDistiller.onLoadWork);
