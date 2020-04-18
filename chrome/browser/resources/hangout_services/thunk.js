// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.runtime.onMessageExternal.addListener(function(
    message, sender, sendResponse) {
  function doSendResponse(value, errorString) {
    var error = null;
    if (errorString) {
      error = {};
      error['name'] = 'ComponentExtensionError';
      error['message'] = errorString;
    }

    var errorMessage = error || chrome.runtime.lastError;
    sendResponse({'value': value, 'error': errorMessage});
  }

  function getHost(url) {
    if (!url)
      return '';
    // Use the DOM to parse the URL. Since we don't add the anchor to
    // the page, this is the only reference to it and it will be
    // deleted once it's gone out of scope.
    var a = document.createElement('a');
    a.href = url;
    var origin = a.protocol + '//' + a.hostname;
    if (a.port != '')
      origin = origin + ':' + a.port;
    origin = origin + '/';
    return origin;
  }

  try {
    var requestInfo = {};

    // Set the tab ID. If it's passed in the message, use that.
    // Otherwise use the sender information.
    if (message['tabId']) {
      requestInfo['tabId'] = +message['tabId'];
      if (isNaN(requestInfo['tabId'])) {
        throw new Error(
            'Cannot convert tab ID string to integer: ' + message['tabId']);
      }
    } else if (sender.tab) {
      requestInfo['tabId'] = sender.tab.id;
    }

    if (sender.guestProcessId) {
      requestInfo['guestProcessId'] = sender.guestProcessId;
    }

    var method = message['method'];

    // Set the origin. If a URL is passed in the message, use that.
    // Otherwise use the sender information.
    var origin;
    if (message['winUrl']) {
      origin = getHost(message['winUrl']);
    } else {
      origin = getHost(sender.url);
    }

    if (method == 'cpu.getInfo') {
      chrome.system.cpu.getInfo(doSendResponse);
      return true;
    } else if (method == 'logging.setMetadata') {
      var metaData = message['metaData'];
      chrome.webrtcLoggingPrivate.setMetaData(
          requestInfo, origin, metaData, doSendResponse);
      return true;
    } else if (method == 'logging.start') {
      chrome.webrtcLoggingPrivate.start(requestInfo, origin, doSendResponse);
      return true;
    } else if (method == 'logging.uploadOnRenderClose') {
      chrome.webrtcLoggingPrivate.setUploadOnRenderClose(
          requestInfo, origin, true);
      doSendResponse();
      return false;
    } else if (method == 'logging.noUploadOnRenderClose') {
      chrome.webrtcLoggingPrivate.setUploadOnRenderClose(
          requestInfo, origin, false);
      doSendResponse();
      return false;
    } else if (method == 'logging.stop') {
      chrome.webrtcLoggingPrivate.stop(requestInfo, origin, doSendResponse);
      return true;
    } else if (method == 'logging.upload') {
      chrome.webrtcLoggingPrivate.upload(requestInfo, origin, doSendResponse);
      return true;
    } else if (method == 'logging.uploadStored') {
      var logId = message['logId'];
      chrome.webrtcLoggingPrivate.uploadStored(
          requestInfo, origin, logId, doSendResponse);
      return true;
    } else if (method == 'logging.stopAndUpload') {
      // Stop everything and upload. This is allowed to be called even if
      // logs have already been stopped or not started. Therefore, ignore
      // any errors along the way, but store them, so that if upload fails
      // they are all reported back.
      // Stop incoming and outgoing RTP dumps separately, otherwise
      // stopRtpDump will fail and not stop anything if either type has not
      // been started.
      var errors = [];
      chrome.webrtcLoggingPrivate.stopRtpDump(
          requestInfo, origin, true /* incoming */, false /* outgoing */,
          function() {
            appendLastErrorMessage(errors);
            chrome.webrtcLoggingPrivate.stopRtpDump(
                requestInfo, origin, false /* incoming */, true /* outgoing */,
                function() {
                  appendLastErrorMessage(errors);
                  chrome.webrtcLoggingPrivate.stop(
                      requestInfo, origin, function() {
                        appendLastErrorMessage(errors);
                        chrome.webrtcLoggingPrivate.upload(
                            requestInfo, origin, function(uploadValue) {
                              var errorMessage = null;
                              // If upload fails, report all previous errors.
                              // Otherwise, throw them away.
                              if (chrome.runtime.lastError !== undefined) {
                                appendLastErrorMessage(errors);
                                errorMessage = errors.join('; ');
                              }
                              doSendResponse(uploadValue, errorMessage);
                            });
                      });
                });
          });
      return true;
    } else if (method == 'logging.store') {
      var logId = message['logId'];
      chrome.webrtcLoggingPrivate.store(
          requestInfo, origin, logId, doSendResponse);
      return true;
    } else if (method == 'logging.discard') {
      chrome.webrtcLoggingPrivate.discard(requestInfo, origin, doSendResponse);
      return true;
    } else if (method == 'getSinks') {
      chrome.webrtcAudioPrivate.getSinks(doSendResponse);
      return true;
    } else if (method == 'getAssociatedSink') {
      var sourceId = message['sourceId'];
      chrome.webrtcAudioPrivate.getAssociatedSink(
          origin, sourceId, doSendResponse);
      return true;
    } else if (method == 'isExtensionEnabled') {
      // This method is necessary because there may be more than one
      // version of this extension, under different extension IDs. By
      // first calling this method on the extension ID, the client can
      // check if it's loaded; if it's not, the extension system will
      // call the callback with no arguments and set
      // chrome.runtime.lastError.
      doSendResponse();
      return false;
    } else if (method == 'getNaclArchitecture') {
      chrome.runtime.getPlatformInfo(function(obj) {
        doSendResponse(obj.nacl_arch);
      });
      return true;
    } else if (method == 'logging.startRtpDump') {
      var incoming = message['incoming'] || false;
      var outgoing = message['outgoing'] || false;
      chrome.webrtcLoggingPrivate.startRtpDump(
          requestInfo, origin, incoming, outgoing, doSendResponse);
      return true;
    } else if (method == 'logging.stopRtpDump') {
      var incoming = message['incoming'] || false;
      var outgoing = message['outgoing'] || false;
      chrome.webrtcLoggingPrivate.stopRtpDump(
          requestInfo, origin, incoming, outgoing, doSendResponse);
      return true;
    } else if (method == 'logging.startAudioDebugRecordings') {
      var seconds = message['seconds'] || 0;
      chrome.webrtcLoggingPrivate.startAudioDebugRecordings(
          requestInfo, origin, seconds, doSendResponse);
      return true;
    } else if (method == 'logging.stopAudioDebugRecordings') {
      chrome.webrtcLoggingPrivate.stopAudioDebugRecordings(
          requestInfo, origin, doSendResponse);
      return true;
    } else if (method == 'logging.startEventLogging') {
      var peerConnectionId = message['peerConnectionId'] || '';
      var maxLogSizeBytes = message['maxLogSizeBytes'] || 0;
      var metadata = message['metadata'] || '';
      chrome.webrtcLoggingPrivate.startEventLogging(
          requestInfo, origin, peerConnectionId, maxLogSizeBytes, metadata,
          doSendResponse);
      return true;
    } else if (method == 'setAudioExperiments') {
      var experiments = message['experiments'];
      chrome.webrtcAudioPrivate.setAudioExperiments(
          requestInfo, origin, experiments, doSendResponse);
      return true;
    }

    throw new Error('Unknown method: ' + method);
  } catch (e) {
    doSendResponse(null, e.name + ': ' + e.message);
  }
});

