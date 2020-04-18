'use strict';

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var Section = require('./section');
var Table = require('table-layout');
var ansi = require('ansi-escape-sequences');
var t = require('typical');
var arrayify = require('array-back');

var OptionList = function (_Section) {
  _inherits(OptionList, _Section);

  function OptionList(data) {
    _classCallCheck(this, OptionList);

    var _this = _possibleConstructorReturn(this, (OptionList.__proto__ || Object.getPrototypeOf(OptionList)).call(this));

    var definitions = arrayify(data.optionList);
    var hide = arrayify(data.hide);
    var groups = arrayify(data.group);

    if (hide.length) {
      definitions = definitions.filter(function (definition) {
        return hide.indexOf(definition.name) === -1;
      });
    }

    if (data.header) _this.header(data.header);

    if (groups.length) {
      definitions = definitions.filter(function (def) {
        var noGroupMatch = groups.indexOf('_none') > -1 && !t.isDefined(def.group);
        var groupMatch = intersect(arrayify(def.group), groups);
        if (noGroupMatch || groupMatch) return def;
      });
    }

    var columns = definitions.map(function (def) {
      return {
        option: getOptionNames(def, 'bold'),
        description: ansi.format(def.description)
      };
    });

    var table = new Table(columns, {
      padding: { left: '  ', right: ' ' },
      columns: [{ name: 'option', noWrap: true }, { name: 'description', maxWidth: 80 }]
    });
    _this.add(table.renderLines());

    _this.emptyLine();
    return _this;
  }

  return OptionList;
}(Section);

function getOptionNames(definition, optionNameStyles) {
  var names = [];
  var type = definition.type ? definition.type.name.toLowerCase() : '';
  var multiple = definition.multiple ? '[]' : '';
  if (type) {
    type = type === 'boolean' ? '' : '[underline]{' + type + multiple + '}';
  }
  type = ansi.format(definition.typeLabel || type);

  if (definition.alias) {
    names.push(ansi.format('-' + definition.alias, optionNameStyles));
  }
  names.push(ansi.format('--' + definition.name, optionNameStyles) + ' ' + type);
  return names.join(', ');
}

function intersect(arr1, arr2) {
  return arr1.some(function (item1) {
    return arr2.some(function (item2) {
      return item1 === item2;
    });
  });
}

module.exports = OptionList;