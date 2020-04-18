(function (global, factory) {
  if (typeof define === "function" && define.amd) {
    define(['exports', './shady-css/common', './shady-css/token', './shady-css/tokenizer', './shady-css/node-factory', './shady-css/node-visitor', './shady-css/stringifier', './shady-css/parser'], factory);
  } else if (typeof exports !== "undefined") {
    factory(exports, require('./shady-css/common'), require('./shady-css/token'), require('./shady-css/tokenizer'), require('./shady-css/node-factory'), require('./shady-css/node-visitor'), require('./shady-css/stringifier'), require('./shady-css/parser'));
  } else {
    var mod = {
      exports: {}
    };
    factory(mod.exports, global.common, global.token, global.tokenizer, global.nodeFactory, global.nodeVisitor, global.stringifier, global.parser);
    global.shadyCss = mod.exports;
  }
})(this, function (exports, _common, _token, _tokenizer, _nodeFactory, _nodeVisitor, _stringifier, _parser) {
  'use strict';

  Object.defineProperty(exports, "__esModule", {
    value: true
  });
  Object.defineProperty(exports, 'nodeType', {
    enumerable: true,
    get: function () {
      return _common.nodeType;
    }
  });
  Object.defineProperty(exports, 'Token', {
    enumerable: true,
    get: function () {
      return _token.Token;
    }
  });
  Object.defineProperty(exports, 'Tokenizer', {
    enumerable: true,
    get: function () {
      return _tokenizer.Tokenizer;
    }
  });
  Object.defineProperty(exports, 'NodeFactory', {
    enumerable: true,
    get: function () {
      return _nodeFactory.NodeFactory;
    }
  });
  Object.defineProperty(exports, 'NodeVisitor', {
    enumerable: true,
    get: function () {
      return _nodeVisitor.NodeVisitor;
    }
  });
  Object.defineProperty(exports, 'Stringifier', {
    enumerable: true,
    get: function () {
      return _stringifier.Stringifier;
    }
  });
  Object.defineProperty(exports, 'Parser', {
    enumerable: true,
    get: function () {
      return _parser.Parser;
    }
  });
});