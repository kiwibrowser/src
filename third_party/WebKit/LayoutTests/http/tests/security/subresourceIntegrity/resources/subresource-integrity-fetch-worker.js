importScripts('/resources/testharness.js');
console.log('hehehe');
const url = '../call-success.js';
const integrity = 'sha256-B0/62fJSJFrdjEFR9ba04m/D+LHQ+zG6PGcaR0Trpxg=';

promise_test(() => {
    return fetch(url).then(res => res.text()).then(text => {
        assert_equals(text, 'success();\n');
    });
}, 'No integrity');

promise_test(() => {
    return fetch(url, {integrity: integrity}).then(res => {
        return res.text();
    }).then(text => {
        assert_equals(text, 'success();\n');
    });
}, 'Good integrity');

promise_test(() => {
    return fetch(url, {integrity: 'sha256-deadbeaf'}).then(res => {
        assert_unreached('the integrity check should fail');
    }, () => {
        // The integrity check should fail.
    });
}, 'Bad integrity');

done();
