importScripts("../../../resources/testharness.js")

test(function() {
    assert_true('URLSearchParams' in self);
    assert_true('toString' in URLSearchParams.prototype);
    assert_true('append' in URLSearchParams.prototype);
    assert_true('delete' in URLSearchParams.prototype);
    assert_true('get' in URLSearchParams.prototype);
    assert_true('getAll' in URLSearchParams.prototype);
    assert_true('has' in URLSearchParams.prototype);
    assert_true('set' in URLSearchParams.prototype);
}, 'URLSearchParams interface');

done();
