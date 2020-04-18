'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var arrayify = require('array-back');
var option = require('./option');
var findReplace = require('find-replace');

var Argv = function () {
  function Argv(argv) {
    _classCallCheck(this, Argv);

    if (argv) {
      argv = arrayify(argv);
    } else {
      argv = process.argv.slice(0);
      argv.splice(0, 2);
    }

    this.list = argv;
  }

  _createClass(Argv, [{
    key: 'clear',
    value: function clear() {
      this.list.length = 0;
    }
  }, {
    key: 'expandOptionEqualsNotation',
    value: function expandOptionEqualsNotation() {
      var _this = this;

      var optEquals = option.optEquals;
      if (this.list.some(optEquals.test.bind(optEquals))) {
        (function () {
          var expandedArgs = [];
          _this.list.forEach(function (arg) {
            var matches = arg.match(optEquals.re);
            if (matches) {
              expandedArgs.push(matches[1], option.VALUE_MARKER + matches[2]);
            } else {
              expandedArgs.push(arg);
            }
          });
          _this.clear();
          _this.list = expandedArgs;
        })();
      }
    }
  }, {
    key: 'expandGetoptNotation',
    value: function expandGetoptNotation() {
      var combinedArg = option.combined;
      var hasGetopt = this.list.some(combinedArg.test.bind(combinedArg));
      if (hasGetopt) {
        findReplace(this.list, combinedArg.re, function (arg) {
          arg = arg.slice(1);
          return arg.split('').map(function (letter) {
            return '-' + letter;
          });
        });
      }
    }
  }, {
    key: 'validate',
    value: function validate(definitions) {
      var invalidOption = void 0;

      var optionWithoutDefinition = this.list.filter(function (arg) {
        return option.isOption(arg);
      }).some(function (arg) {
        if (definitions.get(arg) === undefined) {
          invalidOption = arg;
          return true;
        }
      });
      if (optionWithoutDefinition) {
        halt('UNKNOWN_OPTION', 'Unknown option: ' + invalidOption);
      }
    }
  }]);

  return Argv;
}();

function halt(name, message) {
  var err = new Error(message);
  err.name = name;
  throw err;
}

module.exports = Argv;