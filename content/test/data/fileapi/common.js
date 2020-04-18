// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function debug(message)
{
  document.getElementById('status').innerHTML += '<br/>' + message;
}

function done(message)
{
  if (document.location.hash == '#fail')
    return;
  if (message)
    debug('PASS: ' + message);
  else
    debug('PASS');
  document.location.hash = '#pass';
}

function fail(message)
{
  debug('FAILED: ' + message);
  document.location.hash = '#fail';
}

function getLog()
{
  return '' + document.getElementById('status').innerHTML;
}

function fileErrorToString(e)
{
  switch (e.code) {
    case FileError.QUOTA_EXCEEDED_ERR:
      return 'QUOTA_EXCEEDED_ERR';
    case FileError.NOT_FOUND_ERR:
      return 'NOT_FOUND_ERR';
    case FileError.SECURITY_ERR:
      return 'SECURITY_ERR';
    case FileError.INVALID_MODIFICATION_ERR:
      return 'INVALID_MODIFICATION_ERR';
    case FileError.INVALID_STATE_ERR:
      return 'INVALID_STATE_ERR';
    default:
      return 'Unknown Error';
  }
}

function unexpectedErrorCallback(e)
{
  fail('unexpectedErrorCallback:' + fileErrorToString(e));
}
