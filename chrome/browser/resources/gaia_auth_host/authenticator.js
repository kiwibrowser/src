// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <include src="saml_handler.js">
// Note: webview_event_manager.js is already included by saml_handler.js.

/**
 * @fileoverview An UI component to authenciate to Chrome. The component hosts
 * IdP web pages in a webview. A client who is interested in monitoring
 * authentication events should pass a listener object of type
 * cr.login.GaiaAuthHost.Listener as defined in this file. After initialization,
 * call {@code load} to start the authentication flow.
 *
 * See go/cros-auth-design for details on Google API.
 */

cr.define('cr.login', function() {
  'use strict';

  // TODO(rogerta): should use gaia URL from GaiaUrls::gaia_url() instead
  // of hardcoding the prod URL here.  As is, this does not work with staging
  // environments.
  var IDP_ORIGIN = 'https://accounts.google.com/';
  var IDP_PATH = 'ServiceLogin?skipvpage=true&sarp=1&rm=hide';
  var CONTINUE_URL =
      'chrome-extension://mfffpogegjflfpflabcdkioaeobkgjik/success.html';
  var SIGN_IN_HEADER = 'google-accounts-signin';
  var EMBEDDED_FORM_HEADER = 'google-accounts-embedded';
  var LOCATION_HEADER = 'location';
  var COOKIE_HEADER = 'cookie';
  var SET_COOKIE_HEADER = 'set-cookie';
  var OAUTH_CODE_COOKIE = 'oauth_code';
  var GAPS_COOKIE = 'GAPS';
  var SERVICE_ID = 'chromeoslogin';
  var EMBEDDED_SETUP_CHROMEOS_ENDPOINT = 'embedded/setup/chromeos';
  var EMBEDDED_SETUP_CHROMEOS_ENDPOINT_V2 = 'embedded/setup/v2/chromeos';
  var SAML_REDIRECTION_PATH = 'samlredirect';
  var BLANK_PAGE_URL = 'about:blank';

  /**
   * The source URL parameter for the constrained signin flow.
   */
  var CONSTRAINED_FLOW_SOURCE = 'chrome';

  /**
   * Enum for the authorization mode, must match AuthMode defined in
   * chrome/browser/ui/webui/inline_login_ui.cc.
   * @enum {number}
   */
  var AuthMode = {DEFAULT: 0, OFFLINE: 1, DESKTOP: 2};

  /**
   * Enum for the authorization type.
   * @enum {number}
   */
  var AuthFlow = {DEFAULT: 0, SAML: 1};

  /**
   * Supported Authenticator params.
   * @type {!Array<string>}
   * @const
   */
  var SUPPORTED_PARAMS = [
    'gaiaId',        // Obfuscated GAIA ID to skip the email prompt page
                     // during the re-auth flow.
    'gaiaUrl',       // Gaia url to use.
    'gaiaPath',      // Gaia path to use without a leading slash.
    'hl',            // Language code for the user interface.
    'service',       // Name of Gaia service.
    'continueUrl',   // Continue url to use.
    'frameUrl',      // Initial frame URL to use. If empty defaults to
                     // gaiaUrl.
    'constrained',   // Whether the extension is loaded in a constrained
                     // window.
    'clientId',      // Chrome client id.
    'needPassword',  // Whether the host is interested in getting a password.
                     // If this set to |false|, |confirmPasswordCallback| is
                     // not called before dispatching |authCopleted|.
                     // Default is |true|.
    'flow',          // One of 'default', 'enterprise', or 'theftprotection'.
    'enterpriseDisplayDomain',     // Current domain name to be displayed.
    'enterpriseEnrollmentDomain',  // Domain in which hosting device is (or
                                   // should be) enrolled.
    'emailDomain',                 // Value used to prefill domain for email.
    'chromeType',                // Type of Chrome OS device, e.g. "chromebox".
    'clientVersion',             // Version of the Chrome build.
    'platformVersion',           // Version of the OS build.
    'releaseChannel',            // Installation channel.
    'endpointGen',               // Current endpoint generation.
    'gapsCookie',                // GAPS cookie
    'chromeOSApiVersion',        // GAIA Chrome OS API version
    'menuGuestMode',             // Enables "Guest mode" menu item
    'menuKeyboardOptions',       // Enables "Keyboard options" menu item
    'menuEnterpriseEnrollment',  // Enables "Enterprise enrollment" menu item.
    'lsbReleaseBoard',           // Chrome OS Release board name
    'isFirstUser',               // True if this is non-enterprise device,
                                 // and there are no users yet.
    'obfuscatedOwnerId',         // Obfuscated device owner ID, if neeed.

    // The email fields allow for the following possibilities:
    //
    // 1/ If 'email' is not supplied, then the email text field is blank and the
    // user must type an email to proceed.
    //
    // 2/ If 'email' is supplied, and 'readOnlyEmail' is truthy, then the email
    // is hardcoded and the user cannot change it.  The user is asked for
    // password.  This is useful for re-auth scenarios, where chrome needs the
    // user to authenticate for a specific account and only that account.
    //
    // 3/ If 'email' is supplied, and 'readOnlyEmail' is falsy, gaia will
    // prefill the email text field using the given email address, but the user
    // can still change it and then proceed.  This is used on desktop when the
    // user disconnects their profile then reconnects, to encourage them to use
    // the same account.
    'email',
    'readOnlyEmail',
    'realm',
  ];

  /**
   * Initializes the authenticator component.
   * @param {webview|string} webview The webview element or its ID to host IdP
   *     web pages.
   * @constructor
   */
  function Authenticator(webview) {
    this.isLoaded_ = false;
    this.email_ = null;
    this.password_ = null;
    this.gaiaId_ = null, this.sessionIndex_ = null;
    this.chooseWhatToSync_ = false;
    this.skipForNow_ = false;
    this.authFlow = AuthFlow.DEFAULT;
    this.authDomain = '';
    this.videoEnabled = false;
    this.idpOrigin_ = null;
    this.continueUrl_ = null;
    this.continueUrlWithoutParams_ = null;
    this.initialFrameUrl_ = null;
    this.reloadUrl_ = null;
    this.trusted_ = true;
    this.oauthCode_ = null;
    this.gapsCookie_ = null;
    this.gapsCookieSent_ = false;
    this.newGapsCookie_ = null;
    this.readyFired_ = false;
    this.webviewEventManager_ = WebviewEventManager.create();

    this.clientId_ = null;

    this.confirmPasswordCallback = null;
    this.noPasswordCallback = null;
    this.insecureContentBlockedCallback = null;
    this.samlApiUsedCallback = null;
    this.missingGaiaInfoCallback = null;
    this.needPassword = true;
    this.services_ = null;

    this.bindToWebview_(webview);

    window.addEventListener(
        'message', this.onMessageFromWebview_.bind(this), false);
    window.addEventListener('focus', this.onFocus_.bind(this), false);
    window.addEventListener('popstate', this.onPopState_.bind(this), false);
  }

  Authenticator.prototype = Object.create(cr.EventTarget.prototype);

  /**
   * Reinitializes authentication parameters so that a failed login attempt
   * would not result in an infinite loop.
   */
  Authenticator.prototype.resetStates = function() {
    this.isLoaded_ = false;
    this.email_ = null;
    this.gaiaId_ = null;
    this.password_ = null;
    this.oauthCode_ = null;
    this.gapsCookie_ = null;
    this.gapsCookieSent_ = false;
    this.newGapsCookie_ = null;
    this.readyFired_ = false;
    this.chooseWhatToSync_ = false;
    this.skipForNow_ = false;
    this.sessionIndex_ = null;
    this.trusted_ = true;
    this.authFlow = AuthFlow.DEFAULT;
    this.samlHandler_.reset();
    this.videoEnabled = false;
    this.services_ = null;
  };

  /**
   * Resets the webview to the blank page.
   */
  Authenticator.prototype.resetWebview = function() {
    if (this.webview_.src && this.webview_.src != BLANK_PAGE_URL)
      this.webview_.src = BLANK_PAGE_URL;
  };

  /**
   * Binds this authenticator to the passed webview.
   * @param {!Object} webview the new webview to be used by this Authenticator.
   * @private
   */
  Authenticator.prototype.bindToWebview_ = function(webview) {
    assert(!this.webview_);
    assert(!this.samlHandler_);

    this.webview_ = typeof webview == 'string' ? $(webview) : webview;

    this.samlHandler_ = new cr.login.SamlHandler(this.webview_);
    this.webviewEventManager_.addEventListener(
        this.samlHandler_, 'insecureContentBlocked',
        this.onInsecureContentBlocked_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.samlHandler_, 'authPageLoaded', this.onAuthPageLoaded_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.samlHandler_, 'videoEnabled', this.onVideoEnabled_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.samlHandler_, 'apiPasswordAdded',
        this.onSamlApiPasswordAdded_.bind(this));

    this.webviewEventManager_.addEventListener(
        this.webview_, 'droplink', this.onDropLink_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.webview_, 'newwindow', this.onNewWindow_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.webview_, 'contentload', this.onContentLoad_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.webview_, 'loadabort', this.onLoadAbort_.bind(this));
    this.webviewEventManager_.addEventListener(
        this.webview_, 'loadcommit', this.onLoadCommit_.bind(this));

    this.webviewEventManager_.addWebRequestEventListener(
        this.webview_.request.onCompleted, this.onRequestCompleted_.bind(this),
        {urls: ['<all_urls>'], types: ['main_frame']}, ['responseHeaders']);
    this.webviewEventManager_.addWebRequestEventListener(
        this.webview_.request.onHeadersReceived,
        this.onHeadersReceived_.bind(this),
        {urls: ['<all_urls>'], types: ['main_frame', 'xmlhttprequest']},
        ['responseHeaders']);
  };

  /**
   * Unbinds this Authenticator from the currently bound webview.
   * @private
   */
  Authenticator.prototype.unbindFromWebview_ = function() {
    assert(this.webview_);
    assert(this.samlHandler_);

    this.webviewEventManager_.removeAllListeners();

    this.webview_ = undefined;
    this.samlHandler_.unbindFromWebview();
    this.samlHandler_ = undefined;
  };

  /**
   * Re-binds to another webview.
   * @param {Object} webview the new webview to be used by this Authenticator.
   */
  Authenticator.prototype.rebindWebview = function(webview) {
    this.unbindFromWebview_();
    this.bindToWebview_(webview);
  };

  /**
   * Loads the authenticator component with the given parameters.
   * @param {AuthMode} authMode Authorization mode.
   * @param {Object} data Parameters for the authorization flow.
   */
  Authenticator.prototype.load = function(authMode, data) {
    this.authMode = authMode;
    this.resetStates();
    // gaiaUrl parameter is used for testing. Once defined, it is never changed.
    this.idpOrigin_ = data.gaiaUrl || IDP_ORIGIN;
    this.continueUrl_ = data.continueUrl || CONTINUE_URL;
    this.continueUrlWithoutParams_ =
        this.continueUrl_.substring(0, this.continueUrl_.indexOf('?')) ||
        this.continueUrl_;
    this.isConstrainedWindow_ = data.constrained == '1';
    this.isNewGaiaFlow = data.isNewGaiaFlow;
    this.clientId_ = data.clientId;
    this.gapsCookie_ = data.gapsCookie;
    this.gapsCookieSent_ = false;
    this.newGapsCookie_ = null;
    this.dontResizeNonEmbeddedPages = data.dontResizeNonEmbeddedPages;
    this.chromeOSApiVersion_ = data.chromeOSApiVersion;

    this.initialFrameUrl_ = this.constructInitialFrameUrl_(data);
    this.reloadUrl_ = data.frameUrl || this.initialFrameUrl_;
    // Don't block insecure content for desktop flow because it lands on
    // http. Otherwise, block insecure content as long as gaia is https.
    this.samlHandler_.blockInsecureContent =
        authMode != AuthMode.DESKTOP && this.idpOrigin_.startsWith('https://');
    this.needPassword = !('needPassword' in data) || data.needPassword;

    if (this.isNewGaiaFlow) {
      this.webview_.contextMenus.onShow.addListener(function(e) {
        e.preventDefault();
      });

      if (!this.onBeforeSetHeadersSet_) {
        this.onBeforeSetHeadersSet_ = true;
        var filterPrefix = this.constructChromeOSAPIUrl_();
        // This depends on gaiaUrl parameter, that is why it is here.
        this.webview_.request.onBeforeSendHeaders.addListener(
            this.onBeforeSendHeaders_.bind(this),
            {urls: [filterPrefix + '?*', filterPrefix + '/*']},
            ['requestHeaders', 'blocking']);
      }
    }

    this.webview_.src = this.reloadUrl_;
    this.isLoaded_ = true;
  };

  Authenticator.prototype.constructChromeOSAPIUrl_ = function() {
    if (this.chromeOSApiVersion_ && this.chromeOSApiVersion_ == 2)
      return this.idpOrigin_ + EMBEDDED_SETUP_CHROMEOS_ENDPOINT_V2;

    return this.idpOrigin_ + EMBEDDED_SETUP_CHROMEOS_ENDPOINT;
  };

  /**
   * Reloads the authenticator component.
   */
  Authenticator.prototype.reload = function() {
    this.resetStates();
    this.webview_.src = this.reloadUrl_;
    this.isLoaded_ = true;
  };

  Authenticator.prototype.constructInitialFrameUrl_ = function(data) {
    if (data.doSamlRedirect) {
      var url = this.idpOrigin_ + SAML_REDIRECTION_PATH;
      url = appendParam(url, 'domain', data.enterpriseEnrollmentDomain);
      url = appendParam(
          url, 'continue',
          data.gaiaUrl + 'programmatic_auth_chromeos?hl=' + data.hl +
              '&scope=https%3A%2F%2Fwww.google.com%2Faccounts%2FOAuthLogin&' +
              'client_id=' + encodeURIComponent(data.clientId) +
              '&access_type=offline');

      return url;
    }

    var url;
    if (data.gaiaPath)
      url = this.idpOrigin_ + data.gaiaPath;
    else if (this.isNewGaiaFlow)
      url = this.constructChromeOSAPIUrl_();
    else
      url = this.idpOrigin_ + IDP_PATH;

    if (this.isNewGaiaFlow) {
      if (data.chromeType)
        url = appendParam(url, 'chrometype', data.chromeType);
      if (data.clientId)
        url = appendParam(url, 'client_id', data.clientId);
      if (data.enterpriseDisplayDomain)
        url = appendParam(url, 'manageddomain', data.enterpriseDisplayDomain);
      if (data.clientVersion)
        url = appendParam(url, 'client_version', data.clientVersion);
      if (data.platformVersion)
        url = appendParam(url, 'platform_version', data.platformVersion);
      if (data.releaseChannel)
        url = appendParam(url, 'release_channel', data.releaseChannel);
      if (data.endpointGen)
        url = appendParam(url, 'endpoint_gen', data.endpointGen);
      if (data.chromeOSApiVersion == 2) {
        var mi = '';
        if (data.menuGuestMode)
          mi += 'gm,';
        if (data.menuKeyboardOptions)
          mi += 'ko,';
        if (data.menuEnterpriseEnrollment)
          mi += 'ee,';
        if (mi.length)
          url = appendParam(url, 'mi', mi);

        if (data.lsbReleaseBoard)
          url = appendParam(url, 'chromeos_board', data.lsbReleaseBoard);
        if (data.isFirstUser)
          url = appendParam(url, 'is_first_user', true);
        if (data.obfuscatedOwnerId)
          url = appendParam(url, 'obfuscated_owner_id', data.obfuscatedOwnerId);
      }
    } else {
      url = appendParam(url, 'continue', this.continueUrl_);
      url = appendParam(url, 'service', data.service || SERVICE_ID);
    }
    if (data.hl)
      url = appendParam(url, 'hl', data.hl);
    if (data.gaiaId)
      url = appendParam(url, 'user_id', data.gaiaId);
    if (data.email) {
      if (data.readOnlyEmail) {
        url = appendParam(url, 'Email', data.email);
      } else {
        url = appendParam(url, 'email_hint', data.email);
      }
    }
    if (this.isConstrainedWindow_)
      url = appendParam(url, 'source', CONSTRAINED_FLOW_SOURCE);
    if (data.flow)
      url = appendParam(url, 'flow', data.flow);
    if (data.emailDomain)
      url = appendParam(url, 'emaildomain', data.emailDomain);
    return url;
  };

  /**
   * Dispatches the 'ready' event if it hasn't been dispatched already for the
   * current content.
   * @private
   */
  Authenticator.prototype.fireReadyEvent_ = function() {
    if (!this.readyFired_) {
      this.dispatchEvent(new Event('ready'));
      this.readyFired_ = true;
    }
  };

  /**
   * Invoked when a main frame request in the webview has completed.
   * @private
   */
  Authenticator.prototype.onRequestCompleted_ = function(details) {
    var currentUrl = details.url;

    if (!this.isNewGaiaFlow &&
        currentUrl.lastIndexOf(this.continueUrlWithoutParams_, 0) == 0) {
      if (currentUrl.indexOf('ntp=1') >= 0)
        this.skipForNow_ = true;

      this.maybeCompleteAuth_();
      return;
    }

    if (!currentUrl.startsWith('https'))
      this.trusted_ = false;

    if (this.isConstrainedWindow_) {
      var isEmbeddedPage = false;
      if (this.idpOrigin_ && currentUrl.lastIndexOf(this.idpOrigin_) == 0) {
        var headers = details.responseHeaders;
        for (var i = 0; headers && i < headers.length; ++i) {
          if (headers[i].name.toLowerCase() == EMBEDDED_FORM_HEADER) {
            isEmbeddedPage = true;
            break;
          }
        }
      }

      // In some cases, non-embedded pages should not be resized.  For
      // example, on desktop when reauthenticating for purposes of unlocking
      // a profile, resizing would cause a browser window to open in the
      // system profile, which is not allowed.
      if (!isEmbeddedPage && !this.dontResizeNonEmbeddedPages) {
        this.dispatchEvent(new CustomEvent('resize', {detail: currentUrl}));
        return;
      }
    }

    this.updateHistoryState_(currentUrl);
  };

  /**
   * Manually updates the history. Invoked upon completion of a webview
   * navigation.
   * @param {string} url Request URL.
   * @private
   */
  Authenticator.prototype.updateHistoryState_ = function(url) {
    if (history.state && history.state.url != url)
      history.pushState({url: url}, '');
    else
      history.replaceState({url: url}, '');
  };

  /**
   * Invoked when the sign-in page takes focus.
   * @param {object} e The focus event being triggered.
   * @private
   */
  Authenticator.prototype.onFocus_ = function(e) {
    if (this.authMode == AuthMode.DESKTOP &&
        document.activeElement == document.body) {
      this.webview_.focus();
    }
  };

  /**
   * Invoked when the history state is changed.
   * @param {object} e The popstate event being triggered.
   * @private
   */
  Authenticator.prototype.onPopState_ = function(e) {
    var state = e.state;
    if (state && state.url)
      this.webview_.src = state.url;
  };

  /**
   * Invoked when headers are received in the main frame of the webview. It
   * 1) reads the authenticated user info from a signin header,
   * 2) signals the start of a saml flow upon receiving a saml header.
   * @return {!Object} Modified request headers.
   * @private
   */
  Authenticator.prototype.onHeadersReceived_ = function(details) {
    var currentUrl = details.url;
    if (currentUrl.lastIndexOf(this.idpOrigin_, 0) != 0)
      return;

    var headers = details.responseHeaders;
    for (var i = 0; headers && i < headers.length; ++i) {
      var header = headers[i];
      var headerName = header.name.toLowerCase();
      if (headerName == SIGN_IN_HEADER) {
        var headerValues = header.value.toLowerCase().split(',');
        var signinDetails = {};
        headerValues.forEach(function(e) {
          var pair = e.split('=');
          signinDetails[pair[0].trim()] = pair[1].trim();
        });
        // Removes "" around.
        this.email_ = signinDetails['email'].slice(1, -1);
        this.gaiaId_ = signinDetails['obfuscatedid'].slice(1, -1);
        this.sessionIndex_ = signinDetails['sessionindex'];
      } else if (headerName == LOCATION_HEADER) {
        // If the "choose what to sync" checkbox was clicked, then the continue
        // URL will contain a source=3 field.
        var location = decodeURIComponent(header.value);
        this.chooseWhatToSync_ = !!location.match(/(\?|&)source=3($|&)/);
      } else if (this.isNewGaiaFlow && headerName == SET_COOKIE_HEADER) {
        var headerValue = header.value;
        if (headerValue.startsWith(OAUTH_CODE_COOKIE + '=')) {
          this.oauthCode_ =
              headerValue.substring(OAUTH_CODE_COOKIE.length + 1).split(';')[0];
        }
        if (headerValue.startsWith(GAPS_COOKIE + '=')) {
          this.newGapsCookie_ =
              headerValue.substring(GAPS_COOKIE.length + 1).split(';')[0];
        }
      }
    }
  };

  /**
   * This method replaces cookie value in cookie header.
   * @param@ {string} header_value Original string value of Cookie header.
   * @param@ {string} cookie_name Name of cookie to be replaced.
   * @param@ {string} cookie_value New cookie value.
   * @return {string} New Cookie header value.
   * @private
   */
  Authenticator.prototype.updateCookieValue_ = function(
      header_value, cookie_name, cookie_value) {
    var cookies = header_value.split(/\s*;\s*/);
    var found = false;
    for (var i = 0; i < cookies.length; ++i) {
      if (cookies[i].startsWith(cookie_name + '=')) {
        found = true;
        cookies[i] = cookie_name + '=' + cookie_value;
        break;
      }
    }
    if (!found) {
      cookies.push(cookie_name + '=' + cookie_value);
    }
    return cookies.join('; ');
  };

  /**
   * Handler for webView.request.onBeforeSendHeaders .
   * @return {!Object} Modified request headers.
   * @private
   */
  Authenticator.prototype.onBeforeSendHeaders_ = function(details) {
    // We should re-send cookie if first request was unsuccessful (i.e. no new
    // GAPS cookie was received).
    if (this.isNewGaiaFlow && this.gapsCookie_ &&
        (!this.gapsCookieSent_ || !this.newGapsCookie_)) {
      var headers = details.requestHeaders;
      var found = false;
      var gapsCookie = this.gapsCookie_;

      for (var i = 0, l = headers.length; i < l; ++i) {
        if (headers[i].name == COOKIE_HEADER) {
          headers[i].value = this.updateCookieValue_(
              headers[i].value, GAPS_COOKIE, gapsCookie);
          found = true;
          break;
        }
      }
      if (!found) {
        details.requestHeaders.push(
            {name: COOKIE_HEADER, value: GAPS_COOKIE + '=' + gapsCookie});
      }
      this.gapsCookieSent_ = true;
    }
    return {requestHeaders: details.requestHeaders};
  };

  /**
   * Returns true if given HTML5 message is received from the webview element.
   * @param {object} e Payload of the received HTML5 message.
   */
  Authenticator.prototype.isGaiaMessage = function(e) {
    if (!this.isWebviewEvent_(e))
      return false;

    // The event origin does not have a trailing slash.
    if (e.origin != this.idpOrigin_.substring(0, this.idpOrigin_.length - 1)) {
      return false;
    }

    // Gaia messages must be an object with 'method' property.
    if (typeof e.data != 'object' || !e.data.hasOwnProperty('method')) {
      return false;
    }
    return true;
  };

  /**
   * Invoked when an HTML5 message is received from the webview element.
   * @param {object} e Payload of the received HTML5 message.
   * @private
   */
  Authenticator.prototype.onMessageFromWebview_ = function(e) {
    if (!this.isGaiaMessage(e))
      return;

    var msg = e.data;
    if (msg.method == 'attemptLogin') {
      this.email_ = msg.email;
      if (this.authMode == AuthMode.DESKTOP)
        this.password_ = msg.password;

      this.chooseWhatToSync_ = msg.chooseWhatToSync;
      // We need to dispatch only first event, before user enters password.
      this.dispatchEvent(new CustomEvent('attemptLogin', {detail: msg.email}));
    } else if (msg.method == 'dialogShown') {
      this.dispatchEvent(new Event('dialogShown'));
    } else if (msg.method == 'dialogHidden') {
      this.dispatchEvent(new Event('dialogHidden'));
    } else if (msg.method == 'backButton') {
      this.dispatchEvent(new CustomEvent('backButton', {detail: msg.show}));
    } else if (msg.method == 'showView') {
      this.dispatchEvent(new Event('showView'));
    } else if (msg.method == 'menuItemClicked') {
      this.dispatchEvent(
          new CustomEvent('menuItemClicked', {detail: msg.item}));
    } else if (msg.method == 'identifierEntered') {
      this.dispatchEvent(new CustomEvent(
          'identifierEntered',
          {detail: {accountIdentifier: msg.accountIdentifier}}));
    } else if (msg.method == 'userInfo') {
      this.services_ = msg.services;
      if (this.email_ && this.gaiaId_ && this.sessionIndex_)
        this.maybeCompleteAuth_();
    } else {
      console.warn('Unrecognized message from GAIA: ' + msg.method);
    }
  };

  /**
   * Invoked by the hosting page to verify the Saml password.
   */
  Authenticator.prototype.verifyConfirmedPassword = function(password) {
    if (!this.samlHandler_.verifyConfirmedPassword(password)) {
      // Invoke confirm password callback asynchronously because the
      // verification was based on messages and caller (GaiaSigninScreen)
      // does not expect it to be called immediately.
      // TODO(xiyuan): Change to synchronous call when iframe based code
      // is removed.
      var invokeConfirmPassword =
          (function() {
            this.confirmPasswordCallback(
                this.email_, this.samlHandler_.scrapedPasswordCount);
          }).bind(this);
      window.setTimeout(invokeConfirmPassword, 0);
      return;
    }

    this.password_ = password;
    this.onAuthCompleted_();
  };

  /**
   * Check Saml flow and start password confirmation flow if needed. Otherwise,
   * continue with auto completion.
   * @private
   */
  Authenticator.prototype.maybeCompleteAuth_ = function() {
    var missingGaiaInfo = !this.email_ || !this.gaiaId_ || !this.sessionIndex_;
    if (missingGaiaInfo && !this.skipForNow_) {
      if (this.missingGaiaInfoCallback)
        this.missingGaiaInfoCallback();

      this.webview_.src = this.initialFrameUrl_;
      return;
    }
    // TODO(https://crbug.com/837107): remove this once API is fully stabilized.
    // @example.com is used in tests.
    if (!this.services_ && !this.email_.endsWith('@gmail.com') &&
        !this.email_.endsWith('@example.com')) {
      console.warn('Forcing empty services.');
      this.services_ = [];
    }
    if (!this.services_)
      return;

    if (this.samlHandler_.samlApiUsed) {
      if (this.samlApiUsedCallback) {
        this.samlApiUsedCallback();
      }
      this.password_ = this.samlHandler_.apiPasswordBytes;
      this.onAuthCompleted_();
      return;
    }

    if (this.samlHandler_.scrapedPasswordCount == 0) {
      if (this.noPasswordCallback) {
        this.noPasswordCallback(this.email_);
        return;
      }

      // Fall through to finish the auth flow even if this.needPassword
      // is true. This is because the flag is used as an intention to get
      // password when it is available but not a mandatory requirement.
      console.warn('Authenticator: No password scraped for SAML.');
    } else if (this.needPassword) {
      if (this.samlHandler_.scrapedPasswordCount == 1) {
        // If we scraped exactly one password, we complete the authentication
        // right away.
        this.password_ = this.samlHandler_.firstScrapedPassword;
        this.onAuthCompleted_();
        return;
      }

      if (this.confirmPasswordCallback) {
        // Confirm scraped password. The flow follows in
        // verifyConfirmedPassword.
        this.confirmPasswordCallback(
            this.email_, this.samlHandler_.scrapedPasswordCount);
        return;
      }
    }

    this.onAuthCompleted_();
  };

  /**
   * Invoked to complete the authentication using the password the user enters
   * manually for non-principals API SAML IdPs that we couldn't scrape their
   * password input.
   */
  Authenticator.prototype.completeAuthWithManualPassword = function(password) {
    this.password_ = password;
    this.onAuthCompleted_();
  };

  /**
   * Invoked to process authentication completion.
   * @private
   */
  Authenticator.prototype.onAuthCompleted_ = function() {
    assert(
        this.skipForNow_ ||
        (this.email_ && this.gaiaId_ && this.sessionIndex_));
    // Chrome will crash on incorrect data type, so log some error message here.
    if (this.services_) {
      if (!Array.isArray(this.services_)) {
        console.error('FATAL: Bad services type:' + typeof this.services_);
      } else {
        for (var i = 0; i < this.services_.length; ++i) {
          if (typeof this.services_[i] == 'string')
            continue;

          console.error(
              'FATAL: Bad services[' + i +
              '] type:' + typeof this.services_[i]);
        }
      }
    }
    this.dispatchEvent(new CustomEvent(
        'authCompleted',
        // TODO(rsorokin): get rid of the stub values.
        {
          detail: {
            email: this.email_ || '',
            gaiaId: this.gaiaId_ || '',
            password: this.password_ || '',
            authCode: this.oauthCode_,
            usingSAML: this.authFlow == AuthFlow.SAML,
            chooseWhatToSync: this.chooseWhatToSync_,
            skipForNow: this.skipForNow_,
            sessionIndex: this.sessionIndex_ || '',
            trusted: this.trusted_,
            gapsCookie: this.newGapsCookie_ || this.gapsCookie_ || '',
            services: this.services_ || [],
          }
        }));
    this.resetStates();
  };

  /**
   * Invoked when |samlHandler_| fires 'insecureContentBlocked' event.
   * @private
   */
  Authenticator.prototype.onInsecureContentBlocked_ = function(e) {
    if (!this.isLoaded_)
      return;

    if (this.insecureContentBlockedCallback)
      this.insecureContentBlockedCallback(e.detail.url);
    else
      console.error('Authenticator: Insecure content blocked.');
  };

  /**
   * Invoked when |samlHandler_| fires 'authPageLoaded' event.
   * @private
   */
  Authenticator.prototype.onAuthPageLoaded_ = function(e) {
    if (!this.isLoaded_)
      return;

    if (!e.detail.isSAMLPage)
      return;

    this.authDomain = this.samlHandler_.authDomain;
    this.authFlow = AuthFlow.SAML;

    this.fireReadyEvent_();
  };

  /**
   * Invoked when |samlHandler_| fires 'videoEnabled' event.
   * @private
   */
  Authenticator.prototype.onVideoEnabled_ = function(e) {
    this.videoEnabled = true;
  };

  /**
   * Invoked when |samlHandler_| fires 'apiPasswordAdded' event.
   * @private
   */
  Authenticator.prototype.onSamlApiPasswordAdded_ = function(e) {
    // Saml API 'add' password might be received after the 'loadcommit' event.
    // In such case, maybeCompleteAuth_ should be attempted again if oauth code
    // is available.
    if (this.oauthCode_)
      this.maybeCompleteAuth_();
  };

  /**
   * Invoked when a link is dropped on the webview.
   * @private
   */
  Authenticator.prototype.onDropLink_ = function(e) {
    this.dispatchEvent(new CustomEvent('dropLink', {detail: e.url}));
  };

  /**
   * Invoked when the webview attempts to open a new window.
   * @private
   */
  Authenticator.prototype.onNewWindow_ = function(e) {
    this.dispatchEvent(new CustomEvent('newWindow', {detail: e}));
  };

  /**
   * Invoked when a new document is loaded.
   * @private
   */
  Authenticator.prototype.onContentLoad_ = function(e) {
    if (this.isConstrainedWindow_) {
      // Signin content in constrained windows should not zoom. Isolate the
      // webview from the zooming of other webviews using the 'per-view' zoom
      // mode, and then set it to 100% zoom.
      this.webview_.setZoomMode('per-view');
      this.webview_.setZoom(1);
    }

    // Posts a message to IdP pages to initiate communication.
    var currentUrl = this.webview_.src;
    if (currentUrl.lastIndexOf(this.idpOrigin_) == 0) {
      var msg = {
        'method': 'handshake',
      };

      // |this.webview_.contentWindow| may be null after network error screen
      // is shown. See crbug.com/770999.
      if (this.webview_.contentWindow)
        this.webview_.contentWindow.postMessage(msg, currentUrl);
      else
        console.error('Authenticator: contentWindow is null.');

      if (this.authMode == AuthMode.DEFAULT) {
        chrome.send('metricsHandler:recordBooleanHistogram', [
          'ChromeOS.GAIA.AuthenticatorContentWindowNull',
          !this.webview_.contentWindow
        ]);
      }

      this.fireReadyEvent_();
      // Focus webview after dispatching event when webview is already visible.
      this.webview_.focus();
    } else if (currentUrl == BLANK_PAGE_URL) {
      this.fireReadyEvent_();
    }
  };

  /**
   * Invoked when the webview fails loading a page.
   * @private
   */
  Authenticator.prototype.onLoadAbort_ = function(e) {
    this.dispatchEvent(
        new CustomEvent('loadAbort', {detail: {error: e.reason, src: e.url}}));
  };

  /**
   * Invoked when the webview navigates withing the current document.
   * @private
   */
  Authenticator.prototype.onLoadCommit_ = function(e) {
    if (this.oauthCode_)
      this.maybeCompleteAuth_();
  };

  /**
   * Returns |true| if event |e| was sent from the hosted webview.
   * @private
   */
  Authenticator.prototype.isWebviewEvent_ = function(e) {
    // Note: <webview> prints error message to console if |contentWindow| is not
    // defined.
    // TODO(dzhioev): remove the message. http://crbug.com/469522
    var webviewWindow = this.webview_.contentWindow;
    return !!webviewWindow && webviewWindow === e.source;
  };

  /**
   * The current auth flow of the hosted auth page.
   * @type {AuthFlow}
   */
  cr.defineProperty(Authenticator, 'authFlow');

  /**
   * The domain name of the current auth page.
   * @type {string}
   */
  cr.defineProperty(Authenticator, 'authDomain');

  /**
   * True if the page has requested media access.
   * @type {boolean}
   */
  cr.defineProperty(Authenticator, 'videoEnabled');

  Authenticator.AuthFlow = AuthFlow;
  Authenticator.AuthMode = AuthMode;
  Authenticator.SUPPORTED_PARAMS = SUPPORTED_PARAMS;

  return {
    // TODO(guohui, xiyuan): Rename GaiaAuthHost to Authenticator once the old
    // iframe-based flow is deprecated.
    GaiaAuthHost: Authenticator,
    Authenticator: Authenticator
  };
});
