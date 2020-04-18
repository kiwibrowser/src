// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.module('chromium.BindingsTest');
// Needed to let C++ invoke the test.
goog.module.declareLegacyNamespace();

const Connection = goog.require('chromium.DevTools.Connection');
const DOM = goog.require('chromium.DevTools.DOM');
const Page = goog.require('chromium.DevTools.Page');
const Runtime = goog.require('chromium.DevTools.Runtime');

/**
 * A trivial test which is invoked from C++ by HeadlessJsBindingsTest.
 */
class BindingsTest {
  constructor() {}

  /**
   * Evaluates 1+1 and returns the result over the chromium.DevTools.Connection.
   */
  evalOneAddOne() {
    let connection = new Connection(window.TabSocket);
    let runtime = new Runtime(connection);
    runtime.evaluate({'expression': '1+1'}).then(function(message) {
      connection.sendDevToolsMessage(
          '__Result',
          {'result': JSON.stringify(message.result.value)});
    });
  }

  /**
   * Evaluates 1+1 and returns the result over the chromium.DevTools.Connection.
   */
  listenForChildNodeCountUpdated() {
    let connection = new Connection(window.TabSocket);
    let dom = new DOM(connection);
    dom.onChildNodeCountUpdated(function(params) {
      connection.sendDevToolsMessage('__Result',
                                     {'result': JSON.stringify(params)});
    });
    dom.enable().then(function() {
      return dom.getDocument({});
    }).then(function() {
      // Create a new div which should trigger the event.
      let div = document.createElement('div');
      document.body.appendChild(div);
    });
  }

  /**
   * Uses experimental commands to create an isolated world.
   */
  getIsolatedWorldName() {
    let connection = new Connection(window.TabSocket);
    let page = new Page(connection);
    let runtime = new Runtime(connection);
    runtime.enable().then(function() {
      runtime.onExecutionContextCreated(function(params) {
        if (params.context.auxData['isDefault'] === false) {
          connection.sendDevToolsMessage(
              '__Result', {'result': 'Created ' + params.context.name});
        }
      });
      page.experimental.getResourceTree().then(function(result) {
        page.experimental.createIsolatedWorld({
          'frameId': result.frameTree.frame.id,
          'worldName': 'Test Isolated World'
        });
      });
    });
  }
}

exports = BindingsTest;
