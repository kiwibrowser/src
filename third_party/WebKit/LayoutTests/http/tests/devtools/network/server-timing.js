// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that server-timing headers are parsed correctly.\n');

  function testServerTimingHeader(headerValue, expectedResults) {
    var actualResults = SDK.ServerTiming.createFromHeaderValue(headerValue)
    if (JSON.stringify(actualResults) !== JSON.stringify(expectedResults)) {
      TestRunner.addResult('Test failure header=' + headerValue)
      TestRunner.addResult('  expected=' + JSON.stringify(expectedResults))
      TestRunner.addResult('    actual=' + JSON.stringify(actualResults))
    }
  }

  testServerTimingHeader("", []);

  // name only
  testServerTimingHeader("metric", [{"name":"metric"}]);

  // name and duration
  testServerTimingHeader("metric;dur=123.4", [{"name":"metric","dur":123.4}]);
  testServerTimingHeader("metric;dur=\"123.4\"", [{"name":"metric","dur":123.4}]);

  // name and description
  testServerTimingHeader("metric;desc=description", [{"name":"metric","desc":"description"}]);
  testServerTimingHeader("metric;desc=\"description\"", [{"name":"metric","desc":"description"}]);

  // name, duration, and description
  testServerTimingHeader("metric;dur=123.4;desc=description", [{"name":"metric","dur":123.4,"desc":"description"}]);
  testServerTimingHeader("metric;desc=description;dur=123.4", [{"name":"metric","desc":"description","dur":123.4}]);

  //  special chars in name
  testServerTimingHeader("aB3!#$%&'*+-.^_`|~", [{"name":"aB3!#$%&'*+-.^_`|~"}])

  // spaces
  testServerTimingHeader("metric ; ", [{"name":"metric"}]);
  testServerTimingHeader("metric , ", [{"name":"metric"}]);
  testServerTimingHeader("metric ; dur = 123.4 ; desc = description", [{"name":"metric","dur":123.4,"desc":"description"}]);
  testServerTimingHeader("metric ; desc = description ; dur = 123.4", [{"name":"metric","desc":"description","dur":123.4}]);
  testServerTimingHeader("metric;desc = \"description\"", [{"name":"metric","desc":"description"}]);

  // tabs
  testServerTimingHeader("metric\t;\t", [{"name":"metric"}]);
  testServerTimingHeader("metric\t,\t", [{"name":"metric"}]);
  testServerTimingHeader("metric\t;\tdur\t=\t123.4\t;\tdesc\t=\tdescription", [{"name":"metric","dur":123.4,"desc":"description"}]);
  testServerTimingHeader("metric\t;\tdesc\t=\tdescription\t;\tdur\t=\t123.4", [{"name":"metric","desc":"description","dur":123.4}]);
  testServerTimingHeader("metric;desc\t=\t\"description\"", [{"name":"metric","desc":"description"}]);

  // multiple entries
  testServerTimingHeader("metric1;dur=12.3;desc=description1,metric2;dur=45.6;desc=description2,metric3;dur=78.9;desc=description3", [{"name":"metric1","dur":12.3,"desc":"description1"}, {"name":"metric2","dur":45.6,"desc":"description2"}, {"name":"metric3","dur":78.9,"desc":"description3"}]);
  testServerTimingHeader("metric1,metric2 ,metric3, metric4 , metric5", [{"name":"metric1"}, {"name":"metric2"}, {"name":"metric3"}, {"name":"metric4"}, {"name":"metric5"}]);

   // quoted-strings - happy path
  testServerTimingHeader("metric;desc=\"description\"", [{"name":"metric","desc":"description"}]);
  testServerTimingHeader("metric;desc=\"\t description \t\"", [{"name":"metric","desc":"\t description \t"}]);
  testServerTimingHeader("metric;desc=\"descr\\\"iption\"", [{"name":"metric","desc":"descr\"iption"}]);

  // quoted-strings - others
  testServerTimingHeader("metric;desc=\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\\\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\\\"\"\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\\\"", [{"name":"metric","desc":"\\"}]);
  testServerTimingHeader("metric;desc=\"\\\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\\\"\"", [{"name":"metric","desc":"\""}]);
  testServerTimingHeader("metric;desc=\"\"\\\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\\\"", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\"\\", [{"name":"metric","desc":""}]);
  testServerTimingHeader("metric;desc=\"\"\"\"", [{"name":"metric","desc":""}]);

   // duplicate entry names
  testServerTimingHeader("metric;dur=12.3;desc=description1,metric;dur=45.6;desc=description2", [{"name":"metric","dur":12.3,"desc":"description1"}, {"name":"metric","dur":45.6,"desc":"description2"}]);

  // param name case sensitivity
  testServerTimingHeader("metric;DuR=123.4;DeSc=description", [{"name":"metric","dur":123.4,"desc":"description"}]);

  // non-numeric durations
  testServerTimingHeader("metric;dur=foo", [{"name":"metric","dur":0}]);
  testServerTimingHeader("metric;dur=\"foo\"", [{"name":"metric","dur":0}]);

  // unrecognized param names
  testServerTimingHeader("metric1;foo=bar;desc=description;foo=bar;dur=123.4;foo=bar,metric2", [{"name":"metric1","desc":"description","dur":123.4}, {"name":"metric2"}]);

  // duplicate param names
  testServerTimingHeader("metric;dur=123.4;dur=567.8", [{"name":"metric","dur":123.4}]);
  testServerTimingHeader("metric;dur=foo;dur=567.8", [{"name":"metric","dur":0}]);
  testServerTimingHeader("metric;desc=description1;desc=description2", [{"name":"metric","desc":"description1"}]);

  // incomplete params
  testServerTimingHeader("metric;dur;dur=123.4;desc=description", [{"name":"metric","dur":0,"desc":"description"}]);
  testServerTimingHeader("metric;dur=;dur=123.4;desc=description", [{"name":"metric","dur":0,"desc":"description"}]);
  testServerTimingHeader("metric;desc;desc=description;dur=123.4", [{"name":"metric","desc":"","dur":123.4}]);
  testServerTimingHeader("metric;desc=;desc=description;dur=123.4", [{"name":"metric","desc":"","dur":123.4}]);

  // extraneous characters after param value as token
  testServerTimingHeader("metric;desc=d1 d2;dur=123.4", [{"name":"metric","desc":"d1","dur":123.4}]);
  testServerTimingHeader("metric1;desc=d1 d2,metric2", [{"name":"metric1","desc":"d1"},{"name":"metric2"}]);

  // extraneous characters after param value as quoted-string
  testServerTimingHeader("metric;desc=\"d1\" d2;dur=123.4", [{"name":"metric","desc":"d1","dur":123.4}]);
  testServerTimingHeader("metric1;desc=\"d1\" d2,metric2", [{"name":"metric1","desc":"d1"},{"name":"metric2"}]);

  // nonsense - extraneous characters after entry name token
  testServerTimingHeader("metric==   \"\"foo;dur=123.4", [{"name":"metric"}]);
  testServerTimingHeader("metric1==   \"\"foo,metric2", [{"name":"metric1"}]);

  // nonsense - extraneous characters after param name token
  testServerTimingHeader("metric;dur foo=12", [{"name":"metric","dur":0}]);
  testServerTimingHeader("metric;foo dur=12", [{"name":"metric"}]);

  // nonsense - return zero entries
  testServerTimingHeader(" ", []);
  testServerTimingHeader("=", []);
  testServerTimingHeader(";", []);
  testServerTimingHeader(",", []);
  testServerTimingHeader("=;", []);
  testServerTimingHeader(";=", []);
  testServerTimingHeader("=,", []);
  testServerTimingHeader(",=", []);
  testServerTimingHeader(";,", []);
  testServerTimingHeader(",;", []);

  TestRunner.addResult('Tests ran to completion.\n');
  TestRunner.completeTest();
})();
