// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var TestRunner = class {
  constructor(baseURL, log, completeTest, fetch) {
    this._dumpInspectorProtocolMessages = false;
    this._baseURL = baseURL;
    this._log = log;
    this._completeTest = completeTest;
    this._fetch = fetch;
  }

  startDumpingProtocolMessages() {
    this._dumpInspectorProtocolMessages = true;
  };

  completeTest() {
    this._completeTest.call(null);
  }

  log(item, title, stabilizeNames) {
    if (typeof item === 'object')
      return this._logObject(item, title, stabilizeNames);
    this._log.call(null, item);
  }

  _logObject(object, title, stabilizeNames = ['id', 'nodeId', 'objectId', 'scriptId', 'timestamp', 'backendNodeId', 'parentId', 'frameId', 'loaderId', 'baseURL', 'documentURL', 'styleSheetId']) {
    var lines = [];

    function dumpValue(value, prefix, prefixWithName) {
      if (typeof value === 'object' && value !== null) {
        if (value instanceof Array)
          dumpItems(value, prefix, prefixWithName);
        else
          dumpProperties(value, prefix, prefixWithName);
      } else {
        lines.push(prefixWithName + String(value).replace(/\n/g, ' '));
      }
    }

    function dumpProperties(object, prefix, firstLinePrefix) {
      prefix = prefix || '';
      firstLinePrefix = firstLinePrefix || prefix;
      lines.push(firstLinePrefix + '{');

      var propertyNames = Object.keys(object);
      propertyNames.sort();
      for (var i = 0; i < propertyNames.length; ++i) {
        var name = propertyNames[i];
        if (!object.hasOwnProperty(name))
          continue;
        var prefixWithName = '    ' + prefix + name + ' : ';
        var value = object[name];
        if (stabilizeNames && stabilizeNames.includes(name))
          value = `<${typeof value}>`;
        dumpValue(value, '    ' + prefix, prefixWithName);
      }
      lines.push(prefix + '}');
    }

    function dumpItems(object, prefix, firstLinePrefix) {
      prefix = prefix || '';
      firstLinePrefix = firstLinePrefix || prefix;
      lines.push(firstLinePrefix + '[');
      for (var i = 0; i < object.length; ++i)
        dumpValue(object[i], '    ' + prefix, '    ' + prefix + '[' + i + '] : ');
      lines.push(prefix + ']');
    }

    dumpValue(object, '', title || '');
    this._log.call(null, lines.join('\n'));
  }

  trimURL(url) {
    return url.replace(/^.*(([^/]*[/]){3}[^/]*)$/, '...$1');
  }

  url(relative) {
    if (relative.startsWith('http://') || relative.startsWith('https://'))
      return relative;
    return this._baseURL + relative;
  }

  async runTestSuite(testSuite) {
    for (var test of testSuite) {
      this.log('\nRunning test: ' + test.name);
      try {
        await test();
      } catch (e) {
        this.log(`Error during test: ${e}\n${e.stack}`);
      }
    }
    this.completeTest();
  }

  _checkExpectation(fail, name, messageObject) {
    if (fail === !!messageObject.error) {
      this.log('PASS: ' + name);
      return true;
    }

    this.log('FAIL: ' + name + ': ' + JSON.stringify(messageObject));
    this.completeTest();
    return false;
  }

  expectedSuccess(name, messageObject) {
    return this._checkExpectation(false, name, messageObject);
  }

  expectedError(name, messageObject) {
    return this._checkExpectation(true, name, messageObject);
  }

  die(message, error) {
    this.log(`${message}: ${error}\n${error.stack}`);
    this.completeTest();
    throw new Error(message);
  }

  fail(message) {
    this.log('FAIL: ' + message);
    this.completeTest();
  }

  async loadScript(url) {
    var source = await this._fetch(this.url(url));
    return eval(`${source}\n//# sourceURL=${url}`);
  };

  async createPage() {
    var targetId = (await DevToolsAPI._sendCommandOrDie('Target.createTarget', {url: 'about:blank'})).targetId;
    await DevToolsAPI._sendCommandOrDie('Target.activateTarget', {targetId});
    var page = new TestRunner.Page(this, targetId);
    var dummyURL = DevToolsHost.dummyPageURL;
    if (!dummyURL) {
      dummyURL = window.location.href;
      dummyURL = dummyURL.substring(0, dummyURL.indexOf('inspector-protocol-test.html')) + 'inspector-protocol-page.html';
    }
    await page._navigate(dummyURL);
    return page;
  }

  async _start(description, html, url) {
    try {
      if (!description)
        throw new Error('Please provide a description for the test!');
      this.log(description);
      var page = await this.createPage();
      if (url)
        await page.navigate(url);
      if (html)
        await page.loadHTML(html);
      var session = await page.createSession();
      return { page: page, session: session, dp: session.protocol };
    } catch (e) {
      this.die('Error starting the test', e);
    }
  };

  startBlank(description) {
    return this._start(description, null, null);
  }

  startHTML(html, description) {
    return this._start(description, html, null);
  }

  startURL(url, description) {
    return this._start(description, null, url);
  }

  async logStackTrace(debuggers, stackTrace, debuggerId) {
    while (stackTrace) {
      const {description, callFrames, parent, parentId} = stackTrace;
      if (description)
        this.log(`--${description}--`);
      this.logCallFrames(callFrames);
      if (parentId) {
        if (parentId.debuggerId)
          debuggerId = parentId.debuggerId;
        let result = await debuggers.get(debuggerId).getStackTrace({
          stackTraceId: parentId
        });
        stackTrace = result.stackTrace || result.result.stackTrace;
      } else {
        stackTrace = parent;
      }
    }
  }

  logCallFrames(callFrames) {
    for (let frame of callFrames) {
      let functionName = frame.functionName || '(anonymous)';
      let url = frame.url;
      let location = frame.location || frame;
      this.log(`${functionName} at ${url}:${
                                            location.lineNumber
                                          }:${location.columnNumber}`);
    }
  }
};

