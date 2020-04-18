// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests inspector cookie parser\n`);


  TestRunner.dumpCookie = function(cookie) {
    var requestDate = new Date('Mon Oct 18 2010 17:00:00 GMT+0000');
    var expires = cookie.expiresDate(requestDate);

    var output = 'name: ' + cookie.name() + ', value: ' + cookie.value() + ', httpOnly: ' + cookie.httpOnly() +
        ', sameSite: ' + cookie.sameSite() + ', secure: ' + cookie.secure() + ', session: ' + cookie.session() +
        ', path: ' + cookie.path() + ', domain: ' + cookie.domain() + ', port: ' + cookie.port() +
        ', expires: ' + (expires ? expires.getTime() : 'n/a') + ', size: ' + cookie.size();

    TestRunner.addResult(output);
    TestRunner.addObject(cookie.attributes());
  };

  TestRunner.dumpCookies = function(cookies) {
    for (var i = 0; i < cookies.length; ++i)
      TestRunner.dumpCookie(cookies[i]);
  };

  TestRunner.parseAndDumpCookie = function(header) {
    var parser = new SDK.CookieParser();
    TestRunner.addResult('source: ' + header);
    TestRunner.dumpCookies(parser.parseCookie(header));
  };

  TestRunner.parseAndDumpSetCookie = function(header) {
    var parser = new SDK.CookieParser();
    TestRunner.addResult('source: ' + header);
    TestRunner.dumpCookies(parser.parseSetCookie(header));
  };

  TestRunner.parseAndDumpCookie('cookie=value');
  TestRunner.parseAndDumpCookie('$version=1; a=b,c  =   d, e=f');
  TestRunner.parseAndDumpCookie('$version=1; a=b;c  =   d; e =f');
  TestRunner.parseAndDumpCookie('cooke1 = value1; another cookie = another value');
  TestRunner.parseAndDumpCookie('cooke1 = value; $Path=/; $Domain=.example.com;');
  TestRunner.parseAndDumpCookie(
      'cooke1 = value; $Path=/; $Domain=.example.com ; Cookie2 = value2; $Path = /foo; $DOMAIN = foo.example.com;');
  TestRunner.parseAndDumpCookie(
      'cooke1 = value; $Path=/; $Domain=.example.com\nCookie2 = value2; $Path = /foo; $DOMAIN = foo.example.com; ');
  TestRunner.parseAndDumpCookie(
      '$version =1; cooke1 = value; $Path=/; $Domain   =.example.com;  \n Cookie2 = value2; $Path = /foo; $DOMAIN = foo.example.com;');

  TestRunner.parseAndDumpSetCookie('cookie=value');
  TestRunner.parseAndDumpSetCookie('a=b\n c=d\n f');
  TestRunner.parseAndDumpSetCookie('cooke1 = value; Path=/; Domain=.example.com;');
  TestRunner.parseAndDumpSetCookie(
      'cooke1 = value; Path=/; Domain=  .example.com \nCookie2 = value2; Path = /foo; Domain = foo.example.com');
  TestRunner.parseAndDumpSetCookie(
      'cooke1 = value; expires = Mon, Oct 18 2010 17:00 GMT+0000; Domain   =.example.com\nCookie2 = value2; Path = /foo; DOMAIN = foo.example.com; HttpOnly; Secure; Discard;');
  TestRunner.parseAndDumpSetCookie(
      'cooke1 = value; max-age= 1440; Domain   =.example.com\n Cookie2 = value2; Path = /foo; DOMAIN = foo.example.com; HttpOnly; Secure; Discard;');
  TestRunner.parseAndDumpSetCookie('cooke1 = value; HttpOnly; Secure; SameSite=Lax;');
  TestRunner.parseAndDumpSetCookie('cooke1 = value; HttpOnly; Secure; SameSite=Secure;');
  TestRunner.parseAndDumpSetCookie('cooke1; Path=/; Domain=.example.com;');
  TestRunner.parseAndDumpSetCookie('cooke1=; Path=/; Domain=.example.com;');
  TestRunner.completeTest();
})();
