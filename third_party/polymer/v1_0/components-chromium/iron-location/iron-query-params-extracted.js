'use strict';

  Polymer({
    is: 'iron-query-params',

    properties: {
      paramsString: {
        type: String,
        notify: true,
        observer: 'paramsStringChanged',
      },

      paramsObject: {
        type: Object,
        notify: true,
        value: function() {
          return {};
        }
      },

      _dontReact: {
        type: Boolean,
        value: false
      }
    },

    hostAttributes: {
      hidden: true
    },

    observers: [
      'paramsObjectChanged(paramsObject.*)'
    ],

    paramsStringChanged: function() {
      this._dontReact = true;
      this.paramsObject = this._decodeParams(this.paramsString);
      this._dontReact = false;
    },

    paramsObjectChanged: function() {
      if (this._dontReact) {
        return;
      }
      this.paramsString = this._encodeParams(this.paramsObject)
          .replace(/%3F/g, '?').replace(/%2F/g, '/').replace(/'/g, '%27');
    },

    _encodeParams: function(params) {
      var encodedParams = [];

      for (var key in params) {
        var value = params[key];

        if (value === '') {
          encodedParams.push(encodeURIComponent(key));

        } else if (value) {
          encodedParams.push(
              encodeURIComponent(key) +
              '=' +
              encodeURIComponent(value.toString())
          );
        }
      }
      return encodedParams.join('&');
    },

    _decodeParams: function(paramString) {
      var params = {};
      // Work around a bug in decodeURIComponent where + is not
      // converted to spaces:
      paramString = (paramString || '').replace(/\+/g, '%20');
      var paramList = paramString.split('&');
      for (var i = 0; i < paramList.length; i++) {
        var param = paramList[i].split('=');
        if (param[0]) {
          params[decodeURIComponent(param[0])] =
              decodeURIComponent(param[1] || '');
        }
      }
      return params;
    }
  });