// If Hangouts connects with a port named 'onSinksChangedListener', we
// will register a listener and send it a message {'eventName':
// 'onSinksChanged'} whenever the event fires.
function onSinksChangedPort(port) {
  function clientListener() {
    port.postMessage({'eventName': 'onSinksChanged'});
  }
  chrome.webrtcAudioPrivate.onSinksChanged.addListener(clientListener);

  port.onDisconnect.addListener(function() {
    chrome.webrtcAudioPrivate.onSinksChanged.removeListener(clientListener);
  });
}

// This is a one-time-use port for calling chooseDesktopMedia.  The page
// sends one message, identifying the requested source types, and the
// extension sends a single reply, with the user's selected streamId.  A port
// is used so that if the page is closed before that message is sent, the
// window picker dialog will be closed.
function onChooseDesktopMediaPort(port) {
  function sendResponse(streamId) {
    port.postMessage({'value': {'streamId': streamId}});
    port.disconnect();
  }

  port.onMessage.addListener(function(message) {
    var method = message['method'];
    if (method == 'chooseDesktopMedia') {
      var sources = message['sources'];
      var cancelId = null;
      if (port.sender.tab) {
        cancelId = chrome.desktopCapture.chooseDesktopMedia(
            sources, port.sender.tab, sendResponse);
      } else {
        var requestInfo = {};
        requestInfo['guestProcessId'] = port.sender.guestProcessId || 0;
        requestInfo['guestRenderFrameId'] =
            port.sender.guestRenderFrameRoutingId || 0;
        cancelId = chrome.webrtcDesktopCapturePrivate.chooseDesktopMedia(
            sources, requestInfo, sendResponse);
      }
      port.onDisconnect.addListener(function() {
        // This method has no effect if called after the user has selected a
        // desktop media source, so it does not need to be conditional.
        if (port.sender.tab) {
          chrome.desktopCapture.cancelChooseDesktopMedia(cancelId);
        } else {
          chrome.webrtcDesktopCapturePrivate.cancelChooseDesktopMedia(cancelId);
        }
      });
    }
  });
}

