// OPTIONS: -base-https
if (self.importScripts) {
  importScripts('../resources/fetch-test-helpers.js');
}

var BASE_URL =
  'http://example.test:8000/serviceworker/resources/fetch-access-control.php?ACAOrigin=*&label=';
var HTTPS_BASE_URL =
  'https://example.test:8443/serviceworker/resources/fetch-access-control.php?ACAOrigin=*&label=';
var HTTPS_OTHER_BASE_URL =
  'https://localhost:8443/serviceworker/resources/fetch-access-control.php?ACAOrigin=*&label=';

var REDIRECT_URL =
  'http://example.test:8000/serviceworker/resources/redirect.php?ACAOrigin=*&Redirect=';
var HTTPS_REDIRECT_URL =
  'https://example.test:8443/serviceworker/resources/redirect.php?ACAOrigin=*&Redirect=';
var HTTPS_OTHER_REDIRECT_URL =
  'https://localhost:8443/serviceworker/resources/redirect.php?ACAOrigin=*&Redirect=';

testBlockMixedContent('no-cors');

done();
