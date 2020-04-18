'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var arrayify = require('array-back');
var option = require('./option');
var Definition = require('./definition');
var t = require('typical');

var Definitions = function () {
  function Definitions(definitions) {
    var _this = this;

    _classCallCheck(this, Definitions);

    this.list = [];
    arrayify(definitions).forEach(function (def) {
      return _this.list.push(new Definition(def));
    });
    this.validate();
  }

  _createClass(Definitions, [{
    key: 'validate',
    value: function validate(argv) {
      var someHaveNoName = this.list.some(function (def) {
        return !def.name;
      });
      if (someHaveNoName) {
        halt('NAME_MISSING', 'Invalid option definitions: the `name` property is required on each definition');
      }

      var someDontHaveFunctionType = this.list.some(function (def) {
        return def.type && typeof def.type !== 'function';
      });
      if (someDontHaveFunctionType) {
        halt('INVALID_TYPE', 'Invalid option definitions: the `type` property must be a setter fuction (default: `Boolean`)');
      }

      var invalidOption = void 0;

      var numericAlias = this.list.some(function (def) {
        invalidOption = def;
        return t.isDefined(def.alias) && t.isNumber(def.alias);
      });
      if (numericAlias) {
        halt('INVALID_ALIAS', 'Invalid option definition: to avoid ambiguity an alias cannot be numeric [--' + invalidOption.name + ' alias is -' + invalidOption.alias + ']');
      }

      var multiCharacterAlias = this.list.some(function (def) {
        invalidOption = def;
        return t.isDefined(def.alias) && def.alias.length !== 1;
      });
      if (multiCharacterAlias) {
        halt('INVALID_ALIAS', 'Invalid option definition: an alias must be a single character');
      }

      var hypenAlias = this.list.some(function (def) {
        invalidOption = def;
        return def.alias === '-';
      });
      if (hypenAlias) {
        halt('INVALID_ALIAS', 'Invalid option definition: an alias cannot be "-"');
      }

      var duplicateName = hasDuplicates(this.list.map(function (def) {
        return def.name;
      }));
      if (duplicateName) {
        halt('DUPLICATE_NAME', 'Two or more option definitions have the same name');
      }

      var duplicateAlias = hasDuplicates(this.list.map(function (def) {
        return def.alias;
      }));
      if (duplicateAlias) {
        halt('DUPLICATE_ALIAS', 'Two or more option definitions have the same alias');
      }

      var duplicateDefaultOption = hasDuplicates(this.list.map(function (def) {
        return def.defaultOption;
      }));
      if (duplicateDefaultOption) {
        halt('DUPLICATE_DEFAULT_OPTION', 'Only one option definition can be the defaultOption');
      }
    }
  }, {
    key: 'createOutput',
    value: function createOutput() {
      var output = {};
      this.list.forEach(function (def) {
        if (t.isDefined(def.defaultValue)) output[def.name] = def.defaultValue;
        if (Array.isArray(output[def.name])) {
          output[def.name]._initial = true;
        }
      });
      return output;
    }
  }, {
    key: 'get',
    value: function get(arg) {
      return option.short.test(arg) ? this.list.find(function (def) {
        return def.alias === option.short.name(arg);
      }) : this.list.find(function (def) {
        return def.name === option.long.name(arg);
      });
    }
  }, {
    key: 'getDefault',
    value: function getDefault() {
      return this.list.find(function (def) {
        return def.defaultOption === true;
      });
    }
  }, {
    key: 'isGrouped',
    value: function isGrouped() {
      return this.list.some(function (def) {
        return def.group;
      });
    }
  }, {
    key: 'whereGrouped',
    value: function whereGrouped() {
      return this.list.filter(containsValidGroup);
    }
  }, {
    key: 'whereNotGrouped',
    value: function whereNotGrouped() {
      return this.list.filter(function (def) {
        return !containsValidGroup(def);
      });
    }
  }]);

  return Definitions;
}();

function halt(name, message) {
  var err = new Error(message);
  err.name = name;
  throw err;
}

function containsValidGroup(def) {
  return arrayify(def.group).some(function (group) {
    return group;
  });
}

function hasDuplicates(array) {
  var items = {};
  for (var i = 0; i < array.length; i++) {
    var value = array[i];
    if (items[value]) {
      return true;
    } else {
      if (t.isDefined(value)) items[value] = true;
    }
  }
}

module.exports = Definitions;