// A port for continuously reporting relevant CPU usage information to the page.
function onProcessCpu(port) {
  var tabPid = port.sender.guestProcessId || undefined;
  function processListener(processes) {
    if (tabPid == undefined) {
      // getProcessIdForTab sometimes fails, and does not call the callback.
      // (Tracked at https://crbug.com/368855.)
      // This call retries it on each process update until it succeeds.
      chrome.processes.getProcessIdForTab(port.sender.tab.id, function(x) {
        tabPid = x;
      });
      return;
    }
    var tabProcess = processes[tabPid];
    if (!tabProcess) {
      return;
    }

    var browserProcessCpu, gpuProcessCpu;
    for (var pid in processes) {
      var process = processes[pid];
      if (process.type == 'browser') {
        browserProcessCpu = process.cpu;
      } else if (process.type == 'gpu') {
        gpuProcessCpu = process.cpu;
      }
      if (!!browserProcessCpu && !!gpuProcessCpu)
        break;
    }

    port.postMessage({
      'browserCpuUsage': browserProcessCpu || 0,
      'gpuCpuUsage': gpuProcessCpu || 0,
      'tabCpuUsage': tabProcess.cpu,
      'tabNetworkUsage': tabProcess.network,
      'tabPrivateMemory': tabProcess.privateMemory,
      'tabJsMemoryAllocated': tabProcess.jsMemoryAllocated,
      'tabJsMemoryUsed': tabProcess.jsMemoryUsed
    });
  }

  chrome.processes.onUpdated.addListener(processListener);
  port.onDisconnect.addListener(function() {
    chrome.processes.onUpdated.removeListener(processListener);
  });
}

function appendLastErrorMessage(errors) {
  if (chrome.runtime.lastError !== undefined)
    errors.push(chrome.runtime.lastError.message);
}

chrome.runtime.onConnectExternal.addListener(function(port) {
  if (port.name == 'onSinksChangedListener') {
    onSinksChangedPort(port);
  } else if (port.name == 'chooseDesktopMedia') {
    onChooseDesktopMediaPort(port);
  } else if (port.name == 'processCpu') {
    onProcessCpu(port);
  } else {
    // Unknown port type.
    port.disconnect();
  }
});
