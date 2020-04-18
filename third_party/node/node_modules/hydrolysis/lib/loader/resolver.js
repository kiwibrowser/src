'use strict';

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

require("babel-polyfill");

var Deferred = function Deferred() {
    _classCallCheck(this, Deferred);

    var self = this;
    this.promise = new Promise(function (resolve, reject) {
        self.resolve = resolve;
        self.reject = reject;
    });
};

exports.Deferred = Deferred;