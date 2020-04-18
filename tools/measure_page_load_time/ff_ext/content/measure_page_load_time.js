// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview measure_page_load_time.js implements a Firefox extension
 * for measuring how long a page takes to load. It waits on TCP port
 * 42492 for connections, then accepts URLs and returns strings of the
 * form url,time, where "time" is the load time in milliseconds or the
 * string "timeout" or "error". Load time is measured from the call to
 * loadURI until the load event fires, or until the status changes to
 * STATUS_STOP if the load event doesn't fire (there's an error.)
 * @author jhaas@google.com (Jonathan Haas) */

// Shorthand reference to nsIWebProgress[Listener] interfaces
var IWP  = Components.interfaces.nsIWebProgress;
var IWPL = Components.interfaces.nsIWebProgressListener;
    

var MPLT = {
  /**
   * Constants
   */
  PORT_NUMBER : 42492,      // port to listen for connections on
  TIME_OUT : 4 * 60 * 1000, // timeout in 4 minutes

  /**
   * Incoming URL buffer
   * @type {string}
   */
  textBuffer : '',
  
  /**
   * URL we're currently visiting
   * @type {string}
   */
  URL : '',
  
  /**
   * Listener to accept incoming connections
   * @type {nsIServerSocketListener}
   */
  acceptListener :
  {
    onSocketAccepted : function(serverSocket, transport)
    {
      MPLT.streamInput  = transport.openInputStream(0,0,0);
      MPLT.streamOutput = transport.openOutputStream(0,0,0);
      
      MPLT.scriptStream = Components.classes['@mozilla.org/scriptableinputstream;1']
        .createInstance(Components.interfaces.nsIScriptableInputStream);
      MPLT.scriptStream.init(MPLT.streamInput);
      MPLT.pump = Components.classes['@mozilla.org/network/input-stream-pump;1']
        .createInstance(Components.interfaces.nsIInputStreamPump);
      MPLT.pump.init(MPLT.streamInput, -1, -1, 0, 0, false);
      MPLT.pump.asyncRead(MPLT.dataListener,null);    
    },

    onStopListening : function(){}
  },

  /**
   * Listener for network input
   * @type {nsIStreamListener}
   */
  dataListener : 
  {
    onStartRequest: function(){},
    onStopRequest: function(){},
    onDataAvailable: function(request, context, inputStream, offset, count){
      // Add the received data to the buffer, then process it
      // Change CRLF to newline while we're at it
      MPLT.textBuffer += MPLT.scriptStream.read(count).replace('\r\n', '\n');
      
      MPLT.process();
    }
  },
  
  /**
   * Process the incoming data buffer
   */
  process : function()
  {
    // If we're waiting for a page to finish loading, wait
    if (MPLT.timeLoadStarted)
      return;
    
    // Look for a carriage return
    var firstCR = MPLT.textBuffer.indexOf('\n');
    
    // If we haven't received a carriage return yet, wait
    if (firstCR < 0) 
      return;
      
    // If the first character was a carriage return, we're done!
    if (firstCR == 0) {
      MPLT.textBuffer = '';
      MPLT.streamInput.close();
      MPLT.streamOutput.close();
      
      return;
    }
    
    // Remove the URL from the buffer
    MPLT.URL = MPLT.textBuffer.substr(0, firstCR);
    MPLT.textBuffer = MPLT.textBuffer.substr(firstCR + 1);

    // Remember the current time and navigate to the new URL    
    MPLT.timeLoadStarted = new Date();
    gBrowser.loadURIWithFlags(MPLT.URL, gBrowser.LOAD_FLAGS_BYPASS_CACHE);
    setTimeout('MPLT.onTimeOut()', MPLT.TIME_OUT);
  },
  
  /**
   * Page load completion handler
   */
  onPageLoad : function(e) {
    // Ignore loads of non-HTML documents
    if (!(e.originalTarget instanceof HTMLDocument))
      return;
    
    // Also ignore subframe loads
    if (e.originalTarget.defaultView.frameElement)
      return;
      
    clearTimeout();
    var timeElapsed = new Date() - MPLT.timeLoadStarted;
    
    MPLT.outputResult(timeElapsed);
  },
  
  /**
   * Timeout handler
   */
  onTimeOut : function() {
    gBrowser.stop();
    
    MPLT.outputResult('timeout');
  },
  
  
  /**
   * Sends a properly-formatted result to the client
   * @param {string} result  The value to send along with the URL
   */
  outputResult : function(result) {

    if (MPLT.URL) {
      var outputString = MPLT.URL + ',' + result + '\n';
      MPLT.streamOutput.write(outputString, outputString.length);
      MPLT.URL = '';
    }
    
    MPLT.timeLoadStarted = null;
    MPLT.process();
  },

  /**
   * Time the page load started. If null, we're waiting for the
   * initial page load, or otherwise don't care about the page
   * that's currently loading
   * @type {number}
   */
  timeLoadStarted : null,

  /*
   * TODO(jhaas): add support for nsIWebProgressListener
   * If the URL being visited died as part of a network error
   * (host not found, connection reset by peer, etc), the onload
   * event doesn't fire. The only way to catch it would be in
   * a web progress listener. However, nsIWebProgress is not
   * behaving according to documentation. More research is needed.
   * For now, omitting it means that if any of our URLs are "dirty" 
   * (do not point to real web servers with real responses), we'll log 
   * them as timeouts. This doesn't affect pages where the server
   * exists but returns an error code.
   */
     
  /**
   * Initialize the plugin, create the socket and listen
   */
  initialize: function() {
    // Register for page load events
    gBrowser.addEventListener('load', this.onPageLoad, true);
    
    // Set a timeout to wait for the initial page to load
    MPLT.timeLoadStarted = new Date();
    setTimeout('MPLT.onTimeOut()', MPLT.TIME_OUT);
    
    // Create the listening socket
    MPLT.serverSocket = Components.classes['@mozilla.org/network/server-socket;1']
                         .createInstance(Components.interfaces.nsIServerSocket);

    MPLT.serverSocket.init(MPLT.PORT_NUMBER, true, 1);
    MPLT.serverSocket.asyncListen(this.acceptListener);  
  },

  /**
   * Close the socket(s)
   */
  deinitialize: function() {
    if (MPLT.streamInput)  MPLT.streamInput.close();
    if (MPLT.streamOutput) MPLT.streamOutput.close();
    if (MPLT.serverSocket) MPLT.serverSocket.close();
  }
};

window.addEventListener('load', function(e) { MPLT.initialize(); }, false);
window.addEventListener('unload', function(e) { MPLT.deinitialize(); }, false);
