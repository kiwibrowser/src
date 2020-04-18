// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Rendering for iframed most visited thumbnails.
 */

window.addEventListener('DOMContentLoaded', function() {
  'use strict';

  fillMostVisited(document.location, function(params, data) {
    function displayLink(link) {
      document.body.appendChild(link);
      window.parent.postMessage('linkDisplayed', '{{ORIGIN}}');
    }
    function showDomainElement() {
      var link = createMostVisitedLink(
          params, data.url, data.title, undefined, data.direction);
      var domain = document.createElement('div');
      domain.textContent = data.domain;
      link.appendChild(domain);
      displayLink(link);
    }
    // Called on intentionally empty tiles for which the visuals are handled
    // externally by the page itself.
    function showEmptyTile() {
      displayLink(createMostVisitedLink(
          params, data.url, data.title, undefined, data.direction));
    }
    // Creates and adds an image.
    function createThumbnail(src, imageClass) {
      var image = document.createElement('img');
      if (imageClass) {
        image.classList.add(imageClass);
      }
      image.onload = function() {
        var link = createMostVisitedLink(
            params, data.url, data.title, undefined, data.direction);
        // Use blocker to prevent context menu from showing image-related items.
        var blocker = document.createElement('span');
        blocker.className = 'blocker';
        link.appendChild(blocker);
        link.appendChild(image);
        displayLink(link);
      };
      image.onerror = function() {
        // If no external thumbnail fallback (etfb), and have domain.
        if (!params.etfb && data.domain) {
          showDomainElement();
        } else {
          showEmptyTile();
        }
      };
      image.src = src;
    }

    if (data.dummy) {
      showEmptyTile();
    } else if (data.thumbnailUrl) {
      createThumbnail(data.thumbnailUrl, 'thumbnail');
    } else if (data.domain) {
      showDomainElement();
    } else {
      showEmptyTile();
    }
  });
});
