'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var t = require('typical');

var OptionDefinition = function () {
  function OptionDefinition(definition) {
    _classCallCheck(this, OptionDefinition);

    this.name = definition.name;

    this.type = definition.type;

    this.alias = definition.alias;

    this.multiple = definition.multiple;

    this.defaultOption = definition.defaultOption;

    this.defaultValue = definition.defaultValue;

    this.group = definition.group;

    for (var prop in definition) {
      if (!this[prop]) this[prop] = definition[prop];
    }
  }

  _createClass(OptionDefinition, [{
    key: 'getInitialValue',
    value: function getInitialValue() {
      if (this.multiple) {
        return [];
      } else if (this.isBoolean() || !this.type) {
        return true;
      } else {
        return null;
      }
    }
  }, {
    key: 'isBoolean',
    value: function isBoolean() {
      if (this.type) {
        return this.type === Boolean || t.isFunction(this.type) && this.type.name === 'Boolean';
      } else {
        return false;
      }
    }
  }]);

  return OptionDefinition;
}();

module.exports = OptionDefinition;