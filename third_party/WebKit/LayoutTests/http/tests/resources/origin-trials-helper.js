// This file provides an OriginTrialsHelper object which can be used by
// LayoutTests that are checking members exposed to script by origin trials.
//
// The current available methods are:
// check_properties:
//   Tests for the existence of the given property names, on the given interface
//   names, on the global object. As well, it can test for properties of the
//   global object itself, by giving 'global' as the interface name.
// Example:
//   OriginTrialsHelper.check_properties(
//     this,
//     {'InstallEvent':['registerForeignFetch'], 'global':['onforeignfetch']});
//
// check_properties_missing:
//   Tests that the given property names do not exist on the global object. That
//   is, tests for the opposite of check_properties().
// Example:
//   OriginTrialsHelper.check_properties_missing(
//     this,
//     {'InstallEvent':['registerForeignFetch'], 'global':['onforeignfetch']});
//
// check_interfaces:
//   Tests for the existence of the given interface names, on the global object.
// Example:
//   OriginTrialsHelper.check_interfaces(
//     this,
//     ['USBAlternateInterface', 'USBConfiguration']);
//
// check_interfaces_missing:
//   Tests that the given interface names do not exist on the global object.
//   That is, tests for the opposite of check_interfaces().
// Example:
//   OriginTrialsHelper.check_interfaces_missing(
//     this,
//     ['USBAlternateInterface', 'USBConfiguration']);
//
// add_token:
//   Adds a trial token to the document, to enable a trial via script
// Example:
//   OriginTrialsHelper.add_token('token produced by generate_token.py');
'use strict';

var OriginTrialsHelper = (function() {
  return {
    check_properties_impl: (global_object, property_filters, should_exist) => {
      let interface_names = Object.getOwnPropertyNames(property_filters).sort();
      interface_names.forEach(function(interface_name) {
        let interface_prototype;
        if (interface_name === 'global') {
          interface_prototype = global_object;
        } else {
          let interface_object = global_object[interface_name];
          if (interface_object) {
            interface_prototype = interface_object.prototype;
          }
        }
        assert_true(interface_prototype !== undefined, 'Interface ' + interface_name + ' not found');
        property_filters[interface_name].forEach(function(property_name) {
          assert_equals(interface_prototype.hasOwnProperty(property_name),
              should_exist,
              'Property ' + property_name + ' exists on ' + interface_name);
        });
      });
    },

    check_properties: (global_object, property_filters) => {
      OriginTrialsHelper.check_properties_impl(global_object, property_filters, true);
    },

    check_properties_missing: (global_object, property_filters) => {
      OriginTrialsHelper.check_properties_impl(global_object, property_filters, false);
    },

    check_interfaces_impl: (global_object, interface_names, should_exist) => {
      interface_names.sort();
      interface_names.forEach(function(interface_name) {
        assert_equals(global_object.hasOwnProperty(interface_name), should_exist,
          'Interface ' + interface_name + ' exists on provided object');
      });
    },

    check_interfaces: (global_object, interface_names) => {
      OriginTrialsHelper.check_interfaces_impl(global_object, interface_names, true);
    },

    check_interfaces_missing: (global_object, interface_names) => {
      OriginTrialsHelper.check_interfaces_impl(global_object, interface_names, false);
    },

    add_token: (token_string) => {
      var tokenElement = document.createElement('meta');
      tokenElement.httpEquiv = 'origin-trial';
      tokenElement.content = token_string;
      document.head.appendChild(tokenElement);
    }
  }
})();
