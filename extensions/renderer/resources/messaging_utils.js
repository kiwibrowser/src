// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Routines used to normalize arguments to messaging functions.

function alignSendMessageArguments(args, hasOptionsArgument) {
  // Align missing (optional) function arguments with the arguments that
  // schema validation is expecting, e.g.
  //   extension.sendRequest(req)     -> extension.sendRequest(null, req)
  //   extension.sendRequest(req, cb) -> extension.sendRequest(null, req, cb)
  if (!args || !args.length)
    return null;
  var lastArg = args.length - 1;

  // responseCallback (last argument) is optional.
  var responseCallback = null;
  if (typeof args[lastArg] == 'function')
    responseCallback = args[lastArg--];

  var options = null;
  if (hasOptionsArgument && lastArg >= 1) {
    // options (third argument) is optional. It can also be ambiguous which
    // argument it should match. If there are more than two arguments remaining,
    // options is definitely present:
    if (lastArg > 1) {
      options = args[lastArg--];
    } else {
      // Exactly two arguments remaining. If the first argument is a string,
      // it should bind to targetId, and the second argument should bind to
      // request, which is required. In other words, when two arguments remain,
      // only bind options when the first argument cannot bind to targetId.
      if (!(args[0] === null || typeof args[0] == 'string'))
        options = args[lastArg--];
    }
  }

  // request (second argument) is required.
  var request = args[lastArg--];

  // targetId (first argument, extensionId in the manifest) is optional.
  var targetId = null;
  if (lastArg >= 0)
    targetId = args[lastArg--];

  if (lastArg != -1)
    return null;
  if (hasOptionsArgument)
    return [targetId, request, options, responseCallback];
  return [targetId, request, responseCallback];
}

exports.$set('alignSendMessageArguments', alignSendMessageArguments);
