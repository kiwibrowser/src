'use strict';

var arrayify = require('array-back');
var Definitions = require('./definitions');
var option = require('./option');
var t = require('typical');
var Argv = require('./argv');

module.exports = commandLineArgs;

function commandLineArgs(definitions, argv) {
  definitions = new Definitions(definitions);
  argv = new Argv(argv);
  argv.expandOptionEqualsNotation();
  argv.expandGetoptNotation();
  argv.validate(definitions);

  var output = definitions.createOutput();
  var def = void 0;

  argv.list.forEach(function (item) {
    if (option.isOption(item)) {
      def = definitions.get(item);
      if (!t.isDefined(output[def.name])) outputSet(output, def.name, def.getInitialValue());
      if (def.isBoolean()) {
        outputSet(output, def.name, true);
        def = null;
      }
    } else {
      var reBeginsWithValueMarker = new RegExp('^' + option.VALUE_MARKER);
      var value = reBeginsWithValueMarker.test(item) ? item.replace(reBeginsWithValueMarker, '') : item;
      if (!def) {
        def = definitions.getDefault();
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

  if (definitions.isGrouped()) {
    return groupOutput(definitions, output);
  } else {
    return output;
  }
}

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

function groupOutput(definitions, output) {
  var grouped = {
    _all: output
  };

  definitions.whereGrouped().forEach(function (def) {
    arrayify(def.group).forEach(function (groupName) {
      grouped[groupName] = grouped[groupName] || {};
      if (t.isDefined(output[def.name])) {
        grouped[groupName][def.name] = output[def.name];
      }
    });
  });

  definitions.whereNotGrouped().forEach(function (def) {
    if (t.isDefined(output[def.name])) {
      if (!grouped._none) grouped._none = {};
      grouped._none[def.name] = output[def.name];
    }
  });
  return grouped;
}