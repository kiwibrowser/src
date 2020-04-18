//  Copyright 2009 Google Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.


//  PopupManager is a library to facilitate integration with OpenID
//  identity providers (OP)s that support a pop-up authentication interface.
//  To create a popup window, you first construct a popupOpener customized
//  for your site and a particular identity provider, E.g.:
//
//  var googleOpener = popupManager.createOpener(openidParams);
//
//  where 'openidParams' are customized for Google in this instance.
//  (typically you just change the openidpoint, the version number
//  (the openid.ns parameter) and the extensions based on what
//  the OP supports.
//  OpenID libraries can often discover these properties
//  automatically from the location of an XRD document.
//
//  Then, you can either directly call
//  googleOpener.popup(width, height), where 'width' and 'height' are your choices
//  for popup size, or you can display a button 'Sign in with Google' and set the
//..'onclick' handler of the button to googleOpener.popup()

var popupManager = {};

// Library constants

popupManager.constants = {
  'darkCover' : 'popupManager_darkCover_div',
  'darkCoverStyle' : ['position:absolute;',
                      'top:0px;',
                      'left:0px;',
                      'padding-right:0px;',
                      'padding-bottom:0px;',
                      'background-color:#000000;',
                      'opacity:0.5;', //standard-compliant browsers
                      '-moz-opacity:0.5;',           // old Mozilla
                      'filter:alpha(opacity=0.5);',  // IE
                      'z-index:10000;',
                      'width:100%;',
                      'height:100%;'
                      ].join(''),
  'openidSpec' : {
     'identifier_select' : 'http%3A%2F%2Fspecs.openid.net%2Fauth%2F2.0%2Fidentifier_select',
     'namespace2' : 'http%3A%2F%2Fspecs.openid.net%2Fauth%2F2.0'
  } };

// Computes the size of the window contents. Returns a pair of
// coordinates [width, height] which can be [0, 0] if it was not possible
// to compute the values.
popupManager.getWindowInnerSize = function() {
  var width = 0;
  var height = 0;
  var elem = null;
  if ('innerWidth' in window) {
    // For non-IE
    width = window.innerWidth;
    height = window.innerHeight;
  } else {
    // For IE,
    if (('BackCompat' === window.document.compatMode)
        && ('body' in window.document)) {
        elem = window.document.body;
    } else if ('documentElement' in window.document) {
      elem = window.document.documentElement;
    }
    if (elem !== null) {
      width = elem.offsetWidth;
      height = elem.offsetHeight;
    }
  }
  return [width, height];
};

// Computes the coordinates of the parent window.
// Gets the coordinates of the parent frame
popupManager.getParentCoords = function() {
  var width = 0;
  var height = 0;
  if ('screenLeft' in window) {
    // IE-compatible variants
    width = window.screenLeft;
    height = window.screenTop;
  } else if ('screenX' in window) {
    // Firefox-compatible
    width = window.screenX;
    height = window.screenY;
  }
  return [width, height];
};

// Computes the coordinates of the new window, so as to center it
// over the parent frame
popupManager.getCenteredCoords = function(width, height) {
   var parentSize = this.getWindowInnerSize();
   var parentPos = this.getParentCoords();
   var xPos = parentPos[0] +
       Math.max(0, Math.floor((parentSize[0] - width) / 2));
   var yPos = parentPos[1] +
       Math.max(0, Math.floor((parentSize[1] - height) / 2));
   return [xPos, yPos];
};

//  A utility class, implements an onOpenHandler that darkens the screen
//  by overlaying it with a semi-transparent black layer. To use, ensure that
//  no screen element has a z-index at or above 10000.
//  This layer will be suppressed automatically after the screen closes.
//
//  Note: If you want to perform other operations before opening the popup, but
//  also would like the screen to darken, you can define a custom handler
//  as such:
//  var myOnOpenHandler = function(inputs) {
//    .. do something
//    popupManager.darkenScreen();
//    .. something else
//  };
//  Then you pass myOnOpenHandler as input to the opener, as in:
//  var openidParams = {};
//  openidParams.onOpenHandler = myOnOpenHandler;
//  ... other customizations
//  var myOpener = popupManager.createOpener(openidParams);
popupManager.darkenScreen = function() {
  var darkCover = window.document.getElementById(window.popupManager.constants['darkCover']);
  if (!darkCover) {
    darkCover = window.document.createElement('div');
    darkCover['id'] = window.popupManager.constants['darkCover'];
    darkCover.setAttribute('style', window.popupManager.constants['darkCoverStyle']);
    window.document.body.appendChild(darkCover);
  }
  darkCover.style.visibility = 'visible';
};

