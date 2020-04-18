// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// On iOS, |distiller_on_ios| was set to true before this script.
var distiller_on_ios;
if (typeof distiller_on_ios === 'undefined') {distiller_on_ios = false;}

function addToPage(html) {
  var div = document.createElement('div');
  div.innerHTML = html;
  document.getElementById('content').appendChild(div);
  fillYouTubePlaceholders();
}

function fillYouTubePlaceholders() {
  var placeholders = document.getElementsByClassName('embed-placeholder');
  for (var i = 0; i < placeholders.length; i++) {
    if (!placeholders[i].hasAttribute('data-type') ||
        placeholders[i].getAttribute('data-type') != 'youtube' ||
        !placeholders[i].hasAttribute('data-id')) {
      continue;
    }
    var embed = document.createElement('iframe');
    var url = 'http://www.youtube.com/embed/' +
        placeholders[i].getAttribute('data-id');
    embed.setAttribute('class', 'youtubeIframe');
    embed.setAttribute('src', url);
    embed.setAttribute('type', 'text/html');
    embed.setAttribute('frameborder', '0');

    var parent = placeholders[i].parentElement;
    var container = document.createElement('div');
    container.setAttribute('class', 'youtubeContainer');
    container.appendChild(embed);

    parent.replaceChild(container, placeholders[i]);
  }
}

function showLoadingIndicator(isLastPage) {
  document.getElementById('loadingIndicator').className =
      isLastPage ? 'hidden' : 'visible';
}

// Sets the title.
function setTitle(title) {
  var holder = document.getElementById('titleHolder');

  holder.textContent = title;
  document.title = title;
}

// Set the text direction of the document ('ltr', 'rtl', or 'auto').
function setTextDirection(direction) {
  document.body.setAttribute('dir', direction);
}

// Maps JS Font Family to CSS class and then changes body class name.
// CSS classes must agree with distilledpage.css.
function useFontFamily(fontFamily) {
  var cssClass;
  if (fontFamily == "serif") {
    cssClass = "serif";
  } else if (fontFamily == "monospace") {
    cssClass = "monospace";
  } else {
    cssClass = "sans-serif";
  }
  // Relies on the classname order of the body being Theme class, then Font
  // Family class.
  var themeClass = document.body.className.split(" ")[0];
  document.body.className = themeClass + " " + cssClass;
}

// Maps JS theme to CSS class and then changes body class name.
// CSS classes must agree with distilledpage.css.
function useTheme(theme) {
  var cssClass;
  if (theme == "sepia") {
    cssClass = "sepia";
  } else if (theme == "dark") {
    cssClass = "dark";
  } else {
    cssClass = "light";
  }
  // Relies on the classname order of the body being Theme class, then Font
  // Family class.
  var fontFamilyClass = document.body.className.split(" ")[1];
  document.body.className = cssClass + " " + fontFamilyClass;

  updateToolbarColor();
}

function updateToolbarColor() {
  // Relies on the classname order of the body being Theme class, then Font
  // Family class.
  var themeClass = document.body.className.split(" ")[0];

  var toolbarColor;
  if (themeClass == "sepia") {
    toolbarColor = "#BF9A73";
  } else if (themeClass == "dark") {
    toolbarColor = "#1A1A1A";
  } else {
    toolbarColor = "#F5F5F5";
  }
  document.getElementById('theme-color').content = toolbarColor;
}

function useFontScaling(scaling) {
  pincher.useFontScaling(scaling);
}

function maybeSetWebFont() {
  // On iOS, the web fonts block the rendering until the resources are
  // fetched, which can take a long time on slow networks.
  // In Blink, it times out after 3 seconds and uses fallback fonts.
  // See crbug.com/711650
  if (distiller_on_ios) return;

  var e = document.createElement('link');
  e.href = 'https://fonts.googleapis.com/css?family=Roboto';
  e.rel = 'stylesheet';
  e.type = 'text/css';
  document.head.appendChild(e);
}

// Add a listener to the "View Original" link to report opt-outs.
document.getElementById('closeReaderView').addEventListener('click',
    function(e) {
      if (distiller) {
        distiller.closePanel(true);
      }
    }, true);

updateToolbarColor();
maybeSetWebFont();

