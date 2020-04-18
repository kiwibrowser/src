// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// chrome.runtime.messaging API implementation.
// TODO(robwu): Fix this indentation.

  // TODO(kalman): factor requiring chrome out of here.
  var chrome = requireNative('chrome').GetChrome();
  var logActivity = requireNative('activityLogger');
  var logging = requireNative('logging');
  var messagingNatives = requireNative('messaging_natives');
  var processNatives = requireNative('process');
  var utils = require('utils');
  var messagingUtils = require('messaging_utils');

  // The reserved channel name for the sendRequest/send(Native)Message APIs.
  // Note: sendRequest is deprecated.
  var kRequestChannel = "chrome.extension.sendRequest";
  var kMessageChannel = "chrome.runtime.sendMessage";
  var kNativeMessageChannel = "chrome.runtime.sendNativeMessage";
  var kPortClosedError = 'Attempting to use a disconnected port object';

  var jsEvent;
  function createAnonymousEvent(schema) {
    if (bindingUtil) {
      var supportsFilters = false;
      var supportsLazyListeners = false;
      // Native custom events ignore schema.
      return bindingUtil.createCustomEvent(undefined, undefined,
                                           supportsFilters,
                                           supportsLazyListeners);
    }
    var options = {
      __proto__: null,
      unmanaged: true,
    };
    if (!jsEvent)
      jsEvent = require('event_bindings').Event;
    return new jsEvent(undefined, schema, options);
  }

  function invalidateEvent(event) {
    if (bindingUtil)
      bindingUtil.invalidateEvent(event);
    else
      privates(event).impl.destroy_();
  }

  var jsLastError = bindingUtil ? undefined : require('lastError');
  function setLastError(name, error) {
    if (bindingUtil)
      bindingUtil.setLastError(error);
    else
      jsLastError.set(name, error, null, chrome);
  }

  function clearLastError() {
    if (bindingUtil)
      bindingUtil.clearLastError();
    else
      jsLastError.clear(chrome);
  }

  function hasLastError() {
    if (bindingUtil)
      return bindingUtil.hasLastError();
    else
      return jsLastError.hasError(chrome);
  }

  // Map of port IDs to port object.
  var ports = {__proto__: null};

  // Port object.  Represents a connection to another script context through
  // which messages can be passed.
  function PortImpl(portId, opt_name) {
    this.portId_ = portId;
    this.name = opt_name;

    // Note: Keep these schemas in sync with the documentation in runtime.json
    var portSchema = {
      __proto__: null,
      name: 'port',
      $ref: 'runtime.Port',
    };
    var messageSchema = {
      __proto__: null,
      name: 'message',
      type: 'any',
      optional: true,
    };
    this.onDisconnect = createAnonymousEvent([portSchema]);
    this.onMessage = createAnonymousEvent([messageSchema, portSchema]);
  }
  $Object.setPrototypeOf(PortImpl.prototype, null);

  // Sends a message asynchronously to the context on the other end of this
  // port.
  PortImpl.prototype.postMessage = function(msg) {
    if (!$Object.hasOwnProperty(ports, this.portId_))
      throw new Error(kPortClosedError);

    // JSON.stringify doesn't support a root object which is undefined.
    if (msg === undefined)
      msg = null;
    msg = $JSON.stringify(msg);
    if (msg === undefined) {
      // JSON.stringify can fail with unserializable objects. Log an error and
      // drop the message.
      //
      // TODO(kalman/mpcomplete): it would be better to do the same validation
      // here that we do for runtime.sendMessage (and variants), i.e. throw an
      // schema validation Error, but just maintain the old behavior until
      // there's a good reason not to (http://crbug.com/263077).
      console.error('Illegal argument to Port.postMessage');
      return;
    }
    var error = messagingNatives.PostMessage(this.portId_, msg);
    if (error)
      throw new Error(error);
  };

  // Disconnects the port from the other end.
  PortImpl.prototype.disconnect = function() {
    if (!$Object.hasOwnProperty(ports, this.portId_))
      return;  // disconnect() on an already-closed port is a no-op.
    messagingNatives.CloseChannel(this.portId_, true);
    this.destroy_();
  };

  // Close this specific port without forcing the channel to close. The channel
  // will close if this was the only port at this end of the channel.
  PortImpl.prototype.disconnectSoftly = function() {
    if (!$Object.hasOwnProperty(ports, this.portId_))
      return;
    messagingNatives.CloseChannel(this.portId_, false);
    this.destroy_();
  };

  PortImpl.prototype.destroy_ = function() {
    invalidateEvent(this.onDisconnect);
    invalidateEvent(this.onMessage);
    delete ports[this.portId_];
  };

  // Hidden port creation function.  We don't want to expose an API that lets
  // people add arbitrary port IDs to the port list.
  function createPort(portId, opt_name) {
    if (ports[portId])
      throw new Error("Port '" + portId + "' already exists.");
    var port = new Port(portId, opt_name);
    ports[portId] = port;
    return port;
  };

  // Helper function for dispatchOnRequest.
  function handleSendRequestError(isSendMessage,
                                  responseCallbackPreserved,
                                  sourceExtensionId,
                                  targetExtensionId,
                                  sourceUrl) {
    var errorMsg;
    var eventName = isSendMessage ? 'runtime.onMessage' : 'extension.onRequest';
    if (isSendMessage && !responseCallbackPreserved) {
      errorMsg =
        'The chrome.' + eventName + ' listener must return true if you ' +
        'want to send a response after the listener returns';
    } else {
      errorMsg =
        'Cannot send a response more than once per chrome.' + eventName +
        ' listener per document';
    }
    errorMsg += ' (message was sent by extension' + sourceExtensionId;
    if (sourceExtensionId && sourceExtensionId !== targetExtensionId)
      errorMsg += ' for extension ' + targetExtensionId;
    if (sourceUrl)
      errorMsg += ' for URL ' + sourceUrl;
    errorMsg += ').';
    setLastError(eventName, errorMsg);
  }

  // Helper function for dispatchOnConnect
  function dispatchOnRequest(portId, channelName, sender,
                             sourceExtensionId, targetExtensionId, sourceUrl,
                             isExternal) {
    var isSendMessage = channelName == kMessageChannel;
    var requestEvent = null;
    if (isSendMessage) {
      if (chrome.runtime) {
        requestEvent = isExternal ? chrome.runtime.onMessageExternal
                                  : chrome.runtime.onMessage;
      }
    } else {
      if (chrome.extension) {
        requestEvent = isExternal ? chrome.extension.onRequestExternal
                                  : chrome.extension.onRequest;
      }
    }
    if (!requestEvent)
      return false;
    if (!requestEvent.hasListeners())
      return false;
    var port = createPort(portId, channelName);

    function messageListener(request) {
      var responseCallbackPreserved = false;
      var responseCallback = function(response) {
        if (port) {
          port.postMessage(response);
          // TODO(robwu): This can be changed to disconnect() because there is
          // no point in allowing other receivers at this end of the port to
          // keep the channel alive because the opener port can only receive one
          // message.
          privates(port).impl.disconnectSoftly();
          port = null;
        } else {
          // We nulled out port when sending the response, and now the page
          // is trying to send another response for the same request.
          handleSendRequestError(isSendMessage, responseCallbackPreserved,
                                 sourceExtensionId, targetExtensionId);
        }
      };
      // In case the extension never invokes the responseCallback, and also
      // doesn't keep a reference to it, we need to clean up the port. Do
      // so by attaching to the garbage collection of the responseCallback
      // using some native hackery.
      //
      // If the context is destroyed before this has a chance to execute,
      // BindToGC knows to release |portId| (important for updating C++ state
      // both in this renderer and on the other end). We don't need to clear
      // any JavaScript state, as calling destroy_() would usually do - but
      // the context has been destroyed, so there isn't any JS state to clear.
      messagingNatives.BindToGC(responseCallback, function() {
        if (port) {
          privates(port).impl.disconnectSoftly();
          port = null;
        }
      }, portId);
      var rv = requestEvent.dispatch(request, sender, responseCallback);
      if (isSendMessage) {
        responseCallbackPreserved =
            rv && rv.results && $Array.indexOf(rv.results, true) > -1;
        if (!responseCallbackPreserved && port) {
          // If they didn't access the response callback, they're not
          // going to send a response, so clean up the port immediately.
          privates(port).impl.disconnectSoftly();
          port = null;
        }
      }
    }

    port.onMessage.addListener(messageListener);

    var eventName = isSendMessage ? "runtime.onMessage" : "extension.onRequest";
    if (isExternal)
      eventName += "External";
    logActivity.LogEvent(targetExtensionId,
                         eventName,
                         [sourceExtensionId, sourceUrl]);
    return true;
  }

  // Called by native code when a channel has been opened to this context.
  function dispatchOnConnect(portId,
                             channelName,
                             sourceTab,
                             sourceFrameId,
                             guestProcessId,
                             guestRenderFrameRoutingId,
                             sourceExtensionId,
                             targetExtensionId,
                             sourceUrl,
                             tlsChannelId) {
    var wasPortUsed = dispatchOnConnectImpl(portId, channelName, sourceTab,
                                            sourceFrameId, guestProcessId,
                                            guestRenderFrameRoutingId,
                                            sourceExtensionId,
                                            targetExtensionId, sourceUrl,
                                            tlsChannelId);
    if (!wasPortUsed) {
      // Since the JS to dispatch the connect event can (in rare cases) be
      // executed asynchronously from when we check if there are associated
      // listeners in the native code, it's possible that the listeners have
      // since been removed. If that's the case (though unlikely), remove the
      // port.
      messagingNatives.CloseChannel(portId, false /* force_close */);
    }
  }

  // Helper function to dispatchOnConnect that returns true if the new port
  // was used.
  function dispatchOnConnectImpl(portId,
                                 channelName,
                                 sourceTab,
                                 sourceFrameId,
                                 guestProcessId,
                                 guestRenderFrameRoutingId,
                                 sourceExtensionId,
                                 targetExtensionId,
                                 sourceUrl,
                                 tlsChannelId) {
    // Only create a new Port if someone is actually listening for a connection.
    // In addition to being an optimization, this also fixes a bug where if 2
    // channels were opened to and from the same process, closing one would
    // close both.
    var extensionId = processNatives.GetExtensionId();

    // messaging_bindings.cc should ensure that this method only gets called for
    // the right extension.
    logging.CHECK(targetExtensionId == extensionId);

    // Determine whether this is coming from another extension, so we can use
    // the right event.
    var isExternal = sourceExtensionId != extensionId;

    var sender = {};
    if (sourceExtensionId != '')
      sender.id = sourceExtensionId;
    if (sourceUrl)
      sender.url = sourceUrl;
    if (sourceTab)
      sender.tab = sourceTab;
    if (sourceFrameId >= 0)
      sender.frameId = sourceFrameId;
    if (typeof guestProcessId !== 'undefined' &&
        typeof guestRenderFrameRoutingId !== 'undefined') {
      // Note that |guestProcessId| and |guestRenderFrameRoutingId| are not
      // standard fields on MessageSender and should not be exposed to drive-by
      // extensions; it is only exposed to component extensions.
      logging.CHECK(processNatives.IsComponentExtension(),
          "GuestProcessId can only be exposed to component extensions.");
      sender.guestProcessId = guestProcessId;
      sender.guestRenderFrameRoutingId = guestRenderFrameRoutingId;
    }
    if (typeof tlsChannelId != 'undefined')
      sender.tlsChannelId = tlsChannelId;

    // Special case for sendRequest/onRequest and sendMessage/onMessage.
    if (channelName == kRequestChannel || channelName == kMessageChannel) {
      return dispatchOnRequest(portId, channelName, sender,
                               sourceExtensionId, targetExtensionId, sourceUrl,
                               isExternal);
    }

    var connectEvent = null;
    if (chrome.runtime) {
      connectEvent = isExternal ? chrome.runtime.onConnectExternal
                                : chrome.runtime.onConnect;
    }
    if (!connectEvent)
      return false;
    if (!connectEvent.hasListeners())
      return false;

    var port = createPort(portId, channelName);
    port.sender = sender;
    if (processNatives.manifestVersion < 2)
      port.tab = port.sender.tab;

    var eventName = (isExternal ?
        "runtime.onConnectExternal" : "runtime.onConnect");
    connectEvent.dispatch(port);
    logActivity.LogEvent(targetExtensionId,
                         eventName,
                         [sourceExtensionId]);
    return true;
  };

  // Called by native code when a channel has been closed.
  function dispatchOnDisconnect(portId, errorMessage) {
    var port = ports[portId];
    if (port) {
      delete ports[portId];
      if (errorMessage)
        setLastError('Port', errorMessage);
      try {
        port.onDisconnect.dispatch(port);
      } finally {
        privates(port).impl.destroy_();
        clearLastError();
      }
    }
  };

  // Called by native code when a message has been sent to the given port.
  function dispatchOnMessage(msg, portId) {
    var port = ports[portId];
    if (port) {
      if (msg)
        msg = $JSON.parse(msg);
      port.onMessage.dispatch(msg, port);
    }
  };

  // Shared implementation used by tabs.sendMessage and runtime.sendMessage.
  function sendMessageImpl(port, request, responseCallback) {
    if (port.name != kNativeMessageChannel)
      port.postMessage(request);

    if (port.name == kMessageChannel && !responseCallback) {
      // TODO(mpcomplete): Do this for the old sendRequest API too, after
      // verifying it doesn't break anything.
      // Go ahead and disconnect immediately if the sender is not expecting
      // a response.
      port.disconnect();
      return;
    }

    function sendResponseAndClearCallback(response) {
      // Save a reference so that we don't re-entrantly call responseCallback.
      var sendResponse = responseCallback;
      responseCallback = null;
      if (arguments.length === 0) {
        // According to the documentation of chrome.runtime.sendMessage, the
        // callback is invoked without any arguments when an error occurs.
        sendResponse();
      } else {
        sendResponse(response);
      }
    }


    // Note: make sure to manually remove the onMessage/onDisconnect listeners
    // that we added before destroying the Port, a workaround to a bug in Port
    // where any onMessage/onDisconnect listeners added but not removed will
    // be leaked when the Port is destroyed.
    // http://crbug.com/320723 tracks a sustainable fix.

    function disconnectListener() {
      if (!responseCallback)
        return;

      if (hasLastError()) {
        sendResponseAndClearCallback();
      } else {
        setLastError(
            port.name,
            'The message port closed before a response was received.');
        try {
          sendResponseAndClearCallback();
        } finally {
          clearLastError();
        }
      }
    }

    function messageListener(response) {
      try {
        if (responseCallback)
          sendResponseAndClearCallback(response);
      } finally {
        port.disconnect();
      }
    }

    port.onDisconnect.addListener(disconnectListener);
    port.onMessage.addListener(messageListener);
  };

  function sendMessageUpdateArguments(functionName, hasOptionsArgument) {
    // skip functionName and hasOptionsArgument
    var args = $Array.slice(arguments, 2);
    var alignedArgs = messagingUtils.alignSendMessageArguments(args,
        hasOptionsArgument);
    if (!alignedArgs)
      throw new Error('Invalid arguments to ' + functionName + '.');
    return alignedArgs;
  }

  function Port() {
    privates(Port).constructPrivate(this, arguments);
  }
  utils.expose(Port, PortImpl, {
    functions: [
      'disconnect',
      'postMessage',
    ],
    properties: [
      'name',
      'onDisconnect',
      'onMessage',
    ],
  });

exports.$set('kRequestChannel', kRequestChannel);
exports.$set('kMessageChannel', kMessageChannel);
exports.$set('kNativeMessageChannel', kNativeMessageChannel);
exports.$set('Port', Port);
exports.$set('createPort', createPort);
exports.$set('sendMessageImpl', sendMessageImpl);
exports.$set('sendMessageUpdateArguments', sendMessageUpdateArguments);

// For C++ code to call.
exports.$set('dispatchOnConnect', dispatchOnConnect);
exports.$set('dispatchOnDisconnect', dispatchOnDisconnect);
exports.$set('dispatchOnMessage', dispatchOnMessage);
