// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function Download(url, path, verbose) {
  if (verbose) {
    WScript.StdOut.Write(" *  GET " + url + "...");
  }
  try {
    xml_http = new ActiveXObject("MSXML2.ServerXMLHTTP");
  } catch (e) {
    WScript.StdOut.WriteLine("[-] XMLHTTP " + new Number(e.number).toHex() +
        ": Cannot create Active-X object (" + e.description) + ").";
    WScript.Quit(1);
  }
  try {
    xml_http.open("GET", url, false);
  } catch (e) {
    WScript.StdOut.WriteLine("[-] XMLHTTP " + new Number(e.number).toHex() +
        ": invalid URL.");
    WScript.Quit(1);
  }

  var response_body = null;
  var size_description = "?";
  var file_size;
  try {
    xml_http.send(null);
    if (xml_http.status != 200) {
      WScript.StdOut.WriteLine("[-] HTTP " + xml_http.status + " " +
          xml_http.statusText);
      WScript.Quit(1);
    }
    response_body = xml_http.responseBody;
    size_description = xml_http.getResponseHeader("Content-Length");
    if (size_description != "") {
      file_size = parseInt(size_description)
      size_description = file_size.toBytes();
    } else {
      try {
        file_size = new Number(xml_http.responseText.length)
        size_description = file_size.toBytes();
      } catch(e) {
        size_description = "unknown size";
      }
    }
  } catch (e) {
    WScript.StdOut.WriteLine("[-] XMLHTTP " + new Number(e.number).toHex() +
        ": Cannot make HTTP request (" + e.description) + ")";
    WScript.Quit(1);
  }

  if (verbose) {
    WScript.StdOut.WriteLine("ok (" + size_description + ").");
    WScript.StdOut.Write(" *  Save " + path + "...");
  }

  try {
    var adodb_stream = new ActiveXObject("ADODB.Stream");
    adodb_stream.Mode = 3; // ReadWrite
    adodb_stream.Type = 1; // 1= Binary
    adodb_stream.Open(); // Open the stream
    adodb_stream.Write(response_body); // Write the data
    adodb_stream.SaveToFile(path, 2); // Save to our destination
    adodb_stream.Close();
  } catch(e) {
    WScript.StdOut.WriteLine("[-] ADODB.Stream " + new Number(
        e.number).toHex() + ": Cannot save file (" + e.description + ")");
    WScript.Quit(1);
  }
  if (typeof(file_size) != undefined) {
    var file_system_object = WScript.CreateObject("Scripting.FileSystemObject")
    var file = file_system_object.GetFile(path)
    if (file.Size < file_size) {
      WScript.StdOut.WriteLine("[-] File only partially downloaded.");
      WScript.Quit(1);
    }
  }
  if (verbose) {
    WScript.StdOut.WriteLine("ok.");
  }
}

// Utilities
Number.prototype.isInt = function NumberIsInt() {
  return this % 1 == 0;
};
Number.prototype.toBytes = function NumberToBytes() {
  // Returns a "pretty" string representation of a number of bytes:
  var units = ["KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"];
  var unit = "bytes"
  var limit = 1;
  while(this > limit * 1100 && units.length > 0) {
    limit *= 1024;
    unit = units.shift();
  }
  return (Math.round(this * 100 / limit) / 100).toString() + " " + unit;
};
Number.prototype.toHex = function NumberToHex(length) {
  if (arguments.length == 0) length = 1;
  if (typeof(length) != "number" && !(length instanceof Number)) {
    throw Exception("Length must be a positive integer larger than 0.",
        TypeError, 0);
  }
  if (length < 1 || !length.isInt()) {
    throw Exception("Length must be a positive integer larger than 0.",
        "RangeError", 0);
  }
  var result = (this + (this < 0 ? 0x100000000 : 0)).toString(16);
  while (result.length < length) result = "0" + result;
  return result;
};

if (WScript.Arguments.length != 2) {
  WScript.StdOut.Write("Incorrect arguments to get_file.js")
} else {
  Download(WScript.Arguments(0), WScript.Arguments(1), false);
}