var pincher = (function() {
  'use strict';
  // When users pinch in Reader Mode, the page would zoom in or out as if it
  // is a normal web page allowing user-zoom. At the end of pinch gesture, the
  // page would do text reflow. These pinch-to-zoom and text reflow effects
  // are not native, but are emulated using CSS and JavaScript.
  //
  // In order to achieve near-native zooming and panning frame rate, fake 3D
  // transform is used so that the layer doesn't repaint for each frame.
  //
  // After the text reflow, the web content shown in the viewport should
  // roughly be the same paragraph before zooming.
  //
  // The control point of font size is the html element, so that both "em" and
  // "rem" are adjusted.
  //
  // TODO(wychen): Improve scroll position when elementFromPoint is body.

  var pinching = false;
  var fontSizeAnchor = 1.0;

  var focusElement = null;
  var focusPos = 0;
  var initClientMid;

  var clampedScale = 1;

  var lastSpan;
  var lastClientMid;

  var scale = 1;
  var shiftX;
  var shiftY;

  // The zooming speed relative to pinching speed.
  var FONT_SCALE_MULTIPLIER = 0.5;

  var MIN_SPAN_LENGTH = 20;

  // The font size is guaranteed to be in px.
  var baseSize =
      parseFloat(getComputedStyle(document.documentElement).fontSize);

  var refreshTransform = function() {
    var slowedScale = Math.exp(Math.log(scale) * FONT_SCALE_MULTIPLIER);
    clampedScale = Math.max(0.5, Math.min(2.0, fontSizeAnchor * slowedScale));

    // Use "fake" 3D transform so that the layer is not repainted.
    // With 2D transform, the frame rate would be much lower.
    document.body.style.transform =
        'translate3d(' + shiftX + 'px,' +
                         shiftY + 'px, 0px)' +
        'scale(' + clampedScale/fontSizeAnchor + ')';
  };

  function saveCenter(clientMid) {
    // Try to preserve the pinching center after text reflow.
    // This is accurate to the HTML element level.
    focusElement = document.elementFromPoint(clientMid.x, clientMid.y);
    var rect = focusElement.getBoundingClientRect();
    initClientMid = clientMid;
    focusPos = (initClientMid.y - rect.top) / (rect.bottom - rect.top);
  }

  function restoreCenter() {
    var rect = focusElement.getBoundingClientRect();
    var targetTop = focusPos * (rect.bottom - rect.top) + rect.top +
        document.body.scrollTop - (initClientMid.y + shiftY);
    document.body.scrollTop = targetTop;
  }

  function endPinch() {
    pinching = false;

    document.body.style.transformOrigin = '';
    document.body.style.transform = '';
    document.documentElement.style.fontSize = clampedScale * baseSize + "px";

    restoreCenter();

    var img = document.getElementById('fontscaling-img');
    if (!img) {
      img = document.createElement('img');
      img.id = 'fontscaling-img';
      img.style.display = 'none';
      document.body.appendChild(img);
    }
    img.src = "/savefontscaling/" + clampedScale;
  }

  function touchSpan(e) {
    var count = e.touches.length;
    var mid = touchClientMid(e);
    var sum = 0;
    for (var i = 0; i < count; i++) {
      var dx = (e.touches[i].clientX - mid.x);
      var dy = (e.touches[i].clientY - mid.y);
      sum += Math.hypot(dx, dy);
    }
    // Avoid very small span.
    return Math.max(MIN_SPAN_LENGTH, sum/count);
  }

  function touchClientMid(e) {
    var count = e.touches.length;
    var sumX = 0;
    var sumY = 0;
    for (var i = 0; i < count; i++) {
      sumX += e.touches[i].clientX;
      sumY += e.touches[i].clientY;
    }
    return {x: sumX/count, y: sumY/count};
  }

  function touchPageMid(e) {
    var clientMid = touchClientMid(e);
    return {x: clientMid.x - e.touches[0].clientX + e.touches[0].pageX,
            y: clientMid.y - e.touches[0].clientY + e.touches[0].pageY};
  }

  return {
    handleTouchStart: function(e) {
      if (e.touches.length < 2) return;
      e.preventDefault();

      var span = touchSpan(e);
      var clientMid = touchClientMid(e);

      if (e.touches.length > 2) {
        lastSpan = span;
        lastClientMid = clientMid;
        refreshTransform();
        return;
      }

      scale = 1;
      shiftX = 0;
      shiftY = 0;

      pinching = true;
      fontSizeAnchor =
          parseFloat(getComputedStyle(document.documentElement).fontSize) /
          baseSize;

      var pinchOrigin = touchPageMid(e);
      document.body.style.transformOrigin =
          pinchOrigin.x + 'px ' + pinchOrigin.y  + 'px';

      saveCenter(clientMid);

      lastSpan = span;
      lastClientMid = clientMid;

      refreshTransform();
    },

    handleTouchMove: function(e) {
      if (!pinching) return;
      if (e.touches.length < 2) return;
      e.preventDefault();

      var span = touchSpan(e);
      var clientMid = touchClientMid(e);

      scale *= touchSpan(e) / lastSpan;
      shiftX += clientMid.x - lastClientMid.x;
      shiftY += clientMid.y - lastClientMid.y;

      refreshTransform();

      lastSpan = span;
      lastClientMid = clientMid;
    },

    handleTouchEnd: function(e) {
      if (!pinching) return;
      e.preventDefault();

      var span = touchSpan(e);
      var clientMid = touchClientMid(e);

      if (e.touches.length >= 2) {
        lastSpan = span;
        lastClientMid = clientMid;
        refreshTransform();
        return;
      }

      endPinch();
    },

    handleTouchCancel: function(e) {
      if (!pinching) return;
      endPinch();
    },

    reset: function() {
      scale = 1;
      shiftX = 0;
      shiftY = 0;
      clampedScale = 1;
      document.documentElement.style.fontSize = clampedScale * baseSize + "px";
    },

    status: function() {
      return {
        scale: scale,
        clampedScale: clampedScale,
        shiftX: shiftX,
        shiftY: shiftY
      };
    },

    useFontScaling: function(scaling) {
      saveCenter({x: window.innerWidth/2, y: window.innerHeight/2});
      shiftX = 0;
      shiftY = 0;
      document.documentElement.style.fontSize = scaling * baseSize + "px";
      clampedScale = scaling;
      restoreCenter();
    }
  };
}());

window.addEventListener('touchstart', pincher.handleTouchStart, false);
window.addEventListener('touchmove', pincher.handleTouchMove, false);
window.addEventListener('touchend', pincher.handleTouchEnd, false);
window.addEventListener('touchcancel', pincher.handleTouchCancel, false);
