var BASE_ORIGIN = 'http://127.0.0.1:8000';
var OTHER_ORIGIN = 'http://localhost:8000';
var TEST_OPTIONS = '';
// TEST_OPTIONS is '', '-other-https', '-base-https', or
// '-base-https-other-https'.

if (location.href.indexOf('base-https') >= 0) {
  BASE_ORIGIN = 'https://127.0.0.1:8443';
  TEST_OPTIONS += '-base-https';
}

if (location.href.indexOf('other-https') >= 0) {
  OTHER_ORIGIN = 'https://localhost:8443';
  TEST_OPTIONS += '-other-https';
}
