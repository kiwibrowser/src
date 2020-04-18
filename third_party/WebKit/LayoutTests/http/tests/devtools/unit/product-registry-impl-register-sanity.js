(async function(){
  await TestRunner.loadModule('product_registry_impl');
  TestRunner.addResult("This tests product registry impl's register function.");

  resetProductRegistry();
  ProductRegistryImpl.register([
    "example.(com|org)",
    "*.example.com",
    "test-*.example.com",
    "subdomain.example.com"
  ], [
    {
      hash: "0caaf24ab1a0c334", // Result of: sha1('example.com').substr(0, 16).
      prefixes: {
        "": {
          product: 0 // Reference to index of first argument of ProductRegistryImpl.register.
        },
        "*": {
          product: 1
        },
        "test-": {
          product: 2
        }
      }
    },
    {
      hash: "20116dfd6774a9e7", // Result of: sha1('example.org').substr(0, 16).
      prefixes: {
        "": {
          product: 0
        }
      }
    },
    {
      hash: "e3d90251f85e2064", // Result of: sha1('subdomain.example.com').substr(0, 16).
      prefixes: {
        "": {
          product: 3
        }
      }
    }
  ]);
  var instance = new ProductRegistryImpl.Registry();

  logDomainEntry('example.com');
  logDomainEntry('wild.example.com');
  logDomainEntry('test-1.example.com');
  logDomainEntry('subdomain.example.com');
  logDomainEntry('example.org');
  logDomainEntry('chromium.org');

  TestRunner.completeTest();

  function logDomainEntry(domainStr) {
    TestRunner.addResult("Testing: " + domainStr + " -> " + instance.nameForUrl(('http://' + domainStr).asParsedURL()));
  }

  function resetProductRegistry() {
    TestRunner.addResult("Cleared ProductRegistryImpl");
    ProductRegistryImpl._productsByDomainHash.clear();
  }
})();
