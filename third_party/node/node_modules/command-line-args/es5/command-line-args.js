'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol ? "symbol" : typeof obj; };

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var arrayify = require('array-back');
var Definitions = require('./definitions');
var option = require('./option');
var t = require('typical');
var Argv = require('./argv');

module.exports = commandLineArgs;

var CommandLineArgs = function () {
  function CommandLineArgs(definitions) {
    _classCallCheck(this, CommandLineArgs);

    this.definitions = new Definitions(definitions);
  }

  _createClass(CommandLineArgs, [{
    key: 'parse',
    value: function parse(argv) {
      var _this = this;

      argv = new Argv(argv);
      argv.expandOptionEqualsNotation();
      argv.expandGetoptNotation();
      argv.validate(this.definitions);

      var output = this.definitions.createOutput();
      var def = undefined;

      argv.list.forEach(function (item) {
        if (option.isOption(item)) {
          def = _this.definitions.get(item);
          if (!t.isDefined(output[def.name])) outputSet(output, def.name, def.getInitialValue());
          if (def.isBoolean()) {
            outputSet(output, def.name, true);
            def = null;
          }
        } else {
          var value = item;
          if (!def) {
            def = _this.definitions.getDefault();
            if (!def) return;
            if (!t.isDefined(output[def.name])) outputSet(output, def.name, def.getInitialValue());
          }

          var outputValue = def.type ? def.type(value) : value;
          outputSet(output, def.name, outputValue);

          if (!def.multiple) def = null;
        }
      });

      for (var key in output) {
        var value = output[key];
        if (Array.isArray(value) && value._initial) delete value._initial;
      }

      if (this.definitions.isGrouped()) {
        var _ret = function () {
          var grouped = {
            _all: output
          };

          _this.definitions.whereGrouped().forEach(function (def) {
            arrayify(def.group).forEach(function (groupName) {
              grouped[groupName] = grouped[groupName] || {};
              if (t.isDefined(output[def.name])) {
                grouped[groupName][def.name] = output[def.name];
              }
            });
          });

          _this.definitions.whereNotGrouped().forEach(function (def) {
            if (t.isDefined(output[def.name])) {
              if (!grouped._none) grouped._none = {};
              grouped._none[def.name] = output[def.name];
            }
          });
          return {
            v: grouped
          };
        }();

        if ((typeof _ret === 'undefined' ? 'undefined' : _typeof(_ret)) === "object") return _ret.v;
      } else {
        return output;
      }
    }
  }, {
    key: 'getUsage',
    value: function getUsage(options) {
      var getUsage = require('command-line-usage');
      return getUsage(this.definitions.list, options);
    }
  }]);

  return CommandLineArgs;
}();

function outputSet(output, property, value) {
  if (output[property] && output[property]._initial) {
    output[property] = [];
    delete output[property]._initial;
  }
  if (Array.isArray(output[property])) {
    output[property].push(value);
  } else {
    output[property] = value;
  }
}

function commandLineArgs(definitions) {
  return new CommandLineArgs(definitions);
}