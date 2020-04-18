// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('cloudprint');

/**
 * Event types dispatched by the cloudprint interface.
 * @enum {string}
 */
cloudprint.CloudPrintInterfaceEventType = {
  INVITES_DONE: 'cloudprint.CloudPrintInterface.INVITES_DONE',
  INVITES_FAILED: 'cloudprint.CloudPrintInterface.INVITES_FAILED',
  PRINTER_DONE: 'cloudprint.CloudPrintInterface.PRINTER_DONE',
  PRINTER_FAILED: 'cloudprint.CloudPrintInterface.PRINTER_FAILED',
  PROCESS_INVITE_DONE: 'cloudprint.CloudPrintInterface.PROCESS_INVITE_DONE',
  PROCESS_INVITE_FAILED: 'cloudprint.CloudPrintInterface.PROCESS_INVITE_FAILED',
  SEARCH_DONE: 'cloudprint.CloudPrintInterface.SEARCH_DONE',
  SEARCH_FAILED: 'cloudprint.CloudPrintInterface.SEARCH_FAILED',
  SUBMIT_DONE: 'cloudprint.CloudPrintInterface.SUBMIT_DONE',
  SUBMIT_FAILED: 'cloudprint.CloudPrintInterface.SUBMIT_FAILED',
};

cr.define('cloudprint', function() {
  'use strict';

  const CloudPrintInterfaceEventType = cloudprint.CloudPrintInterfaceEventType;

  class CloudPrintInterface extends cr.EventTarget {
    /**
     * API to the Google Cloud Print service.
     * @param {string} baseUrl Base part of the Google Cloud Print service URL
     *     with no trailing slash. For example,
     *     'https://www.google.com/cloudprint'.
     * @param {!print_preview.NativeLayer} nativeLayer Native layer used to get
     *     Auth2 tokens.
     * @param {!print_preview.UserInfo} userInfo User information repository.
     * @param {boolean} isInAppKioskMode Whether the print preview is in App
     *     Kiosk mode.
     */
    constructor(baseUrl, nativeLayer, userInfo, isInAppKioskMode) {
      super();

      /**
       * The base URL of the Google Cloud Print API.
       * @private {string}
       */
      this.baseUrl_ = baseUrl;

      /**
       * Used to get Auth2 tokens.
       * @private {!print_preview.NativeLayer}
       */
      this.nativeLayer_ = nativeLayer;

      /**
       * User information repository.
       * @private {!print_preview.UserInfo}
       */
      this.userInfo_ = userInfo;

      /**
       * Whether Print Preview is in App Kiosk mode, basically, use only
       * printers available for the device.
       * @private {boolean}
       */
      this.isInAppKioskMode_ = isInAppKioskMode;

      /**
       * Currently logged in users (identified by email) mapped to the Google
       * session index.
       * @private {!Object<number>}
       */
      this.userSessionIndex_ = {};

      /**
       * Stores last received XSRF tokens for each user account. Sent as
       * a parameter with every request.
       * @private {!Object<string>}
       */
      this.xsrfTokens_ = {};

      /**
       * Outstanding cloud destination search requests.
       * @private {!Array<!cloudprint.CloudPrintRequest>}
       */
      this.outstandingCloudSearchRequests_ = [];

      /**
       * Promise that will be resolved when the access token for
       * DestinationOrigin.DEVICE is available. Null if there is no request
       * currently pending.
       * @private {?Promise<string>}
       */
      this.accessTokenRequestPromise_ = null;
    }

    /** @return {string} Base URL of the Google Cloud Print service. */
    get baseUrl() {
      return this.baseUrl_;
    }

    /**
     * @return {boolean} Whether a search for cloud destinations is in progress.
     */
    get isCloudDestinationSearchInProgress() {
      return this.outstandingCloudSearchRequests_.length > 0;
    }

    /**
     * Sends Google Cloud Print search API request.
     * @param {?string=} opt_account Account the search is sent for. When
     *      null or omitted, the search is done on behalf of the primary user.
     * @param {print_preview.DestinationOrigin=} opt_origin When specified,
     *     searches destinations for {@code opt_origin} only, otherwise starts
     *     searches for all origins.
     */
    search(opt_account, opt_origin) {
      const account = opt_account || '';
      let origins = opt_origin && [opt_origin] || CLOUD_ORIGINS_;
      if (this.isInAppKioskMode_) {
        origins = origins.filter(function(origin) {
          return origin != print_preview.DestinationOrigin.COOKIES;
        });
      }
      this.abortSearchRequests_(origins);
      this.search_(true, account, origins);
      this.search_(false, account, origins);
    }

    /**
     * Sends Google Cloud Print search API requests.
     * @param {boolean} isRecent Whether to search for only recently used
     *     printers.
     * @param {string} account Account the search is sent for. It matters for
     *     COOKIES origin only, and can be empty (sent on behalf of the primary
     *     user in this case).
     * @param {!Array<!print_preview.DestinationOrigin>} origins Origins to
     *     search printers for.
     * @private
     */
    search_(isRecent, account, origins) {
      const params = [
        new HttpParam('connection_status', 'ALL'),
        new HttpParam('client', 'chrome'), new HttpParam('use_cdd', 'true')
      ];
      if (isRecent) {
        params.push(new HttpParam('q', '^recent'));
      }
      origins.forEach(function(origin) {
        const cpRequest = this.buildRequest_(
            'GET', 'search', params, origin, account,
            this.onSearchDone_.bind(this, isRecent));
        this.outstandingCloudSearchRequests_.push(cpRequest);
        this.sendOrQueueRequest_(cpRequest);
      }, this);
    }

    /**
     * Sends Google Cloud Print printer sharing invitations API requests.
     * @param {string} account Account the request is sent for.
     */
    invites(account) {
      const params = [
        new HttpParam('client', 'chrome'),
      ];
      this.sendOrQueueRequest_(this.buildRequest_(
          'GET', 'invites', params, print_preview.DestinationOrigin.COOKIES,
          account, this.onInvitesDone_.bind(this)));
    }

    /**
     * Accepts or rejects printer sharing invitation.
     * @param {!print_preview.Invitation} invitation Invitation to process.
     * @param {boolean} accept Whether to accept this invitation.
     */
    processInvite(invitation, accept) {
      const params = [
        new HttpParam('printerid', invitation.destination.id),
        new HttpParam('email', invitation.scopeId),
        new HttpParam('accept', accept ? 'true' : 'false'),
        new HttpParam('use_cdd', 'true'),
      ];
      this.sendOrQueueRequest_(this.buildRequest_(
          'POST', 'processinvite', params, invitation.destination.origin,
          invitation.destination.account,
          this.onProcessInviteDone_.bind(this, invitation, accept)));
    }

    /**
     * Sends a Google Cloud Print submit API request.
     * @param {!print_preview.Destination} destination Cloud destination to
     *     print to.
     * @param {string} printTicket The print ticket to print.
     * @param {!print_preview.DocumentInfo} documentInfo Document data model.
     * @param {string} data Base64 encoded data of the document.
     */
    submit(destination, printTicket, documentInfo, data) {
      const result = VERSION_REGEXP_.exec(navigator.userAgent);
      let chromeVersion = 'unknown';
      if (result && result.length == 2) {
        chromeVersion = result[1];
      }
      const params = [
        new HttpParam('printerid', destination.id),
        new HttpParam('contentType', 'dataUrl'),
        new HttpParam('title', documentInfo.title),
        new HttpParam('ticket', printTicket),
        new HttpParam('content', 'data:application/pdf;base64,' + data),
        new HttpParam('tag', '__google__chrome_version=' + chromeVersion),
        new HttpParam('tag', '__google__os=' + navigator.platform)
      ];
      const cpRequest = this.buildRequest_(
          'POST', 'submit', params, destination.origin, destination.account,
          this.onSubmitDone_.bind(this));
      this.sendOrQueueRequest_(cpRequest);
    }

    /**
     * Sends a Google Cloud Print printer API request.
     * @param {string} printerId ID of the printer to lookup.
     * @param {!print_preview.DestinationOrigin} origin Origin of the printer.
     * @param {string=} account Account this printer is registered for. When
     *     provided for COOKIES {@code origin}, and users sessions are still not
     *     known, will be checked against the response (both success and failure
     *     to get printer) and, if the active user account is not the one
     *     requested, {@code account} is activated and printer request reissued.
     */
    printer(printerId, origin, account) {
      const params = [
        new HttpParam('printerid', printerId), new HttpParam('use_cdd', 'true'),
        new HttpParam('printer_connection_status', 'true')
      ];
      this.sendOrQueueRequest_(this.buildRequest_(
          'GET', 'printer', params, origin, account || '',
          this.onPrinterDone_.bind(this, printerId)));
    }

    /**
     * Builds request to the Google Cloud Print API.
     * @param {string} method HTTP method of the request.
     * @param {string} action Google Cloud Print action to perform.
     * @param {Array<!HttpParam>} params HTTP parameters to include in the
     *     request.
     * @param {!print_preview.DestinationOrigin} origin Origin for destination.
     * @param {?string} account Account the request is sent for. Can be
     *     {@code null} or empty string if the request is not cookie bound or
     *     is sent on behalf of the primary user.
     * @param {function(!cloudprint.CloudPrintRequest)} callback Callback to
     *     invoke when request completes.
     * @return {!cloudprint.CloudPrintRequest} Partially prepared request.
     * @private
     */
    buildRequest_(method, action, params, origin, account, callback) {
      let url = this.baseUrl_ + '/' + action + '?xsrf=';
      if (origin == print_preview.DestinationOrigin.COOKIES) {
        const xsrfToken = this.xsrfTokens_[account];
        if (!xsrfToken) {
          // TODO(rltoscano): Should throw an error if not a read-only action or
          // issue an xsrf token request.
        } else {
          url = url + xsrfToken;
        }
        if (account) {
          const index = this.userSessionIndex_[account] || 0;
          if (index > 0) {
            url += '&authuser=' + index;
          }
        }
      }
      let body = null;
      if (params) {
        if (method == 'GET') {
          url = params.reduce(function(partialUrl, param) {
            return partialUrl + '&' + param.name + '=' +
                encodeURIComponent(param.value);
          }, url);
        } else if (method == 'POST') {
          body = params.reduce(function(partialBody, param) {
            return partialBody + 'Content-Disposition: form-data; name=\"' +
                param.name + '\"\r\n\r\n' + param.value + '\r\n--' +
                MULTIPART_BOUNDARY_ + '\r\n';
          }, '--' + MULTIPART_BOUNDARY_ + '\r\n');
        }
      }

      const headers = {};
      headers['X-CloudPrint-Proxy'] = 'ChromePrintPreview';
      if (method == 'GET') {
        headers['Content-Type'] = URL_ENCODED_CONTENT_TYPE_;
      } else if (method == 'POST') {
        headers['Content-Type'] = MULTIPART_CONTENT_TYPE_;
      }

      const xhr = new XMLHttpRequest();
      xhr.open(method, url, true);
      xhr.withCredentials = (origin == print_preview.DestinationOrigin.COOKIES);
      for (const header in headers) {
        xhr.setRequestHeader(header, headers[header]);
      }

      return new cloudprint.CloudPrintRequest(
          xhr, body, origin, account, callback);
    }

    /**
     * Sends a request to the Google Cloud Print API or queues if it needs to
     *     wait OAuth2 access token.
     * @param {!cloudprint.CloudPrintRequest} request Request to send or queue.
     * @private
     */
    sendOrQueueRequest_(request) {
      if (request.origin == print_preview.DestinationOrigin.COOKIES) {
        this.sendRequest_(request);
        return;
      }

      if (this.accessTokenRequestPromise_ == null) {
        this.accessTokenRequestPromise_ =
            this.nativeLayer_.getAccessToken(request.origin);
      }

      this.accessTokenRequestPromise_.then(
          this.onAccessTokenReady_.bind(this, request));
    }

    /**
     * Sends a request to the Google Cloud Print API.
     * @param {!cloudprint.CloudPrintRequest} request Request to send.
     * @private
     */
    sendRequest_(request) {
      request.xhr.onreadystatechange =
          this.onReadyStateChange_.bind(this, request);
      request.xhr.send(request.body);
    }

    /**
     * Creates a Google Cloud Print interface error that is ready to dispatch.
     * @param {!cloudprint.CloudPrintInterfaceEventType} type Type of the
     *     error.
     * @param {!cloudprint.CloudPrintRequest} request Request that has been
     *     completed.
     * @return {!Event} Google Cloud Print interface error event.
     * @private
     */
    createErrorEvent_(type, request) {
      const errorEvent = new Event(type);
      errorEvent.status = request.xhr.status;
      if (request.xhr.status == 200) {
        errorEvent.errorCode = request.result['errorCode'];
        errorEvent.message = request.result['message'];
      } else {
        errorEvent.errorCode = 0;
        errorEvent.message = '';
      }
      errorEvent.origin = request.origin;
      return errorEvent;
    }

    /**
     * Updates user info and session index from the {@code request} response.
     * @param {!cloudprint.CloudPrintRequest} request Request to extract user
     *     info from.
     * @private
     */
    setUsers_(request) {
      if (request.origin == print_preview.DestinationOrigin.COOKIES) {
        const users = request.result['request']['users'] || [];
        this.userSessionIndex_ = {};
        for (let i = 0; i < users.length; i++) {
          this.userSessionIndex_[users[i]] = i;
        }
        this.userInfo_.setUsers(request.result['request']['user'], users);
      }
    }

    /**
     * Terminates search requests for requested {@code origins}.
     * @param {!Array<print_preview.DestinationOrigin>} origins Origins
     *     to terminate search requests for.
     * @private
     */
    abortSearchRequests_(origins) {
      this.outstandingCloudSearchRequests_ =
          this.outstandingCloudSearchRequests_.filter(function(request) {
            if (origins.indexOf(request.origin) >= 0) {
              request.xhr.abort();
              return false;
            }
            return true;
          });
    }

    /**
     * Called when a native layer receives access token. Assumes that the
     * destination type for this token is DestinationOrigin.DEVICE.
     * @param {cloudprint.CloudPrintRequest} request The pending request that
     *     requires the access token.
     * @param {string} accessToken The access token obtained.
     * @private
     */
    onAccessTokenReady_(request, accessToken) {
      assert(request.origin == print_preview.DestinationOrigin.DEVICE);
      if (accessToken) {
        request.xhr.setRequestHeader(
            'Authorization', 'Bearer ' + accessToken);
        this.sendRequest_(request);
      } else {  // No valid token.
        // Without abort status does not exist.
        request.xhr.abort();
        request.callback(request);
      }
      this.accessTokenRequestPromise_ = null;
    }

    /**
     * Called when the ready-state of a XML http request changes.
     * Calls the successCallback with the result or dispatches an ERROR event.
     * @param {!cloudprint.CloudPrintRequest} request Request that was changed.
     * @private
     */
    onReadyStateChange_(request) {
      if (request.xhr.readyState == 4) {
        if (request.xhr.status == 200) {
          request.result =
              /** @type {Object} */ (JSON.parse(request.xhr.responseText));
          if (request.origin == print_preview.DestinationOrigin.COOKIES &&
              request.result['success']) {
            this.xsrfTokens_[request.result['request']['user']] =
                request.result['xsrf_token'];
          }
        }
        request.callback(request);
      }
    }

    /**
     * Called when the search request completes.
     * @param {boolean} isRecent Whether the search request was for recent
     *     destinations.
     * @param {!cloudprint.CloudPrintRequest} request Request that has been
     *     completed.
     * @private
     */
    onSearchDone_(isRecent, request) {
      let lastRequestForThisOrigin = true;
      this.outstandingCloudSearchRequests_ =
          this.outstandingCloudSearchRequests_.filter(function(item) {
            if (item != request && item.origin == request.origin) {
              lastRequestForThisOrigin = false;
            }
            return item != request;
          });
      let activeUser = '';
      if (request.origin == print_preview.DestinationOrigin.COOKIES) {
        activeUser = request.result && request.result['request'] &&
            request.result['request']['user'];
      }
      let event = null;
      if (request.xhr.status == 200 && request.result['success']) {
        // Extract printers.
        const printerListJson = request.result['printers'] || [];
        const printerList = [];
        printerListJson.forEach(function(printerJson) {
          try {
            printerList.push(cloudprint.parseCloudDestination(
                printerJson, request.origin, activeUser));
          } catch (err) {
            console.error('Unable to parse cloud print destination: ' + err);
          }
        });
        // Extract and store users.
        this.setUsers_(request);
        // Dispatch SEARCH_DONE event.
        event = new Event(CloudPrintInterfaceEventType.SEARCH_DONE);
        event.origin = request.origin;
        event.printers = printerList;
        event.isRecent = isRecent;
      } else {
        event = this.createErrorEvent_(
            CloudPrintInterfaceEventType.SEARCH_FAILED, request);
      }
      event.user = activeUser;
      event.searchDone = lastRequestForThisOrigin;
      this.dispatchEvent(event);
    }

    /**
     * Called when invitations search request completes.
     * @param {!cloudprint.CloudPrintRequest} request Request that has been
     *     completed.
     * @private
     */
    onInvitesDone_(request) {
      let event = null;
      const activeUser = (request.result && request.result['request'] &&
                          request.result['request']['user']) ||
          '';
      if (request.xhr.status == 200 && request.result['success']) {
        // Extract invitations.
        const invitationListJson = request.result['invites'] || [];
        const invitationList = [];
        invitationListJson.forEach(function(invitationJson) {
          try {
            invitationList.push(
                cloudprint.parseInvitation(invitationJson, activeUser));
          } catch (e) {
            console.error('Unable to parse invitation: ' + e);
          }
        });
        // Dispatch INVITES_DONE event.
        event = new Event(CloudPrintInterfaceEventType.INVITES_DONE);
        event.invitations = invitationList;
      } else {
        event = this.createErrorEvent_(
            CloudPrintInterfaceEventType.INVITES_FAILED, request);
      }
      event.user = activeUser;
      this.dispatchEvent(event);
    }

    /**
     * Called when invitation processing request completes.
     * @param {!print_preview.Invitation} invitation Processed invitation.
     * @param {boolean} accept Whether this invitation was accepted or rejected.
     * @param {!cloudprint.CloudPrintRequest} request Request that has been
     *     completed.
     * @private
     */
    onProcessInviteDone_(invitation, accept, request) {
      let event = null;
      const activeUser = (request.result && request.result['request'] &&
                          request.result['request']['user']) ||
          '';
      if (request.xhr.status == 200 && request.result['success']) {
        event = new Event(CloudPrintInterfaceEventType.PROCESS_INVITE_DONE);
        if (accept) {
          try {
            event.printer = cloudprint.parseCloudDestination(
                request.result['printer'], request.origin, activeUser);
          } catch (e) {
            console.error('Failed to parse cloud print destination: ' + e);
          }
        }
      } else {
        event = this.createErrorEvent_(
            CloudPrintInterfaceEventType.PROCESS_INVITE_FAILED, request);
      }
      event.invitation = invitation;
      event.accept = accept;
      event.user = activeUser;
      this.dispatchEvent(event);
    }

    /**
     * Called when the submit request completes.
     * @param {!cloudprint.CloudPrintRequest} request Request that has been
     *     completed.
     * @private
     */
    onSubmitDone_(request) {
      if (request.xhr.status == 200 && request.result['success']) {
        const submitDoneEvent =
            new Event(CloudPrintInterfaceEventType.SUBMIT_DONE);
        submitDoneEvent.jobId = request.result['job']['id'];
        this.dispatchEvent(submitDoneEvent);
      } else {
        const errorEvent = this.createErrorEvent_(
            CloudPrintInterfaceEventType.SUBMIT_FAILED, request);
        this.dispatchEvent(errorEvent);
      }
    }

    /**
     * Called when the printer request completes.
     * @param {string} destinationId ID of the destination that was looked up.
     * @param {!cloudprint.CloudPrintRequest} request Request that has been
     *     completed.
     * @private
     */
    onPrinterDone_(destinationId, request) {
      // Special handling of the first printer request. It does not matter at
      // this point, whether printer was found or not.
      if (request.origin == print_preview.DestinationOrigin.COOKIES &&
          request.result && request.account &&
          request.result['request']['user'] &&
          request.result['request']['users'] &&
          request.account != request.result['request']['user']) {
        this.setUsers_(request);
        // In case the user account is known, but not the primary one,
        // activate it.
        if (this.userSessionIndex_[request.account] > 0) {
          this.userInfo_.activeUser = request.account;
          // Repeat the request for the newly activated account.
          this.printer(
              request.result['request']['params']['printerid'], request.origin,
              request.account);
          // Stop processing this request, wait for the new response.
          return;
        }
      }
      // Process response.
      if (request.xhr.status == 200 && request.result['success']) {
        let activeUser = '';
        if (request.origin == print_preview.DestinationOrigin.COOKIES) {
          activeUser = request.result['request']['user'];
        }
        const printerJson = request.result['printers'][0];
        let printer;
        try {
          printer = cloudprint.parseCloudDestination(
              printerJson, request.origin, activeUser);
        } catch (err) {
          console.error(
              'Failed to parse cloud print destination: ' +
              JSON.stringify(printerJson));
          return;
        }
        const printerDoneEvent =
            new Event(CloudPrintInterfaceEventType.PRINTER_DONE);
        printerDoneEvent.printer = printer;
        this.dispatchEvent(printerDoneEvent);
      } else {
        const errorEvent = this.createErrorEvent_(
            CloudPrintInterfaceEventType.PRINTER_FAILED, request);
        errorEvent.destinationId = destinationId;
        errorEvent.destinationOrigin = request.origin;
        this.dispatchEvent(errorEvent);
      }
    }
  }

  /**
   * Content type header value for a URL encoded HTTP request.
   * @const {string}
   * @private
   */
  const URL_ENCODED_CONTENT_TYPE_ = 'application/x-www-form-urlencoded';

  /**
   * Multi-part POST request boundary used in communication with Google
   * Cloud Print.
   * @const {string}
   * @private
   */
  const MULTIPART_BOUNDARY_ = '----CloudPrintFormBoundaryjc9wuprokl8i';

  /**
   * Content type header value for a multipart HTTP request.
   * @const {string}
   * @private
   */
  const MULTIPART_CONTENT_TYPE_ =
      'multipart/form-data; boundary=' + MULTIPART_BOUNDARY_;

  /**
   * Regex that extracts Chrome's version from the user-agent string.
   * @const {!RegExp}
   * @private
   */
  const VERSION_REGEXP_ = /.*Chrome\/([\d\.]+)/i;

  /**
   * Could Print origins used to search printers.
   * @const {!Array<!print_preview.DestinationOrigin>}
   * @private
   */
  const CLOUD_ORIGINS_ = [
    print_preview.DestinationOrigin.COOKIES,
    print_preview.DestinationOrigin.DEVICE
  ];

  class CloudPrintRequest {
    /**
     * Data structure that holds data for Cloud Print requests.
     * @param {!XMLHttpRequest} xhr Partially prepared http request.
     * @param {string} body Data to send with POST requests.
     * @param {!print_preview.DestinationOrigin} origin Origin for destination.
     * @param {?string} account Account the request is sent for. Can be
     *     {@code null} or empty string if the request is not cookie bound or
     *     is sent on behalf of the primary user.
     * @param {function(!cloudprint.CloudPrintRequest)} callback Callback to
     *     invoke when request completes.
     */
    constructor(xhr, body, origin, account, callback) {
      /**
       * Partially prepared http request.
       * @type {!XMLHttpRequest}
       */
      this.xhr = xhr;

      /**
       * Data to send with POST requests.
       * @type {string}
       */
      this.body = body;

      /**
       * Origin for destination.
       * @type {!print_preview.DestinationOrigin}
       */
      this.origin = origin;

      /**
       * User account this request is expected to be executed for.
       * @type {?string}
       */
      this.account = account;

      /**
       * Callback to invoke when request completes.
       * @type {function(!cloudprint.CloudPrintRequest)}
       */
      this.callback = callback;

      /**
       * Result for requests.
       * @type {Object} JSON response.
       */
      this.result = null;
    }
  }

  class HttpParam {
    /**
     * Data structure that represents an HTTP parameter.
     * @param {string} name Name of the parameter.
     * @param {string} value Value of the parameter.
     */
    constructor(name, value) {
      /**
       * Name of the parameter.
       * @type {string}
       */
      this.name = name;

      /**
       * Name of the value.
       * @type {string}
       */
      this.value = value;
    }
  }

  // Export
  return {
    CloudPrintInterface: CloudPrintInterface,
    CloudPrintRequest: CloudPrintRequest,
  };
});
