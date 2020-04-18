// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Configuration for the Memory Inspector Chrome App.
 */
var MemoryInspectorConfig = {};

/** The port on which the Python server process will listen. */
MemoryInspectorConfig.PORT = 8089;

/** The path to the Python NMF file. */
MemoryInspectorConfig.NMF = '/sandbox/python.nmf';

/** The command-line arguments for the Python server process. */
MemoryInspectorConfig.ARGV = [
    'python.nmf',
    '/memory_inspector/start_web_ui',
    '--port', MemoryInspectorConfig.PORT,
    '--no-browser'];

/** The environment variables for the Python server process. */
MemoryInspectorConfig.ENV = ['NACL_DATA_URL=/sandbox/'];

/** The current working directory of the Python server process. */
MemoryInspectorConfig.CWD = '/';  // Relative to the virtual filesystem.

