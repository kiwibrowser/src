'use strict';

var commandLineArgs = require('../../');
var os = require('os');
var fs = require('fs');
var path = require('path');

var tmpPath = path.join(os.tmpdir(), Date.now() + '-cla.js');

process.stdin.pipe(fs.createWriteStream(tmpPath)).on('close', parseCla);

function parseCla() {
  var cliOptions = require(tmpPath);
  fs.unlinkSync(tmpPath);

  try {
    console.log(commandLineArgs(cliOptions));
  } catch (err) {
    console.error(err.message);
    process.exitCode = 1;
  }
}