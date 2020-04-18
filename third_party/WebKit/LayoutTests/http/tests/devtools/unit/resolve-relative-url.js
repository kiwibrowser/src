(async function () {
  TestRunner.addResult("http://example.com/map.json === " + Common.ParsedURL.completeURL("http://example.com/script.js", "http://example.com/map.json"));
  TestRunner.addResult("http://example.com/map.json === " + Common.ParsedURL.completeURL("http://example.com/script.js", "/map.json"));
  TestRunner.addResult("http://example.com/maps/map.json === " + Common.ParsedURL.completeURL("http://example.com/scripts/script.js", "../maps/map.json"));

  function testCompleteURL(base, lhs, rhs)
  {
      var actual =  Common.ParsedURL.completeURL(base, lhs);
      TestRunner.addResult(lhs + " resolves to " + actual + "===" + rhs + " passes: " + (actual === rhs));
  }

  var rfc3986_5_4_baseURL =  "http://a/b/c/d;p?q";
  TestRunner.addResult("Tests from http://tools.ietf.org/html/rfc3986#section-5.4 using baseURL=\"" + rfc3986_5_4_baseURL + "\"");
  var rfc3986_5_4 = testCompleteURL.bind(null, rfc3986_5_4_baseURL);
  rfc3986_5_4("http://h", "http://h");  // modified from RFC3986
  rfc3986_5_4("g", "http://a/b/c/g");
  rfc3986_5_4("./g", "http://a/b/c/g");
  rfc3986_5_4("g/", "http://a/b/c/g/");
  rfc3986_5_4("/g", "http://a/g");
  rfc3986_5_4("//g", "http://g");
  rfc3986_5_4("?y", "http://a/b/c/d;p?y");
  rfc3986_5_4("g?y", "http://a/b/c/g?y");
  rfc3986_5_4("#s", "http://a/b/c/d;p?q#s");
  rfc3986_5_4("g#s", "http://a/b/c/g#s");
  rfc3986_5_4("g?y#s", "http://a/b/c/g?y#s");
  rfc3986_5_4(";x", "http://a/b/c/;x");
  rfc3986_5_4("g;x", "http://a/b/c/g;x");
  rfc3986_5_4("g;x?y#s", "http://a/b/c/g;x?y#s");
  rfc3986_5_4("", "http://a/b/c/d;p?q");
  rfc3986_5_4(".", "http://a/b/c/");
  rfc3986_5_4("./", "http://a/b/c/");
  rfc3986_5_4("..", "http://a/b/");
  rfc3986_5_4("../", "http://a/b/");
  rfc3986_5_4("../g", "http://a/b/g");
  rfc3986_5_4("../..", "http://a/");
  rfc3986_5_4("../../", "http://a/");
  rfc3986_5_4("../../g", "http://a/g");
  rfc3986_5_4("../../../g", "http://a/g");
  rfc3986_5_4("../../../../g", "http://a/g");
  rfc3986_5_4("/./g", "http://a/g");
  rfc3986_5_4("/../g", "http://a/g");
  rfc3986_5_4("g." , "http://a/b/c/g.");
  rfc3986_5_4(".g" , "http://a/b/c/.g");
  rfc3986_5_4("g..", "http://a/b/c/g..");
  rfc3986_5_4("..g", "http://a/b/c/..g");
  rfc3986_5_4("./../g", "http://a/b/g");
  rfc3986_5_4("./g/.", "http://a/b/c/g/");
  rfc3986_5_4("g/./h", "http://a/b/c/g/h");
  rfc3986_5_4("g/../h", "http://a/b/c/h");
  rfc3986_5_4("g;x=1/./y", "http://a/b/c/g;x=1/y");
  rfc3986_5_4("g;x=1/../y", "http://a/b/c/y");
  rfc3986_5_4("g?y/./x", "http://a/b/c/g?y/./x");
  rfc3986_5_4("g?y/../x", "http://a/b/c/g?y/../x");
  rfc3986_5_4("g#s/./x", "http://a/b/c/g#s/./x");
  rfc3986_5_4("g#s/../x", "http://a/b/c/g#s/../x");

  TestRunner.addResult("Custom completeURL tests");
  testCompleteURL("http://a/b/c/d;p?q", "//secure.com/moo", "http://secure.com/moo");
  testCompleteURL("http://a/b/c/d;p?q", "cat.jpeg", "http://a/b/c/cat.jpeg");
  testCompleteURL("http://example.com/path.css?query#fragment","", "http://example.com/path.css?query");

  TestRunner.completeTest();
})();
