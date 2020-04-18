// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function requestFileSystemSuccess(fs)
{
  fs.root.getFile('foo', {create: true, exclusive: false}, done,
                  function(e) { fail('Open:' + fileErrorToString(e)); } );
}

function test()
{
  debug('Requesting FileSystem');
  window.webkitRequestFileSystem(
      window.TEMPORARY,
      1024 * 1024,
      requestFileSystemSuccess,
      unexpectedErrorCallback);
}