TestRunner.Page = class {
  constructor(testRunner, targetId) {
    this._testRunner = testRunner;
    this._targetId = targetId;
  }

  async createSession() {
    var sessionId = (await DevToolsAPI._sendCommandOrDie('Target.attachToTarget', {targetId: this._targetId})).sessionId;
    var session = new TestRunner.Session(this._testRunner, sessionId);
    DevToolsAPI._sessions.set(sessionId, session);
    return session;
  }

  navigate(url) {
    return this._navigate(this._testRunner.url(url));
  }

  async _navigate(url) {
    var session = await this.createSession();
    await session._navigate(url);
    await session.disconnect();
  }

  async loadHTML(html) {
    html = html.replace(/'/g, "\\'").replace(/\n/g, '\\n');
    var session = await this.createSession();
    await session.protocol.Runtime.evaluate({expression: `document.write('${html}');document.close();`});
    await session.disconnect();
  }
};

TestRunner.Session = class {
  constructor(testRunner, sessionId) {
    this._testRunner = testRunner;
    this._sessionId = sessionId;
    this._requestId = 0;
    this._dispatchTable = new Map();
    this._eventHandlers = new Map();
    this.protocol = this._setupProtocol();
    this._childSessions = null;
    this._parentSession = null;
  }

  async disconnect() {
    await DevToolsAPI._sendCommandOrDie('Target.detachFromTarget', {sessionId: this._sessionId});
    if (this._parentSession)
      this._parentSession._childSessions.delete(this._sessionId);
    else
      DevToolsAPI._sessions.delete(this._sessionId);
  }

  createChild(sessionId) {
    if (!this._childSessions) {
      this._childSessions = new Map();
      this.protocol.Target.onReceivedMessageFromTarget(event => this._dispatchMessageFromTarget(event));
    }
    let session = new TestRunner.Session(this._testRunner, sessionId);
    this._childSessions.set(sessionId, session);
    session._parentSession = this;
    return session;
  }

  _dispatchMessageFromTarget(event) {
    var session = this._childSessions.get(event.params.sessionId);
    if (session)
      session._dispatchMessage(JSON.parse(event.params.message));
  }

  sendRawCommand(requestId, message) {
    if (this._parentSession)
      this._parentSession.sendCommand('Target.sendMessageToTarget', {sessionId: this._sessionId, message: message});
    else
      DevToolsAPI._sendCommandOrDie('Target.sendMessageToTarget', {sessionId: this._sessionId, message: message});
    return new Promise(f => this._dispatchTable.set(requestId, f));
  }

  sendCommand(method, params) {
    var requestId = ++this._requestId;
    var messageObject = {'id': requestId, 'method': method, 'params': params};
    if (this._testRunner._dumpInspectorProtocolMessages)
      this._testRunner.log(`frontend => backend: ${JSON.stringify(messageObject)}`);
    return this.sendRawCommand(requestId, JSON.stringify(messageObject));
  }

  async evaluate(code, ...args) {
    if (typeof code === 'function') {
      var argsString = args.map(JSON.stringify.bind(JSON)).join(', ');
      code = `(${code.toString()})(${argsString})`;
    }
    var response = await this.protocol.Runtime.evaluate({expression: code, returnByValue: true});
    if (response.error || response.result.exceptionDetails) {
      this._testRunner.log(`Error while evaluating '${code}': ${JSON.stringify(response.error || response.result.exceptionDetails)}`);
      this._testRunner.completeTest();
    } else {
      return response.result.result.value;
    }
  }

  async evaluateAsync(code, ...args) {
    if (typeof code === 'function') {
      var argsString = args.map(JSON.stringify.bind(JSON)).join(', ');
      code = `(${code.toString()})(${argsString})`;
    }
    var response = await this.protocol.Runtime.evaluate({expression: code, returnByValue: true, awaitPromise: true});
    if (response.error) {
      this._testRunner.log(`Error while evaluating async '${code}': ${response.error}`);
      this._testRunner.completeTest();
    } else {
      return response.result.result.value;
    }
  }

  navigate(url) {
    return this._navigate(this._testRunner.url(url));
  }

  async _navigate(url) {
    await this.protocol.Page.enable();
    await this.protocol.Page.setLifecycleEventsEnabled({enabled: true});
    await this.protocol.Page.navigate({url: url});
    await this.protocol.Page.onceLifecycleEvent(event => event.params.name === 'load');
  }

  _dispatchMessage(message) {
    if (this._testRunner._dumpInspectorProtocolMessages)
      this._testRunner.log(`backend => frontend: ${JSON.stringify(message)}`);
    if (typeof message.id === 'number') {
      var handler = this._dispatchTable.get(message.id);
      if (handler) {
        this._dispatchTable.delete(message.id);
        handler(message);
      }
    } else {
      var eventName = message.method;
      for (var handler of (this._eventHandlers.get(eventName) || []))
        handler(message);
    }
  }

  _setupProtocol() {
    return new Proxy({}, {
      get: (target, agentName, receiver) => new Proxy({}, {
        get: (target, methodName, receiver) => {
          const eventPattern = /^(on(ce)?|off)([A-Z][A-Za-z0-9]*)/;
          var match = eventPattern.exec(methodName);
          if (!match)
            return args => this.sendCommand(
                       `${agentName}.${methodName}`, args || {});
          var eventName = match[3];
          eventName = eventName.charAt(0).toLowerCase() + eventName.slice(1);
          if (match[1] === 'once')
            return eventMatcher => this._waitForEvent(
                       `${agentName}.${eventName}`, eventMatcher);
          if (match[1] === 'off')
            return listener => this._removeEventHandler(
                       `${agentName}.${eventName}`, listener);
          return listener => this._addEventHandler(
                     `${agentName}.${eventName}`, listener);
        }
      })
    });
  }

  _addEventHandler(eventName, handler) {
    var handlers = this._eventHandlers.get(eventName) || [];
    handlers.push(handler);
    this._eventHandlers.set(eventName, handlers);
  }

  _removeEventHandler(eventName, handler) {
    var handlers = this._eventHandlers.get(eventName) || [];
    var index = handlers.indexOf(handler);
    if (index === -1)
      return;
    handlers.splice(index, 1);
    this._eventHandlers.set(eventName, handlers);
  }

  _waitForEvent(eventName, eventMatcher) {
    return new Promise(callback => {
      var handler = result => {
        if (eventMatcher && !eventMatcher(result))
          return;
        this._removeEventHandler(eventName, handler);
        callback(result);
      };
      this._addEventHandler(eventName, handler);
    });
  }
};

class WorkerProtocol {
  constructor(dp, sessionId) {
    this._sessionId = sessionId;
    this._callbacks = new Map();
    this._dp = dp;
    this._dp.Target.onReceivedMessageFromTarget(
        (message) => this._onMessage(message));
    this.dp = this._setupProtocol();
  }

  _setupProtocol() {
    let lastId = 0;
    return new Proxy({}, {
      get: (target, agentName, receiver) => new Proxy({}, {
        get: (target, methodName, receiver) => {
          const eventPattern = /^(once)?([A-Z][A-Za-z0-9]*)/;
          var match = eventPattern.exec(methodName);
          if (!match || match[1] !== 'once') {
            return args => new Promise(resolve => {
                     let id = ++lastId;
                     this._callbacks.set(id, resolve);
                     this._dp.Target.sendMessageToTarget({
                       sessionId: this._sessionId,
                       message: JSON.stringify({
                         method: `${agentName}.${methodName}`,
                         params: args || {},
                         id: id
                       })
                     });
                   });
          }
          var eventName = match[2];
          eventName = eventName.charAt(0).toLowerCase() + eventName.slice(1);
          return () => new Promise(resolve => {
                   this._callbacks.set(`${agentName}.${eventName}`, resolve);
                 });
        }
      })
    });
  }

  _onMessage(message) {
    if (message.params.sessionId !== this._sessionId)
      return;
    const {id, result, method, params} = JSON.parse(message.params.message);
    if (id && this._callbacks.has(id)) {
      let callback = this._callbacks.get(id);
      this._callbacks.delete(id);
      callback(result);
    }
    if (method && this._callbacks.has(method)) {
      let callback = this._callbacks.get(method);
      this._callbacks.delete(method);
      callback(params);
    }
  }
};

var DevToolsAPI = {};
DevToolsAPI._requestId = 0;
DevToolsAPI._embedderMessageId = 0;
DevToolsAPI._dispatchTable = new Map();
DevToolsAPI._sessions = new Map();
DevToolsAPI._outputElement = null;

DevToolsAPI._log = function(text) {
  if (!DevToolsAPI._outputElement) {
    var intermediate = document.createElement('div');
    document.body.appendChild(intermediate);
    var intermediate2 = document.createElement('div');
    intermediate.appendChild(intermediate2);
    DevToolsAPI._outputElement = document.createElement('div');
    DevToolsAPI._outputElement.className = 'output';
    DevToolsAPI._outputElement.id = 'output';
    DevToolsAPI._outputElement.style.whiteSpace = 'pre';
    intermediate2.appendChild(DevToolsAPI._outputElement);
  }
  DevToolsAPI._outputElement.appendChild(document.createTextNode(text));
  DevToolsAPI._outputElement.appendChild(document.createElement('br'));
};

DevToolsAPI._completeTest = function() {
  testRunner.notifyDone();
};

DevToolsAPI._die = function(message, error) {
  DevToolsAPI._log(`${message}: ${error}\n${error.stack}`);
  DevToolsAPI._completeTest();
  throw new Error();
};

DevToolsAPI.dispatchMessage = function(messageOrObject) {
  var messageObject = (typeof messageOrObject === 'string' ? JSON.parse(messageOrObject) : messageOrObject);
  var messageId = messageObject.id;
  try {
    if (typeof messageId === 'number') {
      var handler = DevToolsAPI._dispatchTable.get(messageId);
      if (handler) {
        DevToolsAPI._dispatchTable.delete(messageId);
        handler(messageObject);
      }
    } else {
      var eventName = messageObject.method;
      if (eventName === 'Target.receivedMessageFromTarget') {
        var sessionId = messageObject.params.sessionId;
        var message = messageObject.params.message;
        var session = DevToolsAPI._sessions.get(sessionId);
        if (session)
          session._dispatchMessage(JSON.parse(message));
      }
    }
  } catch(e) {
    DevToolsAPI._die(`Exception when dispatching message\n${JSON.stringify(messageObject)}`, e);
  }
};

DevToolsAPI._sendCommand = function(method, params) {
  var requestId = ++DevToolsAPI._requestId;
  var messageObject = {'id': requestId, 'method': method, 'params': params};
  var embedderMessage = {'id': ++DevToolsAPI._embedderMessageId, 'method': 'dispatchProtocolMessage', 'params': [JSON.stringify(messageObject)]};
  DevToolsHost.sendMessageToEmbedder(JSON.stringify(embedderMessage));
  return new Promise(f => DevToolsAPI._dispatchTable.set(requestId, f));
};

DevToolsAPI._sendCommandOrDie = function(method, params) {
  return DevToolsAPI._sendCommand(method, params).then(message => {
    if (message.error)
      DevToolsAPI._die('Error communicating with harness', new Error(message.error));
    return message.result;
  });
};

DevToolsAPI._fetch = function(url) {
  return new Promise(fulfill => {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.onreadystatechange = e => {
      if (xhr.readyState !== XMLHttpRequest.DONE)
        return;
      if ([0, 200, 304].indexOf(xhr.status) === -1)  // Testing harness file:/// results in 0.
        DevToolsAPI._die(`${xhr.status} while fetching ${url}`, new Error());
      else
        fulfill(e.target.response);
    };
    xhr.send(null);
  });
};

testRunner.dumpAsText();
testRunner.waitUntilDone();
testRunner.setCanOpenWindows(true);

window.addEventListener('load', () => {
  var params = new URLSearchParams(window.location.search);
  var testScriptURL = params.get('test');
  var baseURL = testScriptURL.substring(0, testScriptURL.lastIndexOf('/') + 1);
  DevToolsAPI._fetch(testScriptURL).then(testScript => {
    var testRunner = new TestRunner(baseURL, DevToolsAPI._log, DevToolsAPI._completeTest, DevToolsAPI._fetch);
    var testFunction = eval(`${testScript}\n//# sourceURL=${testScriptURL}`);
    if (params.get('debug')) {
      var dispatch = DevToolsAPI.dispatchMessage;
      var messages = [];
      DevToolsAPI.dispatchMessage = message => {
        if (!messages.length) {
          setTimeout(() => {
            for (var message of messages.splice(0))
              dispatch(message);
          }, 0);
        }
        messages.push(message);
      };
      testRunner.log = console.log;
      testRunner.completeTest = () => console.log('Test completed');
      window.test = () => testFunction(testRunner);
      return;
    }
    return testFunction(testRunner);
  }).catch(reason => {
    DevToolsAPI._log(`Error while executing test script: ${reason}\n${reason.stack}`);
    DevToolsAPI._completeTest();
  });
}, false);

window['onerror'] = (message, source, lineno, colno, error) => {
  DevToolsAPI._log(`${error}\n${error.stack}`);
  DevToolsAPI._completeTest();
};

window.addEventListener('unhandledrejection', e => {
  DevToolsAPI._log(`Promise rejection: ${e.reason}\n${e.reason ? e.reason.stack : ''}`);
  DevToolsAPI._completeTest();
}, false);