//  Returns a an object that can open a popup window customized for an OP & RP.
//  to use you call var opener = popupManager.cretePopupOpener(openidParams);
//  and then you can assign the 'onclick' handler of a button to
//  opener.popup(width, height), where width and height are the values of the popup size;
//
//  To use it, you would typically have code such as:
//  var myLoginCheckFunction = ...  some AJAXy call or page refresh operation
//  that will cause the user to see the logged-in experience in the current page.
//  var openidParams = { realm : 'openid.realm', returnToUrl : 'openid.return_to',
//  opEndpoint : 'openid.op_endpoint', onCloseHandler : myLoginCheckFunction,
//  shouldEncodeUrls : 'true' (default) or 'false', extensions : myOpenIDExtensions };
//
//  Here extensions include any OpenID extensions that you support. For instance,
//  if you support Attribute Exchange v.1.0, you can say:
//  (Example for attribute exchange request for email and name,
//  assuming that shouldEncodeUrls = 'true':)
//  var myOpenIDExtensions = {
//      'openid.ax.ns' : 'http://openid.net/srv/ax/1.0',
//      'openid.ax.type.email' : 'http://axschema.org/contact/email',
//      'openid.ax.type.name1' : 'http://axschema.org/namePerson/first',
//      'openid.ax.type.name2' : 'http://axschema.org/namePerson/last',
//      'openid.ax.required' : 'email,name1,name2' };
//  Note that the 'ui' namespace is reserved by this library for the OpenID
//  UI extension, and that the mode 'popup' is automatically applied.
//  If you wish to make use of the 'language' feature of the OpenID UI extension
//  simply add the following entry (example assumes the language requested
//  is Swiss French:
//  var my OpenIDExtensions = {
//    ... // other extension parameters
//    'openid.ui.language' : 'fr_CH',
//    ... };
popupManager.createPopupOpener = (function(openidParams) {
  var interval_ = null;
  var popupWindow_ = null;
  var that = this;
  var shouldEscape_ = ('shouldEncodeUrls' in openidParams) ? openidParams.shouldEncodeUrls : true;
  var encodeIfRequested_ = function(url) {
    return (shouldEscape_ ? encodeURIComponent(url) : url);
  };
  var identifier_ = ('identifier' in openidParams) ? encodeIfRequested_(openidParams.identifier) :
      this.constants.openidSpec.identifier_select;
  var identity_ = ('identity' in openidParams) ? encodeIfRequested_(openidParams.identity) :
      this.constants.openidSpec.identifier_select;
  var openidNs_ = ('namespace' in openidParams) ? encodeIfRequested_(openidParams.namespace) :
      this.constants.openidSpec.namespace2;
  var onOpenHandler_ = (('onOpenHandler' in openidParams) &&
      ('function' === typeof(openidParams.onOpenHandler))) ?
          openidParams.onOpenHandler : this.darkenScreen;
  var onCloseHandler_ = (('onCloseHandler' in openidParams) &&
      ('function' === typeof(openidParams.onCloseHandler))) ?
          openidParams.onCloseHandler : null;
  var returnToUrl_ = ('returnToUrl' in openidParams) ? openidParams.returnToUrl : null;
  var realm_ = ('realm' in openidParams) ? openidParams.realm : null;
  var endpoint_ = ('opEndpoint' in openidParams) ? openidParams.opEndpoint : null;
  var extensions_ = ('extensions' in openidParams) ? openidParams.extensions : null;

  // processes key value pairs, escaping any input;
  var keyValueConcat_ = function(keyValuePairs) {
    var result = "";
    for (key in keyValuePairs) {
      result += ['&', key, '=', encodeIfRequested_(keyValuePairs[key])].join('');
    }
    return result;
  };

  //Assembles the OpenID request from customizable parameters
  var buildUrlToOpen_ = function() {
    var connector = '&';
    var encodedUrl = null;
    var urlToOpen = null;
    if ((null === endpoint_) || (null === returnToUrl_)) {
      return;
    }
    if (endpoint_.indexOf('?') === -1) {
      connector = '?';
    }
    encodedUrl = encodeIfRequested_(returnToUrl_);
    urlToOpen = [ endpoint_, connector,
        'openid.ns=', openidNs_,
        '&openid.mode=checkid_setup',
        '&openid.claimed_id=', identifier_,
        '&openid.identity=', identity_,
        '&openid.return_to=', encodedUrl ].join('');
    if (realm_ !== null) {
      urlToOpen += "&openid.realm=" + encodeIfRequested_(realm_);
    }
    if (extensions_ !== null) {
      urlToOpen += keyValueConcat_(extensions_);
    }
    urlToOpen += '&openid.ns.ui=' + encodeURIComponent(
        'http://specs.openid.net/extensions/ui/1.0');
    urlToOpen += '&openid.ui.mode=popup';
    return urlToOpen;
  };

  // Tests that the popup window has closed
  var isPopupClosed_ = function() {
    return (!popupWindow_ || popupWindow_.closed);
  };

  // Check to perform at each execution of the timed loop. It also triggers
  // the action that follows the closing of the popup
  var waitForPopupClose_ = function() {
    if (isPopupClosed_()) {
      popupWindow_ = null;
      var darkCover = window.document.getElementById(window.popupManager.constants['darkCover']);
      if (darkCover) {
        darkCover.style.visibility = 'hidden';
      }
      if (onCloseHandler_ !== null) {
        onCloseHandler_();
      }
      if ((null !== interval_)) {
        window.clearInterval(interval_);
        interval_ = null;
      }
    }
  };

  return {
    // Function that opens the window.
    popup: function(width, height) {
      var urlToOpen = buildUrlToOpen_();
      if (onOpenHandler_ !== null) {
        onOpenHandler_();
      }
      var coordinates = that.getCenteredCoords(width, height);
      popupWindow_ = window.open(urlToOpen, "",
          "width=" + width + ",height=" + height +
          ",status=1,location=1,resizable=yes" +
          ",left=" + coordinates[0] +",top=" + coordinates[1]);
      interval_ = window.setInterval(waitForPopupClose_, 80);
      return true;
    }
  };
});
