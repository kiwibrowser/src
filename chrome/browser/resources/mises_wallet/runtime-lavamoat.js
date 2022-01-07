;(function(){

  const moduleRegistry = new Map()
  const moduleCache = new Map()
  const lavamoatPolicy = { resources: {} }

  // initialize the kernel
  const createKernel = // LavaMoat Prelude
(function () {
  return createKernel

  function createKernel ({
    lavamoatConfig,
    loadModuleData,
    getRelativeModuleId,
    prepareModuleInitializerArgs,
    getExternalCompartment,
    globalThisRefs,
    runWithPrecompiledModules,
  }) {
    const debugMode = false

    // identify the globalRef
    const globalRef = (typeof globalThis !== 'undefined') ? globalThis : (typeof self !== 'undefined') ? self : (typeof global !== 'undefined') ? global : undefined
    if (!globalRef) {
      throw new Error('Lavamoat - unable to identify globalRef')
    }

    // polyfill globalThis
    if (globalRef && !globalRef.globalThis) {
      globalRef.globalThis = globalRef
    }

    // create the SES rootRealm
    // "templateRequire" calls are inlined in "generateKernel"
    // load-bearing semi-colon, do not remove
    ;// define ses
(function(){
  const global = globalRef
  const exports = {}
  const module = { exports }
  ;(function(){
// START of injected code from ses
'use strict';
(functors => {
  function cell(name, value = undefined) {
    const observers = [];
    function set(newValue) {
      value = newValue;
      for (const observe of observers) {
        observe(value);
      }
    }
    function get() {
      return value;
    }
    function observe(observe) {
      observers.push(observe);
      observe(value);
    }
    return { get, set, observe, enumerable: true };
  }

  const cells = [{
        globalThis: cell("globalThis"),
Array: cell("Array"),
Date: cell("Date"),
Float32Array: cell("Float32Array"),
JSON: cell("JSON"),
Map: cell("Map"),
Math: cell("Math"),
Object: cell("Object"),
Promise: cell("Promise"),
Proxy: cell("Proxy"),
Reflect: cell("Reflect"),
FERAL_REG_EXP: cell("FERAL_REG_EXP"),
Set: cell("Set"),
String: cell("String"),
WeakMap: cell("WeakMap"),
WeakSet: cell("WeakSet"),
FERAL_ERROR: cell("FERAL_ERROR"),
RangeError: cell("RangeError"),
ReferenceError: cell("ReferenceError"),
SyntaxError: cell("SyntaxError"),
TypeError: cell("TypeError"),
assign: cell("assign"),
create: cell("create"),
defineProperties: cell("defineProperties"),
entries: cell("entries"),
freeze: cell("freeze"),
getOwnPropertyDescriptor: cell("getOwnPropertyDescriptor"),
getOwnPropertyDescriptors: cell("getOwnPropertyDescriptors"),
getOwnPropertyNames: cell("getOwnPropertyNames"),
getPrototypeOf: cell("getPrototypeOf"),
is: cell("is"),
isExtensible: cell("isExtensible"),
keys: cell("keys"),
objectPrototype: cell("objectPrototype"),
seal: cell("seal"),
setPrototypeOf: cell("setPrototypeOf"),
values: cell("values"),
speciesSymbol: cell("speciesSymbol"),
toStringTagSymbol: cell("toStringTagSymbol"),
iteratorSymbol: cell("iteratorSymbol"),
matchAllSymbol: cell("matchAllSymbol"),
stringifyJson: cell("stringifyJson"),
fromEntries: cell("fromEntries"),
defineProperty: cell("defineProperty"),
apply: cell("apply"),
construct: cell("construct"),
reflectGet: cell("reflectGet"),
reflectGetOwnPropertyDescriptor: cell("reflectGetOwnPropertyDescriptor"),
reflectHas: cell("reflectHas"),
reflectIsExtensible: cell("reflectIsExtensible"),
ownKeys: cell("ownKeys"),
reflectPreventExtensions: cell("reflectPreventExtensions"),
reflectSet: cell("reflectSet"),
isArray: cell("isArray"),
arrayPrototype: cell("arrayPrototype"),
mapPrototype: cell("mapPrototype"),
proxyRevocable: cell("proxyRevocable"),
regexpPrototype: cell("regexpPrototype"),
setPrototype: cell("setPrototype"),
stringPrototype: cell("stringPrototype"),
weakmapPrototype: cell("weakmapPrototype"),
weaksetPrototype: cell("weaksetPrototype"),
functionPrototype: cell("functionPrototype"),
promisePrototype: cell("promisePrototype"),
uncurryThis: cell("uncurryThis"),
objectHasOwnProperty: cell("objectHasOwnProperty"),
arrayFilter: cell("arrayFilter"),
arrayForEach: cell("arrayForEach"),
arrayIncludes: cell("arrayIncludes"),
arrayJoin: cell("arrayJoin"),
arrayMap: cell("arrayMap"),
arrayPop: cell("arrayPop"),
arrayPush: cell("arrayPush"),
arraySlice: cell("arraySlice"),
arraySome: cell("arraySome"),
arraySort: cell("arraySort"),
iterateArray: cell("iterateArray"),
mapSet: cell("mapSet"),
mapGet: cell("mapGet"),
mapHas: cell("mapHas"),
iterateMap: cell("iterateMap"),
setAdd: cell("setAdd"),
setForEach: cell("setForEach"),
setHas: cell("setHas"),
iterateSet: cell("iterateSet"),
regexpTest: cell("regexpTest"),
regexpExec: cell("regexpExec"),
matchAllRegExp: cell("matchAllRegExp"),
stringEndsWith: cell("stringEndsWith"),
stringIncludes: cell("stringIncludes"),
stringIndexOf: cell("stringIndexOf"),
stringMatch: cell("stringMatch"),
stringReplace: cell("stringReplace"),
stringSearch: cell("stringSearch"),
stringSlice: cell("stringSlice"),
stringSplit: cell("stringSplit"),
stringStartsWith: cell("stringStartsWith"),
iterateString: cell("iterateString"),
weakmapDelete: cell("weakmapDelete"),
weakmapGet: cell("weakmapGet"),
weakmapHas: cell("weakmapHas"),
weakmapSet: cell("weakmapSet"),
weaksetAdd: cell("weaksetAdd"),
weaksetGet: cell("weaksetGet"),
weaksetHas: cell("weaksetHas"),
functionToString: cell("functionToString"),
promiseAll: cell("promiseAll"),
promiseCatch: cell("promiseCatch"),
promiseThen: cell("promiseThen"),
getConstructorOf: cell("getConstructorOf"),
immutableObject: cell("immutableObject"),
isObject: cell("isObject"),
isError: cell("isError"),
FERAL_EVAL: cell("FERAL_EVAL"),
FERAL_FUNCTION: cell("FERAL_FUNCTION"),

        
      },{
        
        
      },{
        an: cell("an"),
bestEffortStringify: cell("bestEffortStringify"),

        
      },{
        
        
      },{
        unredactedDetails: cell("unredactedDetails"),
loggedErrorHandler: cell("loggedErrorHandler"),
makeAssert: cell("makeAssert"),
assert: cell("assert"),

        
      },{
        makeEvaluateFactory: cell("makeEvaluateFactory"),

        
      },{
        isValidIdentifierName: cell("isValidIdentifierName"),
getScopeConstants: cell("getScopeConstants"),

        
      },{
        createScopeHandler: cell("createScopeHandler"),

        
      },{
        getSourceURL: cell("getSourceURL"),

        
      },{
        rejectHtmlComments: cell("rejectHtmlComments"),
evadeHtmlCommentTest: cell("evadeHtmlCommentTest"),
rejectImportExpressions: cell("rejectImportExpressions"),
evadeImportExpressionTest: cell("evadeImportExpressionTest"),
rejectSomeDirectEvalExpressions: cell("rejectSomeDirectEvalExpressions"),
mandatoryTransforms: cell("mandatoryTransforms"),
applyTransforms: cell("applyTransforms"),

        
      },{
        prepareEval: cell("prepareEval"),
performEval: cell("performEval"),

        
      },{
        prepareCompartmentEvaluation: cell("prepareCompartmentEvaluation"),
compartmentEvaluate: cell("compartmentEvaluate"),
makeScopeProxy: cell("makeScopeProxy"),

        
      },{
        makeEvalFunction: cell("makeEvalFunction"),

        
      },{
        makeFunctionConstructor: cell("makeFunctionConstructor"),

        
      },{
        constantProperties: cell("constantProperties"),
universalPropertyNames: cell("universalPropertyNames"),
initialGlobalPropertyNames: cell("initialGlobalPropertyNames"),
sharedGlobalPropertyNames: cell("sharedGlobalPropertyNames"),
uniqueGlobalPropertyNames: cell("uniqueGlobalPropertyNames"),
NativeErrors: cell("NativeErrors"),
FunctionInstance: cell("FunctionInstance"),
isAccessorPermit: cell("isAccessorPermit"),
whitelist: cell("whitelist"),

        
      },{
        initGlobalObject: cell("initGlobalObject"),

        
      },{
        makeAlias: cell("makeAlias"),
load: cell("load"),

        
      },{
        deferExports: cell("deferExports"),
getDeferredExports: cell("getDeferredExports"),

        
      },{
        makeThirdPartyModuleInstance: cell("makeThirdPartyModuleInstance"),
makeModuleInstance: cell("makeModuleInstance"),

        
      },{
        link: cell("link"),
instantiate: cell("instantiate"),

        
      },{
        InertCompartment: cell("InertCompartment"),
CompartmentPrototype: cell("CompartmentPrototype"),
makeCompartmentConstructor: cell("makeCompartmentConstructor"),

        
      },{
        getAnonymousIntrinsics: cell("getAnonymousIntrinsics"),

        
      },{
        makeIntrinsicsCollector: cell("makeIntrinsicsCollector"),
getGlobalIntrinsics: cell("getGlobalIntrinsics"),

        
      },{
        minEnablements: cell("minEnablements"),
moderateEnablements: cell("moderateEnablements"),
severeEnablements: cell("severeEnablements"),

        
      },{
        default: cell("default"),

        
      },{
        makeLoggingConsoleKit: cell("makeLoggingConsoleKit"),
makeCausalConsole: cell("makeCausalConsole"),
filterConsole: cell("filterConsole"),
consoleWhitelist: cell("consoleWhitelist"),
BASE_CONSOLE_LEVEL: cell("BASE_CONSOLE_LEVEL"),

        
      },{
        tameConsole: cell("tameConsole"),

        
      },{
        filterFileName: cell("filterFileName"),
shortenCallSiteString: cell("shortenCallSiteString"),
tameV8ErrorConstructor: cell("tameV8ErrorConstructor"),

        
      },{
        default: cell("default"),

        
      },{
        makeHardener: cell("makeHardener"),

        
      },{
        default: cell("default"),

        
      },{
        tameDomains: cell("tameDomains"),

        
      },{
        default: cell("default"),

        
      },{
        tameFunctionToString: cell("tameFunctionToString"),

        
      },{
        default: cell("default"),

        
      },{
        default: cell("default"),

        
      },{
        default: cell("default"),

        
      },{
        default: cell("default"),

        
      },{
        repairIntrinsics: cell("repairIntrinsics"),
makeLockdown: cell("makeLockdown"),

        
      },{
        
        
      },];

  

  const namespaces = cells.map(cells => Object.create(null, cells));

  for (let index = 0; index < namespaces.length; index += 1) {
    cells[index]['*'] = cell('*', namespaces[index]);
  }

          functors[0]({
          imports(entries) {
            const map = new Map(entries);
            
          },
          liveVar: {
            
          },
          onceVar: {
                              universalThis: cells[0].globalThis.set,
                                  Array: cells[0].Array.set,
                                  Date: cells[0].Date.set,
                                  Float32Array: cells[0].Float32Array.set,
                                  JSON: cells[0].JSON.set,
                                  Map: cells[0].Map.set,
                                  Math: cells[0].Math.set,
                                  Object: cells[0].Object.set,
                                  Promise: cells[0].Promise.set,
                                  Proxy: cells[0].Proxy.set,
                                  Reflect: cells[0].Reflect.set,
                                  FERAL_REG_EXP: cells[0].FERAL_REG_EXP.set,
                                  Set: cells[0].Set.set,
                                  String: cells[0].String.set,
                                  WeakMap: cells[0].WeakMap.set,
                                  WeakSet: cells[0].WeakSet.set,
                                  FERAL_ERROR: cells[0].FERAL_ERROR.set,
                                  RangeError: cells[0].RangeError.set,
                                  ReferenceError: cells[0].ReferenceError.set,
                                  SyntaxError: cells[0].SyntaxError.set,
                                  TypeError: cells[0].TypeError.set,
                                  assign: cells[0].assign.set,
                                  create: cells[0].create.set,
                                  defineProperties: cells[0].defineProperties.set,
                                  entries: cells[0].entries.set,
                                  freeze: cells[0].freeze.set,
                                  getOwnPropertyDescriptor: cells[0].getOwnPropertyDescriptor.set,
                                  getOwnPropertyDescriptors: cells[0].getOwnPropertyDescriptors.set,
                                  getOwnPropertyNames: cells[0].getOwnPropertyNames.set,
                                  getPrototypeOf: cells[0].getPrototypeOf.set,
                                  is: cells[0].is.set,
                                  isExtensible: cells[0].isExtensible.set,
                                  keys: cells[0].keys.set,
                                  objectPrototype: cells[0].objectPrototype.set,
                                  seal: cells[0].seal.set,
                                  setPrototypeOf: cells[0].setPrototypeOf.set,
                                  values: cells[0].values.set,
                                  speciesSymbol: cells[0].speciesSymbol.set,
                                  toStringTagSymbol: cells[0].toStringTagSymbol.set,
                                  iteratorSymbol: cells[0].iteratorSymbol.set,
                                  matchAllSymbol: cells[0].matchAllSymbol.set,
                                  stringifyJson: cells[0].stringifyJson.set,
                                  fromEntries: cells[0].fromEntries.set,
                                  defineProperty: cells[0].defineProperty.set,
                                  apply: cells[0].apply.set,
                                  construct: cells[0].construct.set,
                                  reflectGet: cells[0].reflectGet.set,
                                  reflectGetOwnPropertyDescriptor: cells[0].reflectGetOwnPropertyDescriptor.set,
                                  reflectHas: cells[0].reflectHas.set,
                                  reflectIsExtensible: cells[0].reflectIsExtensible.set,
                                  ownKeys: cells[0].ownKeys.set,
                                  reflectPreventExtensions: cells[0].reflectPreventExtensions.set,
                                  reflectSet: cells[0].reflectSet.set,
                                  isArray: cells[0].isArray.set,
                                  arrayPrototype: cells[0].arrayPrototype.set,
                                  mapPrototype: cells[0].mapPrototype.set,
                                  proxyRevocable: cells[0].proxyRevocable.set,
                                  regexpPrototype: cells[0].regexpPrototype.set,
                                  setPrototype: cells[0].setPrototype.set,
                                  stringPrototype: cells[0].stringPrototype.set,
                                  weakmapPrototype: cells[0].weakmapPrototype.set,
                                  weaksetPrototype: cells[0].weaksetPrototype.set,
                                  functionPrototype: cells[0].functionPrototype.set,
                                  promisePrototype: cells[0].promisePrototype.set,
                                  uncurryThis: cells[0].uncurryThis.set,
                                  objectHasOwnProperty: cells[0].objectHasOwnProperty.set,
                                  arrayFilter: cells[0].arrayFilter.set,
                                  arrayForEach: cells[0].arrayForEach.set,
                                  arrayIncludes: cells[0].arrayIncludes.set,
                                  arrayJoin: cells[0].arrayJoin.set,
                                  arrayMap: cells[0].arrayMap.set,
                                  arrayPop: cells[0].arrayPop.set,
                                  arrayPush: cells[0].arrayPush.set,
                                  arraySlice: cells[0].arraySlice.set,
                                  arraySome: cells[0].arraySome.set,
                                  arraySort: cells[0].arraySort.set,
                                  iterateArray: cells[0].iterateArray.set,
                                  mapSet: cells[0].mapSet.set,
                                  mapGet: cells[0].mapGet.set,
                                  mapHas: cells[0].mapHas.set,
                                  iterateMap: cells[0].iterateMap.set,
                                  setAdd: cells[0].setAdd.set,
                                  setForEach: cells[0].setForEach.set,
                                  setHas: cells[0].setHas.set,
                                  iterateSet: cells[0].iterateSet.set,
                                  regexpTest: cells[0].regexpTest.set,
                                  regexpExec: cells[0].regexpExec.set,
                                  matchAllRegExp: cells[0].matchAllRegExp.set,
                                  stringEndsWith: cells[0].stringEndsWith.set,
                                  stringIncludes: cells[0].stringIncludes.set,
                                  stringIndexOf: cells[0].stringIndexOf.set,
                                  stringMatch: cells[0].stringMatch.set,
                                  stringReplace: cells[0].stringReplace.set,
                                  stringSearch: cells[0].stringSearch.set,
                                  stringSlice: cells[0].stringSlice.set,
                                  stringSplit: cells[0].stringSplit.set,
                                  stringStartsWith: cells[0].stringStartsWith.set,
                                  iterateString: cells[0].iterateString.set,
                                  weakmapDelete: cells[0].weakmapDelete.set,
                                  weakmapGet: cells[0].weakmapGet.set,
                                  weakmapHas: cells[0].weakmapHas.set,
                                  weakmapSet: cells[0].weakmapSet.set,
                                  weaksetAdd: cells[0].weaksetAdd.set,
                                  weaksetGet: cells[0].weaksetGet.set,
                                  weaksetHas: cells[0].weaksetHas.set,
                                  functionToString: cells[0].functionToString.set,
                                  promiseAll: cells[0].promiseAll.set,
                                  promiseCatch: cells[0].promiseCatch.set,
                                  promiseThen: cells[0].promiseThen.set,
                                  getConstructorOf: cells[0].getConstructorOf.set,
                                  immutableObject: cells[0].immutableObject.set,
                                  isObject: cells[0].isObject.set,
                                  isError: cells[0].isError.set,
                                  FERAL_EVAL: cells[0].FERAL_EVAL.set,
                                  FERAL_FUNCTION: cells[0].FERAL_FUNCTION.set,
                
          },
        });
              functors[1]({
          imports(entries) {
            const map = new Map(entries);
            
          },
          liveVar: {
            
          },
          onceVar: {
            
          },
        });
              functors[2]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("../commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              an: cells[2].an.set,
                                  bestEffortStringify: cells[2].bestEffortStringify.set,
                
          },
        });
              functors[3]({
          imports(entries) {
            const map = new Map(entries);
            
          },
          liveVar: {
            
          },
          onceVar: {
            
          },
        });
              functors[4]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("../commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./internal-types.js")) {
                    const cell = cells[1][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./stringify-utils.js")) {
                    const cell = cells[2][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./types.js")) {
                    const cell = cells[3][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              unredactedDetails: cells[4].unredactedDetails.set,
                                  loggedErrorHandler: cells[4].loggedErrorHandler.set,
                                  makeAssert: cells[4].makeAssert.set,
                                  assert: cells[4].assert.set,
                
          },
        });
              functors[5]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeEvaluateFactory: cells[5].makeEvaluateFactory.set,
                
          },
        });
              functors[6]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              isValidIdentifierName: cells[6].isValidIdentifierName.set,
                                  getScopeConstants: cells[6].getScopeConstants.set,
                
          },
        });
              functors[7]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              createScopeHandler: cells[7].createScopeHandler.set,
                
          },
        });
              functors[8]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              getSourceURL: cells[8].getSourceURL.set,
                
          },
        });
              functors[9]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./get-source-url.js")) {
                    const cell = cells[8][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              rejectHtmlComments: cells[9].rejectHtmlComments.set,
                                  evadeHtmlCommentTest: cells[9].evadeHtmlCommentTest.set,
                                  rejectImportExpressions: cells[9].rejectImportExpressions.set,
                                  evadeImportExpressionTest: cells[9].evadeImportExpressionTest.set,
                                  rejectSomeDirectEvalExpressions: cells[9].rejectSomeDirectEvalExpressions.set,
                                  mandatoryTransforms: cells[9].mandatoryTransforms.set,
                                  applyTransforms: cells[9].applyTransforms.set,
                
          },
        });
              functors[10]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./make-evaluate-factory.js")) {
                    const cell = cells[5][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./scope-constants.js")) {
                    const cell = cells[6][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./scope-handler.js")) {
                    const cell = cells[7][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./transforms.js")) {
                    const cell = cells[9][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              prepareEval: cells[10].prepareEval.set,
                                  performEval: cells[10].performEval.set,
                
          },
        });
              functors[11]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./evaluate.js")) {
                    const cell = cells[10][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./transforms.js")) {
                    const cell = cells[9][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              prepareCompartmentEvaluation: cells[11].prepareCompartmentEvaluation.set,
                                  compartmentEvaluate: cells[11].compartmentEvaluate.set,
                                  makeScopeProxy: cells[11].makeScopeProxy.set,
                
          },
        });
              functors[12]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./evaluate.js")) {
                    const cell = cells[10][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeEvalFunction: cells[12].makeEvalFunction.set,
                
          },
        });
              functors[13]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./evaluate.js")) {
                    const cell = cells[10][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeFunctionConstructor: cells[13].makeFunctionConstructor.set,
                
          },
        });
              functors[14]({
          imports(entries) {
            const map = new Map(entries);
            
          },
          liveVar: {
            
          },
          onceVar: {
                              constantProperties: cells[14].constantProperties.set,
                                  universalPropertyNames: cells[14].universalPropertyNames.set,
                                  initialGlobalPropertyNames: cells[14].initialGlobalPropertyNames.set,
                                  sharedGlobalPropertyNames: cells[14].sharedGlobalPropertyNames.set,
                                  uniqueGlobalPropertyNames: cells[14].uniqueGlobalPropertyNames.set,
                                  NativeErrors: cells[14].NativeErrors.set,
                                  FunctionInstance: cells[14].FunctionInstance.set,
                                  isAccessorPermit: cells[14].isAccessorPermit.set,
                                  whitelist: cells[14].whitelist.set,
                
          },
        });
              functors[15]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./make-eval-function.js")) {
                    const cell = cells[12][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./make-function-constructor.js")) {
                    const cell = cells[13][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./whitelist.js")) {
                    const cell = cells[14][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              initGlobalObject: cells[15].initGlobalObject.set,
                
          },
        });
              functors[16]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeAlias: cells[16].makeAlias.set,
                                  load: cells[16].load.set,
                
          },
        });
              functors[17]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./module-load.js")) {
                    const cell = cells[16][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              deferExports: cells[17].deferExports.set,
                                  getDeferredExports: cells[17].getDeferredExports.set,
                
          },
        });
              functors[18]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./compartment-evaluate.js")) {
                    const cell = cells[11][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./module-proxy.js")) {
                    const cell = cells[17][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeThirdPartyModuleInstance: cells[18].makeThirdPartyModuleInstance.set,
                                  makeModuleInstance: cells[18].makeModuleInstance.set,
                
          },
        });
              functors[19]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./module-instance.js")) {
                    const cell = cells[18][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              link: cells[19].link.set,
                                  instantiate: cells[19].instantiate.set,
                
          },
        });
              functors[20]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./compartment-evaluate.js")) {
                    const cell = cells[11][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./global-object.js")) {
                    const cell = cells[15][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./module-link.js")) {
                    const cell = cells[19][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./module-load.js")) {
                    const cell = cells[16][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./module-proxy.js")) {
                    const cell = cells[17][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./scope-constants.js")) {
                    const cell = cells[6][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./whitelist.js")) {
                    const cell = cells[14][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              InertCompartment: cells[20].InertCompartment.set,
                                  CompartmentPrototype: cells[20].CompartmentPrototype.set,
                                  makeCompartmentConstructor: cells[20].makeCompartmentConstructor.set,
                
          },
        });
              functors[21]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./compartment-shim.js")) {
                    const cell = cells[20][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              getAnonymousIntrinsics: cells[21].getAnonymousIntrinsics.set,
                
          },
        });
              functors[22]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./whitelist.js")) {
                    const cell = cells[14][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeIntrinsicsCollector: cells[22].makeIntrinsicsCollector.set,
                                  getGlobalIntrinsics: cells[22].getGlobalIntrinsics.set,
                
          },
        });
              functors[23]({
          imports(entries) {
            const map = new Map(entries);
            
          },
          liveVar: {
            
          },
          onceVar: {
                              minEnablements: cells[23].minEnablements.set,
                                  moderateEnablements: cells[23].moderateEnablements.set,
                                  severeEnablements: cells[23].severeEnablements.set,
                
          },
        });
              functors[24]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./enablements.js")) {
                    const cell = cells[23][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[24].default.set,
                
          },
        });
              functors[25]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("../commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./internal-types.js")) {
                    const cell = cells[1][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./types.js")) {
                    const cell = cells[3][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeLoggingConsoleKit: cells[25].makeLoggingConsoleKit.set,
                                  makeCausalConsole: cells[25].makeCausalConsole.set,
                                  filterConsole: cells[25].filterConsole.set,
                                  consoleWhitelist: cells[25].consoleWhitelist.set,
                                  BASE_CONSOLE_LEVEL: cells[25].BASE_CONSOLE_LEVEL.set,
                
          },
        });
              functors[26]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("../commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./console.js")) {
                    const cell = cells[25][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./internal-types.js")) {
                    const cell = cells[1][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./types.js")) {
                    const cell = cells[3][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              tameConsole: cells[26].tameConsole.set,
                
          },
        });
              functors[27]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("../commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              filterFileName: cells[27].filterFileName.set,
                                  shortenCallSiteString: cells[27].shortenCallSiteString.set,
                                  tameV8ErrorConstructor: cells[27].tameV8ErrorConstructor.set,
                
          },
        });
              functors[28]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("../commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("../whitelist.js")) {
                    const cell = cells[14][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-v8-error-constructor.js")) {
                    const cell = cells[27][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[28].default.set,
                
          },
        });
              functors[29]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              makeHardener: cells[29].makeHardener.set,
                
          },
        });
              functors[30]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[30].default.set,
                
          },
        });
              functors[31]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              tameDomains: cells[31].tameDomains.set,
                
          },
        });
              functors[32]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[32].default.set,
                
          },
        });
              functors[33]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              tameFunctionToString: cells[33].tameFunctionToString.set,
                
          },
        });
              functors[34]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[34].default.set,
                
          },
        });
              functors[35]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[35].default.set,
                
          },
        });
              functors[36]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[36].default.set,
                
          },
        });
              functors[37]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./whitelist.js")) {
                    const cell = cells[14][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              default: cells[37].default.set,
                
          },
        });
              functors[38]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./enable-property-overrides.js")) {
                    const cell = cells[24][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/tame-console.js")) {
                    const cell = cells[26][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./error/tame-error-constructor.js")) {
                    const cell = cells[28][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./global-object.js")) {
                    const cell = cells[15][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./intrinsics.js")) {
                    const cell = cells[22][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./make-hardener.js")) {
                    const cell = cells[29][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-date-constructor.js")) {
                    const cell = cells[30][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-domains.js")) {
                    const cell = cells[31][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-function-constructors.js")) {
                    const cell = cells[32][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-function-tostring.js")) {
                    const cell = cells[33][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-locale-methods.js")) {
                    const cell = cells[34][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-math-object.js")) {
                    const cell = cells[35][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./tame-regexp-constructor.js")) {
                    const cell = cells[36][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./whitelist-intrinsics.js")) {
                    const cell = cells[37][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./whitelist.js")) {
                    const cell = cells[14][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
                              repairIntrinsics: cells[38].repairIntrinsics.set,
                                  makeLockdown: cells[38].makeLockdown.set,
                
          },
        });
              functors[39]({
          imports(entries) {
            const map = new Map(entries);
                              for (const [name, observers] of map.get("./src/commons.js")) {
                    const cell = cells[0][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./src/compartment-shim.js")) {
                    const cell = cells[20][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./src/error/assert.js")) {
                    const cell = cells[4][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./src/get-anonymous-intrinsics.js")) {
                    const cell = cells[21][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./src/intrinsics.js")) {
                    const cell = cells[22][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./src/lockdown-shim.js")) {
                    const cell = cells[38][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                                  for (const [name, observers] of map.get("./src/tame-function-tostring.js")) {
                    const cell = cells[33][name];
                    if (cell === undefined) {
                      throw new ReferenceError(`Cannot import name ${name}`);
                    }
                    for (const observer of observers) {
                      cell.observe(observer);
                    }
                  }
                
          },
          liveVar: {
            
          },
          onceVar: {
            
          },
        });
      

})([
  (({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   $h‍_imports([]);   /* global globalThis */
/* eslint-disable no-restricted-globals */

/**
 * commons.js
 * Declare shorthand functions. Sharing these declarations across modules
 * improves on consistency and minification. Unused declarations are
 * dropped by the tree shaking process.
 *
 * We capture these, not just for brevity, but for security. If any code
 * modifies Object to change what 'assign' points to, the Compartment shim
 * would be corrupted.
 */

// We cannot use globalThis as the local name since it would capture the
// lexical name.
const universalThis = globalThis;$h‍_once.universalThis(universalThis);


const {
  Array,
  Date,
  Float32Array,
  JSON,
  Map,
  Math,
  Object,
  Promise,
  Proxy,
  Reflect,
  RegExp: FERAL_REG_EXP,
  Set,
  String,
  WeakMap,
  WeakSet } =
globalThis;$h‍_once.Array(Array);$h‍_once.Date(Date);$h‍_once.Float32Array(Float32Array);$h‍_once.JSON(JSON);$h‍_once.Map(Map);$h‍_once.Math(Math);$h‍_once.Object(Object);$h‍_once.Promise(Promise);$h‍_once.Proxy(Proxy);$h‍_once.Reflect(Reflect);$h‍_once.FERAL_REG_EXP(FERAL_REG_EXP);$h‍_once.Set(Set);$h‍_once.String(String);$h‍_once.WeakMap(WeakMap);$h‍_once.WeakSet(WeakSet);

const {
  // The feral Error constructor is safe for internal use, but must not be
  // revealed to post-lockdown code in any compartment including the start
  // compartment since in V8 at least it bears stack inspection capabilities.
  Error: FERAL_ERROR,
  RangeError,
  ReferenceError,
  SyntaxError,
  TypeError } =
globalThis;$h‍_once.FERAL_ERROR(FERAL_ERROR);$h‍_once.RangeError(RangeError);$h‍_once.ReferenceError(ReferenceError);$h‍_once.SyntaxError(SyntaxError);$h‍_once.TypeError(TypeError);

const {
  assign,
  create,
  defineProperties,
  entries,
  freeze,
  getOwnPropertyDescriptor,
  getOwnPropertyDescriptors,
  getOwnPropertyNames,
  getPrototypeOf,
  is,
  isExtensible,
  keys,
  prototype: objectPrototype,
  seal,
  setPrototypeOf,
  values } =
Object;$h‍_once.assign(assign);$h‍_once.create(create);$h‍_once.defineProperties(defineProperties);$h‍_once.entries(entries);$h‍_once.freeze(freeze);$h‍_once.getOwnPropertyDescriptor(getOwnPropertyDescriptor);$h‍_once.getOwnPropertyDescriptors(getOwnPropertyDescriptors);$h‍_once.getOwnPropertyNames(getOwnPropertyNames);$h‍_once.getPrototypeOf(getPrototypeOf);$h‍_once.is(is);$h‍_once.isExtensible(isExtensible);$h‍_once.keys(keys);$h‍_once.objectPrototype(objectPrototype);$h‍_once.seal(seal);$h‍_once.setPrototypeOf(setPrototypeOf);$h‍_once.values(values);

const {
  species: speciesSymbol,
  toStringTag: toStringTagSymbol,
  iterator: iteratorSymbol,
  matchAll: matchAllSymbol } =
Symbol;$h‍_once.speciesSymbol(speciesSymbol);$h‍_once.toStringTagSymbol(toStringTagSymbol);$h‍_once.iteratorSymbol(iteratorSymbol);$h‍_once.matchAllSymbol(matchAllSymbol);

const { stringify: stringifyJson } = JSON;

// At time of this writing, we still support Node 10 which doesn't have
// `Object.fromEntries`. If it is absent, this should be an adequate
// replacement.
// By the terminology of https://ponyfoo.com/articles/polyfills-or-ponyfills
// it is a ponyfill rather than a polyfill or shim because we do not
// install it on `Object`.
$h‍_once.stringifyJson(stringifyJson);const objectFromEntries = (entryPairs) => {
  const result = {};
  for (const [prop, val] of entryPairs) {
    result[prop] = val;
  }
  return result;
};

const fromEntries = Object.fromEntries || objectFromEntries;

// Needed only for the Safari bug workaround below
$h‍_once.fromEntries(fromEntries);const { defineProperty: originalDefineProperty } = Object;

const defineProperty = (object, prop, descriptor) => {
  // We used to do the following, until we had to reopen Safari bug
  // https://bugs.webkit.org/show_bug.cgi?id=222538#c17
  // Once this is fixed, we may restore it.
  // // Object.defineProperty is allowed to fail silently so we use
  // // Object.defineProperties instead.
  // return defineProperties(object, { [prop]: descriptor });

  // Instead, to workaround the Safari bug
  const result = originalDefineProperty(object, prop, descriptor);
  if (result !== object) {
    throw TypeError(
    `Please report that the original defineProperty silently failed to set ${stringifyJson(
    String(prop))
    }. (SES_DEFINE_PROPERTY_FAILED_SILENTLY)`);

  }
  return result;
};$h‍_once.defineProperty(defineProperty);

const {
  apply,
  construct,
  get: reflectGet,
  getOwnPropertyDescriptor: reflectGetOwnPropertyDescriptor,
  has: reflectHas,
  isExtensible: reflectIsExtensible,
  ownKeys,
  preventExtensions: reflectPreventExtensions,
  set: reflectSet } =
Reflect;$h‍_once.apply(apply);$h‍_once.construct(construct);$h‍_once.reflectGet(reflectGet);$h‍_once.reflectGetOwnPropertyDescriptor(reflectGetOwnPropertyDescriptor);$h‍_once.reflectHas(reflectHas);$h‍_once.reflectIsExtensible(reflectIsExtensible);$h‍_once.ownKeys(ownKeys);$h‍_once.reflectPreventExtensions(reflectPreventExtensions);$h‍_once.reflectSet(reflectSet);

const { isArray, prototype: arrayPrototype } = Array;$h‍_once.isArray(isArray);$h‍_once.arrayPrototype(arrayPrototype);
const { prototype: mapPrototype } = Map;$h‍_once.mapPrototype(mapPrototype);
const { revocable: proxyRevocable } = Proxy;$h‍_once.proxyRevocable(proxyRevocable);
const { prototype: regexpPrototype } = RegExp;$h‍_once.regexpPrototype(regexpPrototype);
const { prototype: setPrototype } = Set;$h‍_once.setPrototype(setPrototype);
const { prototype: stringPrototype } = String;$h‍_once.stringPrototype(stringPrototype);
const { prototype: weakmapPrototype } = WeakMap;$h‍_once.weakmapPrototype(weakmapPrototype);
const { prototype: weaksetPrototype } = WeakSet;$h‍_once.weaksetPrototype(weaksetPrototype);
const { prototype: functionPrototype } = Function;$h‍_once.functionPrototype(functionPrototype);
const { prototype: promisePrototype } = Promise;

/**
 * uncurryThis()
 * This form of uncurry uses Reflect.apply()
 *
 * The original uncurry uses:
 * const bind = Function.prototype.bind;
 * const uncurryThis = bind.bind(bind.call);
 *
 * See those reference for a complete explanation:
 * http://wiki.ecmascript.org/doku.php?id=conventions:safe_meta_programming
 * which only lives at
 * http://web.archive.org/web/20160805225710/http://wiki.ecmascript.org/doku.php?id=conventions:safe_meta_programming
 *
 * @param {(thisArg: Object, ...args: any[]) => any} fn
 */$h‍_once.promisePrototype(promisePrototype);
const uncurryThis = (fn) => (thisArg, ...args) => apply(fn, thisArg, args);$h‍_once.uncurryThis(uncurryThis);

const objectHasOwnProperty = uncurryThis(objectPrototype.hasOwnProperty);
//
$h‍_once.objectHasOwnProperty(objectHasOwnProperty);const arrayFilter = uncurryThis(arrayPrototype.filter);$h‍_once.arrayFilter(arrayFilter);
const arrayForEach = uncurryThis(arrayPrototype.forEach);$h‍_once.arrayForEach(arrayForEach);
const arrayIncludes = uncurryThis(arrayPrototype.includes);$h‍_once.arrayIncludes(arrayIncludes);
const arrayJoin = uncurryThis(arrayPrototype.join);$h‍_once.arrayJoin(arrayJoin);
const arrayMap = uncurryThis(arrayPrototype.map);$h‍_once.arrayMap(arrayMap);
const arrayPop = uncurryThis(arrayPrototype.pop);$h‍_once.arrayPop(arrayPop);
const arrayPush = uncurryThis(arrayPrototype.push);$h‍_once.arrayPush(arrayPush);
const arraySlice = uncurryThis(arrayPrototype.slice);$h‍_once.arraySlice(arraySlice);
const arraySome = uncurryThis(arrayPrototype.some);$h‍_once.arraySome(arraySome);
const arraySort = uncurryThis(arrayPrototype.sort);$h‍_once.arraySort(arraySort);
const iterateArray = uncurryThis(arrayPrototype[iteratorSymbol]);
//
$h‍_once.iterateArray(iterateArray);const mapSet = uncurryThis(mapPrototype.set);$h‍_once.mapSet(mapSet);
const mapGet = uncurryThis(mapPrototype.get);$h‍_once.mapGet(mapGet);
const mapHas = uncurryThis(mapPrototype.has);$h‍_once.mapHas(mapHas);
const iterateMap = uncurryThis(mapPrototype[iteratorSymbol]);
//
$h‍_once.iterateMap(iterateMap);const setAdd = uncurryThis(setPrototype.add);$h‍_once.setAdd(setAdd);
const setForEach = uncurryThis(setPrototype.forEach);$h‍_once.setForEach(setForEach);
const setHas = uncurryThis(setPrototype.has);$h‍_once.setHas(setHas);
const iterateSet = uncurryThis(setPrototype[iteratorSymbol]);
//
$h‍_once.iterateSet(iterateSet);const regexpTest = uncurryThis(regexpPrototype.test);$h‍_once.regexpTest(regexpTest);
const regexpExec = uncurryThis(regexpPrototype.exec);$h‍_once.regexpExec(regexpExec);
const matchAllRegExp = uncurryThis(regexpPrototype[matchAllSymbol]);
//
$h‍_once.matchAllRegExp(matchAllRegExp);const stringEndsWith = uncurryThis(stringPrototype.endsWith);$h‍_once.stringEndsWith(stringEndsWith);
const stringIncludes = uncurryThis(stringPrototype.includes);$h‍_once.stringIncludes(stringIncludes);
const stringIndexOf = uncurryThis(stringPrototype.indexOf);$h‍_once.stringIndexOf(stringIndexOf);
const stringMatch = uncurryThis(stringPrototype.match);$h‍_once.stringMatch(stringMatch);
const stringReplace = uncurryThis(stringPrototype.replace);$h‍_once.stringReplace(stringReplace);
const stringSearch = uncurryThis(stringPrototype.search);$h‍_once.stringSearch(stringSearch);
const stringSlice = uncurryThis(stringPrototype.slice);$h‍_once.stringSlice(stringSlice);
const stringSplit = uncurryThis(stringPrototype.split);$h‍_once.stringSplit(stringSplit);
const stringStartsWith = uncurryThis(stringPrototype.startsWith);$h‍_once.stringStartsWith(stringStartsWith);
const iterateString = uncurryThis(stringPrototype[iteratorSymbol]);
//
$h‍_once.iterateString(iterateString);const weakmapDelete = uncurryThis(weakmapPrototype.delete);$h‍_once.weakmapDelete(weakmapDelete);
const weakmapGet = uncurryThis(weakmapPrototype.get);$h‍_once.weakmapGet(weakmapGet);
const weakmapHas = uncurryThis(weakmapPrototype.has);$h‍_once.weakmapHas(weakmapHas);
const weakmapSet = uncurryThis(weakmapPrototype.set);
//
$h‍_once.weakmapSet(weakmapSet);const weaksetAdd = uncurryThis(weaksetPrototype.add);$h‍_once.weaksetAdd(weaksetAdd);
const weaksetGet = uncurryThis(weaksetPrototype.get);$h‍_once.weaksetGet(weaksetGet);
const weaksetHas = uncurryThis(weaksetPrototype.has);
//
$h‍_once.weaksetHas(weaksetHas);const functionToString = uncurryThis(functionPrototype.toString);
//
$h‍_once.functionToString(functionToString);const { all } = Promise;
const promiseAll = (promises) => apply(all, Promise, [promises]);$h‍_once.promiseAll(promiseAll);
const promiseCatch = uncurryThis(promisePrototype.catch);$h‍_once.promiseCatch(promiseCatch);
const promiseThen = uncurryThis(promisePrototype.then);

/**
 * getConstructorOf()
 * Return the constructor from an instance.
 *
 * @param {Function} fn
 */$h‍_once.promiseThen(promiseThen);
const getConstructorOf = (fn) =>
reflectGet(getPrototypeOf(fn), 'constructor');

/**
 * immutableObject
 * An immutable (frozen) exotic object and is safe to share.
 */$h‍_once.getConstructorOf(getConstructorOf);
const immutableObject = freeze(create(null));

/**
 * isObject tests whether a value is an object.
 * Today, this is equivalent to:
 *
 *   const isObject = value => {
 *     if (value === null) return false;
 *     const type = typeof value;
 *     return type === 'object' || type === 'function';
 *   };
 *
 * But this is not safe in the face of possible evolution of the language, for
 * example new types or semantics of records and tuples.
 * We use this implementation despite the unnecessary allocation implied by
 * attempting to box a primitive.
 *
 * @param {any} value
 */$h‍_once.immutableObject(immutableObject);
const isObject = (value) => Object(value) === value;

/**
 * isError tests whether an object inherits from the intrinsic
 * `Error.prototype`.
 * We capture the original error constructor as FERAL_ERROR to provide a clear
 * signal for reviewers that we are handling an object with excess authority,
 * like stack trace inspection, that we are carefully hiding from client code.
 * Checking instanceof happens to be safe, but to avoid uttering FERAL_ERROR
 * for such a trivial case outside commons.js, we provide a utility function.
 *
 * @param {any} value
 */$h‍_once.isObject(isObject);
const isError = (value) => value instanceof FERAL_ERROR;

// The original unsafe untamed eval function, which must not escape.
// Sample at module initialization time, which is before lockdown can
// repair it.  Use it only to build powerless abstractions.
// eslint-disable-next-line no-eval
$h‍_once.isError(isError);const FERAL_EVAL = eval;

// The original unsafe untamed Function constructor, which must not escape.
// Sample at module initialization time, which is before lockdown can
// repair it.  Use it only to build powerless abstractions.
$h‍_once.FERAL_EVAL(FERAL_EVAL);const FERAL_FUNCTION = Function;$h‍_once.FERAL_FUNCTION(FERAL_FUNCTION);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   $h‍_imports([]);   // @ts-check

/**
 * @typedef {readonly any[]} LogArgs
 *
 * This is an array suitable to be used as arguments of a console
 * level message *after* the format string argument. It is the result of
 * a `details` template string and consists of alternating literal strings
 * and substitution values, starting with a literal string. At least that
 * first literal string is always present.
 */

/**
 * @callback NoteCallback
 *
 * @param {Error} error
 * @param {LogArgs} noteLogArgs
 * @returns {void}
 */

/**
 * @callback GetStackString
 * @param {Error} error
 * @returns {string=}
 */

/**
 * @typedef {Object} LoggedErrorHandler
 *
 * Used to parameterize `makeCausalConsole` to give it access to potentially
 * hidden information to augment the logging of errors.
 *
 * @property {GetStackString} getStackString
 * @property {(error: Error) => string} tagError
 * @property {() => void} resetErrorTagNum for debugging purposes only
 * @property {(error: Error) => (LogArgs | undefined)} getMessageLogArgs
 * @property {(error: Error) => (LogArgs | undefined)} takeMessageLogArgs
 * @property {(error: Error, callback?: NoteCallback) => LogArgs[] } takeNoteLogArgsArray
 */

// /////////////////////////////////////////////////////////////////////////////

/**
 * @typedef {readonly [string, ...any[]]} LogRecord
 */

/**
 * @typedef {Object} LoggingConsoleKit
 * @property {VirtualConsole} loggingConsole
 * @property {() => readonly LogRecord[]} takeLog
 */

/**
 * @typedef {Object} MakeLoggingConsoleKitOptions
 * @property {boolean=} shouldResetForDebugging
 */

/**
 * @callback MakeLoggingConsoleKit
 *
 * A logging console just accumulates the contents of all whitelisted calls,
 * making them available to callers of `takeLog()`. Calling `takeLog()`
 * consumes these, so later calls to `takeLog()` will only provide a log of
 * calls that have happened since then.
 *
 * @param {LoggedErrorHandler} loggedErrorHandler
 * @param {MakeLoggingConsoleKitOptions=} options
 * @returns {LoggingConsoleKit}
 */

/**
 * @typedef {{ NOTE: 'ERROR_NOTE:', MESSAGE: 'ERROR_MESSAGE:' }} ErrorInfo
 */

/**
 * @typedef {ErrorInfo[keyof ErrorInfo]} ErrorInfoKind
 */

/**
 * @callback MakeCausalConsole
 *
 * Makes a causal console wrapper of a `baseConsole`, where the causal console
 * calls methods of the `loggedErrorHandler` to customize how it handles logged
 * errors.
 *
 * @param {VirtualConsole} baseConsole
 * @param {LoggedErrorHandler} loggedErrorHandler
 * @returns {VirtualConsole}
 */
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Set,String,freeze,is,isError,setAdd,setHas,stringStartsWith,stringIncludes,stringifyJson,toStringTagSymbol;$h‍_imports([["../commons.js", [["Set", [$h‍_a => (Set = $h‍_a)]],["String", [$h‍_a => (String = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["is", [$h‍_a => (is = $h‍_a)]],["isError", [$h‍_a => (isError = $h‍_a)]],["setAdd", [$h‍_a => (setAdd = $h‍_a)]],["setHas", [$h‍_a => (setHas = $h‍_a)]],["stringStartsWith", [$h‍_a => (stringStartsWith = $h‍_a)]],["stringIncludes", [$h‍_a => (stringIncludes = $h‍_a)]],["stringifyJson", [$h‍_a => (stringifyJson = $h‍_a)]],["toStringTagSymbol", [$h‍_a => (toStringTagSymbol = $h‍_a)]]]]]);   















/**
 * Prepend the correct indefinite article onto a noun, typically a typeof
 * result, e.g., "an object" vs. "a number"
 *
 * @param {string} str The noun to prepend
 * @returns {string} The noun prepended with a/an
 */
const an = (str) => {
  str = `${str}`;
  if (str.length >= 1 && stringIncludes('aeiouAEIOU', str[0])) {
    return `an ${str}`;
  }
  return `a ${str}`;
};$h‍_once.an(an);
freeze(an);


/**
 * Like `JSON.stringify` but does not blow up if given a cycle or a bigint.
 * This is not
 * intended to be a serialization to support any useful unserialization,
 * or any programmatic use of the resulting string. The string is intended
 * *only* for showing a human under benign conditions, in order to be
 * informative enough for some
 * logging purposes. As such, this `bestEffortStringify` has an
 * imprecise specification and may change over time.
 *
 * The current `bestEffortStringify` possibly emits too many "seen"
 * markings: Not only for cycles, but also for repeated subtrees by
 * object identity.
 *
 * As a best effort only for diagnostic interpretation by humans,
 * `bestEffortStringify` also turns various cases that normal
 * `JSON.stringify` skips or errors on, like `undefined` or bigints,
 * into strings that convey their meaning. To distinguish this from
 * strings in the input, these synthesized strings always begin and
 * end with square brackets. To distinguish those strings from an
 * input string with square brackets, and input string that starts
 * with an open square bracket `[` is itself placed in square brackets.
 *
 * @param {any} payload
 * @param {(string|number)=} spaces
 * @returns {string}
 */
const bestEffortStringify = (payload, spaces = undefined) => {
  const seenSet = new Set();
  const replacer = (_, val) => {
    switch (typeof val) {
      case 'object':{
          if (val === null) {
            return null;
          }
          if (setHas(seenSet, val)) {
            return '[Seen]';
          }
          setAdd(seenSet, val);
          if (isError(val)) {
            return `[${val.name}: ${val.message}]`;
          }
          if (toStringTagSymbol in val) {
            // For the built-ins that have or inherit a `Symbol.toStringTag`-named
            // property, most of them inherit the default `toString` method,
            // which will print in a similar manner: `"[object Foo]"` vs
            // `"[Foo]"`. The exceptions are
            //    * `Symbol.prototype`, `BigInt.prototype`, `String.prototype`
            //      which don't matter to us since we handle primitives
            //      separately and we don't care about primitive wrapper objects.
            //    * TODO
            //      `Date.prototype`, `TypedArray.prototype`.
            //      Hmmm, we probably should make special cases for these. We're
            //      not using these yet, so it's not urgent. But others will run
            //      into these.
            //
            // Once #2018 is closed, the only objects in our code that have or
            // inherit a `Symbol.toStringTag`-named property are remotables
            // or their remote presences.
            // This printing will do a good job for these without
            // violating abstraction layering. This behavior makes sense
            // purely in terms of JavaScript concepts. That's some of the
            // motivation for choosing that representation of remotables
            // and their remote presences in the first place.
            return `[${val[toStringTagSymbol]}]`;
          }
          return val;
        }
      case 'function':{
          return `[Function ${val.name || '<anon>'}]`;
        }
      case 'string':{
          if (stringStartsWith(val, '[')) {
            return `[${val}]`;
          }
          return val;
        }
      case 'undefined':
      case 'symbol':{
          return `[${String(val)}]`;
        }
      case 'bigint':{
          return `[${val}n]`;
        }
      case 'number':{
          if (is(val, NaN)) {
            return '[NaN]';
          } else if (val === Infinity) {
            return '[Infinity]';
          } else if (val === -Infinity) {
            return '[-Infinity]';
          }
          return val;
        }
      default:{
          return val;
        }}

  };
  try {
    return stringifyJson(payload, replacer, spaces);
  } catch (_err) {
    // Don't do anything more fancy here if there is any
    // chance that might throw, unless you surround that
    // with another try-catch-recovery. For example,
    // the caught thing might be a proxy or other exotic
    // object rather than an error. The proxy might throw
    // whenever it is possible for it to.
    return '[Something that failed to stringify]';
  }
};$h‍_once.bestEffortStringify(bestEffortStringify);
freeze(bestEffortStringify);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   $h‍_imports([]);   // @ts-check

/**
 * @callback BaseAssert
 * The `assert` function itself.
 *
 * @param {*} flag The truthy/falsy value
 * @param {Details=} optDetails The details to throw
 * @param {ErrorConstructor=} ErrorConstructor An optional alternate error
 * constructor to use.
 * @returns {asserts flag}
 */

/**
 * @typedef {Object} AssertMakeErrorOptions
 * @property {string=} errorName
 */

/**
 * @callback AssertMakeError
 *
 * The `assert.error` method, recording details for the console.
 *
 * The optional `optDetails` can be a string.
 * @param {Details=} optDetails The details of what was asserted
 * @param {ErrorConstructor=} ErrorConstructor An optional alternate error
 * constructor to use.
 * @param {AssertMakeErrorOptions=} options
 * @returns {Error}
 */

/**
 * @callback AssertFail
 *
 * The `assert.fail` method.
 *
 * Fail an assertion, recording details to the console and
 * raising an exception with just type information.
 *
 * The optional `optDetails` can be a string for backwards compatibility
 * with the nodejs assertion library.
 * @param {Details=} optDetails The details of what was asserted
 * @param {ErrorConstructor=} ErrorConstructor An optional alternate error
 * constructor to use.
 * @returns {never}
 */

/**
 * @callback AssertEqual
 * The `assert.equal` method
 *
 * Assert that two values must be `Object.is`.
 * @param {*} actual The value we received
 * @param {*} expected What we wanted
 * @param {Details=} optDetails The details to throw
 * @param {ErrorConstructor=} ErrorConstructor An optional alternate error
 * constructor to use.
 * @returns {void}
 */

// Type all the overloads of the assertTypeof function.
// There may eventually be a better way to do this, but they break with
// Typescript 4.0.
/**
 * @callback AssertTypeofBigint
 * @param {any} specimen
 * @param {'bigint'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is bigint}
 *
 * @callback AssertTypeofBoolean
 * @param {any} specimen
 * @param {'boolean'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is boolean}
 *
 * @callback AssertTypeofFunction
 * @param {any} specimen
 * @param {'function'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is Function}
 *
 * @callback AssertTypeofNumber
 * @param {any} specimen
 * @param {'number'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is number}
 *
 * @callback AssertTypeofObject
 * @param {any} specimen
 * @param {'object'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is Record<any, any> | null}
 *
 * @callback AssertTypeofString
 * @param {any} specimen
 * @param {'string'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is string}
 *
 * @callback AssertTypeofSymbol
 * @param {any} specimen
 * @param {'symbol'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is symbol}
 *
 * @callback AssertTypeofUndefined
 * @param {any} specimen
 * @param {'undefined'} typename
 * @param {Details=} optDetails
 * @returns {asserts specimen is undefined}
 */

/**
 * The `assert.typeof` method
 *
 * @typedef {AssertTypeofBigint & AssertTypeofBoolean & AssertTypeofFunction & AssertTypeofNumber & AssertTypeofObject & AssertTypeofString & AssertTypeofSymbol & AssertTypeofUndefined} AssertTypeof
 */

/**
 * @callback AssertString
 * The `assert.string` method.
 *
 * `assert.string(v)` is equivalent to `assert.typeof(v, 'string')`. We
 * special case this one because it is the most frequently used.
 *
 * Assert an expected typeof result.
 * @param {any} specimen The value to get the typeof
 * @param {Details=} optDetails The details to throw
 */

/**
 * @callback AssertNote
 * The `assert.note` method.
 *
 * Annotate this error with these details, potentially to be used by an
 * augmented console, like the causal console of `console.js`, to
 * provide extra information associated with logged errors.
 *
 * @param {Error} error
 * @param {Details} detailsNote
 * @returns {void}
 */

// /////////////////////////////////////////////////////////////////////////////

/**
 * @typedef {{}} DetailsToken
 * A call to the `details` template literal makes and returns a fresh details
 * token, which is a frozen empty object associated with the arguments of that
 * `details` template literal expression.
 */

/**
 * @typedef {string | DetailsToken} Details
 * Either a plain string, or made by the `details` template literal tag.
 */

/**
 * @typedef {Object} StringablePayload
 * Holds the payload passed to quote so that its printed form is visible.
 * @property {() => string} toString How to print the payload
 */

/**
 * To "declassify" and quote a substitution value used in a
 * details`...` template literal, enclose that substitution expression
 * in a call to `quote`. This states that the argument should appear quoted
 * (as if with `JSON.stringify`), in the error message of the thrown error. The
 * payload itself is still passed unquoted to the console as it would be
 * without `quote`.
 *
 * Starting from the example in the `details` comment, say instead that the
 * color the sky is supposed to be is also computed. Say that we still don't
 * want to reveal the sky's actual color, but we do want the thrown error's
 * message to reveal what color the sky was supposed to be:
 * ```js
 * assert.equal(
 *   sky.color,
 *   color,
 *   details`${sky.color} should be ${quote(color)}`,
 * );
 * ```
 *
 * // TODO Update SES-shim to new convention, where `details` is
 * // renamed to `X` rather than `d`.
 * The normal convention is to locally rename `quote` to `q` and
 * `details` to `d`
 * ```js
 * const { details: d, quote: q } = assert;
 * ```
 * so the above example would then be
 * ```js
 * assert.equal(
 *   sky.color,
 *   color,
 *   d`${sky.color} should be ${q(color)}`,
 * );
 * ```
 *
 * @callback AssertQuote
 * @param {*} payload What to declassify
 * @param {(string|number)=} spaces
 * @returns {StringablePayload} The declassified payload
 */

/**
 * @callback Raise
 *
 * To make an `assert` which terminates some larger unit of computation
 * like a transaction, vat, or process, call `makeAssert` with a `Raise`
 * callback, where that callback actually performs that larger termination.
 * If possible, the callback should also report its `reason` parameter as
 * the alleged reason for the termination.
 *
 * @param {Error} reason
 */

/**
 * @callback MakeAssert
 *
 * Makes and returns an `assert` function object that shares the bookkeeping
 * state defined by this module with other `assert` function objects made by
 * `makeAssert`. This state is per-module-instance and is exposed by the
 * `loggedErrorHandler` above. We refer to `assert` as a "function object"
 * because it can be called directly as a function, but also has methods that
 * can be called.
 *
 * If `optRaise` is provided, the returned `assert` function object will call
 * `optRaise(reason)` before throwing the error. This enables `optRaise` to
 * engage in even more violent termination behavior, like terminating the vat,
 * that prevents execution from reaching the following throw. However, if
 * `optRaise` returns normally, which would be unusual, the throw following
 * `optRaise(reason)` would still happen.
 *
 * @param {Raise=} optRaise
 * @param {boolean=} unredacted
 * @returns {Assert}
 */

/**
 * @typedef {(template: TemplateStringsArray | string[], ...args: any) => DetailsToken} DetailsTag
 *
 * Use the `details` function as a template literal tag to create
 * informative error messages. The assertion functions take such messages
 * as optional arguments:
 * ```js
 * assert(sky.isBlue(), details`${sky.color} should be "blue"`);
 * ```
 * The details template tag returns a `DetailsToken` object that can print
 * itself with the formatted message in two ways.
 * It will report the real details to
 * the console but include only the typeof information in the thrown error
 * to prevent revealing secrets up the exceptional path. In the example
 * above, the thrown error may reveal only that `sky.color` is a string,
 * whereas the same diagnostic printed to the console reveals that the
 * sky was green.
 *
 * The `raw` member of a `template` is ignored, so a simple
 * `string[]` can also be used as a template.
 */

/**
 * assert that expr is truthy, with an optional details to describe
 * the assertion. It is a tagged template literal like
 * ```js
 * assert(expr, details`....`);`
 * ```
 *
 * The literal portions of the template are assumed non-sensitive, as
 * are the `typeof` types of the substitution values. These are
 * assembled into the thrown error message. The actual contents of the
 * substitution values are assumed sensitive, to be revealed to
 * the console only. We assume only the virtual platform's owner can read
 * what is written to the console, where the owner is in a privileged
 * position over computation running on that platform.
 *
 * The optional `optDetails` can be a string for backwards compatibility
 * with the nodejs assertion library.
 *
 * @typedef { BaseAssert & {
 *   typeof: AssertTypeof,
 *   error: AssertMakeError,
 *   fail: AssertFail,
 *   equal: AssertEqual,
 *   string: AssertString,
 *   note: AssertNote,
 *   details: DetailsTag,
 *   quote: AssertQuote,
 *   makeAssert: MakeAssert,
 * } } Assert
 */

// /////////////////////////////////////////////////////////////////////////////

/**
 * @typedef {Object} VirtualConsole
 * @property {Console['debug']} debug
 * @property {Console['log']} log
 * @property {Console['info']} info
 * @property {Console['warn']} warn
 * @property {Console['error']} error
 *
 * @property {Console['trace']} trace
 * @property {Console['dirxml']} dirxml
 * @property {Console['group']} group
 * @property {Console['groupCollapsed']} groupCollapsed
 *
 * @property {Console['assert']} assert
 * @property {Console['timeLog']} timeLog
 *
 * @property {Console['clear']} clear
 * @property {Console['count']} count
 * @property {Console['countReset']} countReset
 * @property {Console['dir']} dir
 * @property {Console['groupEnd']} groupEnd
 *
 * @property {Console['table']} table
 * @property {Console['time']} time
 * @property {Console['timeEnd']} timeEnd
 * @property {Console['timeStamp']} timeStamp
 */

/* This is deliberately *not* JSDoc, it is a regular comment.
 *
 * TODO: We'd like to add the following properties to the above
 * VirtualConsole, but they currently cause conflicts where
 * some Typescript implementations don't have these properties
 * on the Console type.
 *
 * @property {Console['profile']} profile
 * @property {Console['profileEnd']} profileEnd
 */

/**
 * @typedef {'debug' | 'log' | 'info' | 'warn' | 'error'} LogSeverity
 */

/**
 * @typedef ConsoleFilter
 * @property {(severity: LogSeverity) => boolean} canLog
 */

/**
 * @callback FilterConsole
 * @param {VirtualConsole} baseConsole
 * @param {ConsoleFilter} filter
 * @param {string=} topic
 * @returns {VirtualConsole}
 */
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let RangeError,TypeError,WeakMap,arrayJoin,arrayMap,arrayPop,arrayPush,assign,freeze,globalThis,is,isError,stringIndexOf,stringReplace,stringSlice,stringStartsWith,weakmapDelete,weakmapGet,weakmapHas,weakmapSet,an,bestEffortStringify;$h‍_imports([["../commons.js", [["RangeError", [$h‍_a => (RangeError = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["WeakMap", [$h‍_a => (WeakMap = $h‍_a)]],["arrayJoin", [$h‍_a => (arrayJoin = $h‍_a)]],["arrayMap", [$h‍_a => (arrayMap = $h‍_a)]],["arrayPop", [$h‍_a => (arrayPop = $h‍_a)]],["arrayPush", [$h‍_a => (arrayPush = $h‍_a)]],["assign", [$h‍_a => (assign = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["globalThis", [$h‍_a => (globalThis = $h‍_a)]],["is", [$h‍_a => (is = $h‍_a)]],["isError", [$h‍_a => (isError = $h‍_a)]],["stringIndexOf", [$h‍_a => (stringIndexOf = $h‍_a)]],["stringReplace", [$h‍_a => (stringReplace = $h‍_a)]],["stringSlice", [$h‍_a => (stringSlice = $h‍_a)]],["stringStartsWith", [$h‍_a => (stringStartsWith = $h‍_a)]],["weakmapDelete", [$h‍_a => (weakmapDelete = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]],["weakmapHas", [$h‍_a => (weakmapHas = $h‍_a)]],["weakmapSet", [$h‍_a => (weakmapSet = $h‍_a)]]]],["./stringify-utils.js", [["an", [$h‍_a => (an = $h‍_a)]],["bestEffortStringify", [$h‍_a => (bestEffortStringify = $h‍_a)]]]],["./types.js", []],["./internal-types.js", []]]);   






































// For our internal debugging purposes, uncomment
// const internalDebugConsole = console;

// /////////////////////////////////////////////////////////////////////////////

/** @type {WeakMap<StringablePayload, any>} */
const declassifiers = new WeakMap();

/** @type {AssertQuote} */
const quote = (payload, spaces = undefined) => {
  const result = freeze({
    toString: freeze(() => bestEffortStringify(payload, spaces)) });

  weakmapSet(declassifiers, result, payload);
  return result;
};
freeze(quote);

// /////////////////////////////////////////////////////////////////////////////

/**
 * @typedef {Object} HiddenDetails
 *
 * Captures the arguments passed to the `details` template string tag.
 *
 * @property {TemplateStringsArray | string[]} template
 * @property {any[]} args
 */

/**
 * @type {WeakMap<DetailsToken, HiddenDetails>}
 *
 * Maps from a details token which a `details` template literal returned
 * to a record of the contents of that template literal expression.
 */
const hiddenDetailsMap = new WeakMap();

/**
 * @param {HiddenDetails} hiddenDetails
 * @returns {string}
 */
const getMessageString = ({ template, args }) => {
  const parts = [template[0]];
  for (let i = 0; i < args.length; i += 1) {
    const arg = args[i];
    let argStr;
    if (weakmapHas(declassifiers, arg)) {
      argStr = `${arg}`;
    } else if (isError(arg)) {
      argStr = `(${an(arg.name)})`;
    } else {
      argStr = `(${an(typeof arg)})`;
    }
    arrayPush(parts, argStr, template[i + 1]);
  }
  return arrayJoin(parts, '');
};

/**
 * Give detailsTokens a toString behavior. To minimize the overhead of
 * creating new detailsTokens, we do this with an
 * inherited `this` sensitive `toString` method, even though we normally
 * avoid `this` sensitivity. To protect the method from inappropriate
 * `this` application, it does something interesting only for objects
 * registered in `redactedDetails`, which should be exactly the detailsTokens.
 *
 * The printing behavior must not reveal anything redacted, so we just use
 * the same `getMessageString` we use to construct the redacted message
 * string for a thrown assertion error.
 */
const DetailsTokenProto = freeze({
  toString() {
    const hiddenDetails = weakmapGet(hiddenDetailsMap, this);
    if (hiddenDetails === undefined) {
      return '[Not a DetailsToken]';
    }
    return getMessageString(hiddenDetails);
  } });

freeze(DetailsTokenProto.toString);

/**
 * Normally this is the function exported as `assert.details` and often
 * spelled `d`. However, if the `{errorTaming: 'unsafe'}` option is given to
 * `lockdown`, then `unredactedDetails` is used instead.
 *
 * There are some unconditional uses of `redactedDetails` in this module. All
 * of them should be uses where the template literal has no redacted
 * substitution values. In those cases, the two are equivalent.
 *
 * @type {DetailsTag}
 */
const redactedDetails = (template, ...args) => {
  // Keep in mind that the vast majority of calls to `details` creates
  // a details token that is never used, so this path must remain as fast as
  // possible. Hence we store what we've got with little processing, postponing
  // all the work to happen only if needed, for example, if an assertion fails.
  const detailsToken = freeze({ __proto__: DetailsTokenProto });
  weakmapSet(hiddenDetailsMap, detailsToken, { template, args });
  return detailsToken;
};
freeze(redactedDetails);

/**
 * `unredactedDetails` is like `details` except that it does not redact
 * anything. It acts like `details` would act if all substitution values
 * were wrapped with the `quote` function above (the function normally
 * spelled `q`). If the `{errorTaming: 'unsafe'}` option is given to
 * `lockdown`, then the lockdown-shim arranges for the global `assert` to be
 * one whose `details` property is `unredactedDetails`.
 * This setting optimizes the debugging and testing experience at the price
 * of safety. `unredactedDetails` also sacrifices the speed of `details`,
 * which is usually fine in debugging and testing.
 *
 * @type {DetailsTag}
 */
const unredactedDetails = (template, ...args) => {
  args = arrayMap(args, (arg) =>
  weakmapHas(declassifiers, arg) ? arg : quote(arg));

  return redactedDetails(template, ...args);
};$h‍_once.unredactedDetails(unredactedDetails);
freeze(unredactedDetails);


/**
 * @param {HiddenDetails} hiddenDetails
 * @returns {LogArgs}
 */
const getLogArgs = ({ template, args }) => {
  const logArgs = [template[0]];
  for (let i = 0; i < args.length; i += 1) {
    let arg = args[i];
    if (weakmapHas(declassifiers, arg)) {
      arg = weakmapGet(declassifiers, arg);
    }
    // Remove the extra spaces (since console.error puts them
    // between each cause).
    const priorWithoutSpace = stringReplace(arrayPop(logArgs) || '', / $/, '');
    if (priorWithoutSpace !== '') {
      arrayPush(logArgs, priorWithoutSpace);
    }
    const nextWithoutSpace = stringReplace(template[i + 1], /^ /, '');
    arrayPush(logArgs, arg, nextWithoutSpace);
  }
  if (logArgs[logArgs.length - 1] === '') {
    arrayPop(logArgs);
  }
  return logArgs;
};

/**
 * @type {WeakMap<Error, LogArgs>}
 *
 * Maps from an error object to the log args that are a more informative
 * alternative message for that error. When logging the error, these
 * log args should be preferred to `error.message`.
 */
const hiddenMessageLogArgs = new WeakMap();

// So each error tag will be unique.
let errorTagNum = 0;

/**
 * @type {WeakMap<Error, string>}
 */
const errorTags = new WeakMap();

/**
 * @param {Error} err
 * @param {string=} optErrorName
 * @returns {string}
 */
const tagError = (err, optErrorName = err.name) => {
  let errorTag = weakmapGet(errorTags, err);
  if (errorTag !== undefined) {
    return errorTag;
  }
  errorTagNum += 1;
  errorTag = `${optErrorName}#${errorTagNum}`;
  weakmapSet(errorTags, err, errorTag);
  return errorTag;
};

/**
 * @type {AssertMakeError}
 */
const makeError = (
optDetails = redactedDetails`Assert failed`,
ErrorConstructor = globalThis.Error,
{ errorName = undefined } = {}) =>
{
  if (typeof optDetails === 'string') {
    // If it is a string, use it as the literal part of the template so
    // it doesn't get quoted.
    optDetails = redactedDetails([optDetails]);
  }
  const hiddenDetails = weakmapGet(hiddenDetailsMap, optDetails);
  if (hiddenDetails === undefined) {
    throw new TypeError(`unrecognized details ${quote(optDetails)}`);
  }
  const messageString = getMessageString(hiddenDetails);
  const error = new ErrorConstructor(messageString);
  weakmapSet(hiddenMessageLogArgs, error, getLogArgs(hiddenDetails));
  if (errorName !== undefined) {
    tagError(error, errorName);
  }
  // The next line is a particularly fruitful place to put a breakpoint.
  return error;
};
freeze(makeError);

// /////////////////////////////////////////////////////////////////////////////

/**
 * @type {WeakMap<Error, LogArgs[]>}
 *
 * Maps from an error to an array of log args, where each log args is
 * remembered as an annotation on that error. This can be used, for example,
 * to keep track of additional causes of the error. The elements of any
 * log args may include errors which are associated with further annotations.
 * An augmented console, like the causal console of `console.js`, could
 * then retrieve the graph of such annotations.
 */
const hiddenNoteLogArgsArrays = new WeakMap();

/**
 * @type {WeakMap<Error, NoteCallback[]>}
 *
 * An augmented console will normally only take the hidden noteArgs array once,
 * when it logs the error being annotated. Once that happens, further
 * annotations of that error should go to the console immediately. We arrange
 * that by accepting a note-callback function from the console as an optional
 * part of that taking operation. Normally there will only be at most one
 * callback per error, but that depends on console behavior which we should not
 * assume. We make this an array of callbacks so multiple registrations
 * are independent.
 */
const hiddenNoteCallbackArrays = new WeakMap();

/** @type {AssertNote} */
const note = (error, detailsNote) => {
  if (typeof detailsNote === 'string') {
    // If it is a string, use it as the literal part of the template so
    // it doesn't get quoted.
    detailsNote = redactedDetails([detailsNote]);
  }
  const hiddenDetails = weakmapGet(hiddenDetailsMap, detailsNote);
  if (hiddenDetails === undefined) {
    throw new TypeError(`unrecognized details ${quote(detailsNote)}`);
  }
  const logArgs = getLogArgs(hiddenDetails);
  const callbacks = weakmapGet(hiddenNoteCallbackArrays, error);
  if (callbacks !== undefined) {
    for (const callback of callbacks) {
      callback(error, logArgs);
    }
  } else {
    const logArgsArray = weakmapGet(hiddenNoteLogArgsArrays, error);
    if (logArgsArray !== undefined) {
      arrayPush(logArgsArray, logArgs);
    } else {
      weakmapSet(hiddenNoteLogArgsArrays, error, [logArgs]);
    }
  }
};
freeze(note);

/**
 * The unprivileged form that just uses the de facto `error.stack` property.
 * The start compartment normally has a privileged `globalThis.getStackString`
 * which should be preferred if present.
 *
 * @param {Error} error
 * @returns {string}
 */
const defaultGetStackString = (error) => {
  if (!('stack' in error)) {
    return '';
  }
  const stackString = `${error.stack}`;
  const pos = stringIndexOf(stackString, '\n');
  if (stringStartsWith(stackString, ' ') || pos === -1) {
    return stackString;
  }
  return stringSlice(stackString, pos + 1); // exclude the initial newline
};

/** @type {LoggedErrorHandler} */
const loggedErrorHandler = {
  getStackString: globalThis.getStackString || defaultGetStackString,
  tagError: (error) => tagError(error),
  resetErrorTagNum: () => {
    errorTagNum = 0;
  },
  getMessageLogArgs: (error) => weakmapGet(hiddenMessageLogArgs, error),
  takeMessageLogArgs: (error) => {
    const result = weakmapGet(hiddenMessageLogArgs, error);
    weakmapDelete(hiddenMessageLogArgs, error);
    return result;
  },
  takeNoteLogArgsArray: (error, callback) => {
    const result = weakmapGet(hiddenNoteLogArgsArrays, error);
    weakmapDelete(hiddenNoteLogArgsArrays, error);
    if (callback !== undefined) {
      const callbacks = weakmapGet(hiddenNoteCallbackArrays, error);
      if (callbacks) {
        arrayPush(callbacks, callback);
      } else {
        weakmapSet(hiddenNoteCallbackArrays, error, [callback]);
      }
    }
    return result || [];
  } };$h‍_once.loggedErrorHandler(loggedErrorHandler);

freeze(loggedErrorHandler);


// /////////////////////////////////////////////////////////////////////////////

/**
 * @type {MakeAssert}
 */
const makeAssert = (optRaise = undefined, unredacted = false) => {
  const details = unredacted ? unredactedDetails : redactedDetails;
  /** @type {AssertFail} */
  const fail = (
  optDetails = details`Assert failed`,
  ErrorConstructor = globalThis.Error) =>
  {
    const reason = makeError(optDetails, ErrorConstructor);
    if (optRaise !== undefined) {
      optRaise(reason);
    }
    throw reason;
  };
  freeze(fail);

  // Don't freeze or export `baseAssert` until we add methods.
  // TODO If I change this from a `function` function to an arrow
  // function, I seem to get type errors from TypeScript. Why?
  /** @type {BaseAssert} */
  function baseAssert(
  flag,
  optDetails = details`Check failed`,
  ErrorConstructor = globalThis.Error)
  {
    if (!flag) {
      throw fail(optDetails, ErrorConstructor);
    }
  }

  /** @type {AssertEqual} */
  const equal = (
  actual,
  expected,
  optDetails = details`Expected ${actual} is same as ${expected}`,
  ErrorConstructor = RangeError) =>
  {
    baseAssert(is(actual, expected), optDetails, ErrorConstructor);
  };
  freeze(equal);

  /** @type {AssertTypeof} */
  const assertTypeof = (specimen, typename, optDetails) => {
    baseAssert(
    typeof typename === 'string',
    details`${quote(typename)} must be a string`);

    if (optDetails === undefined) {
      // Like
      // ```js
      // optDetails = details`${specimen} must be ${quote(an(typename))}`;
      // ```
      // except it puts the typename into the literal part of the template
      // so it doesn't get quoted.
      optDetails = details(['', ` must be ${an(typename)}`], specimen);
    }
    equal(typeof specimen, typename, optDetails, TypeError);
  };
  freeze(assertTypeof);

  /** @type {AssertString} */
  const assertString = (specimen, optDetails) =>
  assertTypeof(specimen, 'string', optDetails);

  // Note that "assert === baseAssert"
  /** @type {Assert} */
  const assert = assign(baseAssert, {
    error: makeError,
    fail,
    equal,
    typeof: assertTypeof,
    string: assertString,
    note,
    details,
    quote,
    makeAssert });

  return freeze(assert);
};$h‍_once.makeAssert(makeAssert);
freeze(makeAssert);


/** @type {Assert} */
const assert = makeAssert();$h‍_once.assert(assert);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_FUNCTION,arrayJoin;$h‍_imports([["./commons.js", [["FERAL_FUNCTION", [$h‍_a => (FERAL_FUNCTION = $h‍_a)]],["arrayJoin", [$h‍_a => (arrayJoin = $h‍_a)]]]]]);   

/**
 * buildOptimizer()
 * Given an array of indentifier, the optimizer return a `const` declaration
 * destructring `this`.
 *
 * @param {Array<string>} constants
 */
function buildOptimizer(constants) {
  // No need to build an optimizer when there are no constants.
  if (constants.length === 0) return '';
  // Use 'this' to avoid going through the scope proxy, which is unecessary
  // since the optimizer only needs references to the safe global.
  return `const {${arrayJoin(constants, ',')}} = this;`;
}

/**
 * makeEvaluateFactory()
 * The factory create 'evaluate' functions with the correct optimizer
 * inserted.
 *
 * @param {Array<string>} [constants]
 */
const makeEvaluateFactory = (constants = []) => {
  const optimizer = buildOptimizer(constants);

  // Create a function in sloppy mode, so that we can use 'with'. It returns
  // a function in strict mode that evaluates the provided code using direct
  // eval, and thus in strict mode in the same scope. We must be very careful
  // to not create new names in this scope

  // 1: we use 'with' (around a Proxy) to catch all free variable names. The
  // `this` value holds the Proxy which safely wraps the safeGlobal
  // 2: 'optimizer' catches constant variable names for speed.
  // 3: The inner strict function is effectively passed two parameters:
  //    a) its arguments[0] is the source to be directly evaluated.
  //    b) its 'this' is the this binding seen by the code being
  //       directly evaluated (the globalObject).
  // 4: The outer sloppy function is passed one parameter, the scope proxy.
  //    as the `this` parameter.

  // Notes:
  // - everything in the 'optimizer' string is looked up in the proxy
  //   (including an 'arguments[0]', which points at the Proxy).
  // - keywords like 'function' which are reserved keywords, and cannot be
  //   used as a variable, so they are not part of the optimizer.
  // - when 'eval' is looked up in the proxy, and it's the first time it is
  //   looked up after allowNextEvalToBeUnsafe is turned on, the proxy returns
  //   the powerful, unsafe eval intrinsic, and flips allowNextEvalToBeUnsafe
  //   back to false. Any reference to 'eval' in that string will get the tamed
  //   evaluator.

  // TODO https://github.com/endojs/endo/issues/816
  // The optimizer currently runs under sloppy mode, and although we doubt that
  // there is any vulnerability introduced just by running the optimizer
  // sloppy, we are much more confident in the semantics of strict mode.
  // The motivation for having the optimizer in sloppy mode is that it can be
  // reused for multiple evaluations, but in practice we have no such calls.
  // We could probably both move the optimizer into the inner function
  // and we could also simplify makeEvaluateFactory to simply evaluate.
  return FERAL_FUNCTION(`
    with (this) {
      ${optimizer}
      return function() {
        'use strict';
        return eval(arguments[0]);
      };
    }
  `);
};$h‍_once.makeEvaluateFactory(makeEvaluateFactory);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let arrayFilter,arrayIncludes,getOwnPropertyDescriptor,getOwnPropertyNames,objectHasOwnProperty,regexpTest;$h‍_imports([["./commons.js", [["arrayFilter", [$h‍_a => (arrayFilter = $h‍_a)]],["arrayIncludes", [$h‍_a => (arrayIncludes = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["getOwnPropertyNames", [$h‍_a => (getOwnPropertyNames = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["regexpTest", [$h‍_a => (regexpTest = $h‍_a)]]]]]);   








/**
 * keywords
 * In JavaScript you cannot use these reserved words as variables.
 * See 11.6.1 Identifier Names
 */
const keywords = [
// 11.6.2.1 Keywords
'await',
'break',
'case',
'catch',
'class',
'const',
'continue',
'debugger',
'default',
'delete',
'do',
'else',
'export',
'extends',
'finally',
'for',
'function',
'if',
'import',
'in',
'instanceof',
'new',
'return',
'super',
'switch',
'this',
'throw',
'try',
'typeof',
'var',
'void',
'while',
'with',
'yield',

// Also reserved when parsing strict mode code
'let',
'static',

// 11.6.2.2 Future Reserved Words
'enum',

// Also reserved when parsing strict mode code
'implements',
'package',
'protected',
'interface',
'private',
'public',

// Reserved but not mentioned in specs
'await',

'null',
'true',
'false',

'this',
'arguments'];


/**
 * identifierPattern
 * Simplified validation of indentifier names: may only contain alphanumeric
 * characters (or "$" or "_"), and may not start with a digit. This is safe
 * and does not reduces the compatibility of the shim. The motivation for
 * this limitation was to decrease the complexity of the implementation,
 * and to maintain a resonable level of performance.
 * Note: \w is equivalent [a-zA-Z_0-9]
 * See 11.6.1 Identifier Names
 */
const identifierPattern = /^[a-zA-Z_$][\w$]*$/;

/**
 * isValidIdentifierName()
 * What variable names might it bring into scope? These include all
 * property names which can be variable names, including the names
 * of inherited properties. It excludes symbols and names which are
 * keywords. We drop symbols safely. Currently, this shim refuses
 * service if any of the names are keywords or keyword-like. This is
 * safe and only prevent performance optimization.
 *
 * @param {string} name
 */
const isValidIdentifierName = (name) => {
  // Ensure we have a valid identifier. We use regexpTest rather than
  // /../.test() to guard against the case where RegExp has been poisoned.
  return (
    name !== 'eval' &&
    !arrayIncludes(keywords, name) &&
    regexpTest(identifierPattern, name));

};

/*
 * isImmutableDataProperty
 */$h‍_once.isValidIdentifierName(isValidIdentifierName);

function isImmutableDataProperty(obj, name) {
  const desc = getOwnPropertyDescriptor(obj, name);
  return (
    //
    // The getters will not have .writable, don't let the falsyness of
    // 'undefined' trick us: test with === false, not ! . However descriptors
    // inherit from the (potentially poisoned) global object, so we might see
    // extra properties which weren't really there. Accessor properties have
    // 'get/set/enumerable/configurable', while data properties have
    // 'value/writable/enumerable/configurable'.
    desc.configurable === false &&
    desc.writable === false &&
    //
    // Checks for data properties because they're the only ones we can
    // optimize (accessors are most likely non-constant). Descriptors can't
    // can't have accessors and value properties at the same time, therefore
    // this check is sufficient. Using explicit own property deal with the
    // case where Object.prototype has been poisoned.
    objectHasOwnProperty(desc, 'value'));

}

/**
 * getScopeConstants()
 * What variable names might it bring into scope? These include all
 * property names which can be variable names, including the names
 * of inherited properties. It excludes symbols and names which are
 * keywords. We drop symbols safely. Currently, this shim refuses
 * service if any of the names are keywords or keyword-like. This is
 * safe and only prevent performance optimization.
 *
 * @param {Object} globalObject
 * @param {Object} localObject
 */
const getScopeConstants = (globalObject, localObject = {}) => {
  // getOwnPropertyNames() does ignore Symbols so we don't need to
  // filter them out.
  const globalNames = getOwnPropertyNames(globalObject);
  const localNames = getOwnPropertyNames(localObject);

  // Collect all valid & immutable identifiers from the endowments.
  const localConstants = arrayFilter(
  localNames,
  (name) =>
  isValidIdentifierName(name) && isImmutableDataProperty(localObject, name));


  // Collect all valid & immutable identifiers from the global that
  // are also not present in the endwoments (immutable or not).
  const globalConstants = arrayFilter(
  globalNames,
  (name) =>
  // Can't define a constant: it would prevent a
  // lookup on the endowments.
  !arrayIncludes(localNames, name) &&
  isValidIdentifierName(name) &&
  isImmutableDataProperty(globalObject, name));


  return [...globalConstants, ...localConstants];
};$h‍_once.getScopeConstants(getScopeConstants);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_EVAL,Proxy,String,TypeError,create,freeze,getOwnPropertyDescriptor,getOwnPropertyDescriptors,globalThis,immutableObject,objectHasOwnProperty,reflectGet,reflectSet,assert;$h‍_imports([["./commons.js", [["FERAL_EVAL", [$h‍_a => (FERAL_EVAL = $h‍_a)]],["Proxy", [$h‍_a => (Proxy = $h‍_a)]],["String", [$h‍_a => (String = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["getOwnPropertyDescriptors", [$h‍_a => (getOwnPropertyDescriptors = $h‍_a)]],["globalThis", [$h‍_a => (globalThis = $h‍_a)]],["immutableObject", [$h‍_a => (immutableObject = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["reflectGet", [$h‍_a => (reflectGet = $h‍_a)]],["reflectSet", [$h‍_a => (reflectSet = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   
















const { details: d, quote: q } = assert;

/**
 * alwaysThrowHandler
 * This is an object that throws if any property is called. It's used as
 * a proxy handler which throws on any trap called.
 * It's made from a proxy with a get trap that throws. It's safe to
 * create one and share it between all scopeHandlers.
 */
const alwaysThrowHandler = new Proxy(
immutableObject,
freeze({
  get(_shadow, prop) {
    // eslint-disable-next-line @endo/no-polymorphic-call
    assert.fail(
    d`Please report unexpected scope handler trap: ${q(String(prop))}`);

  } }));



/*
 * createScopeHandler()
 * ScopeHandler manages a Proxy which serves as the global scope for the
 * performEval operation (the Proxy is the argument of a 'with' binding).
 * As described in createSafeEvaluator(), it has several functions:
 * - allow the very first (and only the very first) use of 'eval' to map to
 * the real (unsafe) eval function, so it acts as a 'direct eval' and can
 * access its lexical scope (which maps to the 'with' binding, which the
 * ScopeHandler also controls).
 * - ensure that all subsequent uses of 'eval' map to the safeEvaluator,
 * which lives as the 'eval' property of the safeGlobal.
 * - route all other property lookups at the safeGlobal.
 * - hide the unsafeGlobal which lives on the scope chain above the 'with'.
 * - ensure the Proxy invariants despite some global properties being frozen.
 */
const createScopeHandler = (
globalObject,
localObject = {},
{ sloppyGlobalsMode = false } = {}) =>
{
  // This flag allow us to determine if the eval() call is an done by the
  // compartment's code or if it is user-land invocation, so we can react
  // differently.
  let allowNextEvalToBeUnsafe = false;

  const admitOneUnsafeEvalNext = () => {
    allowNextEvalToBeUnsafe = true;
  };

  const resetOneUnsafeEvalNext = () => {
    const wasSet = allowNextEvalToBeUnsafe;
    allowNextEvalToBeUnsafe = false;
    return wasSet;
  };

  const scopeProxyHandlerProperties = {
    get(_shadow, prop) {
      if (typeof prop === 'symbol') {
        return undefined;
      }

      // Special treatment for eval. The very first lookup of 'eval' gets the
      // unsafe (real direct) eval, so it will get the lexical scope that uses
      // the 'with' context.
      if (prop === 'eval') {
        // test that it is true rather than merely truthy
        if (allowNextEvalToBeUnsafe === true) {
          // revoke before use
          allowNextEvalToBeUnsafe = false;
          return FERAL_EVAL;
        }
        // fall through
      }

      // Properties of the localObject.
      if (prop in localObject) {
        // Use reflect to defeat accessors that could be present on the
        // localObject object itself as `this`.
        // This is done out of an overabundance of caution, as the SES shim
        // only use the localObject carry globalLexicals and live binding
        // traps.
        // The globalLexicals are captured as a snapshot of what's passed to
        // the Compartment constructor, wherein all accessors and setters are
        // eliminated and the result frozen.
        // The live binding traps do use accessors, and none of those accessors
        // make use of their receiver.
        // Live binding traps provide no avenue for user code to observe the
        // receiver.
        return reflectGet(localObject, prop, globalObject);
      }

      // Properties of the global.
      return reflectGet(globalObject, prop);
    },

    set(_shadow, prop, value) {
      // Properties of the localObject.
      if (prop in localObject) {
        const desc = getOwnPropertyDescriptor(localObject, prop);
        if (objectHasOwnProperty(desc, 'value')) {
          // Work around a peculiar behavior in the specs, where
          // value properties are defined on the receiver.
          return reflectSet(localObject, prop, value);
        }
        // Ensure that the 'this' value on setters resolves
        // to the safeGlobal, not to the localObject object.
        return reflectSet(localObject, prop, value, globalObject);
      }

      // Properties of the global.
      return reflectSet(globalObject, prop, value);
    },

    // we need has() to return false for some names to prevent the lookup from
    // climbing the scope chain and eventually reaching the unsafeGlobal
    // object (globalThis), which is bad.

    // todo: we'd like to just have has() return true for everything, and then
    // use get() to raise a ReferenceError for anything not on the safe global.
    // But we want to be compatible with ReferenceError in the normal case and
    // the lack of ReferenceError in the 'typeof' case. Must either reliably
    // distinguish these two cases (the trap behavior might be different), or
    // we rely on a mandatory source-to-source transform to change 'typeof abc'
    // to XXX. We already need a mandatory parse to prevent the 'import',
    // since it's a special form instead of merely being a global variable/

    // note: if we make has() return true always, then we must implement a
    // set() trap to avoid subverting the protection of strict mode (it would
    // accept assignments to undefined globals, when it ought to throw
    // ReferenceError for such assignments)

    has(_shadow, prop) {
      // unsafeGlobal: hide all properties of the current global
      // at the expense of 'typeof' being wrong for those properties. For
      // example, in the browser, evaluating 'document = 3', will add
      // a property to globalObject instead of throwing a ReferenceError.
      return (
        sloppyGlobalsMode ||
        prop === 'eval' ||
        prop in localObject ||
        prop in globalObject ||
        prop in globalThis);

    },

    // note: this is likely a bug of safari
    // https://bugs.webkit.org/show_bug.cgi?id=195534

    getPrototypeOf() {
      return null;
    },

    // Chip has seen this happen single stepping under the Chrome/v8 debugger.
    // TODO record how to reliably reproduce, and to test if this fix helps.
    // TODO report as bug to v8 or Chrome, and record issue link here.

    getOwnPropertyDescriptor(_target, prop) {
      // Coerce with `String` in case prop is a symbol.
      const quotedProp = q(String(prop));
      // eslint-disable-next-line @endo/no-polymorphic-call
      console.warn(
      `getOwnPropertyDescriptor trap on scopeHandler for ${quotedProp}`,
      new TypeError().stack);

      return undefined;
    } };


  // The scope handler's prototype is a proxy that throws if any trap other
  // than get/set/has are run (like getOwnPropertyDescriptors, apply,
  // getPrototypeOf).
  const scopeHandler = freeze(
  create(
  alwaysThrowHandler,
  getOwnPropertyDescriptors(scopeProxyHandlerProperties)));



  return {
    admitOneUnsafeEvalNext,
    resetOneUnsafeEvalNext,
    scopeHandler };

};$h‍_once.createScopeHandler(createScopeHandler);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_REG_EXP,regexpExec,stringSlice;$h‍_imports([["./commons.js", [["FERAL_REG_EXP", [$h‍_a => (FERAL_REG_EXP = $h‍_a)]],["regexpExec", [$h‍_a => (regexpExec = $h‍_a)]],["stringSlice", [$h‍_a => (stringSlice = $h‍_a)]]]]]);   

// Captures a key and value of the form #key=value or @key=value
const sourceMetaEntryRegExp =
'\\s*[@#]\\s*([a-zA-Z][a-zA-Z0-9]*)\\s*=\\s*([^\\s\\*]*)';
// Captures either a one-line or multi-line comment containing
// one #key=value or @key=value.
// Produces two pairs of capture groups, but the initial two may be undefined.
// On account of the mechanics of regular expressions, scanning from the end
// does not allow us to capture every pair, so getSourceURL must capture and
// trim until there are no matching comments.
const sourceMetaEntriesRegExp = new FERAL_REG_EXP(
`(?:\\s*//${sourceMetaEntryRegExp}|/\\*${sourceMetaEntryRegExp}\\s*\\*/)\\s*$`);


const getSourceURL = (src) => {
  let sourceURL = '<unknown>';

  // Our regular expression matches the last one or two comments with key value
  // pairs at the end of the source, avoiding a scan over the entire length of
  // the string, but at the expense of being able to capture all the (key,
  // value) pair meta comments at the end of the source, which may include
  // sourceMapURL in addition to sourceURL.
  // So, we sublimate the comments out of the source until no source or no
  // comments remain.
  while (src.length > 0) {
    const match = regexpExec(sourceMetaEntriesRegExp, src);
    if (match === null) {
      break;
    }
    src = stringSlice(src, 0, src.length - match[0].length);

    // We skip $0 since it contains the entire match.
    // The match contains four capture groups,
    // two (key, value) pairs, the first of which
    // may be undefined.
    // On the off-chance someone put two sourceURL comments in their code with
    // different commenting conventions, the latter has precedence.
    if (match[3] === 'sourceURL') {
      sourceURL = match[4];
    } else if (match[1] === 'sourceURL') {
      sourceURL = match[2];
    }
  }

  return sourceURL;
};$h‍_once.getSourceURL(getSourceURL);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_REG_EXP,SyntaxError,stringReplace,stringSearch,stringSlice,stringSplit,getSourceURL;$h‍_imports([["./commons.js", [["FERAL_REG_EXP", [$h‍_a => (FERAL_REG_EXP = $h‍_a)]],["SyntaxError", [$h‍_a => (SyntaxError = $h‍_a)]],["stringReplace", [$h‍_a => (stringReplace = $h‍_a)]],["stringSearch", [$h‍_a => (stringSearch = $h‍_a)]],["stringSlice", [$h‍_a => (stringSlice = $h‍_a)]],["stringSplit", [$h‍_a => (stringSplit = $h‍_a)]]]],["./get-source-url.js", [["getSourceURL", [$h‍_a => (getSourceURL = $h‍_a)]]]]]);   











/**
 * Find the first occurence of the given pattern and return
 * the location as the approximate line number.
 *
 * @param {string} src
 * @param {RegExp} pattern
 * @returns {number}
 */
function getLineNumber(src, pattern) {
  const index = stringSearch(src, pattern);
  if (index < 0) {
    return -1;
  }

  // The importPattern incidentally captures an initial \n in
  // an attempt to reject a . prefix, so we need to offset
  // the line number in that case.
  const adjustment = src[index] === '\n' ? 1 : 0;

  return stringSplit(stringSlice(src, 0, index), '\n').length + adjustment;
}

// /////////////////////////////////////////////////////////////////////////////

const htmlCommentPattern = new FERAL_REG_EXP(`(?:${'<'}!--|--${'>'})`, 'g');

/**
 * Conservatively reject the source text if it may contain text that some
 * JavaScript parsers may treat as an html-like comment. To reject without
 * parsing, `rejectHtmlComments` will also reject some other text as well.
 *
 * https://www.ecma-international.org/ecma-262/9.0/index.html#sec-html-like-comments
 * explains that JavaScript parsers may or may not recognize html
 * comment tokens "<" immediately followed by "!--" and "--"
 * immediately followed by ">" in non-module source text, and treat
 * them as a kind of line comment. Since otherwise both of these can
 * appear in normal JavaScript source code as a sequence of operators,
 * we have the terrifying possibility of the same source code parsing
 * one way on one correct JavaScript implementation, and another way
 * on another.
 *
 * This shim takes the conservative strategy of just rejecting source
 * text that contains these strings anywhere. Note that this very
 * source file is written strangely to avoid mentioning these
 * character strings explicitly.
 *
 * We do not write the regexp in a straightforward way, so that an
 * apparennt html comment does not appear in this file. Thus, we avoid
 * rejection by the overly eager rejectDangerousSources.
 *
 * @param {string} src
 * @returns {string}
 */
const rejectHtmlComments = (src) => {
  const lineNumber = getLineNumber(src, htmlCommentPattern);
  if (lineNumber < 0) {
    return src;
  }
  const name = getSourceURL(src);
  throw new SyntaxError(
  `Possible HTML comment rejected at ${name}:${lineNumber}. (SES_HTML_COMMENT_REJECTED)`);

};

/**
 * An optional transform to place ahead of `rejectHtmlComments` to evade *that*
 * rejection. However, it may change the meaning of the program.
 *
 * This evasion replaces each alleged html comment with the space-separated
 * JavaScript operator sequence that it may mean, assuming that it appears
 * outside of a comment or literal string, in source code where the JS
 * parser makes no special case for html comments (like module source code).
 * In that case, this evasion preserves the meaning of the program, though it
 * does change the souce column numbers on each effected line.
 *
 * If the html comment appeared in a literal (a string literal, regexp literal,
 * or a template literal), then this evasion will change the meaning of the
 * program by changing the text of that literal.
 *
 * If the html comment appeared in a JavaScript comment, then this evasion does
 * not change the meaning of the program because it only changes the contents of
 * those comments.
 *
 * @param { string } src
 * @returns { string }
 */$h‍_once.rejectHtmlComments(rejectHtmlComments);
const evadeHtmlCommentTest = (src) => {
  const replaceFn = (match) => match[0] === '<' ? '< ! --' : '-- >';
  return stringReplace(src, htmlCommentPattern, replaceFn);
};

// /////////////////////////////////////////////////////////////////////////////
$h‍_once.evadeHtmlCommentTest(evadeHtmlCommentTest);
const importPattern = new FERAL_REG_EXP(
'(^|[^.])\\bimport(\\s*(?:\\(|/[/*]))',
'g');


/**
 * Conservatively reject the source text if it may contain a dynamic
 * import expression. To reject without parsing, `rejectImportExpressions` will
 * also reject some other text as well.
 *
 * The proposed dynamic import expression is the only syntax currently
 * proposed, that can appear in non-module JavaScript code, that
 * enables direct access to the outside world that cannot be
 * suppressed or intercepted without parsing and rewriting. Instead,
 * this shim conservatively rejects any source text that seems to
 * contain such an expression. To do this safely without parsing, we
 * must also reject some valid programs, i.e., those containing
 * apparent import expressions in literal strings or comments.
 *
 * The current conservative rule looks for the identifier "import"
 * followed by either an open paren or something that looks like the
 * beginning of a comment. We assume that we do not need to worry
 * about html comment syntax because that was already rejected by
 * rejectHtmlComments.
 *
 * this \s *must* match all kinds of syntax-defined whitespace. If e.g.
 * U+2028 (LINE SEPARATOR) or U+2029 (PARAGRAPH SEPARATOR) is treated as
 * whitespace by the parser, but not matched by /\s/, then this would admit
 * an attack like: import\u2028('power.js') . We're trying to distinguish
 * something like that from something like importnotreally('power.js') which
 * is perfectly safe.
 *
 * @param { string } src
 * @returns { string }
 */
const rejectImportExpressions = (src) => {
  const lineNumber = getLineNumber(src, importPattern);
  if (lineNumber < 0) {
    return src;
  }
  const name = getSourceURL(src);
  throw new SyntaxError(
  `Possible import expression rejected at ${name}:${lineNumber}. (SES_IMPORT_REJECTED)`);

};

/**
 * An optional transform to place ahead of `rejectImportExpressions` to evade
 * *that* rejection. However, it may change the meaning of the program.
 *
 * This evasion replaces each suspicious `import` identifier with `__import__`.
 * If the alleged import expression appears in a JavaScript comment, this
 * evasion will not change the meaning of the program. If it appears in a
 * literal (string literal, regexp literal, or a template literal), then this
 * evasion will change the contents of that literal. If it appears as code
 * where it would be parsed as an expression, then it might or might not change
 * the meaning of the program, depending on the binding, if any, of the lexical
 * variable `__import__`.
 *
 * @param { string } src
 * @returns { string }
 */$h‍_once.rejectImportExpressions(rejectImportExpressions);
const evadeImportExpressionTest = (src) => {
  const replaceFn = (_, p1, p2) => `${p1}__import__${p2}`;
  return stringReplace(src, importPattern, replaceFn);
};

// /////////////////////////////////////////////////////////////////////////////
$h‍_once.evadeImportExpressionTest(evadeImportExpressionTest);
const someDirectEvalPattern = new FERAL_REG_EXP(
'(^|[^.])\\beval(\\s*\\()',
'g');


/**
 * Heuristically reject some text that seems to contain a direct eval
 * expression, with both false positives and false negavives. To reject without
 * parsing, `rejectSomeDirectEvalExpressions` may will also reject some other
 * text as well. It may also accept source text that contains a direct eval
 * written oddly, such as `(eval)(src)`. This false negative is not a security
 * vulnerability. Rather it is a compat hazard because it will execute as
 * an indirect eval under the SES-shim but as a direct eval on platforms that
 * support SES directly (like XS).
 *
 * The shim cannot correctly emulate a direct eval as explained at
 * https://github.com/Agoric/realms-shim/issues/12
 * If we did not reject direct eval syntax, we would
 * accidentally evaluate these with an emulation of indirect eval. To
 * prevent future compatibility problems, in shifting from use of the
 * shim to genuine platform support for the proposal, we should
 * instead statically reject code that seems to contain a direct eval
 * expression.
 *
 * As with the dynamic import expression, to avoid a full parse, we do
 * this approximately with a regexp, that will also reject strings
 * that appear safely in comments or strings. Unlike dynamic import,
 * if we miss some, this only creates future compat problems, not
 * security problems. Thus, we are only trying to catch innocent
 * occurrences, not malicious one. In particular, `(eval)(...)` is
 * direct eval syntax that would not be caught by the following regexp.
 *
 * Exported for unit tests.
 *
 * @param { string } src
 * @returns { string }
 */
const rejectSomeDirectEvalExpressions = (src) => {
  const lineNumber = getLineNumber(src, someDirectEvalPattern);
  if (lineNumber < 0) {
    return src;
  }
  const name = getSourceURL(src);
  throw new SyntaxError(
  `Possible direct eval expression rejected at ${name}:${lineNumber}. (SES_EVAL_REJECTED)`);

};

// /////////////////////////////////////////////////////////////////////////////

/**
 * A transform that bundles together the transforms that must unconditionally
 * happen last in order to ensure safe evaluation without parsing.
 *
 * @param {string} source
 * @returns {string}
 */$h‍_once.rejectSomeDirectEvalExpressions(rejectSomeDirectEvalExpressions);
const mandatoryTransforms = (source) => {
  source = rejectHtmlComments(source);
  source = rejectImportExpressions(source);
  return source;
};

/**
 * Starting with `source`, apply each transform to the result of the
 * previous one, returning the result of the last transformation.
 *
 * @param {string} source
 * @param {((str: string) => string)[]} transforms
 * @returns {string}
 */$h‍_once.mandatoryTransforms(mandatoryTransforms);
const applyTransforms = (source, transforms) => {
  for (const transform of transforms) {
    source = transform(source);
  }
  return source;
};$h‍_once.applyTransforms(applyTransforms);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let WeakSet,apply,immutableObject,proxyRevocable,weaksetAdd,getScopeConstants,createScopeHandler,applyTransforms,mandatoryTransforms,makeEvaluateFactory,assert;$h‍_imports([["./commons.js", [["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["apply", [$h‍_a => (apply = $h‍_a)]],["immutableObject", [$h‍_a => (immutableObject = $h‍_a)]],["proxyRevocable", [$h‍_a => (proxyRevocable = $h‍_a)]],["weaksetAdd", [$h‍_a => (weaksetAdd = $h‍_a)]]]],["./scope-constants.js", [["getScopeConstants", [$h‍_a => (getScopeConstants = $h‍_a)]]]],["./scope-handler.js", [["createScopeHandler", [$h‍_a => (createScopeHandler = $h‍_a)]]]],["./transforms.js", [["applyTransforms", [$h‍_a => (applyTransforms = $h‍_a)]],["mandatoryTransforms", [$h‍_a => (mandatoryTransforms = $h‍_a)]]]],["./make-evaluate-factory.js", [["makeEvaluateFactory", [$h‍_a => (makeEvaluateFactory = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   















const { details: d } = assert;

/**
 * prepareEval()
 *
 * @param {Object} globalObject
 * @param {Objeect} localObject
 * @param {Object} [options]
 * @param {bool} [options.sloppyGlobalsMode]
 * @param {WeakSet} [options.knownScopeProxies]
 */
const prepareEval = (
globalObject,
localObject = {},
{ sloppyGlobalsMode = false, knownScopeProxies = new WeakSet() } = {}) =>
{
  const {
    scopeHandler,
    admitOneUnsafeEvalNext,
    resetOneUnsafeEvalNext } =
  createScopeHandler(globalObject, localObject, {
    sloppyGlobalsMode });

  const { proxy: scopeProxy, revoke: revokeScopeProxy } = proxyRevocable(
  immutableObject,
  scopeHandler);

  weaksetAdd(knownScopeProxies, scopeProxy);

  return {
    scopeProxy,
    revokeScopeProxy,
    admitOneUnsafeEvalNext,
    resetOneUnsafeEvalNext };

};

/**
 * performEval()
 * The low-level operation used by all evaluators:
 * eval(), Function(), Evalutator.prototype.evaluate().
 *
 * @param {string} source
 * @param {Object} globalObject
 * @param {Objeect} localObject
 * @param {Object} [options]
 * @param {Array<Transform>} [options.localTransforms]
 * @param {Array<Transform>} [options.globalTransforms]
 * @param {bool} [options.sloppyGlobalsMode]
 * @param {WeakSet} [options.knownScopeProxies]
 */$h‍_once.prepareEval(prepareEval);
const performEval = (
source,
globalObject,
localObject = {},
{
  localTransforms = [],
  globalTransforms = [],
  sloppyGlobalsMode = false,
  knownScopeProxies = new WeakSet() } =
{}) =>
{
  // Execute the mandatory transforms last to ensure that any rewritten code
  // meets those mandatory requirements.
  source = applyTransforms(source, [
  ...localTransforms,
  ...globalTransforms,
  mandatoryTransforms]);


  const {
    scopeProxy,
    revokeScopeProxy,
    admitOneUnsafeEvalNext,
    resetOneUnsafeEvalNext } =
  prepareEval(globalObject, localObject, {
    sloppyGlobalsMode,
    knownScopeProxies });


  const constants = getScopeConstants(globalObject, localObject);
  const evaluateFactory = makeEvaluateFactory(constants);
  const evaluate = apply(evaluateFactory, scopeProxy, []);

  admitOneUnsafeEvalNext();
  let err;
  try {
    // Ensure that "this" resolves to the safe global.
    return apply(evaluate, globalObject, [source]);
  } catch (e) {
    // stash the child-code error in hopes of debugging the internal failure
    err = e;
    throw e;
  } finally {
    if (resetOneUnsafeEvalNext()) {
      // Barring a defect in the SES shim, the scope proxy should allow the
      // powerful, unsafe  `eval` to be used by `evaluate` exactly once, as the
      // very first name that it attempts to access from the lexical scope.
      // A defect in the SES shim could throw an exception after our call to
      // `admitOneUnsafeEvalNext()` and before `evaluate` calls `eval`
      // internally.
      // If we get here, SES is very broken.
      // This condition is one where this vat is now hopelessly confused, and
      // the vat as a whole should be aborted.
      // No further code should run.
      // All immediately reachable state should be abandoned.
      // However, that is not yet possible, so we at least prevent further
      // variable resolution via the scopeHandler, and throw an error with
      // diagnostic info including the thrown error if any from evaluating the
      // source code.
      revokeScopeProxy();
      // TODO A GOOD PLACE TO PANIC(), i.e., kill the vat incarnation.
      // See https://github.com/Agoric/SES-shim/issues/490
      // eslint-disable-next-line @endo/no-polymorphic-call
      assert.fail(d`handler did not reset allowNextEvalToBeUnsafe ${err}`);
    }
  }
};$h‍_once.performEval(performEval);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let TypeError,arrayPush,create,defineProperties,getOwnPropertyDescriptors,evadeHtmlCommentTest,evadeImportExpressionTest,rejectSomeDirectEvalExpressions,prepareEval,performEval;$h‍_imports([["./commons.js", [["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["arrayPush", [$h‍_a => (arrayPush = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["getOwnPropertyDescriptors", [$h‍_a => (getOwnPropertyDescriptors = $h‍_a)]]]],["./transforms.js", [["evadeHtmlCommentTest", [$h‍_a => (evadeHtmlCommentTest = $h‍_a)]],["evadeImportExpressionTest", [$h‍_a => (evadeImportExpressionTest = $h‍_a)]],["rejectSomeDirectEvalExpressions", [$h‍_a => (rejectSomeDirectEvalExpressions = $h‍_a)]]]],["./evaluate.js", [["prepareEval", [$h‍_a => (prepareEval = $h‍_a)]],["performEval", [$h‍_a => (performEval = $h‍_a)]]]]]);   














const prepareCompartmentEvaluation = (compartmentFields, options) => {
  // Extract options, and shallow-clone transforms.
  const {
    transforms = [],
    sloppyGlobalsMode = false,
    __moduleShimLexicals__ = undefined,
    __evadeHtmlCommentTest__ = false,
    __evadeImportExpressionTest__ = false,
    __rejectSomeDirectEvalExpressions__ = true // Note default on
  } = options;
  const localTransforms = [...transforms];
  if (__evadeHtmlCommentTest__ === true) {
    arrayPush(localTransforms, evadeHtmlCommentTest);
  }
  if (__evadeImportExpressionTest__ === true) {
    arrayPush(localTransforms, evadeImportExpressionTest);
  }
  if (__rejectSomeDirectEvalExpressions__ === true) {
    arrayPush(localTransforms, rejectSomeDirectEvalExpressions);
  }
  let { globalTransforms } = compartmentFields;
  const { globalObject, globalLexicals, knownScopeProxies } = compartmentFields;
  let localObject = globalLexicals;
  if (__moduleShimLexicals__ !== undefined) {
    // When using `evaluate` for ESM modules, as should only occur from the
    // module-shim's module-instance.js, we do not reveal the SES-shim's
    // module-to-program translation, as this is not standardizable behavior.
    // However, the `localTransforms` will come from the `__shimTransforms__`
    // Compartment option in this case, which is a non-standardizable escape
    // hatch so programs designed specifically for the SES-shim
    // implementation may opt-in to use the same transforms for `evaluate`
    // and `import`, at the expense of being tightly coupled to SES-shim.
    globalTransforms = undefined;
    localObject = create(null, getOwnPropertyDescriptors(globalLexicals));
    defineProperties(
    localObject,
    getOwnPropertyDescriptors(__moduleShimLexicals__));

  }

  return {
    globalObject,
    localObject,
    globalTransforms,
    localTransforms,
    sloppyGlobalsMode,
    knownScopeProxies };

};$h‍_once.prepareCompartmentEvaluation(prepareCompartmentEvaluation);

const compartmentEvaluate = (compartmentFields, source, options) => {
  // Perform this check first to avoid unecessary sanitizing.
  // TODO Maybe relax string check and coerce instead:
  // https://github.com/tc39/proposal-dynamic-code-brand-checks
  if (typeof source !== 'string') {
    throw new TypeError('first argument of evaluate() must be a string');
  }

  const {
    globalObject,
    localObject,
    globalTransforms,
    localTransforms,
    sloppyGlobalsMode,
    knownScopeProxies } =
  prepareCompartmentEvaluation(compartmentFields, options);

  return performEval(source, globalObject, localObject, {
    globalTransforms,
    localTransforms,
    sloppyGlobalsMode,
    knownScopeProxies });

};$h‍_once.compartmentEvaluate(compartmentEvaluate);

const makeScopeProxy = (compartmentFields, options) => {
  const {
    globalObject,
    localObject,
    sloppyGlobalsMode,
    knownScopeProxies } =
  prepareCompartmentEvaluation(compartmentFields, options);
  const { scopeProxy } = prepareEval(globalObject, localObject, {
    sloppyGlobalsMode,
    knownScopeProxies });

  return scopeProxy;
};$h‍_once.makeScopeProxy(makeScopeProxy);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let performEval;$h‍_imports([["./evaluate.js", [["performEval", [$h‍_a => (performEval = $h‍_a)]]]]]);   

/*
 * makeEvalFunction()
 * A safe version of the native eval function which relies on
 * the safety of performEval for confinement.
 */
const makeEvalFunction = (globalObject, options = {}) => {
  // We use the the concise method syntax to create an eval without a
  // [[Construct]] behavior (such that the invocation "new eval()" throws
  // TypeError: eval is not a constructor"), but which still accepts a
  // 'this' binding.
  const newEval = {
    eval(source) {
      if (typeof source !== 'string') {
        // As per the runtime semantic of PerformEval [ECMAScript 18.2.1.1]:
        // If Type(source) is not String, return source.
        // TODO Recent proposals from Mike Samuel may change this non-string
        // rule. Track.
        return source;
      }
      return performEval(source, globalObject, {}, options);
    } }.
  eval;

  return newEval;
};$h‍_once.makeEvalFunction(makeEvalFunction);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_FUNCTION,arrayJoin,arrayPop,defineProperties,getPrototypeOf,performEval,assert;$h‍_imports([["./commons.js", [["FERAL_FUNCTION", [$h‍_a => (FERAL_FUNCTION = $h‍_a)]],["arrayJoin", [$h‍_a => (arrayJoin = $h‍_a)]],["arrayPop", [$h‍_a => (arrayPop = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["getPrototypeOf", [$h‍_a => (getPrototypeOf = $h‍_a)]]]],["./evaluate.js", [["performEval", [$h‍_a => (performEval = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   









/*
 * makeFunctionConstructor()
 * A safe version of the native Function which relies on
 * the safety of performEval for confinement.
 */
const makeFunctionConstructor = (globaObject, options = {}) => {
  // Define an unused parameter to ensure Function.length === 1
  const newFunction = function Function(_body) {
    // Sanitize all parameters at the entry point.
    // eslint-disable-next-line prefer-rest-params
    const bodyText = `${arrayPop(arguments) || ''}`;
    // eslint-disable-next-line prefer-rest-params
    const parameters = `${arrayJoin(arguments, ',')}`;

    // Are parameters and bodyText valid code, or is someone
    // attempting an injection attack? This will throw a SyntaxError if:
    // - parameters doesn't parse as parameters
    // - bodyText doesn't parse as a function body
    // - either contain a call to super() or references a super property.
    //
    // It seems that XS may still be vulnerable to the attack explained at
    // https://github.com/tc39/ecma262/pull/2374#issuecomment-813769710
    // where `new Function('/*', '*/ ) {')` would incorrectly validate.
    // Before we worried about this, we check the parameters and bodyText
    // together in one call
    // ```js
    // new FERAL_FUNCTION(parameters, bodyTest);
    // ```
    // However, this check is vulnerable to that bug. Aside from that case,
    // all engines do seem to validate the parameters, taken by themselves,
    // correctly. And all engines do seem to validate the bodyText, taken
    // by itself correctly. So with the following two checks, SES builds a
    // correct safe `Function` constructor by composing two calls to an
    // original unsafe `Function` constructor that may suffer from this bug
    // but is otherwise correctly validating.
    //
    // eslint-disable-next-line no-new
    new FERAL_FUNCTION(parameters, '');
    // eslint-disable-next-line no-new
    new FERAL_FUNCTION(bodyText);

    // Safe to be combined. Defeat potential trailing comments.
    // TODO: since we create an anonymous function, the 'this' value
    // isn't bound to the global object as per specs, but set as undefined.
    const src = `(function anonymous(${parameters}\n) {\n${bodyText}\n})`;
    return performEval(src, globaObject, {}, options);
  };

  defineProperties(newFunction, {
    // Ensure that any function created in any evaluator in a realm is an
    // instance of Function in any evaluator of the same realm.
    prototype: {
      value: FERAL_FUNCTION.prototype,
      writable: false,
      enumerable: false,
      configurable: false } });



  // Assert identity of Function.__proto__ accross all compartments
  assert(
  getPrototypeOf(FERAL_FUNCTION) === FERAL_FUNCTION.prototype,
  'Function prototype is the same accross compartments');

  assert(
  getPrototypeOf(newFunction) === FERAL_FUNCTION.prototype,
  'Function constructor prototype is the same accross compartments');


  return newFunction;
};$h‍_once.makeFunctionConstructor(makeFunctionConstructor);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   $h‍_imports([]);   /* eslint-disable no-restricted-globals */
/**
 * @file Exports {@code whitelist}, a recursively defined
 * JSON record enumerating all intrinsics and their properties
 * according to ECMA specs.
 *
 * @author JF Paradis
 * @author Mark S. Miller
 */

/* eslint max-lines: 0 */

/**
 * constantProperties
 * non-configurable, non-writable data properties of all global objects.
 * Must be powerless.
 * Maps from property name to the actual value
 */
const constantProperties = {
  // *** Value Properties of the Global Object

  Infinity,
  NaN,
  undefined };


/**
 * universalPropertyNames
 * Properties of all global objects.
 * Must be powerless.
 * Maps from property name to the intrinsic name in the whitelist.
 */$h‍_once.constantProperties(constantProperties);
const universalPropertyNames = {
  // *** Function Properties of the Global Object

  isFinite: 'isFinite',
  isNaN: 'isNaN',
  parseFloat: 'parseFloat',
  parseInt: 'parseInt',

  decodeURI: 'decodeURI',
  decodeURIComponent: 'decodeURIComponent',
  encodeURI: 'encodeURI',
  encodeURIComponent: 'encodeURIComponent',

  // *** Constructor Properties of the Global Object

  Array: 'Array',
  ArrayBuffer: 'ArrayBuffer',
  BigInt: 'BigInt',
  BigInt64Array: 'BigInt64Array',
  BigUint64Array: 'BigUint64Array',
  Boolean: 'Boolean',
  DataView: 'DataView',
  EvalError: 'EvalError',
  Float32Array: 'Float32Array',
  Float64Array: 'Float64Array',
  Int8Array: 'Int8Array',
  Int16Array: 'Int16Array',
  Int32Array: 'Int32Array',
  Map: 'Map',
  Number: 'Number',
  Object: 'Object',
  Promise: 'Promise',
  Proxy: 'Proxy',
  RangeError: 'RangeError',
  ReferenceError: 'ReferenceError',
  Set: 'Set',
  String: 'String',
  Symbol: 'Symbol',
  SyntaxError: 'SyntaxError',
  TypeError: 'TypeError',
  Uint8Array: 'Uint8Array',
  Uint8ClampedArray: 'Uint8ClampedArray',
  Uint16Array: 'Uint16Array',
  Uint32Array: 'Uint32Array',
  URIError: 'URIError',
  WeakMap: 'WeakMap',
  WeakSet: 'WeakSet',

  // *** Other Properties of the Global Object

  JSON: 'JSON',
  Reflect: 'Reflect',

  // *** Annex B

  escape: 'escape',
  unescape: 'unescape',

  // ESNext

  lockdown: 'lockdown',
  harden: 'harden',
  HandledPromise: 'HandledPromise' // TODO: Until Promise.delegate (see below).
};

/**
 * initialGlobalPropertyNames
 * Those found only on the initial global, i.e., the global of the
 * start compartment, as well as any compartments created before lockdown.
 * These may provide much of the power provided by the original.
 * Maps from property name to the intrinsic name in the whitelist.
 */$h‍_once.universalPropertyNames(universalPropertyNames);
const initialGlobalPropertyNames = {
  // *** Constructor Properties of the Global Object

  Date: '%InitialDate%',
  Error: '%InitialError%',
  RegExp: '%InitialRegExp%',

  // *** Other Properties of the Global Object

  Math: '%InitialMath%',

  // ESNext

  // From Error-stack proposal
  // Only on initial global. No corresponding
  // powerless form for other globals.
  getStackString: '%InitialGetStackString%'

  // TODO https://github.com/Agoric/SES-shim/issues/551
  // Need initial WeakRef and FinalizationGroup in
  // start compartment only.
};

/**
 * sharedGlobalPropertyNames
 * Those found only on the globals of new compartments created after lockdown,
 * which must therefore be powerless.
 * Maps from property name to the intrinsic name in the whitelist.
 */$h‍_once.initialGlobalPropertyNames(initialGlobalPropertyNames);
const sharedGlobalPropertyNames = {
  // *** Constructor Properties of the Global Object

  Date: '%SharedDate%',
  Error: '%SharedError%',
  RegExp: '%SharedRegExp%',

  // *** Other Properties of the Global Object

  Math: '%SharedMath%' };


/**
 * uniqueGlobalPropertyNames
 * Those made separately for each global, including the initial global
 * of the start compartment.
 * Maps from property name to the intrinsic name in the whitelist
 * (which is currently always the same).
 */$h‍_once.sharedGlobalPropertyNames(sharedGlobalPropertyNames);
const uniqueGlobalPropertyNames = {
  // *** Value Properties of the Global Object

  globalThis: '%UniqueGlobalThis%',

  // *** Function Properties of the Global Object

  eval: '%UniqueEval%',

  // *** Constructor Properties of the Global Object

  Function: '%UniqueFunction%',

  // *** Other Properties of the Global Object

  // ESNext

  Compartment: '%UniqueCompartment%'
  // According to current agreements, eventually the Realm constructor too.
  // 'Realm',
};

// All the "subclasses" of Error. These are collectively represented in the
// ECMAScript spec by the meta variable NativeError.
// TODO Add AggregateError https://github.com/Agoric/SES-shim/issues/550
$h‍_once.uniqueGlobalPropertyNames(uniqueGlobalPropertyNames);const NativeErrors = [
EvalError,
RangeError,
ReferenceError,
SyntaxError,
TypeError,
URIError];


/**
 * <p>Each JSON record enumerates the disposition of the properties on
 *    some corresponding intrinsic object.
 *
 * <p>All records are made of key-value pairs where the key
 *    is the property to process, and the value is the associated
 *    dispositions a.k.a. the "permit". Those permits can be:
 * <ul>
 * <li>The boolean value "false", in which case this property is
 *     blacklisted and simply removed. Properties not mentioned
 *     are also considered blacklisted and are removed.
 * <li>A string value equal to a primitive ("number", "string", etc),
 *     in which case the property is whitelisted if its value property
 *     is typeof the given type. For example, {@code "Infinity"} leads to
 *     "number" and property values that fail {@code typeof "number"}.
 *     are removed.
 * <li>A string value equal to an intinsic name ("ObjectPrototype",
 *     "Array", etc), in which case the property whitelisted if its
 *     value property is equal to the value of the corresponfing
 *     intrinsics. For example, {@code Map.prototype} leads to
 *     "MapPrototype" and the property is removed if its value is
 *     not equal to %MapPrototype%
 * <li>Another record, in which case this property is simply
 *     whitelisted and that next record represents the disposition of
 *     the object which is its value. For example, {@code "Object"}
 *     leads to another record explaining what properties {@code
 *     "Object"} may have and how each such property should be treated.
 *
 * <p>Notes:
 * <li>"[[Proto]]" is used to refer to the "[[Prototype]]" internal
 *     slot, which says which object this object inherits from.
 * <li>"--proto--" is used to refer to the "__proto__" property name,
 *     which is the name of an accessor property on Object.prototype.
 *     In practice, it is used to access the [[Proto]] internal slot,
 *     but is distinct from the internal slot itself. We use
 *     "--proto--" rather than "__proto__" below because "__proto__"
 *     in an object literal is special syntax rather than a normal
 *     property definition.
 * <li>"ObjectPrototype" is the default "[[Proto]]" (when not specified).
 * <li>Constants "fn" and "getter" are used to keep the structure DRY.
 * <li>Symbol properties are listed using the "@@name" form.
 */

// Function Instances
$h‍_once.NativeErrors(NativeErrors);const FunctionInstance = {
  '[[Proto]]': '%FunctionPrototype%',
  length: 'number',
  name: 'string'
  // Do not specify "prototype" here, since only Function instances that can
  // be used as a constructor have a prototype property. For constructors,
  // since prototype properties are instance-specific, we define it there.
};

// AsyncFunction Instances
$h‍_once.FunctionInstance(FunctionInstance);const AsyncFunctionInstance = {
  // This property is not mentioned in ECMA 262, but is present in V8 and
  // necessary for lockdown to succeed.
  '[[Proto]]': '%AsyncFunctionPrototype%' };


// Aliases
const fn = FunctionInstance;
const asyncFn = AsyncFunctionInstance;

const getter = {
  get: fn,
  set: 'undefined' };


// Possible but not encountered in the specs
// export const setter = {
//   get: 'undefined',
//   set: fn,
// };

const accessor = {
  get: fn,
  set: fn };


const isAccessorPermit = (permit) => {
  return permit === getter || permit === accessor;
};

// NativeError Object Structure
$h‍_once.isAccessorPermit(isAccessorPermit);function NativeError(prototype) {
  return {
    // Properties of the NativeError Constructors
    '[[Proto]]': '%SharedError%',

    // NativeError.prototype
    prototype };

}

function NativeErrorPrototype(constructor) {
  return {
    // Properties of the NativeError Prototype Objects
    '[[Proto]]': '%ErrorPrototype%',
    constructor,
    message: 'string',
    name: 'string',
    // Redundantly present only on v8. Safe to remove.
    toString: false };

}

// The TypedArray Constructors
function TypedArray(prototype) {
  return {
    // Properties of the TypedArray Constructors
    '[[Proto]]': '%TypedArray%',
    BYTES_PER_ELEMENT: 'number',
    prototype };

}

function TypedArrayPrototype(constructor) {
  return {
    // Properties of the TypedArray Prototype Objects
    '[[Proto]]': '%TypedArrayPrototype%',
    BYTES_PER_ELEMENT: 'number',
    constructor };

}

// Without Math.random
const SharedMath = {
  E: 'number',
  LN10: 'number',
  LN2: 'number',
  LOG10E: 'number',
  LOG2E: 'number',
  PI: 'number',
  SQRT1_2: 'number',
  SQRT2: 'number',
  '@@toStringTag': 'string',
  abs: fn,
  acos: fn,
  acosh: fn,
  asin: fn,
  asinh: fn,
  atan: fn,
  atanh: fn,
  atan2: fn,
  cbrt: fn,
  ceil: fn,
  clz32: fn,
  cos: fn,
  cosh: fn,
  exp: fn,
  expm1: fn,
  floor: fn,
  fround: fn,
  hypot: fn,
  imul: fn,
  log: fn,
  log1p: fn,
  log10: fn,
  log2: fn,
  max: fn,
  min: fn,
  pow: fn,
  round: fn,
  sign: fn,
  sin: fn,
  sinh: fn,
  sqrt: fn,
  tan: fn,
  tanh: fn,
  trunc: fn,
  // See https://github.com/Moddable-OpenSource/moddable/issues/523
  idiv: false,
  // See https://github.com/Moddable-OpenSource/moddable/issues/523
  idivmod: false,
  // See https://github.com/Moddable-OpenSource/moddable/issues/523
  imod: false,
  // See https://github.com/Moddable-OpenSource/moddable/issues/523
  imuldiv: false,
  // See https://github.com/Moddable-OpenSource/moddable/issues/523
  irem: false,
  // See https://github.com/Moddable-OpenSource/moddable/issues/523
  mod: false };


const whitelist = {
  // ECMA https://tc39.es/ecma262

  // The intrinsics object has no prototype to avoid conflicts.
  '[[Proto]]': null,

  // %ThrowTypeError%
  '%ThrowTypeError%': fn,

  // *** The Global Object

  // *** Value Properties of the Global Object
  Infinity: 'number',
  NaN: 'number',
  undefined: 'undefined',

  // *** Function Properties of the Global Object

  // eval
  '%UniqueEval%': fn,
  isFinite: fn,
  isNaN: fn,
  parseFloat: fn,
  parseInt: fn,
  decodeURI: fn,
  decodeURIComponent: fn,
  encodeURI: fn,
  encodeURIComponent: fn,

  // *** Fundamental Objects

  Object: {
    // Properties of the Object Constructor
    '[[Proto]]': '%FunctionPrototype%',
    assign: fn,
    create: fn,
    defineProperties: fn,
    defineProperty: fn,
    entries: fn,
    freeze: fn,
    fromEntries: fn,
    getOwnPropertyDescriptor: fn,
    getOwnPropertyDescriptors: fn,
    getOwnPropertyNames: fn,
    getOwnPropertySymbols: fn,
    getPrototypeOf: fn,
    is: fn,
    isExtensible: fn,
    isFrozen: fn,
    isSealed: fn,
    keys: fn,
    preventExtensions: fn,
    prototype: '%ObjectPrototype%',
    seal: fn,
    setPrototypeOf: fn,
    values: fn },


  '%ObjectPrototype%': {
    // Properties of the Object Prototype Object
    '[[Proto]]': null,
    constructor: 'Object',
    hasOwnProperty: fn,
    isPrototypeOf: fn,
    propertyIsEnumerable: fn,
    toLocaleString: fn,
    toString: fn,
    valueOf: fn,

    // Annex B: Additional Properties of the Object.prototype Object

    // See note in header about the difference between [[Proto]] and --proto--
    // special notations.
    '--proto--': accessor,
    __defineGetter__: fn,
    __defineSetter__: fn,
    __lookupGetter__: fn,
    __lookupSetter__: fn },


  '%UniqueFunction%': {
    // Properties of the Function Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%FunctionPrototype%' },


  '%InertFunction%': {
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%FunctionPrototype%' },


  '%FunctionPrototype%': {
    apply: fn,
    bind: fn,
    call: fn,
    constructor: '%InertFunction%',
    toString: fn,
    '@@hasInstance': fn,
    // proposed but not yet std yet. To be removed if there
    caller: false,
    // proposed but not yet std yet. To be removed if there
    arguments: false },


  Boolean: {
    // Properties of the Boolean Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%BooleanPrototype%' },


  '%BooleanPrototype%': {
    constructor: 'Boolean',
    toString: fn,
    valueOf: fn },


  Symbol: {
    // Properties of the Symbol Constructor
    '[[Proto]]': '%FunctionPrototype%',
    asyncIterator: 'symbol',
    for: fn,
    hasInstance: 'symbol',
    isConcatSpreadable: 'symbol',
    iterator: 'symbol',
    keyFor: fn,
    match: 'symbol',
    matchAll: 'symbol',
    prototype: '%SymbolPrototype%',
    replace: 'symbol',
    search: 'symbol',
    species: 'symbol',
    split: 'symbol',
    toPrimitive: 'symbol',
    toStringTag: 'symbol',
    unscopables: 'symbol' },


  '%SymbolPrototype%': {
    // Properties of the Symbol Prototype Object
    constructor: 'Symbol',
    description: getter,
    toString: fn,
    valueOf: fn,
    '@@toPrimitive': fn,
    '@@toStringTag': 'string' },


  '%InitialError%': {
    // Properties of the Error Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%ErrorPrototype%',
    // Non standard, v8 only, used by tap
    captureStackTrace: fn,
    // Non standard, v8 only, used by tap, tamed to accessor
    stackTraceLimit: accessor,
    // Non standard, v8 only, used by several, tamed to accessor
    prepareStackTrace: accessor },


  '%SharedError%': {
    // Properties of the Error Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%ErrorPrototype%',
    // Non standard, v8 only, used by tap
    captureStackTrace: fn,
    // Non standard, v8 only, used by tap, tamed to accessor
    stackTraceLimit: accessor,
    // Non standard, v8 only, used by several, tamed to accessor
    prepareStackTrace: accessor },


  '%ErrorPrototype%': {
    constructor: '%SharedError%',
    message: 'string',
    name: 'string',
    toString: fn,
    // proposed de-facto, assumed TODO
    // Seen on FF Nightly 88.0a1
    at: false,
    // Seen on FF and XS
    stack: false },


  // NativeError

  EvalError: NativeError('%EvalErrorPrototype%'),
  RangeError: NativeError('%RangeErrorPrototype%'),
  ReferenceError: NativeError('%ReferenceErrorPrototype%'),
  SyntaxError: NativeError('%SyntaxErrorPrototype%'),
  TypeError: NativeError('%TypeErrorPrototype%'),
  URIError: NativeError('%URIErrorPrototype%'),

  '%EvalErrorPrototype%': NativeErrorPrototype('EvalError'),
  '%RangeErrorPrototype%': NativeErrorPrototype('RangeError'),
  '%ReferenceErrorPrototype%': NativeErrorPrototype('ReferenceError'),
  '%SyntaxErrorPrototype%': NativeErrorPrototype('SyntaxError'),
  '%TypeErrorPrototype%': NativeErrorPrototype('TypeError'),
  '%URIErrorPrototype%': NativeErrorPrototype('URIError'),

  // *** Numbers and Dates

  Number: {
    // Properties of the Number Constructor
    '[[Proto]]': '%FunctionPrototype%',
    EPSILON: 'number',
    isFinite: fn,
    isInteger: fn,
    isNaN: fn,
    isSafeInteger: fn,
    MAX_SAFE_INTEGER: 'number',
    MAX_VALUE: 'number',
    MIN_SAFE_INTEGER: 'number',
    MIN_VALUE: 'number',
    NaN: 'number',
    NEGATIVE_INFINITY: 'number',
    parseFloat: fn,
    parseInt: fn,
    POSITIVE_INFINITY: 'number',
    prototype: '%NumberPrototype%' },


  '%NumberPrototype%': {
    // Properties of the Number Prototype Object
    constructor: 'Number',
    toExponential: fn,
    toFixed: fn,
    toLocaleString: fn,
    toPrecision: fn,
    toString: fn,
    valueOf: fn },


  BigInt: {
    // Properties of the BigInt Constructor
    '[[Proto]]': '%FunctionPrototype%',
    asIntN: fn,
    asUintN: fn,
    prototype: '%BigIntPrototype%',
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    bitLength: false,
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    fromArrayBuffer: false },


  '%BigIntPrototype%': {
    constructor: 'BigInt',
    toLocaleString: fn,
    toString: fn,
    valueOf: fn,
    '@@toStringTag': 'string' },


  '%InitialMath%': {
    ...SharedMath,
    // random is standard but omitted from SharedMath
    random: fn },


  '%SharedMath%': SharedMath,

  '%InitialDate%': {
    // Properties of the Date Constructor
    '[[Proto]]': '%FunctionPrototype%',
    now: fn,
    parse: fn,
    prototype: '%DatePrototype%',
    UTC: fn },


  '%SharedDate%': {
    // Properties of the Date Constructor
    '[[Proto]]': '%FunctionPrototype%',
    now: fn,
    parse: fn,
    prototype: '%DatePrototype%',
    UTC: fn },


  '%DatePrototype%': {
    constructor: '%SharedDate%',
    getDate: fn,
    getDay: fn,
    getFullYear: fn,
    getHours: fn,
    getMilliseconds: fn,
    getMinutes: fn,
    getMonth: fn,
    getSeconds: fn,
    getTime: fn,
    getTimezoneOffset: fn,
    getUTCDate: fn,
    getUTCDay: fn,
    getUTCFullYear: fn,
    getUTCHours: fn,
    getUTCMilliseconds: fn,
    getUTCMinutes: fn,
    getUTCMonth: fn,
    getUTCSeconds: fn,
    setDate: fn,
    setFullYear: fn,
    setHours: fn,
    setMilliseconds: fn,
    setMinutes: fn,
    setMonth: fn,
    setSeconds: fn,
    setTime: fn,
    setUTCDate: fn,
    setUTCFullYear: fn,
    setUTCHours: fn,
    setUTCMilliseconds: fn,
    setUTCMinutes: fn,
    setUTCMonth: fn,
    setUTCSeconds: fn,
    toDateString: fn,
    toISOString: fn,
    toJSON: fn,
    toLocaleDateString: fn,
    toLocaleString: fn,
    toLocaleTimeString: fn,
    toString: fn,
    toTimeString: fn,
    toUTCString: fn,
    valueOf: fn,
    '@@toPrimitive': fn,

    // Annex B: Additional Properties of the Date.prototype Object
    getYear: fn,
    setYear: fn,
    toGMTString: fn },


  // Text Processing

  String: {
    // Properties of the String Constructor
    '[[Proto]]': '%FunctionPrototype%',
    fromCharCode: fn,
    fromCodePoint: fn,
    prototype: '%StringPrototype%',
    raw: fn,
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    fromArrayBuffer: false },


  '%StringPrototype%': {
    // Properties of the String Prototype Object
    length: 'number',
    charAt: fn,
    charCodeAt: fn,
    codePointAt: fn,
    concat: fn,
    constructor: 'String',
    endsWith: fn,
    includes: fn,
    indexOf: fn,
    lastIndexOf: fn,
    localeCompare: fn,
    match: fn,
    matchAll: fn,
    normalize: fn,
    padEnd: fn,
    padStart: fn,
    repeat: fn,
    replace: fn,
    replaceAll: fn, // ES2021
    search: fn,
    slice: fn,
    split: fn,
    startsWith: fn,
    substring: fn,
    toLocaleLowerCase: fn,
    toLocaleUpperCase: fn,
    toLowerCase: fn,
    toString: fn,
    toUpperCase: fn,
    trim: fn,
    trimEnd: fn,
    trimStart: fn,
    valueOf: fn,
    '@@iterator': fn,

    // Annex B: Additional Properties of the String.prototype Object
    substr: fn,
    anchor: fn,
    big: fn,
    blink: fn,
    bold: fn,
    fixed: fn,
    fontcolor: fn,
    fontsize: fn,
    italics: fn,
    link: fn,
    small: fn,
    strike: fn,
    sub: fn,
    sup: fn,
    trimLeft: fn,
    trimRight: fn,
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    compare: false,

    // Stage 3:
    // https://tc39.es/proposal-relative-indexing-method/
    at: fn },


  '%StringIteratorPrototype%': {
    '[[Proto]]': '%IteratorPrototype%',
    next: fn,
    '@@toStringTag': 'string' },


  '%InitialRegExp%': {
    // Properties of the RegExp Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%RegExpPrototype%',
    '@@species': getter,

    // The https://github.com/tc39/proposal-regexp-legacy-features
    // are all optional, unsafe, and omitted
    input: false,
    $_: false,
    lastMatch: false,
    '$&': false,
    lastParen: false,
    '$+': false,
    leftContext: false,
    '$`': false,
    rightContext: false,
    "$'": false,
    $1: false,
    $2: false,
    $3: false,
    $4: false,
    $5: false,
    $6: false,
    $7: false,
    $8: false,
    $9: false },


  '%SharedRegExp%': {
    // Properties of the RegExp Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%RegExpPrototype%',
    '@@species': getter },


  '%RegExpPrototype%': {
    // Properties of the RegExp Prototype Object
    constructor: '%SharedRegExp%',
    exec: fn,
    dotAll: getter,
    flags: getter,
    global: getter,
    ignoreCase: getter,
    '@@match': fn,
    '@@matchAll': fn,
    multiline: getter,
    '@@replace': fn,
    '@@search': fn,
    source: getter,
    '@@split': fn,
    sticky: getter,
    test: fn,
    toString: fn,
    unicode: getter,

    // Annex B: Additional Properties of the RegExp.prototype Object
    compile: false, // UNSAFE and suppressed.
    // Seen on FF Nightly 88.0a1, Chrome Canary 91.0.4446.0,
    // Safari Tech Preview Release 122 (Safari 14.2, WebKit 16612.1.6.2)
    hasIndices: false },


  '%RegExpStringIteratorPrototype%': {
    // The %RegExpStringIteratorPrototype% Object
    '[[Proto]]': '%IteratorPrototype%',
    next: fn,
    '@@toStringTag': 'string' },


  // Indexed Collections

  Array: {
    // Properties of the Array Constructor
    '[[Proto]]': '%FunctionPrototype%',
    from: fn,
    isArray: fn,
    of: fn,
    prototype: '%ArrayPrototype%',
    '@@species': getter,

    // Stage 3:
    // https://tc39.es/proposal-relative-indexing-method/
    at: fn },


  '%ArrayPrototype%': {
    // Properties of the Array Prototype Object
    length: 'number',
    concat: fn,
    constructor: 'Array',
    copyWithin: fn,
    entries: fn,
    every: fn,
    fill: fn,
    filter: fn,
    find: fn,
    findIndex: fn,
    flat: fn,
    flatMap: fn,
    forEach: fn,
    includes: fn,
    indexOf: fn,
    join: fn,
    keys: fn,
    lastIndexOf: fn,
    map: fn,
    pop: fn,
    push: fn,
    reduce: fn,
    reduceRight: fn,
    reverse: fn,
    shift: fn,
    slice: fn,
    some: fn,
    sort: fn,
    splice: fn,
    toLocaleString: fn,
    toString: fn,
    unshift: fn,
    values: fn,
    '@@iterator': fn,
    '@@unscopables': {
      '[[Proto]]': null,
      copyWithin: 'boolean',
      entries: 'boolean',
      fill: 'boolean',
      find: 'boolean',
      findIndex: 'boolean',
      flat: 'boolean',
      flatMap: 'boolean',
      includes: 'boolean',
      keys: 'boolean',
      values: 'boolean',
      // Failed tc39 proposal
      // Seen on FF Nightly 88.0a1
      at: false },

    // Failed tc39 proposal
    // Seen on FF Nightly 88.0a1
    at: false },


  '%ArrayIteratorPrototype%': {
    // The %ArrayIteratorPrototype% Object
    '[[Proto]]': '%IteratorPrototype%',
    next: fn,
    '@@toStringTag': 'string' },


  // *** TypedArray Objects

  '%TypedArray%': {
    // Properties of the %TypedArray% Intrinsic Object
    '[[Proto]]': '%FunctionPrototype%',
    from: fn,
    of: fn,
    prototype: '%TypedArrayPrototype%',
    '@@species': getter },


  '%TypedArrayPrototype%': {
    buffer: getter,
    byteLength: getter,
    byteOffset: getter,
    constructor: '%TypedArray%',
    copyWithin: fn,
    entries: fn,
    every: fn,
    fill: fn,
    filter: fn,
    find: fn,
    findIndex: fn,
    forEach: fn,
    includes: fn,
    indexOf: fn,
    join: fn,
    keys: fn,
    lastIndexOf: fn,
    length: getter,
    map: fn,
    reduce: fn,
    reduceRight: fn,
    reverse: fn,
    set: fn,
    slice: fn,
    some: fn,
    sort: fn,
    subarray: fn,
    toLocaleString: fn,
    toString: fn,
    values: fn,
    '@@iterator': fn,
    '@@toStringTag': getter,
    // Failed tc39 proposal
    // Seen on FF Nightly 88.0a1
    at: false },


  // The TypedArray Constructors

  BigInt64Array: TypedArray('%BigInt64ArrayPrototype%'),
  BigUint64Array: TypedArray('%BigUint64ArrayPrototype%'),
  Float32Array: TypedArray('%Float32ArrayPrototype%'),
  Float64Array: TypedArray('%Float64ArrayPrototype%'),
  Int16Array: TypedArray('%Int16ArrayPrototype%'),
  Int32Array: TypedArray('%Int32ArrayPrototype%'),
  Int8Array: TypedArray('%Int8ArrayPrototype%'),
  Uint16Array: TypedArray('%Uint16ArrayPrototype%'),
  Uint32Array: TypedArray('%Uint32ArrayPrototype%'),
  Uint8Array: TypedArray('%Uint8ArrayPrototype%'),
  Uint8ClampedArray: TypedArray('%Uint8ClampedArrayPrototype%'),

  '%BigInt64ArrayPrototype%': TypedArrayPrototype('BigInt64Array'),
  '%BigUint64ArrayPrototype%': TypedArrayPrototype('BigUint64Array'),
  '%Float32ArrayPrototype%': TypedArrayPrototype('Float32Array'),
  '%Float64ArrayPrototype%': TypedArrayPrototype('Float64Array'),
  '%Int16ArrayPrototype%': TypedArrayPrototype('Int16Array'),
  '%Int32ArrayPrototype%': TypedArrayPrototype('Int32Array'),
  '%Int8ArrayPrototype%': TypedArrayPrototype('Int8Array'),
  '%Uint16ArrayPrototype%': TypedArrayPrototype('Uint16Array'),
  '%Uint32ArrayPrototype%': TypedArrayPrototype('Uint32Array'),
  '%Uint8ArrayPrototype%': TypedArrayPrototype('Uint8Array'),
  '%Uint8ClampedArrayPrototype%': TypedArrayPrototype('Uint8ClampedArray'),

  // *** Keyed Collections

  Map: {
    // Properties of the Map Constructor
    '[[Proto]]': '%FunctionPrototype%',
    '@@species': getter,
    prototype: '%MapPrototype%' },


  '%MapPrototype%': {
    clear: fn,
    constructor: 'Map',
    delete: fn,
    entries: fn,
    forEach: fn,
    get: fn,
    has: fn,
    keys: fn,
    set: fn,
    size: getter,
    values: fn,
    '@@iterator': fn,
    '@@toStringTag': 'string' },


  '%MapIteratorPrototype%': {
    // The %MapIteratorPrototype% Object
    '[[Proto]]': '%IteratorPrototype%',
    next: fn,
    '@@toStringTag': 'string' },


  Set: {
    // Properties of the Set Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%SetPrototype%',
    '@@species': getter },


  '%SetPrototype%': {
    add: fn,
    clear: fn,
    constructor: 'Set',
    delete: fn,
    entries: fn,
    forEach: fn,
    has: fn,
    keys: fn,
    size: getter,
    values: fn,
    '@@iterator': fn,
    '@@toStringTag': 'string' },


  '%SetIteratorPrototype%': {
    // The %SetIteratorPrototype% Object
    '[[Proto]]': '%IteratorPrototype%',
    next: fn,
    '@@toStringTag': 'string' },


  WeakMap: {
    // Properties of the WeakMap Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%WeakMapPrototype%' },


  '%WeakMapPrototype%': {
    constructor: 'WeakMap',
    delete: fn,
    get: fn,
    has: fn,
    set: fn,
    '@@toStringTag': 'string' },


  WeakSet: {
    // Properties of the WeakSet Constructor
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%WeakSetPrototype%' },


  '%WeakSetPrototype%': {
    add: fn,
    constructor: 'WeakSet',
    delete: fn,
    has: fn,
    '@@toStringTag': 'string' },


  // *** Structured Data

  ArrayBuffer: {
    // Properties of the ArrayBuffer Constructor
    '[[Proto]]': '%FunctionPrototype%',
    isView: fn,
    prototype: '%ArrayBufferPrototype%',
    '@@species': getter,
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    fromString: false,
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    fromBigInt: false },


  '%ArrayBufferPrototype%': {
    byteLength: getter,
    constructor: 'ArrayBuffer',
    slice: fn,
    '@@toStringTag': 'string',
    // See https://github.com/Moddable-OpenSource/moddable/issues/523
    concat: false },


  // SharedArrayBuffer Objects
  SharedArrayBuffer: false, // UNSAFE and purposely suppressed.
  '%SharedArrayBufferPrototype%': false, // UNSAFE and purposely suppressed.

  DataView: {
    // Properties of the DataView Constructor
    '[[Proto]]': '%FunctionPrototype%',
    BYTES_PER_ELEMENT: 'number', // Non std but undeletable on Safari.
    prototype: '%DataViewPrototype%' },


  '%DataViewPrototype%': {
    buffer: getter,
    byteLength: getter,
    byteOffset: getter,
    constructor: 'DataView',
    getBigInt64: fn,
    getBigUint64: fn,
    getFloat32: fn,
    getFloat64: fn,
    getInt8: fn,
    getInt16: fn,
    getInt32: fn,
    getUint8: fn,
    getUint16: fn,
    getUint32: fn,
    setBigInt64: fn,
    setBigUint64: fn,
    setFloat32: fn,
    setFloat64: fn,
    setInt8: fn,
    setInt16: fn,
    setInt32: fn,
    setUint8: fn,
    setUint16: fn,
    setUint32: fn,
    '@@toStringTag': 'string' },


  // Atomics
  Atomics: false, // UNSAFE and suppressed.

  JSON: {
    parse: fn,
    stringify: fn,
    '@@toStringTag': 'string' },


  // *** Control Abstraction Objects

  '%IteratorPrototype%': {
    // The %IteratorPrototype% Object
    '@@iterator': fn },


  '%AsyncIteratorPrototype%': {
    // The %AsyncIteratorPrototype% Object
    '@@asyncIterator': fn },


  '%InertGeneratorFunction%': {
    // Properties of the GeneratorFunction Constructor
    '[[Proto]]': '%InertFunction%',
    prototype: '%Generator%' },


  '%Generator%': {
    // Properties of the GeneratorFunction Prototype Object
    '[[Proto]]': '%FunctionPrototype%',
    constructor: '%InertGeneratorFunction%',
    prototype: '%GeneratorPrototype%',
    '@@toStringTag': 'string' },


  '%InertAsyncGeneratorFunction%': {
    // Properties of the AsyncGeneratorFunction Constructor
    '[[Proto]]': '%InertFunction%',
    prototype: '%AsyncGenerator%' },


  '%AsyncGenerator%': {
    // Properties of the AsyncGeneratorFunction Prototype Object
    '[[Proto]]': '%FunctionPrototype%',
    constructor: '%InertAsyncGeneratorFunction%',
    prototype: '%AsyncGeneratorPrototype%',
    '@@toStringTag': 'string' },


  '%GeneratorPrototype%': {
    // Properties of the Generator Prototype Object
    '[[Proto]]': '%IteratorPrototype%',
    constructor: '%Generator%',
    next: fn,
    return: fn,
    throw: fn,
    '@@toStringTag': 'string' },


  '%AsyncGeneratorPrototype%': {
    // Properties of the AsyncGenerator Prototype Object
    '[[Proto]]': '%AsyncIteratorPrototype%',
    constructor: '%AsyncGenerator%',
    next: fn,
    return: fn,
    throw: fn,
    '@@toStringTag': 'string' },


  // TODO: To be replaced with Promise.delegate
  //
  // The HandledPromise global variable shimmed by `@agoric/eventual-send/shim`
  // implements an initial version of the eventual send specification at:
  // https://github.com/tc39/proposal-eventual-send
  //
  // We will likely change this to add a property to Promise called
  // Promise.delegate and put static methods on it, which will necessitate
  // another whitelist change to update to the current proposed standard.
  HandledPromise: {
    '[[Proto]]': 'Promise',
    applyFunction: fn,
    applyFunctionSendOnly: fn,
    applyMethod: fn,
    applyMethodSendOnly: fn,
    get: fn,
    getSendOnly: fn,
    prototype: '%PromisePrototype%',
    resolve: fn },


  Promise: {
    // Properties of the Promise Constructor
    '[[Proto]]': '%FunctionPrototype%',
    all: fn,
    allSettled: fn,
    // To transition from `false` to `fn` once we also have `AggregateError`
    // TODO https://github.com/Agoric/SES-shim/issues/550
    any: false, // ES2021
    prototype: '%PromisePrototype%',
    race: fn,
    reject: fn,
    resolve: fn,
    '@@species': getter },


  '%PromisePrototype%': {
    // Properties of the Promise Prototype Object
    catch: fn,
    constructor: 'Promise',
    finally: fn,
    then: fn,
    '@@toStringTag': 'string' },


  '%InertAsyncFunction%': {
    // Properties of the AsyncFunction Constructor
    '[[Proto]]': '%InertFunction%',
    prototype: '%AsyncFunctionPrototype%' },


  '%AsyncFunctionPrototype%': {
    // Properties of the AsyncFunction Prototype Object
    '[[Proto]]': '%FunctionPrototype%',
    constructor: '%InertAsyncFunction%',
    '@@toStringTag': 'string' },


  // Reflection

  Reflect: {
    // The Reflect Object
    // Not a function object.
    apply: fn,
    construct: fn,
    defineProperty: fn,
    deleteProperty: fn,
    get: fn,
    getOwnPropertyDescriptor: fn,
    getPrototypeOf: fn,
    has: fn,
    isExtensible: fn,
    ownKeys: fn,
    preventExtensions: fn,
    set: fn,
    setPrototypeOf: fn,
    '@@toStringTag': 'string' },


  Proxy: {
    // Properties of the Proxy Constructor
    '[[Proto]]': '%FunctionPrototype%',
    revocable: fn },


  // Appendix B

  // Annex B: Additional Properties of the Global Object

  escape: fn,
  unescape: fn,

  // Proposed

  '%UniqueCompartment%': {
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%CompartmentPrototype%',
    toString: fn },


  '%InertCompartment%': {
    '[[Proto]]': '%FunctionPrototype%',
    prototype: '%CompartmentPrototype%',
    toString: fn },


  '%CompartmentPrototype%': {
    constructor: '%InertCompartment%',
    evaluate: fn,
    globalThis: getter,
    name: getter,
    // Should this be proposed?
    toString: fn,
    __isKnownScopeProxy__: fn,
    __makeScopeProxy__: fn,
    import: asyncFn,
    load: asyncFn,
    importNow: fn,
    module: fn },


  lockdown: fn,
  harden: fn,

  '%InitialGetStackString%': fn };$h‍_once.whitelist(whitelist);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let defineProperty,objectHasOwnProperty,entries,makeEvalFunction,makeFunctionConstructor,constantProperties,universalPropertyNames;$h‍_imports([["./commons.js", [["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["entries", [$h‍_a => (entries = $h‍_a)]]]],["./make-eval-function.js", [["makeEvalFunction", [$h‍_a => (makeEvalFunction = $h‍_a)]]]],["./make-function-constructor.js", [["makeFunctionConstructor", [$h‍_a => (makeFunctionConstructor = $h‍_a)]]]],["./whitelist.js", [["constantProperties", [$h‍_a => (constantProperties = $h‍_a)]],["universalPropertyNames", [$h‍_a => (universalPropertyNames = $h‍_a)]]]]]);   




/**
 * initGlobalObject()
 * Create new global object using a process similar to ECMA specifications
 * (portions of SetRealmGlobalObject and SetDefaultGlobalBindings).
 * `newGlobalPropertyNames` should be either `initialGlobalPropertyNames` or
 * `sharedGlobalPropertyNames`.
 *
 * @param {Object} globalObject
 * @param {Object} intrinsics
 * @param {Object} newGlobalPropertyNames
 * @param {Function} makeCompartmentConstructor
 * @param {Object} compartmentPrototype
 * @param {Object} [options]
 * @param {Array<Transform>} [options.globalTransforms]
 * @param {(Object) => void} [options.markVirtualizedNativeFunction]
 */
const initGlobalObject = (
globalObject,
intrinsics,
newGlobalPropertyNames,
makeCompartmentConstructor,
compartmentPrototype,
{ globalTransforms, markVirtualizedNativeFunction }) =>
{
  for (const [name, constant] of entries(constantProperties)) {
    defineProperty(globalObject, name, {
      value: constant,
      writable: false,
      enumerable: false,
      configurable: false });

  }

  for (const [name, intrinsicName] of entries(universalPropertyNames)) {
    if (objectHasOwnProperty(intrinsics, intrinsicName)) {
      defineProperty(globalObject, name, {
        value: intrinsics[intrinsicName],
        writable: true,
        enumerable: false,
        configurable: true });

    }
  }

  for (const [name, intrinsicName] of entries(newGlobalPropertyNames)) {
    if (objectHasOwnProperty(intrinsics, intrinsicName)) {
      defineProperty(globalObject, name, {
        value: intrinsics[intrinsicName],
        writable: true,
        enumerable: false,
        configurable: true });

    }
  }

  const perCompartmentGlobals = {
    globalThis: globalObject,
    eval: makeEvalFunction(globalObject, {
      globalTransforms }),

    Function: makeFunctionConstructor(globalObject, {
      globalTransforms }) };



  perCompartmentGlobals.Compartment = makeCompartmentConstructor(
  makeCompartmentConstructor,
  intrinsics,
  markVirtualizedNativeFunction);


  // TODO These should still be tamed according to the whitelist before
  // being made available.
  for (const [name, value] of entries(perCompartmentGlobals)) {
    defineProperty(globalObject, name, {
      value,
      writable: true,
      enumerable: false,
      configurable: true });

    if (typeof value === 'function') {
      markVirtualizedNativeFunction(value);
    }
  }
};$h‍_once.initGlobalObject(initGlobalObject);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let ReferenceError,TypeError,Map,Set,arrayJoin,arrayMap,arrayPush,create,freeze,mapGet,mapHas,mapSet,setAdd,promiseCatch,promiseThen,values,weakmapGet,assert;$h‍_imports([["./commons.js", [["ReferenceError", [$h‍_a => (ReferenceError = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["Map", [$h‍_a => (Map = $h‍_a)]],["Set", [$h‍_a => (Set = $h‍_a)]],["arrayJoin", [$h‍_a => (arrayJoin = $h‍_a)]],["arrayMap", [$h‍_a => (arrayMap = $h‍_a)]],["arrayPush", [$h‍_a => (arrayPush = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["mapGet", [$h‍_a => (mapGet = $h‍_a)]],["mapHas", [$h‍_a => (mapHas = $h‍_a)]],["mapSet", [$h‍_a => (mapSet = $h‍_a)]],["setAdd", [$h‍_a => (setAdd = $h‍_a)]],["promiseCatch", [$h‍_a => (promiseCatch = $h‍_a)]],["promiseThen", [$h‍_a => (promiseThen = $h‍_a)]],["values", [$h‍_a => (values = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   




























const { details: d, quote: q } = assert;

const noop = () => {};

// `makeAlias` constructs compartment specifier tuples for the `aliases`
// private field of compartments.
// These aliases allow a compartment to alias an internal module specifier to a
// module specifier in an external compartment, and also to create internal
// aliases.
// Both are facilitated by the moduleMap Compartment constructor option.
const makeAlias = (compartment, specifier) =>
freeze({
  compartment,
  specifier });


// `resolveAll` pre-computes resolutions of all imports within the compartment
// in which a module was loaded.
$h‍_once.makeAlias(makeAlias);const resolveAll = (imports, resolveHook, fullReferrerSpecifier) => {
  const resolvedImports = create(null);
  for (const importSpecifier of imports) {
    const fullSpecifier = resolveHook(importSpecifier, fullReferrerSpecifier);
    resolvedImports[importSpecifier] = fullSpecifier;
  }
  return freeze(resolvedImports);
};

const loadRecord = async (
compartmentPrivateFields,
moduleAliases,
compartment,
moduleSpecifier,
staticModuleRecord,
pendingJobs,
moduleLoads,
errors) =>
{
  const { resolveHook, moduleRecords } = weakmapGet(
  compartmentPrivateFields,
  compartment);


  // resolve all imports relative to this referrer module.
  const resolvedImports = resolveAll(
  staticModuleRecord.imports,
  resolveHook,
  moduleSpecifier);

  const moduleRecord = freeze({
    compartment,
    staticModuleRecord,
    moduleSpecifier,
    resolvedImports });


  // Enqueue jobs to load this module's shallow dependencies.
  for (const fullSpecifier of values(resolvedImports)) {
    // Behold: recursion.
    // eslint-disable-next-line no-use-before-define
    const dependencyLoaded = memoizedLoadWithErrorAnnotation(
    compartmentPrivateFields,
    moduleAliases,
    compartment,
    fullSpecifier,
    pendingJobs,
    moduleLoads,
    errors);

    setAdd(
    pendingJobs,
    promiseThen(dependencyLoaded, noop, (error) => {
      arrayPush(errors, error);
    }));

  }

  // Memoize.
  mapSet(moduleRecords, moduleSpecifier, moduleRecord);
  return moduleRecord;
};

const loadWithoutErrorAnnotation = async (
compartmentPrivateFields,
moduleAliases,
compartment,
moduleSpecifier,
pendingJobs,
moduleLoads,
errors) =>
{
  const { importHook, moduleMap, moduleMapHook, moduleRecords } = weakmapGet(
  compartmentPrivateFields,
  compartment);


  // Follow moduleMap, or moduleMapHook if present.
  let aliasNamespace = moduleMap[moduleSpecifier];
  if (aliasNamespace === undefined && moduleMapHook !== undefined) {
    aliasNamespace = moduleMapHook(moduleSpecifier);
  }
  if (typeof aliasNamespace === 'string') {
    // eslint-disable-next-line @endo/no-polymorphic-call
    assert.fail(
    d`Cannot map module ${q(moduleSpecifier)} to ${q(
    aliasNamespace)
    } in parent compartment, not yet implemented`,
    TypeError);

  } else if (aliasNamespace !== undefined) {
    const alias = weakmapGet(moduleAliases, aliasNamespace);
    if (alias === undefined) {
      // eslint-disable-next-line @endo/no-polymorphic-call
      assert.fail(
      d`Cannot map module ${q(
      moduleSpecifier)
      } because the value is not a module exports namespace, or is from another realm`,
      ReferenceError);

    }
    // Behold: recursion.
    // eslint-disable-next-line no-use-before-define
    const aliasRecord = await memoizedLoadWithErrorAnnotation(
    compartmentPrivateFields,
    moduleAliases,
    alias.compartment,
    alias.specifier,
    pendingJobs,
    moduleLoads,
    errors);

    mapSet(moduleRecords, moduleSpecifier, aliasRecord);
    return aliasRecord;
  }

  if (mapHas(moduleRecords, moduleSpecifier)) {
    return mapGet(moduleRecords, moduleSpecifier);
  }

  const staticModuleRecord = await importHook(moduleSpecifier);

  if (staticModuleRecord === null || typeof staticModuleRecord !== 'object') {
    // eslint-disable-next-line @endo/no-polymorphic-call
    assert.fail(
    d`importHook must return a promise for an object, for module ${q(
    moduleSpecifier)
    } in compartment ${q(compartment.name)}`);

  }

  if (staticModuleRecord.record !== undefined) {
    const {
      compartment: aliasCompartment = compartment,
      specifier: aliasSpecifier = moduleSpecifier,
      record: aliasModuleRecord } =
    staticModuleRecord;

    const aliasRecord = await loadRecord(
    compartmentPrivateFields,
    moduleAliases,
    aliasCompartment,
    aliasSpecifier,
    aliasModuleRecord,
    pendingJobs,
    moduleLoads,
    errors);

    mapSet(moduleRecords, moduleSpecifier, aliasRecord);
    return aliasRecord;
  }

  return loadRecord(
  compartmentPrivateFields,
  moduleAliases,
  compartment,
  moduleSpecifier,
  staticModuleRecord,
  pendingJobs,
  moduleLoads,
  errors);

};

const memoizedLoadWithErrorAnnotation = async (
compartmentPrivateFields,
moduleAliases,
compartment,
moduleSpecifier,
pendingJobs,
moduleLoads,
errors) =>
{
  const { name: compartmentName } = weakmapGet(
  compartmentPrivateFields,
  compartment);


  // Prevent data-lock from recursion into branches visited in dependent loads.
  let compartmentLoading = mapGet(moduleLoads, compartment);
  if (compartmentLoading === undefined) {
    compartmentLoading = new Map();
    mapSet(moduleLoads, compartment, compartmentLoading);
  }
  let moduleLoading = mapGet(compartmentLoading, moduleSpecifier);
  if (moduleLoading !== undefined) {
    return moduleLoading;
  }

  moduleLoading = promiseCatch(
  loadWithoutErrorAnnotation(
  compartmentPrivateFields,
  moduleAliases,
  compartment,
  moduleSpecifier,
  pendingJobs,
  moduleLoads,
  errors),

  (error) => {
    // eslint-disable-next-line @endo/no-polymorphic-call
    assert.note(
    error,
    d`${error.message}, loading ${q(moduleSpecifier)} in compartment ${q(
    compartmentName)
    }`);

    throw error;
  });


  mapSet(compartmentLoading, moduleSpecifier, moduleLoading);

  return moduleLoading;
};

/*
 * `load` asynchronously gathers the `StaticModuleRecord`s for a module and its
 * transitive dependencies.
 * The module records refer to each other by a reference to the dependency's
 * compartment and the specifier of the module within its own compartment.
 * This graph is then ready to be synchronously linked and executed.
 */
const load = async (
compartmentPrivateFields,
moduleAliases,
compartment,
moduleSpecifier) =>
{
  const { name: compartmentName } = weakmapGet(
  compartmentPrivateFields,
  compartment);


  /** @type {Set<Promise<undefined>>} */
  const pendingJobs = new Set();
  /** @type {Map<Object, Map<string, Promise<Record>>} */
  const moduleLoads = new Map();
  /** @type {Array<Error>} */
  const errors = [];

  const dependencyLoaded = memoizedLoadWithErrorAnnotation(
  compartmentPrivateFields,
  moduleAliases,
  compartment,
  moduleSpecifier,
  pendingJobs,
  moduleLoads,
  errors);

  setAdd(
  pendingJobs,
  promiseThen(dependencyLoaded, noop, (error) => {
    arrayPush(errors, error);
  }));


  // Drain pending jobs queue.
  // Each job is a promise for undefined, regardless of success or failure.
  // Before we add a job to the queue, we catch any error and push it into the
  // `errors` accumulator.
  for (const job of pendingJobs) {
    // eslint-disable-next-line no-await-in-loop
    await job;
  }

  // Throw an aggregate error if there were any errors.
  if (errors.length > 0) {
    throw new TypeError(
    `Failed to load module ${q(moduleSpecifier)} in package ${q(
    compartmentName)
    } (${errors.length} underlying failures: ${arrayJoin(
    arrayMap(errors, (error) => error.message),
    ', ')
    }`);

  }
};$h‍_once.load(load);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let makeAlias,Proxy,TypeError,create,freeze,mapGet,mapHas,mapSet,ownKeys,reflectGet,reflectGetOwnPropertyDescriptor,reflectHas,reflectIsExtensible,reflectPreventExtensions,weakmapSet,assert;$h‍_imports([["./module-load.js", [["makeAlias", [$h‍_a => (makeAlias = $h‍_a)]]]],["./commons.js", [["Proxy", [$h‍_a => (Proxy = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["mapGet", [$h‍_a => (mapGet = $h‍_a)]],["mapHas", [$h‍_a => (mapHas = $h‍_a)]],["mapSet", [$h‍_a => (mapSet = $h‍_a)]],["ownKeys", [$h‍_a => (ownKeys = $h‍_a)]],["reflectGet", [$h‍_a => (reflectGet = $h‍_a)]],["reflectGetOwnPropertyDescriptor", [$h‍_a => (reflectGetOwnPropertyDescriptor = $h‍_a)]],["reflectHas", [$h‍_a => (reflectHas = $h‍_a)]],["reflectIsExtensible", [$h‍_a => (reflectIsExtensible = $h‍_a)]],["reflectPreventExtensions", [$h‍_a => (reflectPreventExtensions = $h‍_a)]],["weakmapSet", [$h‍_a => (weakmapSet = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   






























const { quote: q } = assert;

// `deferExports` creates a module's exports proxy, proxied exports, and
// activator.
// A `Compartment` can create a module for any module specifier, regardless of
// whether it is loadable or executable, and use that object as a token that
// can be fed into another compartment's module map.
// Only after the specified module has been analyzed is it possible for the
// module namespace proxy to behave properly, so it throws exceptions until
// after the compartment has begun executing the module.
// The module instance must freeze the proxied exports and activate the exports
// proxy before executing the module.
//
// The module exports proxy's behavior differs from the ECMAScript 262
// specification for "module namespace exotic objects" only in that according
// to the specification value property descriptors have a non-writable "value"
// and this implementation models all properties with accessors.
//
// https://tc39.es/ecma262/#sec-module-namespace-exotic-objects
//
const deferExports = () => {
  let active = false;
  const proxiedExports = create(null);
  return freeze({
    activate() {
      active = true;
    },
    proxiedExports,
    exportsProxy: new Proxy(proxiedExports, {
      get(_target, name, receiver) {
        if (!active) {
          throw new TypeError(
          `Cannot get property ${q(
          name)
          } of module exports namespace, the module has not yet begun to execute`);

        }
        return reflectGet(proxiedExports, name, receiver);
      },
      set(_target, name, _value) {
        throw new TypeError(
        `Cannot set property ${q(name)} of module exports namespace`);

      },
      has(_target, name) {
        if (!active) {
          throw new TypeError(
          `Cannot check property ${q(
          name)
          }, the module has not yet begun to execute`);

        }
        return reflectHas(proxiedExports, name);
      },
      deleteProperty(_target, name) {
        throw new TypeError(
        `Cannot delete property ${q(name)}s of module exports namespace`);

      },
      ownKeys(_target) {
        if (!active) {
          throw new TypeError(
          'Cannot enumerate keys, the module has not yet begun to execute');

        }
        return ownKeys(proxiedExports);
      },
      getOwnPropertyDescriptor(_target, name) {
        if (!active) {
          throw new TypeError(
          `Cannot get own property descriptor ${q(
          name)
          }, the module has not yet begun to execute`);

        }
        return reflectGetOwnPropertyDescriptor(proxiedExports, name);
      },
      preventExtensions(_target) {
        if (!active) {
          throw new TypeError(
          'Cannot prevent extensions of module exports namespace, the module has not yet begun to execute');

        }
        return reflectPreventExtensions(proxiedExports);
      },
      isExtensible() {
        if (!active) {
          throw new TypeError(
          'Cannot check extensibility of module exports namespace, the module has not yet begun to execute');

        }
        return reflectIsExtensible(proxiedExports);
      },
      getPrototypeOf(_target) {
        return null;
      },
      setPrototypeOf(_target, _proto) {
        throw new TypeError('Cannot set prototype of module exports namespace');
      },
      defineProperty(_target, name, _descriptor) {
        throw new TypeError(
        `Cannot define property ${q(name)} of module exports namespace`);

      },
      apply(_target, _thisArg, _args) {
        throw new TypeError(
        'Cannot call module exports namespace, it is not a function');

      },
      construct(_target, _args) {
        throw new TypeError(
        'Cannot construct module exports namespace, it is not a constructor');

      } }) });


};

// `getDeferredExports` memoizes the creation of a deferred module exports
// namespace proxy for any abritrary full specifier in a compartment.
// It also records the compartment and specifier affiliated with that module
// exports namespace proxy so it can be used as an alias into another
// compartment when threaded through a compartment's `moduleMap` argument.
$h‍_once.deferExports(deferExports);const getDeferredExports = (
compartment,
compartmentPrivateFields,
moduleAliases,
specifier) =>
{
  const { deferredExports } = compartmentPrivateFields;
  if (!mapHas(deferredExports, specifier)) {
    const deferred = deferExports();
    weakmapSet(
    moduleAliases,
    deferred.exportsProxy,
    makeAlias(compartment, specifier));

    mapSet(deferredExports, specifier, deferred);
  }
  return mapGet(deferredExports, specifier);
};$h‍_once.getDeferredExports(getDeferredExports);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let assert,getDeferredExports,ReferenceError,SyntaxError,TypeError,arrayForEach,arrayIncludes,arrayPush,arraySome,arraySort,create,defineProperty,entries,freeze,isArray,keys,mapGet,weakmapGet,compartmentEvaluate;$h‍_imports([["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]],["./module-proxy.js", [["getDeferredExports", [$h‍_a => (getDeferredExports = $h‍_a)]]]],["./commons.js", [["ReferenceError", [$h‍_a => (ReferenceError = $h‍_a)]],["SyntaxError", [$h‍_a => (SyntaxError = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["arrayForEach", [$h‍_a => (arrayForEach = $h‍_a)]],["arrayIncludes", [$h‍_a => (arrayIncludes = $h‍_a)]],["arrayPush", [$h‍_a => (arrayPush = $h‍_a)]],["arraySome", [$h‍_a => (arraySome = $h‍_a)]],["arraySort", [$h‍_a => (arraySort = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["entries", [$h‍_a => (entries = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["isArray", [$h‍_a => (isArray = $h‍_a)]],["keys", [$h‍_a => (keys = $h‍_a)]],["mapGet", [$h‍_a => (mapGet = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]]]],["./compartment-evaluate.js", [["compartmentEvaluate", [$h‍_a => (compartmentEvaluate = $h‍_a)]]]]]);   





















const { quote: q } = assert;

const makeThirdPartyModuleInstance = (
compartmentPrivateFields,
staticModuleRecord,
compartment,
moduleAliases,
moduleSpecifier,
resolvedImports) =>
{
  const { exportsProxy, proxiedExports, activate } = getDeferredExports(
  compartment,
  weakmapGet(compartmentPrivateFields, compartment),
  moduleAliases,
  moduleSpecifier);


  const notifiers = create(null);

  if (staticModuleRecord.exports) {
    if (
    !isArray(staticModuleRecord.exports) ||
    arraySome(staticModuleRecord.exports, (name) => typeof name !== 'string'))
    {
      throw new TypeError(
      `SES third-party static module record "exports" property must be an array of strings for module ${moduleSpecifier}`);

    }
    arrayForEach(staticModuleRecord.exports, (name) => {
      let value = proxiedExports[name];
      const updaters = [];

      const get = () => value;

      const set = (newValue) => {
        value = newValue;
        for (const updater of updaters) {
          updater(newValue);
        }
      };

      defineProperty(proxiedExports, name, {
        get,
        set,
        enumerable: true,
        configurable: false });


      notifiers[name] = (update) => {
        arrayPush(updaters, update);
        update(value);
      };
    });
  }

  let activated = false;
  return freeze({
    notifiers,
    exportsProxy,
    execute() {
      if (!activated) {
        activate();
        activated = true;
        // eslint-disable-next-line @endo/no-polymorphic-call
        staticModuleRecord.execute(
        proxiedExports,
        compartment,
        resolvedImports);

      }
    } });

};

// `makeModuleInstance` takes a module's compartment record, the live import
// namespace, and a global object; and produces a module instance.
// The module instance carries the proxied module exports namespace (the
// "exports"), notifiers to update the module's internal import namespace, and
// an idempotent execute function.
// The module exports namespace is a proxy to the proxied exports namespace
// that the execution of the module instance populates.
$h‍_once.makeThirdPartyModuleInstance(makeThirdPartyModuleInstance);const makeModuleInstance = (
privateFields,
moduleAliases,
moduleRecord,
importedInstances) =>
{
  const { compartment, moduleSpecifier, staticModuleRecord } = moduleRecord;
  const {
    reexports: exportAlls = [],
    __syncModuleProgram__: functorSource,
    __fixedExportMap__: fixedExportMap = {},
    __liveExportMap__: liveExportMap = {} } =
  staticModuleRecord;

  const compartmentFields = weakmapGet(privateFields, compartment);

  const { __shimTransforms__ } = compartmentFields;

  const { exportsProxy, proxiedExports, activate } = getDeferredExports(
  compartment,
  compartmentFields,
  moduleAliases,
  moduleSpecifier);


  // {_exportName_: getter} module exports namespace
  // object (eventually proxied).
  const exportsProps = create(null);

  // {_localName_: accessor} proxy traps for globalLexicals and live bindings.
  // The globalLexicals object is frozen and the corresponding properties of
  // localLexicals must be immutable, so we copy the descriptors.
  const localLexicals = create(null);

  // {_localName_: init(initValue) -> initValue} used by the
  // rewritten code to initialize exported fixed bindings.
  const onceVar = create(null);

  // {_localName_: update(newValue)} used by the rewritten code to
  // both initialize and update live bindings.
  const liveVar = create(null);

  // {_localName_: [{get, set, notify}]} used to merge all the export updaters.
  const localGetNotify = create(null);

  // {[importName: string]: notify(update(newValue))} Used by code that imports
  // one of this module's exports, so that their update function will
  // be notified when this binding is initialized or updated.
  const notifiers = create(null);

  arrayForEach(entries(fixedExportMap), ([fixedExportName, [localName]]) => {
    let fixedGetNotify = localGetNotify[localName];
    if (!fixedGetNotify) {
      // fixed binding state
      let value;
      let tdz = true;
      let optUpdaters = [];

      // tdz sensitive getter
      const get = () => {
        if (tdz) {
          throw new ReferenceError(
          `binding ${q(localName)} not yet initialized`);

        }
        return value;
      };

      // leave tdz once
      const init = freeze((initValue) => {
        // init with initValue of a declared const binding, and return
        // it.
        if (!tdz) {
          throw new TypeError(
          `Internal: binding ${q(localName)} already initialized`);

        }
        value = initValue;
        const updaters = optUpdaters;
        optUpdaters = null;
        tdz = false;
        for (const updater of updaters) {
          updater(initValue);
        }
        return initValue;
      });

      // If still tdz, register update for notification later.
      // Otherwise, update now.
      const notify = (updater) => {
        if (updater === init) {
          // Prevent recursion.
          return;
        }
        if (tdz) {
          arrayPush(optUpdaters, updater);
        } else {
          updater(value);
        }
      };

      // Need these for additional exports of the local variable.
      fixedGetNotify = {
        get,
        notify };

      localGetNotify[localName] = fixedGetNotify;
      onceVar[localName] = init;
    }

    exportsProps[fixedExportName] = {
      get: fixedGetNotify.get,
      set: undefined,
      enumerable: true,
      configurable: false };


    notifiers[fixedExportName] = fixedGetNotify.notify;
  });

  arrayForEach(
  entries(liveExportMap),
  ([liveExportName, [localName, setProxyTrap]]) => {
    let liveGetNotify = localGetNotify[localName];
    if (!liveGetNotify) {
      // live binding state
      let value;
      let tdz = true;
      const updaters = [];

      // tdz sensitive getter
      const get = () => {
        if (tdz) {
          throw new ReferenceError(
          `binding ${q(liveExportName)} not yet initialized`);

        }
        return value;
      };

      // This must be usable locally for the translation of initializing
      // a declared local live binding variable.
      //
      // For reexported variable, this is also an update function to
      // register for notification with the downstream import, which we
      // must assume to be live. Thus, it can be called independent of
      // tdz but always leaves tdz. Such reexporting creates a tree of
      // bindings. This lets the tree be hooked up even if the imported
      // module instance isn't initialized yet, as may happen in cycles.
      const update = freeze((newValue) => {
        value = newValue;
        tdz = false;
        for (const updater of updaters) {
          updater(newValue);
        }
      });

      // tdz sensitive setter
      const set = (newValue) => {
        if (tdz) {
          throw new ReferenceError(
          `binding ${q(localName)} not yet initialized`);

        }
        value = newValue;
        for (const updater of updaters) {
          updater(newValue);
        }
      };

      // Always register the updater function.
      // If not in tdz, also update now.
      const notify = (updater) => {
        if (updater === update) {
          // Prevent recursion.
          return;
        }
        arrayPush(updaters, updater);
        if (!tdz) {
          updater(value);
        }
      };

      liveGetNotify = {
        get,
        notify };


      localGetNotify[localName] = liveGetNotify;
      if (setProxyTrap) {
        defineProperty(localLexicals, localName, {
          get,
          set,
          enumerable: true,
          configurable: false });

      }
      liveVar[localName] = update;
    }

    exportsProps[liveExportName] = {
      get: liveGetNotify.get,
      set: undefined,
      enumerable: true,
      configurable: false };


    notifiers[liveExportName] = liveGetNotify.notify;
  });


  const notifyStar = (update) => {
    update(proxiedExports);
  };
  notifiers['*'] = notifyStar;

  // Per the calling convention for the moduleFunctor generated from
  // an ESM, the `imports` function gets called once up front
  // to populate or arrange the population of imports and reexports.
  // The generated code produces an `updateRecord`: the means for
  // the linker to update the imports and exports of the module.
  // The updateRecord must conform to moduleAnalysis.imports
  // updateRecord = Map<specifier, importUpdaters>
  // importUpdaters = Map<importName, [update(newValue)*]>
  function imports(updateRecord) {
    // By the time imports is called, the importedInstances should already be
    // initialized with module instances that satisfy
    // imports.
    // importedInstances = Map[_specifier_, { notifiers, module, execute }]
    // notifiers = { [importName: string]: notify(update(newValue))}

    // export * cannot export default.
    const candidateAll = create(null);
    candidateAll.default = false;
    for (const [specifier, importUpdaters] of updateRecord) {
      const instance = mapGet(importedInstances, specifier);
      // The module instance object is an internal literal, does not bind this,
      // and never revealed outside the SES shim.
      // There are two instantiation sites for instances and they are both in
      // this module.
      // eslint-disable-next-line @endo/no-polymorphic-call
      instance.execute(); // bottom up cycle tolerant
      const { notifiers: importNotifiers } = instance;
      for (const [importName, updaters] of importUpdaters) {
        const importNotify = importNotifiers[importName];
        if (!importNotify) {
          throw SyntaxError(
          `The requested module '${specifier}' does not provide an export named '${importName}'`);

        }
        for (const updater of updaters) {
          importNotify(updater);
        }
      }
      if (arrayIncludes(exportAlls, specifier)) {
        // Make all these imports candidates.
        for (const [importName, importNotify] of entries(importNotifiers)) {
          if (candidateAll[importName] === undefined) {
            candidateAll[importName] = importNotify;
          } else {
            // Already a candidate: remove ambiguity.
            candidateAll[importName] = false;
          }
        }
      }
    }

    for (const [importName, notify] of entries(candidateAll)) {
      if (!notifiers[importName] && notify !== false) {
        notifiers[importName] = notify;

        // exported live binding state
        let value;
        const update = (newValue) => value = newValue;
        notify(update);
        exportsProps[importName] = {
          get() {
            return value;
          },
          set: undefined,
          enumerable: true,
          configurable: false };

      }
    }

    // Sort the module exports namespace as per spec.
    // The module exports namespace will be wrapped in a module namespace
    // exports proxy which will serve as a "module exports namespace exotic
    // object".
    // Sorting properties is not generally reliable because some properties may
    // be symbols, and symbols do not have an inherent relative order, but
    // since all properties of the exports namespace must be keyed by a string
    // and the string must correspond to a valid identifier, sorting these
    // properties works for this specific case.
    arrayForEach(arraySort(keys(exportsProps)), (k) =>
    defineProperty(proxiedExports, k, exportsProps[k]));


    freeze(proxiedExports);
    activate();
  }

  let optFunctor = compartmentEvaluate(compartmentFields, functorSource, {
    globalObject: compartment.globalThis,
    transforms: __shimTransforms__,
    __moduleShimLexicals__: localLexicals });

  let didThrow = false;
  let thrownError;
  function execute() {
    if (optFunctor) {
      // uninitialized
      const functor = optFunctor;
      optFunctor = null;
      // initializing - call with `this` of `undefined`.
      try {
        functor(
        freeze({
          imports: freeze(imports),
          onceVar: freeze(onceVar),
          liveVar: freeze(liveVar) }));


      } catch (e) {
        didThrow = true;
        thrownError = e;
      }
      // initialized
    }
    if (didThrow) {
      throw thrownError;
    }
  }

  return freeze({
    notifiers,
    exportsProxy,
    execute });

};$h‍_once.makeModuleInstance(makeModuleInstance);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let assert,makeModuleInstance,makeThirdPartyModuleInstance,Map,ReferenceError,TypeError,entries,isArray,isObject,mapGet,mapHas,mapSet,weakmapGet;$h‍_imports([["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]],["./module-instance.js", [["makeModuleInstance", [$h‍_a => (makeModuleInstance = $h‍_a)]],["makeThirdPartyModuleInstance", [$h‍_a => (makeThirdPartyModuleInstance = $h‍_a)]]]],["./commons.js", [["Map", [$h‍_a => (Map = $h‍_a)]],["ReferenceError", [$h‍_a => (ReferenceError = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["entries", [$h‍_a => (entries = $h‍_a)]],["isArray", [$h‍_a => (isArray = $h‍_a)]],["isObject", [$h‍_a => (isObject = $h‍_a)]],["mapGet", [$h‍_a => (mapGet = $h‍_a)]],["mapHas", [$h‍_a => (mapHas = $h‍_a)]],["mapSet", [$h‍_a => (mapSet = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]]]]]);   



























const { quote: q } = assert;

// `link` creates `ModuleInstances` and `ModuleNamespaces` for a module and its
// transitive dependencies and connects their imports and exports.
// After linking, the resulting working set is ready to be executed.
// The linker only concerns itself with module namespaces that are objects with
// property descriptors for their exports, which the Compartment proxies with
// the actual `ModuleNamespace`.
const link = (
compartmentPrivateFields,
moduleAliases,
compartment,
moduleSpecifier) =>
{
  const { name: compartmentName, moduleRecords } = weakmapGet(
  compartmentPrivateFields,
  compartment);


  const moduleRecord = mapGet(moduleRecords, moduleSpecifier);
  if (moduleRecord === undefined) {
    throw new ReferenceError(
    `Missing link to module ${q(moduleSpecifier)} from compartment ${q(
    compartmentName)
    }`);

  }

  // Mutual recursion so there's no confusion about which
  // compartment is in context: the module record may be in another
  // compartment, denoted by moduleRecord.compartment.
  // eslint-disable-next-line no-use-before-define
  return instantiate(compartmentPrivateFields, moduleAliases, moduleRecord);
};$h‍_once.link(link);

function isPrecompiled(staticModuleRecord) {
  return typeof staticModuleRecord.__syncModuleProgram__ === 'string';
}

function validatePrecompiledStaticModuleRecord(
staticModuleRecord,
moduleSpecifier)
{
  const { __fixedExportMap__, __liveExportMap__ } = staticModuleRecord;
  assert(
  isObject(__fixedExportMap__),
  `Property '__fixedExportMap__' of a precompiled module record must be an object, got ${q(
  __fixedExportMap__)
  }, for module ${q(moduleSpecifier)}`);

  assert(
  isObject(__liveExportMap__),
  `Property '__liveExportMap__' of a precompiled module record must be an object, got ${q(
  __liveExportMap__)
  }, for module ${q(moduleSpecifier)}`);

}

function isThirdParty(staticModuleRecord) {
  return typeof staticModuleRecord.execute === 'function';
}

function validateThirdPartyStaticModuleRecord(
staticModuleRecord,
moduleSpecifier)
{
  const { exports } = staticModuleRecord;
  assert(
  isArray(exports),
  `Property 'exports' of a third-party static module record must be an array, got ${q(
  exports)
  }, for module ${q(moduleSpecifier)}`);

}

function validateStaticModuleRecord(staticModuleRecord, moduleSpecifier) {
  assert(
  isObject(staticModuleRecord),
  `Static module records must be of type object, got ${q(
  staticModuleRecord)
  }, for module ${q(moduleSpecifier)}`);

  const { imports, exports, reexports = [] } = staticModuleRecord;
  assert(
  isArray(imports),
  `Property 'imports' of a static module record must be an array, got ${q(
  imports)
  }, for module ${q(moduleSpecifier)}`);

  assert(
  isArray(exports),
  `Property 'exports' of a precompiled module record must be an array, got ${q(
  exports)
  }, for module ${q(moduleSpecifier)}`);

  assert(
  isArray(reexports),
  `Property 'reexports' of a precompiled module record must be an array if present, got ${q(
  reexports)
  }, for module ${q(moduleSpecifier)}`);

}

const instantiate = (
compartmentPrivateFields,
moduleAliases,
moduleRecord) =>
{
  const {
    compartment,
    moduleSpecifier,
    resolvedImports,
    staticModuleRecord } =
  moduleRecord;
  const { instances } = weakmapGet(compartmentPrivateFields, compartment);

  // Memoize.
  if (mapHas(instances, moduleSpecifier)) {
    return mapGet(instances, moduleSpecifier);
  }

  validateStaticModuleRecord(staticModuleRecord, moduleSpecifier);

  const importedInstances = new Map();
  let moduleInstance;
  if (isPrecompiled(staticModuleRecord)) {
    validatePrecompiledStaticModuleRecord(staticModuleRecord, moduleSpecifier);
    moduleInstance = makeModuleInstance(
    compartmentPrivateFields,
    moduleAliases,
    moduleRecord,
    importedInstances);

  } else if (isThirdParty(staticModuleRecord)) {
    validateThirdPartyStaticModuleRecord(staticModuleRecord, moduleSpecifier);
    moduleInstance = makeThirdPartyModuleInstance(
    compartmentPrivateFields,
    staticModuleRecord,
    compartment,
    moduleAliases,
    moduleSpecifier,
    resolvedImports);

  } else {
    throw new TypeError(
    `importHook must return a static module record, got ${q(
    staticModuleRecord)
    }`);

  }

  // Memoize.
  mapSet(instances, moduleSpecifier, moduleInstance);

  // Link dependency modules.
  for (const [importSpecifier, resolvedSpecifier] of entries(resolvedImports)) {
    const importedInstance = link(
    compartmentPrivateFields,
    moduleAliases,
    compartment,
    resolvedSpecifier);

    mapSet(importedInstances, importSpecifier, importedInstance);
  }

  return moduleInstance;
};$h‍_once.instantiate(instantiate);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Map,ReferenceError,TypeError,WeakMap,WeakSet,arrayFilter,arrayJoin,assign,defineProperties,entries,freeze,getOwnPropertyNames,promiseThen,weakmapGet,weakmapSet,weaksetHas,initGlobalObject,isValidIdentifierName,sharedGlobalPropertyNames,load,link,getDeferredExports,assert,compartmentEvaluate,makeScopeProxy;$h‍_imports([["./commons.js", [["Map", [$h‍_a => (Map = $h‍_a)]],["ReferenceError", [$h‍_a => (ReferenceError = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["WeakMap", [$h‍_a => (WeakMap = $h‍_a)]],["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["arrayFilter", [$h‍_a => (arrayFilter = $h‍_a)]],["arrayJoin", [$h‍_a => (arrayJoin = $h‍_a)]],["assign", [$h‍_a => (assign = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["entries", [$h‍_a => (entries = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["getOwnPropertyNames", [$h‍_a => (getOwnPropertyNames = $h‍_a)]],["promiseThen", [$h‍_a => (promiseThen = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]],["weakmapSet", [$h‍_a => (weakmapSet = $h‍_a)]],["weaksetHas", [$h‍_a => (weaksetHas = $h‍_a)]]]],["./global-object.js", [["initGlobalObject", [$h‍_a => (initGlobalObject = $h‍_a)]]]],["./scope-constants.js", [["isValidIdentifierName", [$h‍_a => (isValidIdentifierName = $h‍_a)]]]],["./whitelist.js", [["sharedGlobalPropertyNames", [$h‍_a => (sharedGlobalPropertyNames = $h‍_a)]]]],["./module-load.js", [["load", [$h‍_a => (load = $h‍_a)]]]],["./module-link.js", [["link", [$h‍_a => (link = $h‍_a)]]]],["./module-proxy.js", [["getDeferredExports", [$h‍_a => (getDeferredExports = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]],["./compartment-evaluate.js", [["compartmentEvaluate", [$h‍_a => (compartmentEvaluate = $h‍_a)]],["makeScopeProxy", [$h‍_a => (makeScopeProxy = $h‍_a)]]]]]);   






























const { quote: q } = assert;

// moduleAliases associates every public module exports namespace with its
// corresponding compartment and specifier so they can be used to link modules
// across compartments.
// The mechanism to thread an alias is to use the compartment.module function
// to obtain the exports namespace of a foreign module and pass it into another
// compartment's moduleMap constructor option.
const moduleAliases = new WeakMap();

// privateFields captures the private state for each compartment.
const privateFields = new WeakMap();

/**
 * @typedef {(source: string) => string} Transform
 */

// Compartments do not need an importHook or resolveHook to be useful
// as a vessel for evaluating programs.
// However, any method that operates the module system will throw an exception
// if these hooks are not available.
const assertModuleHooks = (compartment) => {
  const { importHook, resolveHook } = weakmapGet(privateFields, compartment);
  if (typeof importHook !== 'function' || typeof resolveHook !== 'function') {
    throw new TypeError(
    'Compartment must be constructed with an importHook and a resolveHook for it to be able to load modules');

  }
};

const InertCompartment = function Compartment(
_endowments = {},
_modules = {},
_options = {})
{
  throw new TypeError(
  'Compartment.prototype.constructor is not a valid constructor.');

};

/**
 * @param {Compartment} compartment
 * @param {string} specifier
 */$h‍_once.InertCompartment(InertCompartment);
const compartmentImportNow = (compartment, specifier) => {
  const { execute, exportsProxy } = link(
  privateFields,
  moduleAliases,
  compartment,
  specifier);

  execute();
  return exportsProxy;
};

const CompartmentPrototype = {
  constructor: InertCompartment,

  get globalThis() {
    return weakmapGet(privateFields, this).globalObject;
  },

  get name() {
    return weakmapGet(privateFields, this).name;
  },

  /**
   * @param {string} source is a JavaScript program grammar construction.
   * @param {Object} [options]
   * @param {Array<Transform>} [options.transforms]
   * @param {boolean} [options.sloppyGlobalsMode]
   * @param {Object} [options.__moduleShimLexicals__]
   * @param {boolean} [options.__evadeHtmlCommentTest__]
   * @param {boolean} [options.__evadeImportExpressionTest__]
   * @param {boolean} [options.__rejectSomeDirectEvalExpressions__]
   */
  evaluate(source, options = {}) {
    const compartmentFields = weakmapGet(privateFields, this);
    return compartmentEvaluate(compartmentFields, source, options);
  },

  toString() {
    return '[object Compartment]';
  },

  /* eslint-disable-next-line no-underscore-dangle */
  __isKnownScopeProxy__(value) {
    const { knownScopeProxies } = weakmapGet(privateFields, this);
    return weaksetHas(knownScopeProxies, value);
  },

  /**
   * @param {Object} [options]
   * @param {boolean} [options.sloppyGlobalsMode]
   * @param {Object} [options.__moduleShimLexicals__]
   */
  /* eslint-disable-next-line no-underscore-dangle */
  __makeScopeProxy__(options = {}) {
    const compartmentFields = weakmapGet(privateFields, this);
    const scopeProxy = makeScopeProxy(compartmentFields, options);
    return { scopeProxy };
  },

  module(specifier) {
    if (typeof specifier !== 'string') {
      throw new TypeError('first argument of module() must be a string');
    }

    assertModuleHooks(this);

    const { exportsProxy } = getDeferredExports(
    this,
    weakmapGet(privateFields, this),
    moduleAliases,
    specifier);


    return exportsProxy;
  },

  async import(specifier) {
    if (typeof specifier !== 'string') {
      throw new TypeError('first argument of import() must be a string');
    }

    assertModuleHooks(this);

    return promiseThen(
    load(privateFields, moduleAliases, this, specifier),
    () => {
      // The namespace box is a contentious design and likely to be a breaking
      // change in an appropriately numbered future version.
      const namespace = compartmentImportNow(this, specifier);
      return { namespace };
    });

  },

  async load(specifier) {
    if (typeof specifier !== 'string') {
      throw new TypeError('first argument of load() must be a string');
    }

    assertModuleHooks(this);

    return load(privateFields, moduleAliases, this, specifier);
  },

  importNow(specifier) {
    if (typeof specifier !== 'string') {
      throw new TypeError('first argument of importNow() must be a string');
    }

    assertModuleHooks(this);

    return compartmentImportNow(this, specifier);
  } };$h‍_once.CompartmentPrototype(CompartmentPrototype);


defineProperties(InertCompartment, {
  prototype: { value: CompartmentPrototype } });


/**
 * @callback MakeCompartmentConstructor
 * @param {MakeCompartmentConstructor} targetMakeCompartmentConstructor
 * @param {Record<string, any>} intrinsics
 * @param {(object: Object) => void} markVirtualizedNativeFunction
 * @returns {Compartment['constructor']}
 */

/** @type {MakeCompartmentConstructor} */
const makeCompartmentConstructor = (
targetMakeCompartmentConstructor,
intrinsics,
markVirtualizedNativeFunction) =>
{
  function Compartment(endowments = {}, moduleMap = {}, options = {}) {
    if (new.target === undefined) {
      throw new TypeError(
      "Class constructor Compartment cannot be invoked without 'new'");

    }

    // Extract options, and shallow-clone transforms.
    const {
      name = '<unknown>',
      transforms = [],
      __shimTransforms__ = [],
      globalLexicals = {},
      resolveHook,
      importHook,
      moduleMapHook } =
    options;
    const globalTransforms = [...transforms, ...__shimTransforms__];

    // Map<FullSpecifier, ModuleCompartmentRecord>
    const moduleRecords = new Map();
    // Map<FullSpecifier, ModuleInstance>
    const instances = new Map();
    // Map<FullSpecifier, {ExportsProxy, ProxiedExports, activate()}>
    const deferredExports = new Map();

    // Validate given moduleMap.
    // The module map gets translated on-demand in module-load.js and the
    // moduleMap can be invalid in ways that cannot be detected in the
    // constructor, but these checks allow us to throw early for a better
    // developer experience.
    for (const [specifier, aliasNamespace] of entries(moduleMap || {})) {
      if (typeof aliasNamespace === 'string') {
        // TODO implement parent module record retrieval.
        throw new TypeError(
        `Cannot map module ${q(specifier)} to ${q(
        aliasNamespace)
        } in parent compartment`);

      } else if (weakmapGet(moduleAliases, aliasNamespace) === undefined) {
        // TODO create and link a synthetic module instance from the given
        // namespace object.
        throw ReferenceError(
        `Cannot map module ${q(
        specifier)
        } because it has no known compartment in this realm`);

      }
    }

    const globalObject = {};
    initGlobalObject(
    globalObject,
    intrinsics,
    sharedGlobalPropertyNames,
    targetMakeCompartmentConstructor,
    this.constructor.prototype,
    {
      globalTransforms,
      markVirtualizedNativeFunction });



    assign(globalObject, endowments);

    const invalidNames = arrayFilter(
    getOwnPropertyNames(globalLexicals),
    (identifier) => !isValidIdentifierName(identifier));

    if (invalidNames.length) {
      throw new TypeError(
      `Cannot create compartment with invalid names for global lexicals: ${arrayJoin(
      invalidNames,
      ', ')
      }; these names would not be lexically mentionable`);

    }

    const knownScopeProxies = new WeakSet();

    weakmapSet(privateFields, this, {
      name,
      globalTransforms,
      globalObject,
      knownScopeProxies,
      // The caller continues to own the globalLexicals object they passed to
      // the compartment constructor, but the compartment only respects the
      // original values and they are constants in the scope of evaluated
      // programs and executed modules.
      // This shallow copy captures only the values of enumerable own
      // properties, erasing accessors.
      // The snapshot is frozen to ensure that the properties are immutable
      // when transferred-by-property-descriptor onto local scope objects.
      globalLexicals: freeze({ ...globalLexicals }),
      resolveHook,
      importHook,
      moduleMap,
      moduleMapHook,
      moduleRecords,
      __shimTransforms__,
      deferredExports,
      instances });

  }

  Compartment.prototype = CompartmentPrototype;

  return Compartment;
};$h‍_once.makeCompartmentConstructor(makeCompartmentConstructor);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_FUNCTION,Float32Array,Map,Set,String,getOwnPropertyDescriptor,getPrototypeOf,iterateArray,iterateMap,iterateSet,iterateString,matchAllRegExp,matchAllSymbol,regexpPrototype,InertCompartment;$h‍_imports([["./commons.js", [["FERAL_FUNCTION", [$h‍_a => (FERAL_FUNCTION = $h‍_a)]],["Float32Array", [$h‍_a => (Float32Array = $h‍_a)]],["Map", [$h‍_a => (Map = $h‍_a)]],["Set", [$h‍_a => (Set = $h‍_a)]],["String", [$h‍_a => (String = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["getPrototypeOf", [$h‍_a => (getPrototypeOf = $h‍_a)]],["iterateArray", [$h‍_a => (iterateArray = $h‍_a)]],["iterateMap", [$h‍_a => (iterateMap = $h‍_a)]],["iterateSet", [$h‍_a => (iterateSet = $h‍_a)]],["iterateString", [$h‍_a => (iterateString = $h‍_a)]],["matchAllRegExp", [$h‍_a => (matchAllRegExp = $h‍_a)]],["matchAllSymbol", [$h‍_a => (matchAllSymbol = $h‍_a)]],["regexpPrototype", [$h‍_a => (regexpPrototype = $h‍_a)]]]],["./compartment-shim.js", [["InertCompartment", [$h‍_a => (InertCompartment = $h‍_a)]]]]]);   

















/**
 * Object.getConstructorOf()
 * Helper function to improve readability, similar to Object.getPrototypeOf().
 *
 * @param {Object} obj
 */
function getConstructorOf(obj) {
  return getPrototypeOf(obj).constructor;
}

// getAnonymousIntrinsics uses a utility function to construct an arguments
// object, since it cannot have one of its own and also be a const export.
function makeArguments() {
  // eslint-disable-next-line prefer-rest-params
  return arguments;
}

/**
 * getAnonymousIntrinsics()
 * Get the intrinsics not otherwise reachable by named own property
 * traversal from the global object.
 *
 * @returns {Object}
 */
const getAnonymousIntrinsics = () => {
  const InertFunction = FERAL_FUNCTION.prototype.constructor;

  // 9.2.4.1 %ThrowTypeError%

  const ThrowTypeError = getOwnPropertyDescriptor(makeArguments(), 'callee').
  get;

  // 21.1.5.2 The %StringIteratorPrototype% Object

  // eslint-disable-next-line no-new-wrappers
  const StringIteratorObject = iterateString(new String());
  const StringIteratorPrototype = getPrototypeOf(StringIteratorObject);

  // 21.2.7.1 The %RegExpStringIteratorPrototype% Object
  const RegExpStringIterator =
  regexpPrototype[matchAllSymbol] && matchAllRegExp(/./);
  const RegExpStringIteratorPrototype =
  RegExpStringIterator && getPrototypeOf(RegExpStringIterator);

  // 22.1.5.2 The %ArrayIteratorPrototype% Object

  // eslint-disable-next-line no-array-constructor
  const ArrayIteratorObject = iterateArray([]);
  const ArrayIteratorPrototype = getPrototypeOf(ArrayIteratorObject);

  // 22.2.1 The %TypedArray% Intrinsic Object

  const TypedArray = getPrototypeOf(Float32Array);

  // 23.1.5.2 The %MapIteratorPrototype% Object

  const MapIteratorObject = iterateMap(new Map());
  const MapIteratorPrototype = getPrototypeOf(MapIteratorObject);

  // 23.2.5.2 The %SetIteratorPrototype% Object

  const SetIteratorObject = iterateSet(new Set());
  const SetIteratorPrototype = getPrototypeOf(SetIteratorObject);

  // 25.1.2 The %IteratorPrototype% Object

  const IteratorPrototype = getPrototypeOf(ArrayIteratorPrototype);

  // 25.2.1 The GeneratorFunction Constructor

  // eslint-disable-next-line no-empty-function
  function* GeneratorFunctionInstance() {}
  const GeneratorFunction = getConstructorOf(GeneratorFunctionInstance);

  // 25.2.3 Properties of the GeneratorFunction Prototype Object

  const Generator = GeneratorFunction.prototype;

  // 25.3.1 The AsyncGeneratorFunction Constructor

  // eslint-disable-next-line no-empty-function
  async function* AsyncGeneratorFunctionInstance() {}
  const AsyncGeneratorFunction = getConstructorOf(
  AsyncGeneratorFunctionInstance);


  // 25.3.2.2 AsyncGeneratorFunction.prototype
  const AsyncGenerator = AsyncGeneratorFunction.prototype;
  // 25.5.1 Properties of the AsyncGenerator Prototype Object
  const AsyncGeneratorPrototype = AsyncGenerator.prototype;
  const AsyncIteratorPrototype = getPrototypeOf(AsyncGeneratorPrototype);

  // 25.7.1 The AsyncFunction Constructor

  // eslint-disable-next-line no-empty-function
  async function AsyncFunctionInstance() {}
  const AsyncFunction = getConstructorOf(AsyncFunctionInstance);

  const intrinsics = {
    '%InertFunction%': InertFunction,
    '%ArrayIteratorPrototype%': ArrayIteratorPrototype,
    '%InertAsyncFunction%': AsyncFunction,
    '%AsyncGenerator%': AsyncGenerator,
    '%InertAsyncGeneratorFunction%': AsyncGeneratorFunction,
    '%AsyncGeneratorPrototype%': AsyncGeneratorPrototype,
    '%AsyncIteratorPrototype%': AsyncIteratorPrototype,
    '%Generator%': Generator,
    '%InertGeneratorFunction%': GeneratorFunction,
    '%IteratorPrototype%': IteratorPrototype,
    '%MapIteratorPrototype%': MapIteratorPrototype,
    '%RegExpStringIteratorPrototype%': RegExpStringIteratorPrototype,
    '%SetIteratorPrototype%': SetIteratorPrototype,
    '%StringIteratorPrototype%': StringIteratorPrototype,
    '%ThrowTypeError%': ThrowTypeError,
    '%TypedArray%': TypedArray,
    '%InertCompartment%': InertCompartment };


  return intrinsics;
};$h‍_once.getAnonymousIntrinsics(getAnonymousIntrinsics);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Object,TypeError,WeakSet,arrayFilter,defineProperty,entries,freeze,getOwnPropertyDescriptor,getOwnPropertyDescriptors,globalThis,is,objectHasOwnProperty,values,weaksetHas,constantProperties,sharedGlobalPropertyNames,universalPropertyNames,whitelist;$h‍_imports([["./commons.js", [["Object", [$h‍_a => (Object = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["arrayFilter", [$h‍_a => (arrayFilter = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["entries", [$h‍_a => (entries = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["getOwnPropertyDescriptors", [$h‍_a => (getOwnPropertyDescriptors = $h‍_a)]],["globalThis", [$h‍_a => (globalThis = $h‍_a)]],["is", [$h‍_a => (is = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["values", [$h‍_a => (values = $h‍_a)]],["weaksetHas", [$h‍_a => (weaksetHas = $h‍_a)]]]],["./whitelist.js", [["constantProperties", [$h‍_a => (constantProperties = $h‍_a)]],["sharedGlobalPropertyNames", [$h‍_a => (sharedGlobalPropertyNames = $h‍_a)]],["universalPropertyNames", [$h‍_a => (universalPropertyNames = $h‍_a)]],["whitelist", [$h‍_a => (whitelist = $h‍_a)]]]]]);   























const isFunction = (obj) => typeof obj === 'function';

// Like defineProperty, but throws if it would modify an existing property.
// We use this to ensure that two conflicting attempts to define the same
// property throws, causing SES initialization to fail. Otherwise, a
// conflict between, for example, two of SES's internal whitelists might
// get masked as one overwrites the other. Accordingly, the thrown error
// complains of a "Conflicting definition".
function initProperty(obj, name, desc) {
  if (objectHasOwnProperty(obj, name)) {
    const preDesc = getOwnPropertyDescriptor(obj, name);
    if (
    !is(preDesc.value, desc.value) ||
    preDesc.get !== desc.get ||
    preDesc.set !== desc.set ||
    preDesc.writable !== desc.writable ||
    preDesc.enumerable !== desc.enumerable ||
    preDesc.configurable !== desc.configurable)
    {
      throw new TypeError(`Conflicting definitions of ${name}`);
    }
  }
  defineProperty(obj, name, desc);
}

// Like defineProperties, but throws if it would modify an existing property.
// This ensures that the intrinsics added to the intrinsics collector object
// graph do not overlap.
function initProperties(obj, descs) {
  for (const [name, desc] of entries(descs)) {
    initProperty(obj, name, desc);
  }
}

// sampleGlobals creates an intrinsics object, suitable for
// interinsicsCollector.addIntrinsics, from the named properties of a global
// object.
function sampleGlobals(globalObject, newPropertyNames) {
  const newIntrinsics = { __proto__: null };
  for (const [globalName, intrinsicName] of entries(newPropertyNames)) {
    if (objectHasOwnProperty(globalObject, globalName)) {
      newIntrinsics[intrinsicName] = globalObject[globalName];
    }
  }
  return newIntrinsics;
}

const makeIntrinsicsCollector = () => {
  const intrinsics = { __proto__: null };
  let pseudoNatives;

  const addIntrinsics = (newIntrinsics) => {
    initProperties(intrinsics, getOwnPropertyDescriptors(newIntrinsics));
  };
  freeze(addIntrinsics);

  // For each intrinsic, if it has a `.prototype` property, use the
  // whitelist to find out the intrinsic name for that prototype and add it
  // to the intrinsics.
  const completePrototypes = () => {
    for (const [name, intrinsic] of entries(intrinsics)) {
      if (intrinsic !== Object(intrinsic)) {
        // eslint-disable-next-line no-continue
        continue;
      }
      if (!objectHasOwnProperty(intrinsic, 'prototype')) {
        // eslint-disable-next-line no-continue
        continue;
      }
      const permit = whitelist[name];
      if (typeof permit !== 'object') {
        throw new TypeError(`Expected permit object at whitelist.${name}`);
      }
      const namePrototype = permit.prototype;
      if (!namePrototype) {
        throw new TypeError(`${name}.prototype property not whitelisted`);
      }
      if (
      typeof namePrototype !== 'string' ||
      !objectHasOwnProperty(whitelist, namePrototype))
      {
        throw new TypeError(`Unrecognized ${name}.prototype whitelist entry`);
      }
      const intrinsicPrototype = intrinsic.prototype;
      if (objectHasOwnProperty(intrinsics, namePrototype)) {
        if (intrinsics[namePrototype] !== intrinsicPrototype) {
          throw new TypeError(`Conflicting bindings of ${namePrototype}`);
        }
        // eslint-disable-next-line no-continue
        continue;
      }
      intrinsics[namePrototype] = intrinsicPrototype;
    }
  };
  freeze(completePrototypes);

  const finalIntrinsics = () => {
    freeze(intrinsics);
    pseudoNatives = new WeakSet(arrayFilter(values(intrinsics), isFunction));
    return intrinsics;
  };
  freeze(finalIntrinsics);

  const isPseudoNative = (obj) => {
    if (!pseudoNatives) {
      throw new TypeError(
      'isPseudoNative can only be called after finalIntrinsics');

    }
    return weaksetHas(pseudoNatives, obj);
  };
  freeze(isPseudoNative);

  const intrinsicsCollector = {
    addIntrinsics,
    completePrototypes,
    finalIntrinsics,
    isPseudoNative };

  freeze(intrinsicsCollector);

  addIntrinsics(constantProperties);
  addIntrinsics(sampleGlobals(globalThis, universalPropertyNames));

  return intrinsicsCollector;
};

/**
 * getGlobalIntrinsics()
 * Doesn't tame, delete, or modify anything. Samples globalObject to create an
 * intrinsics record containing only the whitelisted global variables, listed
 * by the intrinsic names appropriate for new globals, i.e., the globals of
 * newly constructed compartments.
 *
 * WARNING:
 * If run before lockdown, the returned intrinsics record will carry the
 * *original* unsafe (feral, untamed) bindings of these global variables.
 *
 * @param {Object} globalObject
 */$h‍_once.makeIntrinsicsCollector(makeIntrinsicsCollector);
const getGlobalIntrinsics = (globalObject) => {
  const { addIntrinsics, finalIntrinsics } = makeIntrinsicsCollector();

  addIntrinsics(sampleGlobals(globalObject, sharedGlobalPropertyNames));

  return finalIntrinsics();
};$h‍_once.getGlobalIntrinsics(getGlobalIntrinsics);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   $h‍_imports([]);   /**
 * @file Exports {@code enablements}, a recursively defined
 * JSON record defining the optimum set of intrinsics properties
 * that need to be "repaired" before hardening is applied on
 * enviromments subject to the override mistake.
 *
 * @author JF Paradis
 * @author Mark S. Miller
 */

/**
 * <p>Because "repairing" replaces data properties with accessors, every
 * time a repaired property is accessed, the associated getter is invoked,
 * which degrades the runtime performance of all code executing in the
 * repaired enviromment, compared to the non-repaired case. In order
 * to maintain performance, we only repair the properties of objects
 * for which hardening causes a breakage of their normal intended usage.
 *
 * There are three unwanted cases:
 * <ul>
 * <li>Overriding properties on objects typically used as records,
 *     namely {@code "Object"} and {@code "Array"}. In the case of arrays,
 *     the situation is unintentional, a given program might not be aware
 *     that non-numerical properties are stored on the underlying object
 *     instance, not on the array. When an object is typically used as a
 *     map, we repair all of its prototype properties.
 * <li>Overriding properties on objects that provide defaults on their
 *     prototype and that programs typically set using an assignment, such as
 *     {@code "Error.prototype.message"} and {@code "Function.prototype.name"}
 *     (both default to "").
 * <li>Setting-up a prototype chain, where a constructor is set to extend
 *     another one. This is typically set by assignment, for example
 *     {@code "Child.prototype.constructor = Child"}, instead of invoking
 *     Object.defineProperty();
 *
 * <p>Each JSON record enumerates the disposition of the properties on
 * some corresponding intrinsic object.
 *
 * <p>For each such record, the values associated with its property
 * names can be:
 * <ul>
 * <li>true, in which case this property is simply repaired. The
 *     value associated with that property is not traversed. For
 * 	   example, {@code "Function.prototype.name"} leads to true,
 *     meaning that the {@code "name"} property of {@code
 *     "Function.prototype"} should be repaired (which is needed
 *     when inheriting from @code{Function} and setting the subclass's
 *     {@code "prototype.name"} property). If the property is
 *     already an accessor property, it is not repaired (because
 *     accessors are not subject to the override mistake).
 * <li>"*", in which case this property is not repaired but the
 *     value associated with that property are traversed and repaired.
 * <li>Another record, in which case this property is not repaired
 *     and that next record represents the disposition of the object
 *     which is its value. For example,{@code "FunctionPrototype"}
 *     leads to another record explaining which properties {@code
 *     Function.prototype} need to be repaired.
 */

/**
 * Minimal enablements when all the code is modern and known not to
 * step into the override mistake, except for the following pervasive
 * cases.
 */
const minEnablements = {
  '%ObjectPrototype%': {
    toString: true },


  '%FunctionPrototype%': {
    toString: true // set by "rollup"
  },

  '%ErrorPrototype%': {
    name: true // set by "precond", "ava", "node-fetch"
  } };


/**
 * Moderate enablements are usually good enough for legacy compat.
 */$h‍_once.minEnablements(minEnablements);
const moderateEnablements = {
  '%ObjectPrototype%': {
    toString: true,
    valueOf: true },


  '%ArrayPrototype%': {
    toString: true,
    push: true // set by "Google Analytics"
  },

  // Function.prototype has no 'prototype' property to enable.
  // Function instances have their own 'name' and 'length' properties
  // which are configurable and non-writable. Thus, they are already
  // non-assignable anyway.
  '%FunctionPrototype%': {
    constructor: true, // set by "regenerator-runtime"
    bind: true, // set by "underscore", "express"
    toString: true // set by "rollup"
  },

  '%ErrorPrototype%': {
    constructor: true, // set by "fast-json-patch", "node-fetch"
    message: true,
    name: true, // set by "precond", "ava", "node-fetch", "node 14"
    toString: true // set by "bluebird"
  },

  '%TypeErrorPrototype%': {
    constructor: true, // set by "readable-stream"
    message: true, // set by "tape"
    name: true // set by "readable-stream", "node 14"
  },

  '%SyntaxErrorPrototype%': {
    message: true, // to match TypeErrorPrototype.message
    name: true // set by "node 14"
  },

  '%RangeErrorPrototype%': {
    message: true, // to match TypeErrorPrototype.message
    name: true // set by "node 14"
  },

  '%URIErrorPrototype%': {
    message: true, // to match TypeErrorPrototype.message
    name: true // set by "node 14"
  },

  '%EvalErrorPrototype%': {
    message: true, // to match TypeErrorPrototype.message
    name: true // set by "node 14"
  },

  '%ReferenceErrorPrototype%': {
    message: true, // to match TypeErrorPrototype.message
    name: true // set by "node 14"
  },

  '%PromisePrototype%': {
    constructor: true // set by "core-js"
  },

  '%TypedArrayPrototype%': '*', // set by https://github.com/feross/buffer

  '%Generator%': {
    constructor: true,
    name: true,
    toString: true },


  '%IteratorPrototype%': {
    toString: true } };



/**
 * The 'severe' enablement are needed because of issues tracked at
 * https://github.com/endojs/endo/issues/576
 *
 * They are like the `moderate` enablements except for the entries below.
 */$h‍_once.moderateEnablements(moderateEnablements);
const severeEnablements = {
  ...moderateEnablements,

  /**
   * Rollup(as used at least by vega) and webpack
   * (as used at least by regenerator) both turn exports into assignments
   * to a big `exports` object that inherits directly from
   * `Object.prototype`.Some of the exported names we've seen include
   * `hasOwnProperty`, `constructor`, and `toString`. But the strategy used
   * by rollup and webpack means potentionally turns any exported name
   * into an assignment rejected by the override mistake.That's why
   * we take the extreme step of enabling everything on`Object.prototype`.
   *
   * In addition, code doing inheritance manually will often override
   * the `constructor` property on the new prototype by assignment. We've
   * seen this several times.
   *
   * The cost of enabling all these is that they create a miserable debugging
   * experience. https://github.com/Agoric/agoric-sdk/issues/2324 explains
   * how it confused the Node console.
   *
   * The vscode debugger's object inspector shows the own data properties of
   * an object, which is typically what you want, but also shows both getter
   * and setter for every accessor property whether inherited or own.
   * With the `'*'` setting here, all the properties inherited from
   * `Object.prototype` are accessors, creating an unusable display as seen
   * at As explained at
   * https://github.com/endojs/endo/blob/master/packages/ses/lockdown-options.md#overridetaming-options
   * Open the triangles at the bottom of that section.
   */
  '%ObjectPrototype%': '*',

  /**
   * The widely used Buffer defined at https://github.com/feross/buffer
   * on initialization, manually creates the equivalent of a subclass of
   * `TypedArray`, which it then initializes by assignment. These assignments
   * include enough of the `TypeArray` methods that here, we just enable
   * them all.
   */
  '%TypedArrayPrototype%': '*' };$h‍_once.severeEnablements(severeEnablements);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Set,String,TypeError,arrayForEach,defineProperty,getOwnPropertyDescriptor,getOwnPropertyDescriptors,getOwnPropertyNames,isObject,objectHasOwnProperty,ownKeys,setHas,minEnablements,moderateEnablements,severeEnablements;$h‍_imports([["./commons.js", [["Set", [$h‍_a => (Set = $h‍_a)]],["String", [$h‍_a => (String = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["arrayForEach", [$h‍_a => (arrayForEach = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["getOwnPropertyDescriptors", [$h‍_a => (getOwnPropertyDescriptors = $h‍_a)]],["getOwnPropertyNames", [$h‍_a => (getOwnPropertyNames = $h‍_a)]],["isObject", [$h‍_a => (isObject = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["ownKeys", [$h‍_a => (ownKeys = $h‍_a)]],["setHas", [$h‍_a => (setHas = $h‍_a)]]]],["./enablements.js", [["minEnablements", [$h‍_a => (minEnablements = $h‍_a)]],["moderateEnablements", [$h‍_a => (moderateEnablements = $h‍_a)]],["severeEnablements", [$h‍_a => (severeEnablements = $h‍_a)]]]]]);   

























/**
 * For a special set of properties defined in the `enablement` whitelist,
 * `enablePropertyOverrides` ensures that the effect of freezing does not
 * suppress the ability to override these properties on derived objects by
 * simple assignment.
 *
 * Because of lack of sufficient foresight at the time, ES5 unfortunately
 * specified that a simple assignment to a non-existent property must fail if
 * it would override an non-writable data property of the same name in the
 * shadow of the prototype chain. In retrospect, this was a mistake, the
 * so-called "override mistake". But it is now too late and we must live with
 * the consequences.
 *
 * As a result, simply freezing an object to make it tamper proof has the
 * unfortunate side effect of breaking previously correct code that is
 * considered to have followed JS best practices, if this previous code used
 * assignment to override.
 *
 * For the enabled properties, `enablePropertyOverrides` effectively shims what
 * the assignment behavior would have been in the absence of the override
 * mistake. However, the shim produces an imperfect emulation. It shims the
 * behavior by turning these data properties into accessor properties, where
 * the accessor's getter and setter provide the desired behavior. For
 * non-reflective operations, the illusion is perfect. However, reflective
 * operations like `getOwnPropertyDescriptor` see the descriptor of an accessor
 * property rather than the descriptor of a data property. At the time of this
 * writing, this is the best we know how to do.
 *
 * To the getter of the accessor we add a property named
 * `'originalValue'` whose value is, as it says, the value that the
 * data property had before being converted to an accessor property. We add
 * this extra property to the getter for two reason:
 *
 * The harden algorithm walks the own properties reflectively, i.e., with
 * `getOwnPropertyDescriptor` semantics, rather than `[[Get]]` semantics. When
 * it sees an accessor property, it does not invoke the getter. Rather, it
 * proceeds to walk both the getter and setter as part of its transitive
 * traversal. Without this extra property, `enablePropertyOverrides` would have
 * hidden the original data property value from `harden`, which would be bad.
 * Instead, by exposing that value in an own data property on the getter,
 * `harden` finds and walks it anyway.
 *
 * We enable a form of cooperative emulation, giving reflective code an
 * opportunity to cooperate in upholding the illusion. When such cooperative
 * reflective code sees an accessor property, where the accessor's getter
 * has an `originalValue` property, it knows that the getter is
 * alleging that it is the result of the `enablePropertyOverrides` conversion
 * pattern, so it can decide to cooperatively "pretend" that it sees a data
 * property with that value.
 *
 * @param {Record<string, any>} intrinsics
 * @param {'min' | 'moderate' | 'severe'} overrideTaming
 * @param {Iterable<string | symbol>} [overrideDebug]
 */
function enablePropertyOverrides(
intrinsics,
overrideTaming,
overrideDebug = [])
{
  const debugProperties = new Set(overrideDebug);
  function enable(path, obj, prop, desc) {
    if ('value' in desc && desc.configurable) {
      const { value } = desc;

      function getter() {
        return value;
      }
      defineProperty(getter, 'originalValue', {
        value,
        writable: false,
        enumerable: false,
        configurable: false });


      const isDebug = setHas(debugProperties, prop);

      function setter(newValue) {
        if (obj === this) {
          throw new TypeError(
          `Cannot assign to read only property '${String(
          prop)
          }' of '${path}'`);

        }
        if (objectHasOwnProperty(this, prop)) {
          this[prop] = newValue;
        } else {
          if (isDebug) {
            // eslint-disable-next-line @endo/no-polymorphic-call
            console.error(new TypeError(`Override property ${prop}`));
          }
          defineProperty(this, prop, {
            value: newValue,
            writable: true,
            enumerable: true,
            configurable: true });

        }
      }

      defineProperty(obj, prop, {
        get: getter,
        set: setter,
        enumerable: desc.enumerable,
        configurable: desc.configurable });

    }
  }

  function enableProperty(path, obj, prop) {
    const desc = getOwnPropertyDescriptor(obj, prop);
    if (!desc) {
      return;
    }
    enable(path, obj, prop, desc);
  }

  function enableAllProperties(path, obj) {
    const descs = getOwnPropertyDescriptors(obj);
    if (!descs) {
      return;
    }
    // TypeScript does not allow symbols to be used as indexes because it
    // cannot recokon types of symbolized properties.
    // @ts-ignore
    arrayForEach(ownKeys(descs), (prop) => enable(path, obj, prop, descs[prop]));
  }

  function enableProperties(path, obj, plan) {
    for (const prop of getOwnPropertyNames(plan)) {
      const desc = getOwnPropertyDescriptor(obj, prop);
      if (!desc || desc.get || desc.set) {
        // No not a value property, nothing to do.
        // eslint-disable-next-line no-continue
        continue;
      }

      // Plan has no symbol keys and we use getOwnPropertyNames()
      // so `prop` cannot only be a string, not a symbol. We coerce it in place
      // with `String(..)` anyway just as good hygiene, since these paths are just
      // for diagnostic purposes.
      const subPath = `${path}.${String(prop)}`;
      const subPlan = plan[prop];

      if (subPlan === true) {
        enableProperty(subPath, obj, prop);
      } else if (subPlan === '*') {
        enableAllProperties(subPath, desc.value);
      } else if (isObject(subPlan)) {
        enableProperties(subPath, desc.value, subPlan);
      } else {
        throw new TypeError(`Unexpected override enablement plan ${subPath}`);
      }
    }
  }

  let plan;
  switch (overrideTaming) {
    case 'min':{
        plan = minEnablements;
        break;
      }
    case 'moderate':{
        plan = moderateEnablements;
        break;
      }
    case 'severe':{
        plan = severeEnablements;
        break;
      }
    default:{
        throw new TypeError(`unrecognized overrideTaming ${overrideTaming}`);
      }}


  // Do the repair.
  enableProperties('root', intrinsics, plan);
}$h‍_once.default(enablePropertyOverrides);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let WeakSet,arrayFilter,arrayMap,arrayPush,defineProperty,freeze,fromEntries,isError,stringEndsWith,weaksetAdd,weaksetHas;$h‍_imports([["../commons.js", [["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["arrayFilter", [$h‍_a => (arrayFilter = $h‍_a)]],["arrayMap", [$h‍_a => (arrayMap = $h‍_a)]],["arrayPush", [$h‍_a => (arrayPush = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["fromEntries", [$h‍_a => (fromEntries = $h‍_a)]],["isError", [$h‍_a => (isError = $h‍_a)]],["stringEndsWith", [$h‍_a => (stringEndsWith = $h‍_a)]],["weaksetAdd", [$h‍_a => (weaksetAdd = $h‍_a)]],["weaksetHas", [$h‍_a => (weaksetHas = $h‍_a)]]]],["./types.js", []],["./internal-types.js", []]]);   






















// For our internal debugging purposes, uncomment
// const internalDebugConsole = console;

// The whitelists of console methods, from:
// Whatwg "living standard" https://console.spec.whatwg.org/
// Node https://nodejs.org/dist/latest-v14.x/docs/api/console.html
// MDN https://developer.mozilla.org/en-US/docs/Web/API/Console_API
// TypeScript https://openstapps.gitlab.io/projectmanagement/interfaces/_node_modules__types_node_globals_d_.console.html
// Chrome https://developers.google.com/web/tools/chrome-devtools/console/api

// All console level methods have parameters (fmt?, ...args)
// where the argument sequence `fmt?, ...args` formats args according to
// fmt if fmt is a format string. Otherwise, it just renders them all as values
// separated by spaces.
// https://console.spec.whatwg.org/#formatter
// https://nodejs.org/docs/latest/api/util.html#util_util_format_format_args

// For the causal console, all occurrences of `fmt, ...args` or `...args` by
// itself must check for the presence of an error to ask the
// `loggedErrorHandler` to handle.
// In theory we should do a deep inspection to detect for example an array
// containing an error. We currently do not detect these and may never.

/** @typedef {keyof VirtualConsole | 'profile' | 'profileEnd'} ConsoleProps */

/** @type {readonly [ConsoleProps, LogSeverity | undefined][]} */
const consoleLevelMethods = freeze([
['debug', 'debug'], // (fmt?, ...args) verbose level on Chrome
['log', 'log'], // (fmt?, ...args) info level on Chrome
['info', 'info'], // (fmt?, ...args)
['warn', 'warn'], // (fmt?, ...args)
['error', 'error'], // (fmt?, ...args)

['trace', 'log'], // (fmt?, ...args)
['dirxml', 'log'], // (fmt?, ...args)
['group', 'log'], // (fmt?, ...args)
['groupCollapsed', 'log'] // (fmt?, ...args)
]);

/** @type {readonly [ConsoleProps, LogSeverity | undefined][]} */
const consoleOtherMethods = freeze([
['assert', 'error'], // (value, fmt?, ...args)
['timeLog', 'log'], // (label?, ...args) no fmt string

// Insensitive to whether any argument is an error. All arguments can pass
// thru to baseConsole as is.
['clear', undefined], // ()
['count', 'info'], // (label?)
['countReset', undefined], // (label?)
['dir', 'log'], // (item, options?)
['groupEnd', 'log'], // ()
// In theory tabular data may be or contain an error. However, we currently
// do not detect these and may never.
['table', 'log'], // (tabularData, properties?)
['time', 'info'], // (label?)
['timeEnd', 'info'], // (label?)

// Node Inspector only, MDN, and TypeScript, but not whatwg
['profile', undefined], // (label?)
['profileEnd', undefined], // (label?)
['timeStamp', undefined] // (label?)
]);

/** @type {readonly [ConsoleProps, LogSeverity | undefined][]} */
const consoleWhitelist = freeze([
...consoleLevelMethods,
...consoleOtherMethods]);


/**
 * consoleOmittedProperties is currently unused. I record and maintain it here
 * with the intention that it be treated like the `false` entries in the main
 * SES whitelist: that seeing these on the original console is expected, but
 * seeing anything else that's outside the whitelist is surprising and should
 * provide a diagnostic.
 *
 * const consoleOmittedProperties = freeze([
 *   'memory', // Chrome
 *   'exception', // FF, MDN
 *   '_ignoreErrors', // Node
 *   '_stderr', // Node
 *   '_stderrErrorHandler', // Node
 *   '_stdout', // Node
 *   '_stdoutErrorHandler', // Node
 *   '_times', // Node
 *   'context', // Chrome, Node
 *   'record', // Safari
 *   'recordEnd', // Safari
 *
 *   'screenshot', // Safari
 *   // Symbols
 *   '@@toStringTag', // Chrome: "Object", Safari: "Console"
 *   // A variety of other symbols also seen on Node
 * ]);
 */

// /////////////////////////////////////////////////////////////////////////////

/** @type {MakeLoggingConsoleKit} */$h‍_once.consoleWhitelist(consoleWhitelist);
const makeLoggingConsoleKit = (
loggedErrorHandler,
{ shouldResetForDebugging = false } = {}) =>
{
  if (shouldResetForDebugging) {
    // eslint-disable-next-line @endo/no-polymorphic-call
    loggedErrorHandler.resetErrorTagNum();
  }

  // Not frozen!
  let logArray = [];

  const loggingConsole = fromEntries(
  arrayMap(consoleWhitelist, ([name, _]) => {
    // Use an arrow function so that it doesn't come with its own name in
    // its printed form. Instead, we're hoping that tooling uses only
    // the `.name` property set below.
    /**
     * @param {...any} args
     */
    const method = (...args) => {
      arrayPush(logArray, [name, ...args]);
    };
    defineProperty(method, 'name', { value: name });
    return [name, freeze(method)];
  }));

  freeze(loggingConsole);

  const takeLog = () => {
    const result = freeze(logArray);
    logArray = [];
    return result;
  };
  freeze(takeLog);

  const typedLoggingConsole = /** @type {VirtualConsole} */loggingConsole;

  return freeze({ loggingConsole: typedLoggingConsole, takeLog });
};$h‍_once.makeLoggingConsoleKit(makeLoggingConsoleKit);
freeze(makeLoggingConsoleKit);


// /////////////////////////////////////////////////////////////////////////////

/** @type {ErrorInfo} */
const ErrorInfo = {
  NOTE: 'ERROR_NOTE:',
  MESSAGE: 'ERROR_MESSAGE:' };

freeze(ErrorInfo);

/**
 * The error annotations are sent to the baseConsole by calling some level
 * method. The 'debug' level seems best, because the Chrome console classifies
 * `debug` as verbose and does not show it by default. But we keep it symbolic
 * so we can change our mind. (On Node, `debug`, `log`, and `info` are aliases
 * for the same function and so will behave the same there.)
 */
const BASE_CONSOLE_LEVEL = 'debug';

/** @type {MakeCausalConsole} */$h‍_once.BASE_CONSOLE_LEVEL(BASE_CONSOLE_LEVEL);
const makeCausalConsole = (baseConsole, loggedErrorHandler) => {
  const {
    getStackString,
    tagError,
    takeMessageLogArgs,
    takeNoteLogArgsArray } =
  loggedErrorHandler;

  /**
   * @param {ReadonlyArray<any>} logArgs
   * @param {Array<any>} subErrorsSink
   * @returns {any}
   */
  const extractErrorArgs = (logArgs, subErrorsSink) => {
    const argTags = arrayMap(logArgs, (arg) => {
      if (isError(arg)) {
        arrayPush(subErrorsSink, arg);
        return `(${tagError(arg)})`;
      }
      return arg;
    });
    return argTags;
  };

  /**
   * @param {Error} error
   * @param {ErrorInfoKind} kind
   * @param {readonly any[]} logArgs
   * @param {Array<Error>} subErrorsSink
   */
  const logErrorInfo = (error, kind, logArgs, subErrorsSink) => {
    const errorTag = tagError(error);
    const errorName =
    kind === ErrorInfo.MESSAGE ? `${errorTag}:` : `${errorTag} ${kind}`;
    const argTags = extractErrorArgs(logArgs, subErrorsSink);
    // eslint-disable-next-line @endo/no-polymorphic-call
    baseConsole[BASE_CONSOLE_LEVEL](errorName, ...argTags);
  };

  /**
   * Logs the `subErrors` within a group name mentioning `optTag`.
   *
   * @param {Error[]} subErrors
   * @param {string | undefined} optTag
   * @returns {void}
   */
  const logSubErrors = (subErrors, optTag = undefined) => {
    if (subErrors.length === 0) {
      return;
    }
    if (subErrors.length === 1 && optTag === undefined) {
      // eslint-disable-next-line no-use-before-define
      logError(subErrors[0]);
      return;
    }
    let label;
    if (subErrors.length === 1) {
      label = `Nested error`;
    } else {
      label = `Nested ${subErrors.length} errors`;
    }
    if (optTag !== undefined) {
      label = `${label} under ${optTag}`;
    }
    // eslint-disable-next-line @endo/no-polymorphic-call
    baseConsole.group(label);
    try {
      for (const subError of subErrors) {
        // eslint-disable-next-line no-use-before-define
        logError(subError);
      }
    } finally {
      // eslint-disable-next-line @endo/no-polymorphic-call
      baseConsole.groupEnd();
    }
  };

  const errorsLogged = new WeakSet();

  /** @type {NoteCallback} */
  const noteCallback = (error, noteLogArgs) => {
    const subErrors = [];
    // Annotation arrived after the error has already been logged,
    // so just log the annotation immediately, rather than remembering it.
    logErrorInfo(error, ErrorInfo.NOTE, noteLogArgs, subErrors);
    logSubErrors(subErrors, tagError(error));
  };

  /**
   * @param {Error} error
   */
  const logError = (error) => {
    if (weaksetHas(errorsLogged, error)) {
      return;
    }
    const errorTag = tagError(error);
    weaksetAdd(errorsLogged, error);
    const subErrors = [];
    const messageLogArgs = takeMessageLogArgs(error);
    const noteLogArgsArray = takeNoteLogArgsArray(error, noteCallback);
    // Show the error's most informative error message
    if (messageLogArgs === undefined) {
      // If there is no message log args, then just show the message that
      // the error itself carries.
      // eslint-disable-next-line @endo/no-polymorphic-call
      baseConsole[BASE_CONSOLE_LEVEL](`${errorTag}:`, error.message);
    } else {
      // If there is one, we take it to be strictly more informative than the
      // message string carried by the error, so show it *instead*.
      logErrorInfo(error, ErrorInfo.MESSAGE, messageLogArgs, subErrors);
    }
    // After the message but before any other annotations, show the stack.
    let stackString = getStackString(error);
    if (
    typeof stackString === 'string' &&
    stackString.length >= 1 &&
    !stringEndsWith(stackString, '\n'))
    {
      stackString += '\n';
    }
    // eslint-disable-next-line @endo/no-polymorphic-call
    baseConsole[BASE_CONSOLE_LEVEL](stackString);
    // Show the other annotations on error
    for (const noteLogArgs of noteLogArgsArray) {
      logErrorInfo(error, ErrorInfo.NOTE, noteLogArgs, subErrors);
    }
    // explain all the errors seen in the messages already emitted.
    logSubErrors(subErrors, errorTag);
  };

  const levelMethods = arrayMap(consoleLevelMethods, ([level, _]) => {
    /**
     * @param {...any} logArgs
     */
    const levelMethod = (...logArgs) => {
      const subErrors = [];
      const argTags = extractErrorArgs(logArgs, subErrors);
      // @ts-ignore
      // eslint-disable-next-line @endo/no-polymorphic-call
      baseConsole[level](...argTags);
      logSubErrors(subErrors);
    };
    defineProperty(levelMethod, 'name', { value: level });
    return [level, freeze(levelMethod)];
  });
  const otherMethodNames = arrayFilter(
  consoleOtherMethods,
  ([name, _]) => name in baseConsole);

  const otherMethods = arrayMap(otherMethodNames, ([name, _]) => {
    /**
     * @param {...any} args
     */
    const otherMethod = (...args) => {
      // @ts-ignore
      // eslint-disable-next-line @endo/no-polymorphic-call
      baseConsole[name](...args);
      return undefined;
    };
    defineProperty(otherMethod, 'name', { value: name });
    return [name, freeze(otherMethod)];
  });

  const causalConsole = fromEntries([...levelMethods, ...otherMethods]);
  return (/** @type {VirtualConsole} */freeze(causalConsole));
};$h‍_once.makeCausalConsole(makeCausalConsole);
freeze(makeCausalConsole);


// /////////////////////////////////////////////////////////////////////////////

/** @type {FilterConsole} */
const filterConsole = (baseConsole, filter, _topic = undefined) => {
  // TODO do something with optional topic string
  const whitelist = arrayFilter(
  consoleWhitelist,
  ([name, _]) => name in baseConsole);

  const methods = arrayMap(whitelist, ([name, severity]) => {
    /**
     * @param {...any} args
     */
    const method = (...args) => {
      // eslint-disable-next-line @endo/no-polymorphic-call
      if (severity === undefined || filter.canLog(severity)) {
        // @ts-ignore
        // eslint-disable-next-line @endo/no-polymorphic-call
        baseConsole[name](...args);
      }
    };
    return [name, freeze(method)];
  });
  const filteringConsole = fromEntries(methods);
  return (/** @type {VirtualConsole} */freeze(filteringConsole));
};$h‍_once.filterConsole(filterConsole);
freeze(filterConsole);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let TypeError,globalThis,defaultHandler,makeCausalConsole;$h‍_imports([["../commons.js", [["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["globalThis", [$h‍_a => (globalThis = $h‍_a)]]]],["./assert.js", [["loggedErrorHandler", [$h‍_a => (defaultHandler = $h‍_a)]]]],["./console.js", [["makeCausalConsole", [$h‍_a => (makeCausalConsole = $h‍_a)]]]],["./types.js", []],["./internal-types.js", []]]);   







// eslint-disable-next-line no-restricted-globals
const originalConsole = console;

/**
 * Wrap console unless suppressed.
 * At the moment, the console is considered a host power in the start
 * compartment, and not a primordial. Hence it is absent from the whilelist
 * and bypasses the intrinsicsCollector.
 *
 * @param {"safe" | "unsafe"} consoleTaming
 * @param {"platform" | "exit" | "abort" | "report" | "none"} [errorTrapping]
 * @param {GetStackString=} optGetStackString
 */
const tameConsole = (
consoleTaming = 'safe',
errorTrapping = 'platform',
optGetStackString = undefined) =>
{
  if (consoleTaming !== 'safe' && consoleTaming !== 'unsafe') {
    throw new TypeError(`unrecognized consoleTaming ${consoleTaming}`);
  }

  if (consoleTaming === 'unsafe') {
    return { console: originalConsole };
  }
  let loggedErrorHandler;
  if (optGetStackString === undefined) {
    loggedErrorHandler = defaultHandler;
  } else {
    loggedErrorHandler = {
      ...defaultHandler,
      getStackString: optGetStackString };

  }
  const causalConsole = makeCausalConsole(originalConsole, loggedErrorHandler);

  // Attach platform-specific error traps such that any error that gets thrown
  // at top-of-turn (the bottom of stack) will get logged by our causal
  // console, revealing the diagnostic information associated with the error,
  // including the stack from when the error was created.

  // In the following Node.js and web browser cases, `process` and `window` are
  // spelled as `globalThis` properties to avoid the overweaning gaze of
  // Parcel, which dutifully installs an unnecessary `process` shim if we ever
  // utter that. That unnecessary shim forces the whole bundle into sloppy mode,
  // which in turn breaks SES's strict mode invariant.

  // Node.js
  if (errorTrapping !== 'none' && globalThis.process !== undefined) {
    // eslint-disable-next-line @endo/no-polymorphic-call
    globalThis.process.on('uncaughtException', (error) => {
      // causalConsole is born frozen so not vulnerable to method tampering.
      // eslint-disable-next-line @endo/no-polymorphic-call
      causalConsole.error(error);
      if (errorTrapping === 'platform' || errorTrapping === 'exit') {
        // eslint-disable-next-line @endo/no-polymorphic-call
        globalThis.process.exit(globalThis.process.exitCode || -1);
      } else if (errorTrapping === 'abort') {
        // eslint-disable-next-line @endo/no-polymorphic-call
        globalThis.process.abort();
      }
    });
  }

  // Browser
  if (errorTrapping !== 'none' && globalThis.window !== undefined) {
    // eslint-disable-next-line @endo/no-polymorphic-call
    globalThis.window.addEventListener('error', (event) => {
      // eslint-disable-next-line @endo/no-polymorphic-call
      event.preventDefault();
      // eslint-disable-next-line @endo/no-polymorphic-call
      const stackString = loggedErrorHandler.getStackString(event.error);
      // eslint-disable-next-line @endo/no-polymorphic-call
      causalConsole.error(stackString);
      if (errorTrapping === 'exit' || errorTrapping === 'abort') {
        globalThis.window.location.href = `about:blank`;
      }
    });
  }

  return { console: causalConsole };
};$h‍_once.tameConsole(tameConsole);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let WeakMap,WeakSet,apply,arrayFilter,arrayJoin,arrayMap,arraySlice,create,defineProperties,fromEntries,reflectSet,regexpExec,regexpTest,weakmapGet,weakmapHas,weakmapSet,weaksetAdd,weaksetHas;$h‍_imports([["../commons.js", [["WeakMap", [$h‍_a => (WeakMap = $h‍_a)]],["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["apply", [$h‍_a => (apply = $h‍_a)]],["arrayFilter", [$h‍_a => (arrayFilter = $h‍_a)]],["arrayJoin", [$h‍_a => (arrayJoin = $h‍_a)]],["arrayMap", [$h‍_a => (arrayMap = $h‍_a)]],["arraySlice", [$h‍_a => (arraySlice = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["fromEntries", [$h‍_a => (fromEntries = $h‍_a)]],["reflectSet", [$h‍_a => (reflectSet = $h‍_a)]],["regexpExec", [$h‍_a => (regexpExec = $h‍_a)]],["regexpTest", [$h‍_a => (regexpTest = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]],["weakmapHas", [$h‍_a => (weakmapHas = $h‍_a)]],["weakmapSet", [$h‍_a => (weakmapSet = $h‍_a)]],["weaksetAdd", [$h‍_a => (weaksetAdd = $h‍_a)]],["weaksetHas", [$h‍_a => (weaksetHas = $h‍_a)]]]]]);   




















// Whitelist names from https://v8.dev/docs/stack-trace-api
// Whitelisting only the names used by error-stack-shim/src/v8StackFrames
// callSiteToFrame to shim the error stack proposal.
const safeV8CallSiteMethodNames = [
// suppress 'getThis' definitely
'getTypeName',
// suppress 'getFunction' definitely
'getFunctionName',
'getMethodName',
'getFileName',
'getLineNumber',
'getColumnNumber',
'getEvalOrigin',
'isToplevel',
'isEval',
'isNative',
'isConstructor',
'isAsync',
// suppress 'isPromiseAll' for now
// suppress 'getPromiseIndex' for now

// Additional names found by experiment, absent from
// https://v8.dev/docs/stack-trace-api
'getPosition',
'getScriptNameOrSourceURL',

'toString' // TODO replace to use only whitelisted info
];

// TODO this is a ridiculously expensive way to attenuate callsites.
// Before that matters, we should switch to a reasonable representation.
const safeV8CallSiteFacet = (callSite) => {
  const methodEntry = (name) => {
    const method = callSite[name];
    return [name, () => apply(method, callSite, [])];
  };
  const o = fromEntries(arrayMap(safeV8CallSiteMethodNames, methodEntry));
  return create(o, {});
};

const safeV8SST = (sst) => arrayMap(sst, safeV8CallSiteFacet);

// If it has `/node_modules/` anywhere in it, on Node it is likely
// to be a dependent package of the current package, and so to
// be an infrastructure frame to be dropped from concise stack traces.
const FILENAME_NODE_DEPENDENTS_CENSOR = /\/node_modules\//;

// If it begins with `internal/` or `node:internal` then it is likely
// part of the node infrustructre itself, to be dropped from concise
// stack traces.
const FILENAME_NODE_INTERNALS_CENSOR = /^(?:node:)?internal\//;

// Frames within the `assert.js` package should be dropped from
// concise stack traces, as these are just steps towards creating the
// error object in question.
const FILENAME_ASSERT_CENSOR = /\/packages\/ses\/src\/error\/assert.js$/;

// Frames within the `eventual-send` shim should be dropped so that concise
// deep stacks omit the internals of the eventual-sending mechanism causing
// asynchronous messages to be sent.
// Note that the eventual-send package will move from agoric-sdk to
// Endo, so this rule will be of general interest.
const FILENAME_EVENTUAL_SEND_CENSOR = /\/packages\/eventual-send\/src\//;

// Any stack frame whose `fileName` matches any of these censor patterns
// will be omitted from concise stacks.
// TODO Enable users to configure FILENAME_CENSORS via `lockdown` options.
const FILENAME_CENSORS = [
FILENAME_NODE_DEPENDENTS_CENSOR,
FILENAME_NODE_INTERNALS_CENSOR,
FILENAME_ASSERT_CENSOR,
FILENAME_EVENTUAL_SEND_CENSOR];


// Should a stack frame with this as its fileName be included in a concise
// stack trace?
// Exported only so it can be unit tested.
// TODO Move so that it applies not just to v8.
const filterFileName = (fileName) => {
  if (!fileName) {
    // Stack frames with no fileName should appear in concise stack traces.
    return true;
  }
  for (const filter of FILENAME_CENSORS) {
    if (regexpTest(filter, fileName)) {
      return false;
    }
  }
  return true;
};

// The ad-hoc rule of the current pattern is that any likely-file-path or
// likely url-path prefix, ending in a `/.../` should get dropped.
// Anything to the left of the likely path text is kept.
// Everything to the right of `/.../` is kept. Thus
// `'Object.bar (/vat-v1/.../eventual-send/test/test-deep-send.js:13:21)'`
// simplifies to
// `'Object.bar (eventual-send/test/test-deep-send.js:13:21)'`.
//
// See thread starting at
// https://github.com/Agoric/agoric-sdk/issues/2326#issuecomment-773020389
$h‍_once.filterFileName(filterFileName);const CALLSITE_ELLIPSES_PATTERN = /^((?:.*[( ])?)[:/\w_-]*\/\.\.\.\/(.+)$/;

// The ad-hoc rule of the current pattern is that any likely-file-path or
// likely url-path prefix, ending in a `/` and prior to `package/` should get
// dropped.
// Anything to the left of the likely path prefix text is kept. `package/` and
// everything to its right is kept. Thus
// `'Object.bar (/Users/markmiller/src/ongithub/agoric/agoric-sdk/packages/eventual-send/test/test-deep-send.js:13:21)'`
// simplifies to
// `'Object.bar (packages/eventual-send/test/test-deep-send.js:13:21)'`.
// Note that `/packages/` is a convention for monorepos encouraged by
// lerna.
const CALLSITE_PACKAGES_PATTERN = /^((?:.*[( ])?)[:/\w_-]*\/(packages\/.+)$/;

// The use of these callSite patterns below assumes that any match will bind
// capture groups containing the parts of the original string we want
// to keep. The parts outside those capture groups will be dropped from concise
// stacks.
// TODO Enable users to configure CALLSITE_PATTERNS via `lockdown` options.
const CALLSITE_PATTERNS = [
CALLSITE_ELLIPSES_PATTERN,
CALLSITE_PACKAGES_PATTERN];


// For a stack frame that should be included in a concise stack trace, if
// `callSiteString` is the original stringified stack frame, return the
// possibly-shorter stringified stack frame that should be shown instead.
// Exported only so it can be unit tested.
// TODO Move so that it applies not just to v8.
const shortenCallSiteString = (callSiteString) => {
  for (const filter of CALLSITE_PATTERNS) {
    const match = regexpExec(filter, callSiteString);
    if (match) {
      return arrayJoin(arraySlice(match, 1), '');
    }
  }
  return callSiteString;
};$h‍_once.shortenCallSiteString(shortenCallSiteString);

const tameV8ErrorConstructor = (
OriginalError,
InitialError,
errorTaming,
stackFiltering) =>
{
  const originalCaptureStackTrace = OriginalError.captureStackTrace;

  // const callSiteFilter = _callSite => true;
  const callSiteFilter = (callSite) => {
    if (stackFiltering === 'verbose') {
      return true;
    }
    // eslint-disable-next-line @endo/no-polymorphic-call
    return filterFileName(callSite.getFileName());
  };

  const callSiteStringifier = (callSite) => {
    let callSiteString = `${callSite}`;
    if (stackFiltering === 'concise') {
      callSiteString = shortenCallSiteString(callSiteString);
    }
    return `\n  at ${callSiteString}`;
  };

  const stackStringFromSST = (_error, sst) =>
  arrayJoin(
  arrayMap(arrayFilter(sst, callSiteFilter), callSiteStringifier),
  '');


  // Mapping from error instance to the structured stack trace capturing the
  // stack for that instance.
  const ssts = new WeakMap();

  // Use concise methods to obtain named functions without constructors.
  const tamedMethods = {
    // The optional `optFn` argument is for cutting off the bottom of
    // the stack --- for capturing the stack only above the topmost
    // call to that function. Since this isn't the "real" captureStackTrace
    // but instead calls the real one, if no other cutoff is provided,
    // we cut this one off.
    captureStackTrace(error, optFn = tamedMethods.captureStackTrace) {
      if (typeof originalCaptureStackTrace === 'function') {
        // OriginalError.captureStackTrace is only on v8
        apply(originalCaptureStackTrace, OriginalError, [error, optFn]);
        return;
      }
      reflectSet(error, 'stack', '');
    },
    // Shim of proposed special power, to reside by default only
    // in the start compartment, for getting the stack traceback
    // string associated with an error.
    // See https://tc39.es/proposal-error-stacks/
    getStackString(error) {
      if (!weakmapHas(ssts, error)) {
        // eslint-disable-next-line no-void
        void error.stack;
      }
      const sst = weakmapGet(ssts, error);
      if (!sst) {
        return '';
      }
      return stackStringFromSST(error, sst);
    },
    prepareStackTrace(error, sst) {
      weakmapSet(ssts, error, sst);
      if (errorTaming === 'unsafe') {
        const stackString = stackStringFromSST(error, sst);
        return `${error}${stackString}`;
      }
      return '';
    } };


  // A prepareFn is a prepareStackTrace function.
  // An sst is a `structuredStackTrace`, which is an array of
  // callsites.
  // A user prepareFn is a prepareFn defined by a client of this API,
  // and provided by assigning to `Error.prepareStackTrace`.
  // A user prepareFn should only receive an attenuated sst, which
  // is an array of attenuated callsites.
  // A system prepareFn is the prepareFn created by this module to
  // be installed on the real `Error` constructor, to receive
  // an original sst, i.e., an array of unattenuated callsites.
  // An input prepareFn is a function the user assigns to
  // `Error.prepareStackTrace`, which might be a user prepareFn or
  // a system prepareFn previously obtained by reading
  // `Error.prepareStackTrace`.

  const defaultPrepareFn = tamedMethods.prepareStackTrace;

  OriginalError.prepareStackTrace = defaultPrepareFn;

  // A weakset branding some functions as system prepareFns, all of which
  // must be defined by this module, since they can receive an
  // unattenuated sst.
  const systemPrepareFnSet = new WeakSet([defaultPrepareFn]);

  const systemPrepareFnFor = (inputPrepareFn) => {
    if (weaksetHas(systemPrepareFnSet, inputPrepareFn)) {
      return inputPrepareFn;
    }
    // Use concise methods to obtain named functions without constructors.
    const systemMethods = {
      prepareStackTrace(error, sst) {
        weakmapSet(ssts, error, sst);
        return inputPrepareFn(error, safeV8SST(sst));
      } };

    weaksetAdd(systemPrepareFnSet, systemMethods.prepareStackTrace);
    return systemMethods.prepareStackTrace;
  };

  // Note `stackTraceLimit` accessor already defined by
  // tame-error-constructor.js
  defineProperties(InitialError, {
    captureStackTrace: {
      value: tamedMethods.captureStackTrace,
      writable: true,
      enumerable: false,
      configurable: true },

    prepareStackTrace: {
      get() {
        return OriginalError.prepareStackTrace;
      },
      set(inputPrepareStackTraceFn) {
        if (typeof inputPrepareStackTraceFn === 'function') {
          const systemPrepareFn = systemPrepareFnFor(inputPrepareStackTraceFn);
          OriginalError.prepareStackTrace = systemPrepareFn;
        } else {
          OriginalError.prepareStackTrace = defaultPrepareFn;
        }
      },
      enumerable: false,
      configurable: true } });



  return tamedMethods.getStackString;
};$h‍_once.tameV8ErrorConstructor(tameV8ErrorConstructor);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_ERROR,TypeError,apply,construct,defineProperties,setPrototypeOf,getOwnPropertyDescriptor,NativeErrors,tameV8ErrorConstructor;$h‍_imports([["../commons.js", [["FERAL_ERROR", [$h‍_a => (FERAL_ERROR = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["apply", [$h‍_a => (apply = $h‍_a)]],["construct", [$h‍_a => (construct = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["setPrototypeOf", [$h‍_a => (setPrototypeOf = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]]]],["../whitelist.js", [["NativeErrors", [$h‍_a => (NativeErrors = $h‍_a)]]]],["./tame-v8-error-constructor.js", [["tameV8ErrorConstructor", [$h‍_a => (tameV8ErrorConstructor = $h‍_a)]]]]]);   











// Present on at least FF. Proposed by Error-proposal. Not on SES whitelist
// so grab it before it is removed.
const stackDesc = getOwnPropertyDescriptor(FERAL_ERROR.prototype, 'stack');
const stackGetter = stackDesc && stackDesc.get;

// Use concise methods to obtain named functions without constructors.
const tamedMethods = {
  getStackString(error) {
    if (typeof stackGetter === 'function') {
      return apply(stackGetter, error, []);
    } else if ('stack' in error) {
      // The fallback is to just use the de facto `error.stack` if present
      return `${error.stack}`;
    }
    return '';
  } };


function tameErrorConstructor(
errorTaming = 'safe',
stackFiltering = 'concise')
{
  if (errorTaming !== 'safe' && errorTaming !== 'unsafe') {
    throw new TypeError(`unrecognized errorTaming ${errorTaming}`);
  }
  if (stackFiltering !== 'concise' && stackFiltering !== 'verbose') {
    throw new TypeError(`unrecognized stackFiltering ${stackFiltering}`);
  }
  const ErrorPrototype = FERAL_ERROR.prototype;

  const platform =
  typeof FERAL_ERROR.captureStackTrace === 'function' ? 'v8' : 'unknown';
  const { captureStackTrace: originalCaptureStackTrace } = FERAL_ERROR;

  const makeErrorConstructor = (_ = {}) => {
    // eslint-disable-next-line no-shadow
    const ResultError = function Error(...rest) {
      let error;
      if (new.target === undefined) {
        error = apply(FERAL_ERROR, this, rest);
      } else {
        error = construct(FERAL_ERROR, rest, new.target);
      }
      if (platform === 'v8') {
        // TODO Likely expensive!
        apply(originalCaptureStackTrace, FERAL_ERROR, [error, ResultError]);
      }
      return error;
    };
    defineProperties(ResultError, {
      length: { value: 1 },
      prototype: {
        value: ErrorPrototype,
        writable: false,
        enumerable: false,
        configurable: false } });


    return ResultError;
  };
  const InitialError = makeErrorConstructor({ powers: 'original' });
  const SharedError = makeErrorConstructor({ powers: 'none' });
  defineProperties(ErrorPrototype, {
    constructor: { value: SharedError }
    /* TODO
    stack: {
      get() {
        return '';
      },
      set(_) {
        // ignore
      },
    },
    */ });


  for (const NativeError of NativeErrors) {
    setPrototypeOf(NativeError, SharedError);
  }

  // https://v8.dev/docs/stack-trace-api#compatibility advises that
  // programmers can "always" set `Error.stackTraceLimit`
  // even on non-v8 platforms. On non-v8
  // it will have no effect, but this advice only makes sense
  // if the assignment itself does not fail, which it would
  // if `Error` were naively frozen. Hence, we add setters that
  // accept but ignore the assignment on non-v8 platforms.
  defineProperties(InitialError, {
    stackTraceLimit: {
      get() {
        if (typeof FERAL_ERROR.stackTraceLimit === 'number') {
          // FERAL_ERROR.stackTraceLimit is only on v8
          return FERAL_ERROR.stackTraceLimit;
        }
        return undefined;
      },
      set(newLimit) {
        if (typeof newLimit !== 'number') {
          // silently do nothing. This behavior doesn't precisely
          // emulate v8 edge-case behavior. But given the purpose
          // of this emulation, having edge cases err towards
          // harmless seems the safer option.
          return;
        }
        if (typeof FERAL_ERROR.stackTraceLimit === 'number') {
          // FERAL_ERROR.stackTraceLimit is only on v8
          FERAL_ERROR.stackTraceLimit = newLimit;
          // We place the useless return on the next line to ensure
          // that anything we place after the if in the future only
          // happens if the then-case does not.
          // eslint-disable-next-line no-useless-return
          return;
        }
      },
      // WTF on v8 stackTraceLimit is enumerable
      enumerable: false,
      configurable: true } });



  // The default SharedError much be completely powerless even on v8,
  // so the lenient `stackTraceLimit` accessor does nothing on all
  // platforms.
  defineProperties(SharedError, {
    stackTraceLimit: {
      get() {
        return undefined;
      },
      set(_newLimit) {
        // do nothing
      },
      enumerable: false,
      configurable: true } });



  let initialGetStackString = tamedMethods.getStackString;
  if (platform === 'v8') {
    initialGetStackString = tameV8ErrorConstructor(
    FERAL_ERROR,
    InitialError,
    errorTaming,
    stackFiltering);

  }
  return {
    '%InitialGetStackString%': initialGetStackString,
    '%InitialError%': InitialError,
    '%SharedError%': SharedError };

}$h‍_once.default(tameErrorConstructor);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Set,String,TypeError,WeakMap,WeakSet,arrayForEach,freeze,getOwnPropertyDescriptors,getPrototypeOf,isObject,objectHasOwnProperty,ownKeys,setAdd,setForEach,setHas,weakmapGet,weakmapSet,weaksetAdd,weaksetHas;$h‍_imports([["./commons.js", [["Set", [$h‍_a => (Set = $h‍_a)]],["String", [$h‍_a => (String = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["WeakMap", [$h‍_a => (WeakMap = $h‍_a)]],["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["arrayForEach", [$h‍_a => (arrayForEach = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["getOwnPropertyDescriptors", [$h‍_a => (getOwnPropertyDescriptors = $h‍_a)]],["getPrototypeOf", [$h‍_a => (getPrototypeOf = $h‍_a)]],["isObject", [$h‍_a => (isObject = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["ownKeys", [$h‍_a => (ownKeys = $h‍_a)]],["setAdd", [$h‍_a => (setAdd = $h‍_a)]],["setForEach", [$h‍_a => (setForEach = $h‍_a)]],["setHas", [$h‍_a => (setHas = $h‍_a)]],["weakmapGet", [$h‍_a => (weakmapGet = $h‍_a)]],["weakmapSet", [$h‍_a => (weakmapSet = $h‍_a)]],["weaksetAdd", [$h‍_a => (weaksetAdd = $h‍_a)]],["weaksetHas", [$h‍_a => (weaksetHas = $h‍_a)]]]]]);   












































/**
 * @typedef {import('../index.js').Harden} Harden
 */

/**
 * Create a `harden` function.
 *
 * @returns {Harden}
 */
const makeHardener = () => {
  const hardened = new WeakSet();

  const { harden } = {
    /**
     * @template T
     * @param {T} root
     * @returns {T}
     */
    harden(root) {
      const toFreeze = new Set();
      const paths = new WeakMap();

      // If val is something we should be freezing but aren't yet,
      // add it to toFreeze.
      /**
       * @param {any} val
       * @param {string} [path]
       */
      function enqueue(val, path = undefined) {
        if (!isObject(val)) {
          // ignore primitives
          return;
        }
        const type = typeof val;
        if (type !== 'object' && type !== 'function') {
          // future proof: break until someone figures out what it should do
          throw new TypeError(`Unexpected typeof: ${type}`);
        }
        if (weaksetHas(hardened, val) || setHas(toFreeze, val)) {
          // Ignore if this is an exit, or we've already visited it
          return;
        }
        // console.log(`adding ${val} to toFreeze`, val);
        setAdd(toFreeze, val);
        weakmapSet(paths, val, path);
      }

      /**
       * @param {any} obj
       */
      function freezeAndTraverse(obj) {
        // Now freeze the object to ensure reactive
        // objects such as proxies won't add properties
        // during traversal, before they get frozen.

        // Object are verified before being enqueued,
        // therefore this is a valid candidate.
        // Throws if this fails (strict mode).
        freeze(obj);

        // we rely upon certain commitments of Object.freeze and proxies here

        // get stable/immutable outbound links before a Proxy has a chance to do
        // something sneaky.
        const path = weakmapGet(paths, obj) || 'unknown';
        const descs = getOwnPropertyDescriptors(obj);
        const proto = getPrototypeOf(obj);
        enqueue(proto, `${path}.__proto__`);

        arrayForEach(ownKeys(descs), ( /** @type {string | symbol} */name) => {
          const pathname = `${path}.${String(name)}`;
          // The 'name' may be a symbol, and TypeScript doesn't like us to
          // index arbitrary symbols on objects, so we pretend they're just
          // strings.
          const desc = descs[/** @type {string} */name];
          // getOwnPropertyDescriptors is guaranteed to return well-formed
          // descriptors, but they still inherit from Object.prototype. If
          // someone has poisoned Object.prototype to add 'value' or 'get'
          // properties, then a simple 'if ("value" in desc)' or 'desc.value'
          // test could be confused. We use hasOwnProperty to be sure about
          // whether 'value' is present or not, which tells us for sure that
          // this is a data property.
          if (objectHasOwnProperty(desc, 'value')) {
            enqueue(desc.value, `${pathname}`);
          } else {
            enqueue(desc.get, `${pathname}(get)`);
            enqueue(desc.set, `${pathname}(set)`);
          }
        });
      }

      function dequeue() {
        // New values added before forEach() has finished will be visited.
        setForEach(toFreeze, freezeAndTraverse);
      }

      /** @param {any} value */
      function markHardened(value) {
        weaksetAdd(hardened, value);
      }

      function commit() {
        setForEach(toFreeze, markHardened);
      }

      enqueue(root);
      dequeue();
      // console.log("toFreeze set:", toFreeze);
      commit();

      return root;
    } };


  return harden;
};$h‍_once.makeHardener(makeHardener);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Date,TypeError,apply,construct,defineProperties;$h‍_imports([["./commons.js", [["Date", [$h‍_a => (Date = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["apply", [$h‍_a => (apply = $h‍_a)]],["construct", [$h‍_a => (construct = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]]]]]);   









function tameDateConstructor(dateTaming = 'safe') {
  if (dateTaming !== 'safe' && dateTaming !== 'unsafe') {
    throw new TypeError(`unrecognized dateTaming ${dateTaming}`);
  }
  const OriginalDate = Date;
  const DatePrototype = OriginalDate.prototype;

  // Use concise methods to obtain named functions without constructors.
  const tamedMethods = {
    now() {
      return NaN;
    } };


  // Tame the Date constructor.
  // Common behavior
  //   * new Date(x) coerces x into a number and then returns a Date
  //     for that number of millis since the epoch
  //   * new Date(NaN) returns a Date object which stringifies to
  //     'Invalid Date'
  //   * new Date(undefined) returns a Date object which stringifies to
  //     'Invalid Date'
  // OriginalDate (normal standard) behavior
  //   * Date(anything) gives a string with the current time
  //   * new Date() returns the current time, as a Date object
  // SharedDate behavior
  //   * Date(anything) returned 'Invalid Date'
  //   * new Date() returns a Date object which stringifies to
  //     'Invalid Date'
  const makeDateConstructor = ({ powers = 'none' } = {}) => {
    let ResultDate;
    if (powers === 'original') {
      // eslint-disable-next-line no-shadow
      ResultDate = function Date(...rest) {
        if (new.target === undefined) {
          return apply(OriginalDate, undefined, rest);
        }
        return construct(OriginalDate, rest, new.target);
      };
    } else {
      // eslint-disable-next-line no-shadow
      ResultDate = function Date(...rest) {
        if (new.target === undefined) {
          return 'Invalid Date';
        }
        if (rest.length === 0) {
          rest = [NaN];
        }
        return construct(OriginalDate, rest, new.target);
      };
    }

    defineProperties(ResultDate, {
      length: { value: 7 },
      prototype: {
        value: DatePrototype,
        writable: false,
        enumerable: false,
        configurable: false },

      parse: {
        value: Date.parse,
        writable: true,
        enumerable: false,
        configurable: true },

      UTC: {
        value: Date.UTC,
        writable: true,
        enumerable: false,
        configurable: true } });


    return ResultDate;
  };
  const InitialDate = makeDateConstructor({ powers: 'original' });
  const SharedDate = makeDateConstructor({ powers: 'none' });

  defineProperties(InitialDate, {
    now: {
      value: Date.now,
      writable: true,
      enumerable: false,
      configurable: true } });


  defineProperties(SharedDate, {
    now: {
      value: tamedMethods.now,
      writable: true,
      enumerable: false,
      configurable: true } });



  defineProperties(DatePrototype, {
    constructor: { value: SharedDate } });


  return {
    '%InitialDate%': InitialDate,
    '%SharedDate%': SharedDate };

}$h‍_once.default(tameDateConstructor);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let TypeError,globalThis,getOwnPropertyDescriptor,defineProperty;$h‍_imports([["./commons.js", [["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["globalThis", [$h‍_a => (globalThis = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]]]]]);Object.defineProperty(tameDomains, 'name', {value: "tameDomains"});$h‍_once.tameDomains(tameDomains);   








function tameDomains(domainTaming = 'safe') {
  if (domainTaming !== 'safe' && domainTaming !== 'unsafe') {
    throw new TypeError(`unrecognized domainTaming ${domainTaming}`);
  }

  if (domainTaming === 'unsafe') {
    return;
  }

  // Protect against the hazard presented by Node.js domains.
  if (typeof globalThis.process === 'object' && globalThis.process !== null) {
    // Check whether domains were initialized.
    const domainDescriptor = getOwnPropertyDescriptor(
    globalThis.process,
    'domain');

    if (domainDescriptor !== undefined && domainDescriptor.get !== undefined) {
      // The domain descriptor on Node.js initially has value: null, which
      // becomes a get, set pair after domains initialize.
      throw new TypeError(
      `SES failed to lockdown, Node.js domains have been initialized (SES_NO_DOMAINS)`);

    }
    // Prevent domains from initializing.
    // This is clunky because the exception thrown from the domains package does
    // not direct the user's gaze toward a knowledge base about the problem.
    // The domain module merely throws an exception when it attempts to define
    // the domain property of the process global during its initialization.
    // We have no better recourse because Node.js uses defineProperty too.
    defineProperty(globalThis.process, 'domain', {
      value: null,
      configurable: false,
      writable: false,
      enumerable: false });

  }
}
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_FUNCTION,SyntaxError,TypeError,defineProperties,getPrototypeOf,setPrototypeOf;$h‍_imports([["./commons.js", [["FERAL_FUNCTION", [$h‍_a => (FERAL_FUNCTION = $h‍_a)]],["SyntaxError", [$h‍_a => (SyntaxError = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["getPrototypeOf", [$h‍_a => (getPrototypeOf = $h‍_a)]],["setPrototypeOf", [$h‍_a => (setPrototypeOf = $h‍_a)]]]]]);   








// This module replaces the original `Function` constructor, and the original
// `%GeneratorFunction%`, `%AsyncFunction%` and `%AsyncGeneratorFunction%`,
// with safe replacements that throw if invoked.
//
// These are all reachable via syntax, so it isn't sufficient to just
// replace global properties with safe versions. Our main goal is to prevent
// access to the `Function` constructor through these starting points.
//
// After modules block is done, the originals must no longer be reachable,
// unless a copy has been made, and functions can only be created by syntax
// (using eval) or by invoking a previously saved reference to the originals.
//
// Typically, this module will not be used directly, but via the
// [lockdown - shim] which handles all necessary repairs and taming in SES.
//
// Relation to ECMA specifications
//
// The taming of constructors really wants to be part of the standard, because
// new constructors may be added in the future, reachable from syntax, and this
// list must be updated to match.
//
// In addition, the standard needs to define four new intrinsics for the safe
// replacement functions. See [./whitelist intrinsics].
//
// Adapted from SES/Caja
// Copyright (C) 2011 Google Inc.
// https://github.com/google/caja/blob/master/src/com/google/caja/ses/startSES.js
// https://github.com/google/caja/blob/master/src/com/google/caja/ses/repairES5.js

/**
 * tameFunctionConstructors()
 * This block replaces the original Function constructor, and the original
 * %GeneratorFunction% %AsyncFunction% and %AsyncGeneratorFunction%, with
 * safe replacements that throw if invoked.
 */
function tameFunctionConstructors() {
  try {
    // Verify that the method is not callable.
    // eslint-disable-next-line @endo/no-polymorphic-call
    FERAL_FUNCTION.prototype.constructor('return 1');
  } catch (ignore) {
    // Throws, no need to patch.
    return {};
  }

  const newIntrinsics = {};

  /*
   * The process to repair constructors:
   * 1. Create an instance of the function by evaluating syntax
   * 2. Obtain the prototype from the instance
   * 3. Create a substitute tamed constructor
   * 4. Replace the original constructor with the tamed constructor
   * 5. Replace tamed constructor prototype property with the original one
   * 6. Replace its [[Prototype]] slot with the tamed constructor of Function
   */
  function repairFunction(name, intrinsicName, declaration) {
    let FunctionInstance;
    try {
      // eslint-disable-next-line no-eval, no-restricted-globals
      FunctionInstance = (0, eval)(declaration);
    } catch (e) {
      if (e instanceof SyntaxError) {
        // Prevent failure on platforms where async and/or generators
        // are not supported.
        return;
      }
      // Re-throw
      throw e;
    }
    const FunctionPrototype = getPrototypeOf(FunctionInstance);

    // Prevents the evaluation of source when calling constructor on the
    // prototype of functions.
    // eslint-disable-next-line func-names
    const InertConstructor = function () {
      throw new TypeError(
      'Function.prototype.constructor is not a valid constructor.');

    };
    defineProperties(InertConstructor, {
      prototype: { value: FunctionPrototype },
      name: {
        value: name,
        writable: false,
        enumerable: false,
        configurable: true } });



    defineProperties(FunctionPrototype, {
      constructor: { value: InertConstructor } });


    // Reconstructs the inheritance among the new tamed constructors
    // to mirror the original specified in normal JS.
    if (InertConstructor !== FERAL_FUNCTION.prototype.constructor) {
      setPrototypeOf(InertConstructor, FERAL_FUNCTION.prototype.constructor);
    }

    newIntrinsics[intrinsicName] = InertConstructor;
  }

  // Here, the order of operation is important: Function needs to be repaired
  // first since the other repaired constructors need to inherit from the
  // tamed Function function constructor.

  repairFunction('Function', '%InertFunction%', '(function(){})');
  repairFunction(
  'GeneratorFunction',
  '%InertGeneratorFunction%',
  '(function*(){})');

  repairFunction(
  'AsyncFunction',
  '%InertAsyncFunction%',
  '(async function(){})');

  repairFunction(
  'AsyncGeneratorFunction',
  '%InertAsyncGeneratorFunction%',
  '(async function*(){})');


  return newIntrinsics;
}$h‍_once.default(tameFunctionConstructors);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let WeakSet,defineProperty,freeze,functionPrototype,functionToString,stringEndsWith,weaksetAdd,weaksetHas;$h‍_imports([["./commons.js", [["WeakSet", [$h‍_a => (WeakSet = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["freeze", [$h‍_a => (freeze = $h‍_a)]],["functionPrototype", [$h‍_a => (functionPrototype = $h‍_a)]],["functionToString", [$h‍_a => (functionToString = $h‍_a)]],["stringEndsWith", [$h‍_a => (stringEndsWith = $h‍_a)]],["weaksetAdd", [$h‍_a => (weaksetAdd = $h‍_a)]],["weaksetHas", [$h‍_a => (weaksetHas = $h‍_a)]]]]]);   










const nativeSuffix = ') { [native code] }';

// Note: Top level mutable state. Does not make anything worse, since the
// patching of `Function.prototype.toString` is also globally stateful. We
// use this top level state so that multiple calls to `tameFunctionToString` are
// idempotent, rather than creating redundant indirections.
let markVirtualizedNativeFunction;

/**
 * Replace `Function.prototype.toString` with one that recognizes
 * shimmed functions as honorary native functions.
 */
const tameFunctionToString = () => {
  if (markVirtualizedNativeFunction === undefined) {
    const virtualizedNativeFunctions = new WeakSet();

    const tamingMethods = {
      toString() {
        const str = functionToString(this, []);
        if (
        stringEndsWith(str, nativeSuffix) ||
        !weaksetHas(virtualizedNativeFunctions, this))
        {
          return str;
        }
        return `function ${this.name}() { [native code] }`;
      } };


    defineProperty(functionPrototype, 'toString', {
      value: tamingMethods.toString });


    markVirtualizedNativeFunction = freeze((func) =>
    weaksetAdd(virtualizedNativeFunctions, func));

  }
  return markVirtualizedNativeFunction;
};$h‍_once.tameFunctionToString(tameFunctionToString);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Object,String,TypeError,defineProperty,getOwnPropertyNames,regexpExec,assert;$h‍_imports([["./commons.js", [["Object", [$h‍_a => (Object = $h‍_a)]],["String", [$h‍_a => (String = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["defineProperty", [$h‍_a => (defineProperty = $h‍_a)]],["getOwnPropertyNames", [$h‍_a => (getOwnPropertyNames = $h‍_a)]],["regexpExec", [$h‍_a => (regexpExec = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   









const { details: d, quote: q } = assert;

const localePattern = /^(\w*[a-z])Locale([A-Z]\w*)$/;

// Use concise methods to obtain named functions without constructor
// behavior or `.prototype` property.
const tamedMethods = {
  // See https://tc39.es/ecma262/#sec-string.prototype.localecompare
  localeCompare(that) {
    if (this === null || this === undefined) {
      throw new TypeError(
      'Cannot localeCompare with null or undefined "this" value');

    }
    const s = `${this}`;
    that = `${that}`;
    if (s < that) {
      return -1;
    }
    if (s > that) {
      return 1;
    }
    assert(s === that, d`expected ${q(s)} and ${q(that)} to compare`);
    return 0;
  } };


const nonLocaleCompare = tamedMethods.localeCompare;

function tameLocaleMethods(intrinsics, localeTaming = 'safe') {
  if (localeTaming !== 'safe' && localeTaming !== 'unsafe') {
    throw new TypeError(`unrecognized localeTaming ${localeTaming}`);
  }
  if (localeTaming === 'unsafe') {
    return;
  }

  defineProperty(String.prototype, 'localeCompare', {
    value: nonLocaleCompare });


  for (const intrinsicName of getOwnPropertyNames(intrinsics)) {
    const intrinsic = intrinsics[intrinsicName];
    if (intrinsic === Object(intrinsic)) {
      for (const methodName of getOwnPropertyNames(intrinsic)) {
        const match = regexpExec(localePattern, methodName);
        if (match) {
          assert(
          typeof intrinsic[methodName] === 'function',
          d`expected ${q(methodName)} to be a function`);

          const nonLocaleMethodName = `${match[1]}${match[2]}`;
          const method = intrinsic[nonLocaleMethodName];
          assert(
          typeof method === 'function',
          d`function ${q(nonLocaleMethodName)} not found`);

          defineProperty(intrinsic, methodName, { value: method });
        }
      }
    }
  }
}$h‍_once.default(tameLocaleMethods);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let Math,TypeError,create,getOwnPropertyDescriptors,objectPrototype;$h‍_imports([["./commons.js", [["Math", [$h‍_a => (Math = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["create", [$h‍_a => (create = $h‍_a)]],["getOwnPropertyDescriptors", [$h‍_a => (getOwnPropertyDescriptors = $h‍_a)]],["objectPrototype", [$h‍_a => (objectPrototype = $h‍_a)]]]]]);   







function tameMathObject(mathTaming = 'safe') {
  if (mathTaming !== 'safe' && mathTaming !== 'unsafe') {
    throw new TypeError(`unrecognized mathTaming ${mathTaming}`);
  }
  const originalMath = Math;
  const initialMath = originalMath; // to follow the naming pattern

  const { random: _, ...otherDescriptors } = getOwnPropertyDescriptors(
  originalMath);


  const sharedMath = create(objectPrototype, otherDescriptors);

  return {
    '%InitialMath%': initialMath,
    '%SharedMath%': sharedMath };

}$h‍_once.default(tameMathObject);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let FERAL_REG_EXP,TypeError,construct,defineProperties,getOwnPropertyDescriptor,speciesSymbol;$h‍_imports([["./commons.js", [["FERAL_REG_EXP", [$h‍_a => (FERAL_REG_EXP = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["construct", [$h‍_a => (construct = $h‍_a)]],["defineProperties", [$h‍_a => (defineProperties = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["speciesSymbol", [$h‍_a => (speciesSymbol = $h‍_a)]]]]]);   








function tameRegExpConstructor(regExpTaming = 'safe') {
  if (regExpTaming !== 'safe' && regExpTaming !== 'unsafe') {
    throw new TypeError(`unrecognized regExpTaming ${regExpTaming}`);
  }
  const RegExpPrototype = FERAL_REG_EXP.prototype;

  const makeRegExpConstructor = (_ = {}) => {
    // RegExp has non-writable static properties we need to omit.
    const ResultRegExp = function RegExp(...rest) {
      if (new.target === undefined) {
        return FERAL_REG_EXP(...rest);
      }
      return construct(FERAL_REG_EXP, rest, new.target);
    };

    defineProperties(ResultRegExp, {
      length: { value: 2 },
      prototype: {
        value: RegExpPrototype,
        writable: false,
        enumerable: false,
        configurable: false },

      [speciesSymbol]: getOwnPropertyDescriptor(FERAL_REG_EXP, speciesSymbol) });

    return ResultRegExp;
  };

  const InitialRegExp = makeRegExpConstructor();
  const SharedRegExp = makeRegExpConstructor();

  if (regExpTaming !== 'unsafe') {
    delete RegExpPrototype.compile;
  }
  defineProperties(RegExpPrototype, {
    constructor: { value: SharedRegExp } });


  return {
    '%InitialRegExp%': InitialRegExp,
    '%SharedRegExp%': SharedRegExp };

}$h‍_once.default(tameRegExpConstructor);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let whitelist,FunctionInstance,isAccessorPermit,String,TypeError,arrayIncludes,getOwnPropertyDescriptor,getPrototypeOf,isObject,objectHasOwnProperty,ownKeys,stringSlice;$h‍_imports([["./whitelist.js", [["whitelist", [$h‍_a => (whitelist = $h‍_a)]],["FunctionInstance", [$h‍_a => (FunctionInstance = $h‍_a)]],["isAccessorPermit", [$h‍_a => (isAccessorPermit = $h‍_a)]]]],["./commons.js", [["String", [$h‍_a => (String = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["arrayIncludes", [$h‍_a => (arrayIncludes = $h‍_a)]],["getOwnPropertyDescriptor", [$h‍_a => (getOwnPropertyDescriptor = $h‍_a)]],["getPrototypeOf", [$h‍_a => (getPrototypeOf = $h‍_a)]],["isObject", [$h‍_a => (isObject = $h‍_a)]],["objectHasOwnProperty", [$h‍_a => (objectHasOwnProperty = $h‍_a)]],["ownKeys", [$h‍_a => (ownKeys = $h‍_a)]],["stringSlice", [$h‍_a => (stringSlice = $h‍_a)]]]]]);   

























































/**
 * asStringPropertyName()
 *
 * @param {string} path
 * @param {string | symbol} prop
 */
function asStringPropertyName(path, prop) {
  if (typeof prop === 'string') {
    return prop;
  }

  if (typeof prop === 'symbol') {
    return `@@${stringSlice(String(prop), 14, -1)}`;
  }

  throw new TypeError(`Unexpected property name type ${path} ${prop}`);
}

/**
 * whitelistIntrinsics()
 * Removes all non-allowed properties found by recursively and
 * reflectively walking own property chains.
 *
 * @param {Object} intrinsics
 * @param {(Object) => void} markVirtualizedNativeFunction
 */
function whitelistIntrinsics(
intrinsics,
markVirtualizedNativeFunction)
{
  // These primities are allowed allowed for permits.
  const primitives = ['undefined', 'boolean', 'number', 'string', 'symbol'];

  /*
   * visitPrototype()
   * Validate the object's [[prototype]] against a permit.
   */
  function visitPrototype(path, obj, protoName) {
    if (!isObject(obj)) {
      throw new TypeError(`Object expected: ${path}, ${obj}, ${protoName}`);
    }
    const proto = getPrototypeOf(obj);

    // Null prototype.
    if (proto === null && protoName === null) {
      return;
    }

    // Assert: protoName, if provided, is a string.
    if (protoName !== undefined && typeof protoName !== 'string') {
      throw new TypeError(`Malformed whitelist permit ${path}.__proto__`);
    }

    // If permit not specified, default to Object.prototype.
    if (proto === intrinsics[protoName || '%ObjectPrototype%']) {
      return;
    }

    // We can't clean [[prototype]], therefore abort.
    throw new TypeError(
    `Unexpected intrinsic ${path}.__proto__ at ${protoName}`);

  }

  /*
   * isAllowedPropertyValue()
   * Whitelist a single property value against a permit.
   */
  function isAllowedPropertyValue(path, value, prop, permit) {
    if (typeof permit === 'object') {
      // eslint-disable-next-line no-use-before-define
      visitProperties(path, value, permit);
      // The property is allowed.
      return true;
    }

    if (permit === false) {
      // A boolan 'false' permit specifies the removal of a property.
      // We require a more specific permit instead of allowing 'true'.
      return false;
    }

    if (typeof permit === 'string') {
      // A string permit can have one of two meanings:

      if (prop === 'prototype' || prop === 'constructor') {
        // For prototype and constructor value properties, the permit
        // is the name of an intrinsic.
        // Assumption: prototype and constructor cannot be primitives.
        // Assert: the permit is the name of an intrinsic.
        // Assert: the property value is equal to that intrinsic.

        if (objectHasOwnProperty(intrinsics, permit)) {
          if (value !== intrinsics[permit]) {
            throw new TypeError(`Does not match whitelist ${path}`);
          }
          return true;
        }
      } else {
        // For all other properties, the permit is the name of a primitive.
        // Assert: the permit is the name of a primitive.
        // Assert: the property value type is equal to that primitive.

        // eslint-disable-next-line no-lonely-if
        if (arrayIncludes(primitives, permit)) {
          // eslint-disable-next-line valid-typeof
          if (typeof value !== permit) {
            throw new TypeError(
            `At ${path} expected ${permit} not ${typeof value}`);

          }
          return true;
        }
      }
    }

    throw new TypeError(`Unexpected whitelist permit ${permit} at ${path}`);
  }

  /*
   * isAllowedProperty()
   * Check whether a single property is allowed.
   */
  function isAllowedProperty(path, obj, prop, permit) {
    const desc = getOwnPropertyDescriptor(obj, prop);

    // Is this a value property?
    if (objectHasOwnProperty(desc, 'value')) {
      if (isAccessorPermit(permit)) {
        throw new TypeError(`Accessor expected at ${path}`);
      }
      return isAllowedPropertyValue(path, desc.value, prop, permit);
    }
    if (!isAccessorPermit(permit)) {
      throw new TypeError(`Accessor not expected at ${path}`);
    }
    return (
      isAllowedPropertyValue(`${path}<get>`, desc.get, prop, permit.get) &&
      isAllowedPropertyValue(`${path}<set>`, desc.set, prop, permit.set));

  }

  /*
   * getSubPermit()
   */
  function getSubPermit(obj, permit, prop) {
    const permitProp = prop === '__proto__' ? '--proto--' : prop;
    if (objectHasOwnProperty(permit, permitProp)) {
      return permit[permitProp];
    }

    if (typeof obj === 'function') {
      markVirtualizedNativeFunction(obj);
      if (objectHasOwnProperty(FunctionInstance, permitProp)) {
        return FunctionInstance[permitProp];
      }
    }

    return undefined;
  }

  /*
   * visitProperties()
   * Visit all properties for a permit.
   */
  function visitProperties(path, obj, permit) {
    if (obj === undefined) {
      return;
    }

    const protoName = permit['[[Proto]]'];
    visitPrototype(path, obj, protoName);

    for (const prop of ownKeys(obj)) {
      const propString = asStringPropertyName(path, prop);
      const subPath = `${path}.${propString}`;
      const subPermit = getSubPermit(obj, permit, propString);

      if (!subPermit || !isAllowedProperty(subPath, obj, prop, subPermit)) {
        // Either the object lacks a permit or the object doesn't match the
        // permit.
        // If the permit is specifically false, not merely undefined,
        // this is a property we expect to see because we know it exists in
        // some environments and we have expressly decided to exclude it.
        // Any other disallowed property is one we have not audited and we log
        // that we are removing it so we know to look into it, as happens when
        // the language evolves new features to existing intrinsics.
        if (subPermit !== false) {
          // This call to `console.log` is intentional. It is not a vestige
          // of a debugging attempt. See the comment at top of file for an
          // explanation.
          // eslint-disable-next-line @endo/no-polymorphic-call
          console.log(`Removing ${subPath}`);
        }
        try {
          delete obj[prop];
        } catch (err) {
          if (prop in obj) {
            // eslint-disable-next-line @endo/no-polymorphic-call
            console.error(`failed to delete ${subPath}`, err);
          } else {
            // eslint-disable-next-line @endo/no-polymorphic-call
            console.error(`deleting ${subPath} threw`, err);
          }
          throw err;
        }
      }
    }
  }

  // Start path with 'intrinsics' to clarify that properties are not
  // removed from the global object by the whitelisting operation.
  visitProperties('intrinsics', intrinsics, whitelist);
}$h‍_once.default(whitelistIntrinsics);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let globalThis,is,keys,ownKeys,makeHardener,makeIntrinsicsCollector,whitelistIntrinsics,tameFunctionConstructors,tameDateConstructor,tameMathObject,tameRegExpConstructor,enablePropertyOverrides,tameLocaleMethods,initGlobalObject,initialGlobalPropertyNames,tameFunctionToString,tameDomains,tameConsole,tameErrorConstructor,assert,makeAssert;$h‍_imports([["./commons.js", [["globalThis", [$h‍_a => (globalThis = $h‍_a)]],["is", [$h‍_a => (is = $h‍_a)]],["keys", [$h‍_a => (keys = $h‍_a)]],["ownKeys", [$h‍_a => (ownKeys = $h‍_a)]]]],["./make-hardener.js", [["makeHardener", [$h‍_a => (makeHardener = $h‍_a)]]]],["./intrinsics.js", [["makeIntrinsicsCollector", [$h‍_a => (makeIntrinsicsCollector = $h‍_a)]]]],["./whitelist-intrinsics.js", [["default", [$h‍_a => (whitelistIntrinsics = $h‍_a)]]]],["./tame-function-constructors.js", [["default", [$h‍_a => (tameFunctionConstructors = $h‍_a)]]]],["./tame-date-constructor.js", [["default", [$h‍_a => (tameDateConstructor = $h‍_a)]]]],["./tame-math-object.js", [["default", [$h‍_a => (tameMathObject = $h‍_a)]]]],["./tame-regexp-constructor.js", [["default", [$h‍_a => (tameRegExpConstructor = $h‍_a)]]]],["./enable-property-overrides.js", [["default", [$h‍_a => (enablePropertyOverrides = $h‍_a)]]]],["./tame-locale-methods.js", [["default", [$h‍_a => (tameLocaleMethods = $h‍_a)]]]],["./global-object.js", [["initGlobalObject", [$h‍_a => (initGlobalObject = $h‍_a)]]]],["./whitelist.js", [["initialGlobalPropertyNames", [$h‍_a => (initialGlobalPropertyNames = $h‍_a)]]]],["./tame-function-tostring.js", [["tameFunctionToString", [$h‍_a => (tameFunctionToString = $h‍_a)]]]],["./tame-domains.js", [["tameDomains", [$h‍_a => (tameDomains = $h‍_a)]]]],["./error/tame-console.js", [["tameConsole", [$h‍_a => (tameConsole = $h‍_a)]]]],["./error/tame-error-constructor.js", [["default", [$h‍_a => (tameErrorConstructor = $h‍_a)]]]],["./error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]],["makeAssert", [$h‍_a => (makeAssert = $h‍_a)]]]]]);   


































/** @typedef {import('../index.js').LockdownOptions} LockdownOptions */

const { details: d, quote: q } = assert;

let firstOptions;

// Build a harden() with an empty fringe.
// Gate it on lockdown.
/**
 * @template T
 * @param {T} ref
 * @returns {T}
 */
const harden = makeHardener();

const alreadyHardenedIntrinsics = () => false;

/**
 * @callback Transform
 * @param {string} source
 * @returns {string}
 */

/**
 * @callback CompartmentConstructor
 * @param {Object} endowments
 * @param {Object} moduleMap
 * @param {Object} [options]
 * @param {Array<Transform>} [options.transforms]
 * @param {Array<Transform>} [options.__shimTransforms__]
 * @param {Object} [options.globalLexicals]
 */

/**
 * @callback CompartmentConstructorMaker
 * @param {CompartmentConstructorMaker} targetMakeCompartmentConstructor
 * @param {Object} intrinsics
 * @param {(func: Function) => void} markVirtualizedNativeFunction
 * @returns {CompartmentConstructor}
 */

// TODO https://github.com/endojs/endo/issues/814
// Lockdown currently allows multiple calls provided that the specified options
// of every call agree.  With experience, we have observed that lockdown should
// only ever need to be called once and that simplifying lockdown will improve
// the quality of audits.

// TODO https://github.com/endojs/endo/issues/815
// Lockdown receives makeCompartmentConstructor and compartmentPrototype.
// This is a vestige of an earlier version of SES where makeLockdown was called
// from two different entry points: one for a layer of SES that had a
// Compartment that only supported evaluating programs, and a second layer that
// extended Compartment to support modules, but at the expense of entraining a
// dependency on Babel.
// SES currently externalizes the dependency on Babel and one version of
// makeCompartmentConstructor is sufficient for all uses, so this can be
// simplified.

/**
 * @param {CompartmentConstructorMaker} makeCompartmentConstructor
 * @param {Object} compartmentPrototype
 * @param {() => Object} getAnonymousIntrinsics
 * @param {LockdownOptions} [options]
 * @returns {() => {}} repairIntrinsics
 */
const repairIntrinsics = (
makeCompartmentConstructor,
compartmentPrototype,
getAnonymousIntrinsics,
options = {}) =>
{
  // First time, absent options default to 'safe'.
  // Subsequent times, absent options default to first options.
  // Thus, all present options must agree with first options.
  // Reconstructing `option` here also ensures that it is a well
  // behaved record, with only own data properties.
  //
  // The `overrideTaming` is not a safety issue. Rather it is a tradeoff
  // between code compatibility, which is better with the `'moderate'`
  // setting, and tool compatibility, which is better with the `'min'`
  // setting. See
  // https://github.com/Agoric/SES-shim/blob/master/packages/ses/README.md#enabling-override-by-assignment)
  // for an explanation of when to use which.
  //
  // The `stackFiltering` is not a safety issue. Rather it is a tradeoff
  // between relevance and completeness of the stack frames shown on the
  // console. Setting`stackFiltering` to `'verbose'` applies no filters, providing
  // the raw stack frames that can be quite versbose. Setting
  // `stackFrameFiltering` to`'concise'` limits the display to the stack frame
  // information most likely to be relevant, eliminating distracting frames
  // such as those from the infrastructure. However, the bug you're trying to
  // track down might be in the infrastrure, in which case the `'verbose'` setting
  // is useful. See
  // [`stackFiltering` options](https://github.com/Agoric/SES-shim/blob/master/packages/ses/lockdown-options.md#stackfiltering-options)
  // for an explanation.
  options = /** @type {LockdownOptions} */{ ...firstOptions, ...options };
  const {
    dateTaming = 'safe', // deprecated
    errorTaming = 'safe',
    mathTaming = 'safe', // deprecated
    errorTrapping = 'platform',
    regExpTaming = 'safe',
    localeTaming = 'safe',
    consoleTaming = 'safe',
    overrideTaming = 'moderate',
    overrideDebug = [],
    stackFiltering = 'concise',
    domainTaming = 'unsafe', // TODO become 'safe' by default in next-breaking-release.
    __allowUnsafeMonkeyPatching__ = 'safe',

    ...extraOptions } =
  options;

  // Assert that only supported options were passed.
  // Use Reflect.ownKeys to reject symbol-named properties as well.
  const extraOptionsNames = ownKeys(extraOptions);
  assert(
  extraOptionsNames.length === 0,
  d`lockdown(): non supported option ${q(extraOptionsNames)}`);


  // Asserts for multiple invocation of lockdown().
  if (firstOptions) {
    for (const name of keys(firstOptions)) {
      assert(
      options[name] === firstOptions[name],
      d`lockdown(): cannot re-invoke with different option ${q(name)}`);

    }
    return alreadyHardenedIntrinsics;
  }

  firstOptions = {
    dateTaming, // deprecated
    errorTaming,
    mathTaming, // deprecated
    regExpTaming,
    localeTaming,
    consoleTaming,
    overrideTaming,
    overrideDebug,
    stackFiltering,
    domainTaming,
    __allowUnsafeMonkeyPatching__ };


  /**
   * Because of packagers and bundlers, etc, multiple invocations of lockdown
   * might happen in separate instantiations of the source of this module.
   * In that case, each one sees its own `firstOptions` variable, so the test
   * above will not detect that lockdown has already happened. Instead, we
   * unreliably test some telltale signs that lockdown has run, to avoid
   * trying to lock down a locked down environment. Although the test is
   * unreliable, this is consistent with the SES threat model. SES provides
   * security only if it runs first in a given realm, or if everything that
   * runs before it is SES-aware and cooperative. Neither SES nor anything
   * can protect itself from corrupting code that runs first. For these
   * purposes, code that turns a realm into something that passes these
   * tests without actually locking down counts as corrupting code.
   *
   * The specifics of what this tests for may change over time, but it
   * should be consistent with any setting of the lockdown options. We
   * do no checking that the state is consistent with current lockdown
   * options. So a call to lockdown with one set of options may silently
   * succeed with a state not reflecting those options, but only
   * if a previous lockdown happened from something other than this
   * instance of this module.
   */
  const seemsToBeLockedDown = () => {
    return (
      globalThis.Function.prototype.constructor !== globalThis.Function &&
      typeof globalThis.harden === 'function' &&
      typeof globalThis.lockdown === 'function' &&
      globalThis.Date.prototype.constructor !== globalThis.Date &&
      typeof globalThis.Date.now === 'function' &&
      // @ts-ignore
      // eslint-disable-next-line @endo/no-polymorphic-call
      is(globalThis.Date.prototype.constructor.now(), NaN));

  };

  if (seemsToBeLockedDown()) {
    // eslint-disable-next-line @endo/no-polymorphic-call
    console.log('Seems to already be locked down. Skipping second lockdown');
    return alreadyHardenedIntrinsics;
  }

  /**
   * 1. TAME powers & gather intrinsics first.
   */

  tameDomains(domainTaming);

  const {
    addIntrinsics,
    completePrototypes,
    finalIntrinsics } =
  makeIntrinsicsCollector();

  addIntrinsics({ harden });

  addIntrinsics(tameFunctionConstructors());

  addIntrinsics(tameDateConstructor(dateTaming));
  addIntrinsics(tameErrorConstructor(errorTaming, stackFiltering));
  addIntrinsics(tameMathObject(mathTaming));
  addIntrinsics(tameRegExpConstructor(regExpTaming));

  addIntrinsics(getAnonymousIntrinsics());

  completePrototypes();

  const intrinsics = finalIntrinsics();

  // Wrap console unless suppressed.
  // At the moment, the console is considered a host power in the start
  // compartment, and not a primordial. Hence it is absent from the whilelist
  // and bypasses the intrinsicsCollector.
  let optGetStackString;
  if (errorTaming !== 'unsafe') {
    optGetStackString = intrinsics['%InitialGetStackString%'];
  }
  const consoleRecord = tameConsole(
  consoleTaming,
  errorTrapping,
  optGetStackString);

  globalThis.console = /** @type {Console} */consoleRecord.console;

  if (errorTaming === 'unsafe' && globalThis.assert === assert) {
    // If errorTaming is 'unsafe' we replace the global assert with
    // one whose `details` template literal tag does not redact
    // unmarked substitution values. IOW, it blabs information that
    // was supposed to be secret from callers, as an aid to debugging
    // at a further cost in safety.
    globalThis.assert = makeAssert(undefined, true);
  }

  // Replace *Locale* methods with their non-locale equivalents
  tameLocaleMethods(intrinsics, localeTaming);

  // Replace Function.prototype.toString with one that recognizes
  // shimmed functions as honorary native functions.
  const markVirtualizedNativeFunction = tameFunctionToString();

  /**
   * 2. WHITELIST to standardize the environment.
   */

  // Remove non-standard properties.
  // All remaining function encountered during whitelisting are
  // branded as honorary native functions.
  whitelistIntrinsics(intrinsics, markVirtualizedNativeFunction);

  // Initialize the powerful initial global, i.e., the global of the
  // start compartment, from the intrinsics.
  initGlobalObject(
  globalThis,
  intrinsics,
  initialGlobalPropertyNames,
  makeCompartmentConstructor,
  compartmentPrototype,
  {
    markVirtualizedNativeFunction });



  /**
   * 3. HARDEN to share the intrinsics.
   */

  function hardenIntrinsics() {
    // Circumvent the override mistake.
    // TODO consider moving this to the end of the repair phase, and
    // therefore before vetted shims rather than afterwards. It is not
    // clear yet which is better.
    enablePropertyOverrides(intrinsics, overrideTaming, overrideDebug);

    if (__allowUnsafeMonkeyPatching__ !== 'unsafe') {
      // Finally register and optionally freeze all the intrinsics. This
      // must be the operation that modifies the intrinsics.
      harden(intrinsics);
    }

    // Reveal harden after lockdown.
    // Harden is dangerous before lockdown because hardening just
    // about anything will inadvertently render intrinsics irreparable.
    // Also, for modules that must work both before or after lockdown (code
    // that is portable between JS and SES), the existence of harden in global
    // scope signals whether such code should attempt to use harden in the
    // defense of its own API.
    globalThis.harden = harden;

    // Returning `true` indicates that this is a JS to SES transition.
    return true;
  }

  return hardenIntrinsics;
};

/**
 * @param {CompartmentConstructorMaker} makeCompartmentConstructor
 * @param {Object} compartmentPrototype
 * @param {() => Object} getAnonymousIntrinsics
 * @returns {import('../index.js').Lockdown}
 */$h‍_once.repairIntrinsics(repairIntrinsics);
const makeLockdown = (
makeCompartmentConstructor,
compartmentPrototype,
getAnonymousIntrinsics) =>
{
  /**
   * @param {LockdownOptions} [options]
   */
  const lockdown = (options = {}) => {
    const maybeHardenIntrinsics = repairIntrinsics(
    makeCompartmentConstructor,
    compartmentPrototype,
    getAnonymousIntrinsics,
    options);

    return maybeHardenIntrinsics();
  };
  return lockdown;
};$h‍_once.makeLockdown(makeLockdown);
})

,
(({   imports: $h‍_imports,   liveVar: $h‍_live,   onceVar: $h‍_once,  }) => {   let globalThis,TypeError,assign,tameFunctionToString,getGlobalIntrinsics,getAnonymousIntrinsics,makeLockdown,makeCompartmentConstructor,CompartmentPrototype,assert;$h‍_imports([["./src/commons.js", [["globalThis", [$h‍_a => (globalThis = $h‍_a)]],["TypeError", [$h‍_a => (TypeError = $h‍_a)]],["assign", [$h‍_a => (assign = $h‍_a)]]]],["./src/tame-function-tostring.js", [["tameFunctionToString", [$h‍_a => (tameFunctionToString = $h‍_a)]]]],["./src/intrinsics.js", [["getGlobalIntrinsics", [$h‍_a => (getGlobalIntrinsics = $h‍_a)]]]],["./src/get-anonymous-intrinsics.js", [["getAnonymousIntrinsics", [$h‍_a => (getAnonymousIntrinsics = $h‍_a)]]]],["./src/lockdown-shim.js", [["makeLockdown", [$h‍_a => (makeLockdown = $h‍_a)]]]],["./src/compartment-shim.js", [["makeCompartmentConstructor", [$h‍_a => (makeCompartmentConstructor = $h‍_a)]],["CompartmentPrototype", [$h‍_a => (CompartmentPrototype = $h‍_a)]]]],["./src/error/assert.js", [["assert", [$h‍_a => (assert = $h‍_a)]]]]]);   

























/** getThis returns globalThis in sloppy mode or undefined in strict mode. */
function getThis() {
  return this;
}

if (getThis()) {
  throw new TypeError(`SES failed to initialize, sloppy mode (SES_NO_SLOPPY)`);
}

const markVirtualizedNativeFunction = tameFunctionToString();

const Compartment = makeCompartmentConstructor(
makeCompartmentConstructor,
getGlobalIntrinsics(globalThis),
markVirtualizedNativeFunction);


assign(globalThis, {
  lockdown: makeLockdown(
  makeCompartmentConstructor,
  CompartmentPrototype,
  getAnonymousIntrinsics),

  Compartment,
  assert });
})

,

]);

// END of injected code from ses
  })()
  return module.exports
})()

    const lockdownOptions = {
      // gives a semi-high resolution timer
      dateTaming: 'unsafe',
      // this is introduces non-determinism, but is otherwise safe
      mathTaming: 'unsafe',
      // lets code observe call stack, but easier debuggability
      errorTaming: 'unsafe',
      // shows the full call stack
      stackFiltering: 'verbose',
      // deep stacks
      consoleTaming: 'unsafe',
    }

    lockdown(lockdownOptions)

    // initialize the kernel
    const createKernelCore = (function () {
  "use strict"
  return createKernel

  function createKernel ({
    // the platform api global
    globalRef,
    // package policy object
    lavamoatConfig,
    // kernel configuration
    loadModuleData,
    getRelativeModuleId,
    prepareModuleInitializerArgs,
    getExternalCompartment,
    globalThisRefs,
    // security options
    debugMode,
    runWithPrecompiledModules
  }) {
    // create SES-wrapped LavaMoat kernel
    // endowments:
    // - console is included for convenience
    // - Math is for untamed Math.random
    // - Date is for untamed Date.now
    const kernelCompartment = new Compartment({ console, Math, Date })
    let makeKernel
    if (runWithPrecompiledModules) {
      makeKernel = unsafeMakeKernel
    } else {
      makeKernel = kernelCompartment.evaluate(`(${unsafeMakeKernel})\n//# sourceURL=LavaMoat/core/kernel`)
    }
    const lavamoatKernel = makeKernel({
      globalRef,
      lavamoatConfig,
      loadModuleData,
      getRelativeModuleId,
      prepareModuleInitializerArgs,
      getExternalCompartment,
      globalThisRefs,
      debugMode,
      runWithPrecompiledModules
    })

    return lavamoatKernel
  }

  // this is serialized and run in SES
  // mostly just exists to expose variables to internalRequire and loadBundle
  function unsafeMakeKernel ({
    globalRef,
    lavamoatConfig,
    loadModuleData,
    getRelativeModuleId,
    prepareModuleInitializerArgs,
    getExternalCompartment,
    globalThisRefs = ['globalThis'],
    debugMode = false,
    runWithPrecompiledModules = false
  }) {
    // "templateRequire" calls are inlined in "generateKernel"
    const generalUtils = // define makeGeneralUtils
(function(){
  const global = globalRef
  const exports = {}
  const module = { exports }
  ;(function(){
// START of injected code from makeGeneralUtils
module.exports = makeGeneralUtils

function makeGeneralUtils () {
  return {
    createFunctionWrapper
  }

  function createFunctionWrapper (sourceValue, unwrapTest, unwrapTo) {
    const newValue = function (...args) {
      if (new.target) {
        // handle constructor calls
        return Reflect.construct(sourceValue, args, new.target)
      } else {
        // handle function calls
        // unwrap to target value if this value is the source package compartment's globalThis
        const thisRef = unwrapTest(this) ? unwrapTo : this
        return Reflect.apply(sourceValue, thisRef, args)
      }
    }
    Object.defineProperties(newValue, Object.getOwnPropertyDescriptors(sourceValue))
    return newValue
  }
}
// END of injected code from makeGeneralUtils
  })()
  return module.exports
})()()
    const { getEndowmentsForConfig, makeMinimalViewOfRef, applyEndowmentPropDescTransforms } = // define makeGetEndowmentsForConfig
(function(){
  const global = globalRef
  const exports = {}
  const module = { exports }
  ;(function(){
// START of injected code from makeGetEndowmentsForConfig
// the contents of this file will be copied into the prelude template
// this module has been written so that it required directly or copied and added to the template with a small wrapper
module.exports = makeGetEndowmentsForConfig

// utilities for generating the endowments object based on a globalRef and a config

// The config uses a period-deliminated path notation to pull out deep values from objects
// These utilities help create an object populated with only the deep properties specified in the config

function makeGetEndowmentsForConfig ({ createFunctionWrapper }) {
  return {
    getEndowmentsForConfig,
    makeMinimalViewOfRef,
    copyValueAtPath,
    applyGetSetPropDescTransforms,
    applyEndowmentPropDescTransforms
  }

  /**
   *
   * @function getEndowmentsForConfig
   * @param {object} sourceRef - Object from which to copy properties
   * @param {object} config - LavaMoat package config
   * @param {object} unwrapTo - For getters and setters, when the this-value is unwrapFrom, is replaced as unwrapTo
   * @param {object} unwrapFrom - For getters and setters, the this-value to replace (default: targetRef)
   * @return {object} - The targetRef
   *
   */
  function getEndowmentsForConfig (sourceRef, config, unwrapTo, unwrapFrom) {
    if (!config.globals) return {}
    // validate read access from config
    const whitelistedReads = []
    Object.entries(config.globals).forEach(([path, configValue]) => {
      const pathParts = path.split('.')
      // disallow dunder proto in path
      const pathContainsDunderProto = pathParts.some(pathPart => pathPart === '__proto__')
      if (pathContainsDunderProto) {
        throw new Error(`Lavamoat - "__proto__" disallowed when creating minial view. saw "${path}"`)
      }
      // write access handled elsewhere
      if (configValue === 'write') return
      if (configValue !== true) {
        throw new Error(`LavaMoat - unknown value for config (${typeof configValue})`)
      }
      whitelistedReads.push(path)
    })
    return makeMinimalViewOfRef(sourceRef, whitelistedReads, unwrapTo, unwrapFrom)
  }

  function makeMinimalViewOfRef (sourceRef, paths, unwrapTo, unwrapFrom) {
    const targetRef = {}
    paths.forEach(path => {
      copyValueAtPath(path.split('.'), sourceRef, targetRef, unwrapTo, unwrapFrom)
    })
    return targetRef
  }

  function copyValueAtPath (pathParts, sourceRef, targetRef, unwrapTo = sourceRef, unwrapFrom = targetRef) {
    if (pathParts.length === 0) {
      throw new Error('unable to copy, must have pathParts, was empty')
    }
    const [nextPart, ...remainingParts] = pathParts
    // get the property from any depth in the property chain
    const { prop: sourcePropDesc } = getPropertyDescriptorDeep(sourceRef, nextPart)

    // if source missing the value to copy, just skip it
    if (!sourcePropDesc) {
      return
    }

    // if target already has a value, it must be extensible
    const targetPropDesc = Reflect.getOwnPropertyDescriptor(targetRef, nextPart)
    if (targetPropDesc) {
      // dont attempt to extend a getter or trigger a setter
      if (!('value' in targetPropDesc)) {
        throw new Error(`unable to copy on to targetRef, targetRef has a getter at "${nextPart}"`)
      }
      // value must be extensible (cant write properties onto it)
      const targetValue = targetPropDesc.value
      const valueType = typeof targetValue
      if (valueType !== 'object' && valueType !== 'function') {
        throw new Error(`unable to copy on to targetRef, targetRef value is not an obj or func "${nextPart}"`)
      }
    }

    // if this is not the last path in the assignment, walk into the containing reference
    if (remainingParts.length > 0) {
      const { sourceValue, sourceWritable } = getSourceValue()
      const nextSourceRef = sourceValue
      let nextTargetRef
      // check if value exists on target
      if (targetPropDesc) {
        // a value already exists, we should walk into it
        nextTargetRef = targetPropDesc.value
      } else {
        // its not populated so lets write to it
        // put an object to serve as a container
        const containerRef = {}
        const newPropDesc = {
          value: containerRef,
          writable: sourceWritable,
          enumerable: sourcePropDesc.enumerable,
          configurable: sourcePropDesc.configurable
        }
        Reflect.defineProperty(targetRef, nextPart, newPropDesc)
        // the newly created container will be the next target
        nextTargetRef = containerRef
      }
      copyValueAtPath(remainingParts, nextSourceRef, nextTargetRef)
      return
    }

    // this is the last part of the path, the value we're trying to actually copy
    // if has getter/setter - apply this-value unwrapping
    if (!('value' in sourcePropDesc)) {
      // wrapper setter/getter with correct receiver
      const wrapperPropDesc = applyGetSetPropDescTransforms(sourcePropDesc, unwrapFrom, unwrapTo)
      Reflect.defineProperty(targetRef, nextPart, wrapperPropDesc)
      return
    }

    // need to determine the value type in order to copy it with
    // this-value unwrapping support
    const { sourceValue, sourceWritable } = getSourceValue()

    // not a function - copy as is
    if (typeof sourceValue !== 'function') {
      Reflect.defineProperty(targetRef, nextPart, sourcePropDesc)
      return
    }
    // otherwise add workaround for functions to swap back to the sourceal "this" reference
    const unwrapTest = thisValue => thisValue === unwrapFrom
    const newValue = createFunctionWrapper(sourceValue, unwrapTest, unwrapTo)
    const newPropDesc = {
      value: newValue,
      writable: sourceWritable,
      enumerable: sourcePropDesc.enumerable,
      configurable: sourcePropDesc.configurable
    }
    Reflect.defineProperty(targetRef, nextPart, newPropDesc)

    function getSourceValue () {
      // determine the source value, this coerces getters to values
      // im deeply sorry, respecting getters was complicated and
      // my brain is not very good
      let sourceValue, sourceWritable
      if ('value' in sourcePropDesc) {
        sourceValue = sourcePropDesc.value
        sourceWritable = sourcePropDesc.writable
      } else if ('get' in sourcePropDesc) {
        sourceValue = sourcePropDesc.get.call(unwrapTo)
        sourceWritable = 'set' in sourcePropDesc
      } else {
        throw new Error('getEndowmentsForConfig - property descriptor missing a getter')
      }
      return { sourceValue, sourceWritable }
    }
  }

  function applyEndowmentPropDescTransforms (propDesc, unwrapFromCompartment, unwrapToGlobalThis) {
    let newPropDesc = propDesc
    newPropDesc = applyFunctionPropDescTransform(newPropDesc, unwrapFromCompartment, unwrapToGlobalThis)
    newPropDesc = applyGetSetPropDescTransforms(newPropDesc, unwrapFromCompartment.globalThis, unwrapToGlobalThis)
    return newPropDesc
  }

  function applyGetSetPropDescTransforms (sourcePropDesc, unwrapFromGlobalThis, unwrapToGlobalThis) {
    const wrappedPropDesc = { ...sourcePropDesc }
    if (sourcePropDesc.get) {
      wrappedPropDesc.get = function () {
        const receiver = this
        // replace the "receiver" value if it points to fake parent
        const receiverRef = receiver === unwrapFromGlobalThis ? unwrapToGlobalThis : receiver
        // sometimes getters replace themselves with static properties, as seen wih the FireFox runtime
        const result = Reflect.apply(sourcePropDesc.get, receiverRef, [])
        if (typeof result === 'function') {
          // functions must be wrapped to ensure a good this-value.
          // lockdown causes some propDescs to go to value -> getter,
          // eg "Function.prototype.bind". we need to wrap getter results
          // as well in order to ensure they have their this-value wrapped correctly
          // if this ends up being problematic we can maybe take advantage of lockdown's
          // "getter.originalValue" property being available
          return createFunctionWrapper(result, (thisValue) => thisValue === unwrapFromGlobalThis, unwrapToGlobalThis)
        } else {
          return result
        }
      }
    }
    if (sourcePropDesc.set) {
      wrappedPropDesc.set = function (value) {
        // replace the "receiver" value if it points to fake parent
        const receiver = this
        const receiverRef = receiver === unwrapFromGlobalThis ? unwrapToGlobalThis : receiver
        return Reflect.apply(sourcePropDesc.set, receiverRef, [value])
      }
    }
    return wrappedPropDesc
  }

  function applyFunctionPropDescTransform (propDesc, unwrapFromCompartment, unwrapToGlobalThis) {
    if (!('value' in propDesc && typeof propDesc.value === 'function')) {
      return propDesc
    }
    const unwrapTest = (thisValue) => {
      // unwrap function calls this-value to unwrapToGlobalThis when:
      // this value is globalThis ex. globalThis.abc()
      // scope proxy leak workaround ex. abc()
      return thisValue === unwrapFromCompartment.globalThis || unwrapFromCompartment.__isKnownScopeProxy__(thisValue)
    }
    const newFn = createFunctionWrapper(propDesc.value, unwrapTest, unwrapToGlobalThis)
    return { ...propDesc, value: newFn }
  }
}

function getPropertyDescriptorDeep (target, key) {
  let receiver = target
  while (true) {
    // support lookup on objects and primitives
    const typeofReceiver = typeof receiver
    if (typeofReceiver === 'object' || typeofReceiver === 'function') {
      const prop = Reflect.getOwnPropertyDescriptor(receiver, key)
      if (prop) {
        return { receiver, prop }
      }
      // try next in the prototype chain
      receiver = Reflect.getPrototypeOf(receiver)
    } else {
      // prototype lookup for primitives
      // eslint-disable-next-line no-proto
      receiver = receiver.__proto__
    }
    // abort if this is the end of the prototype chain.
    if (!receiver) return { prop: null, receiver: null }
  }
}

// END of injected code from makeGetEndowmentsForConfig
  })()
  return module.exports
})()(generalUtils)
    const { prepareCompartmentGlobalFromConfig } = // define makePrepareRealmGlobalFromConfig
(function(){
  const global = globalRef
  const exports = {}
  const module = { exports }
  ;(function(){
// START of injected code from makePrepareRealmGlobalFromConfig
// the contents of this file will be copied into the prelude template
// this module has been written so that it required directly or copied and added to the template with a small wrapper
module.exports = makePrepareRealmGlobalFromConfig

// utilities for exposing configuring the exposed endowments on the container global

// The config uses a period-deliminated path notation to pull out deep values from objects
// These utilities help modify the container global to expose the allowed globals from the globalStore OR the platform global

function makePrepareRealmGlobalFromConfig ({ createFunctionWrapper }) {
  return {
    prepareCompartmentGlobalFromConfig,
    getTopLevelReadAccessFromPackageConfig,
    getTopLevelWriteAccessFromPackageConfig
  }

  function getTopLevelReadAccessFromPackageConfig (globalsConfig) {
    const result = Object.entries(globalsConfig)
      .filter(([key, value]) => value === 'read' || value === true || (value === 'write' && key.split('.').length > 1))
      .map(([key]) => key.split('.')[0])
    // return unique array
    return Array.from(new Set(result))
  }

  function getTopLevelWriteAccessFromPackageConfig (globalsConfig) {
    const result = Object.entries(globalsConfig)
      .filter(([key, value]) => value === 'write' && key.split('.').length === 1)
      .map(([key]) => key)
    return result
  }

  function prepareCompartmentGlobalFromConfig (packageCompartment, globalsConfig, endowments, globalStore, globalThisRefs) {
    const packageCompartmentGlobal = packageCompartment.globalThis
    // lookup top level read + write access keys
    const topLevelWriteAccessKeys = getTopLevelWriteAccessFromPackageConfig(globalsConfig)
    const topLevelReadAccessKeys = getTopLevelReadAccessFromPackageConfig(globalsConfig)

    // define accessors

    // allow read access via globalStore or packageCompartmentGlobal
    topLevelReadAccessKeys.forEach(key => {
      Object.defineProperty(packageCompartmentGlobal, key, {
        get () {
          if (globalStore.has(key)) {
            return globalStore.get(key)
          } else {
            return Reflect.get(endowments, key, this)
          }
        },
        set () {
          // TODO: there should be a config to throw vs silently ignore
          console.warn(`LavaMoat: ignoring write attempt to read-access global "${key}"`)
        }
      })
    })

    // allow write access to globalStore
    // read access via globalStore or packageCompartmentGlobal
    topLevelWriteAccessKeys.forEach(key => {
      Object.defineProperty(packageCompartmentGlobal, key, {
        get () {
          if (globalStore.has(key)) {
            return globalStore.get(key)
          } else {
            return endowments[key]
          }
        },
        set (value) {
          globalStore.set(key, value)
        },
        enumerable: true,
        configurable: true
      })
    })

    // set circular globalRefs
    globalThisRefs.forEach(key => {
      // if globalRef is actually an endowment, ignore
      if (topLevelReadAccessKeys.includes(key)) return
      if (topLevelWriteAccessKeys.includes(key)) return
      // set circular ref to global
      packageCompartmentGlobal[key] = packageCompartmentGlobal
    })

    // bind Function constructor this value to globalThis
    // legacy globalThis shim
    const origFunction = packageCompartmentGlobal.Function
    const newFunction = function (...args) {
      const fn = origFunction(...args)
      const unwrapTest = thisValue => thisValue === undefined
      return createFunctionWrapper(fn, unwrapTest, packageCompartmentGlobal)
    }
    Object.defineProperties(newFunction, Object.getOwnPropertyDescriptors(origFunction))
    packageCompartmentGlobal.Function = newFunction
  }
}

// END of injected code from makePrepareRealmGlobalFromConfig
  })()
  return module.exports
})()(generalUtils)

    const moduleCache = new Map()
    const packageCompartmentCache = new Map()
    const globalStore = new Map()

    const rootPackageName = '<root>'
    const rootPackageCompartment = createRootPackageCompartment(globalRef)

    return {
      internalRequire
    }

    // this function instantiaties a module from a moduleId.
    // 1. loads the module metadata and policy
    // 2. prepares the execution environment
    // 3. instantiates the module, recursively instantiating dependencies
    // 4. returns the module exports
    function internalRequire (moduleId) {
      // use cached module.exports if module is already instantiated
      if (moduleCache.has(moduleId)) {
        const moduleExports = moduleCache.get(moduleId).exports
        return moduleExports
      }

      // load and validate module metadata
      // if module metadata is missing, throw an error
      const moduleData = loadModuleData(moduleId)
      if (!moduleData) {
        const err = new Error('Cannot find module \'' + moduleId + '\'')
        err.code = 'MODULE_NOT_FOUND'
        throw err
      }
      if (moduleData.id === undefined) {
        throw new Error('LavaMoat - moduleId is not defined correctly.')
      }

      // parse and validate module data
      const { package: packageName, source: moduleSource } = moduleData
      if (!packageName) throw new Error(`LavaMoat - invalid packageName for module "${moduleId}"`)
      const packagePolicy = getPolicyForPackage(lavamoatConfig, packageName)

      // create the moduleObj and initializer
      const { moduleInitializer, moduleObj } = prepareModuleInitializer(moduleData, packagePolicy)

      // cache moduleObj here
      // this is important to inf loops when hitting cycles in the dep graph
      // must cache before running the moduleInitializer
      moduleCache.set(moduleId, moduleObj)

      // validate moduleInitializer
      if (typeof moduleInitializer !== 'function') {
        throw new Error(`LavaMoat - moduleInitializer is not defined correctly. got "${typeof moduleInitializer}"\n${moduleSource}`)
      }

      // initialize the module with the correct context
      const initializerArgs = prepareModuleInitializerArgs(requireRelativeWithContext, moduleObj, moduleData)
      moduleInitializer.apply(moduleObj.exports, initializerArgs)
      const moduleExports = moduleObj.exports

      return moduleExports

      // this is passed to the module initializer
      // it adds the context of the parent module
      // this could be replaced via "Function.prototype.bind" if its more performant
      function requireRelativeWithContext (requestedName) {
        const parentModuleExports = moduleObj.exports
        const parentModuleData = moduleData
        const parentPackagePolicy = packagePolicy
        const parentModuleId = moduleId
        return requireRelative({ requestedName, parentModuleExports, parentModuleData, parentPackagePolicy, parentModuleId })
      }
    }

    // this resolves a module given a requestedName (eg relative path to parent) and a parentModule context
    // the exports are processed via "protectExportsRequireTime" per the module's configuration
    function requireRelative ({ requestedName, parentModuleExports, parentModuleData, parentPackagePolicy, parentModuleId }) {
      const parentModulePackageName = parentModuleData.package
      const parentPackagesWhitelist = parentPackagePolicy.packages
      const parentBuiltinsWhitelist = Object.entries(parentPackagePolicy.builtin)
        .filter(([_, allowed]) => allowed === true)
        .map(([packagePath, allowed]) => packagePath.split('.')[0])

      // resolve the moduleId from the requestedName
      const moduleId = getRelativeModuleId(parentModuleId, requestedName)

      // browserify goop:
      // recursive requires dont hit cache so it inf loops, so we shortcircuit
      // this only seems to happen with a few browserify builtins (nodejs builtin module polyfills)
      // we could likely allow any requestedName since it can only refer to itself
      if (moduleId === parentModuleId) {
        if (['timers', 'buffer'].includes(requestedName) === false) {
          throw new Error(`LavaMoat - recursive require detected: "${requestedName}"`)
        }
        return parentModuleExports
      }

      // load module
      let moduleExports = internalRequire(moduleId)

      // look up config for module
      const moduleData = loadModuleData(moduleId)
      const packageName = moduleData.package

      // disallow requiring packages that are not in the parent's whitelist
      const isSamePackage = packageName === parentModulePackageName
      const parentIsEntryModule = parentModulePackageName === rootPackageName
      let isInParentWhitelist = false
      if (moduleData.type === 'builtin') {
        isInParentWhitelist = parentBuiltinsWhitelist.includes(packageName)
      } else {
        isInParentWhitelist = (parentPackagesWhitelist[packageName] === true)
      }

      // validate that the import is allowed
      if (!parentIsEntryModule && !isSamePackage && !isInParentWhitelist) {
        let typeText = ' '
        if (moduleData.type === 'builtin') typeText = ' node builtin '
        throw new Error(`LavaMoat - required${typeText}package not in whitelist: package "${parentModulePackageName}" requested "${packageName}" as "${requestedName}"`)
      }

      // create minimal selection if its a builtin and the whole path is not selected for
      if (!parentIsEntryModule && moduleData.type === 'builtin' && !parentPackagePolicy.builtin[moduleId]) {
        const builtinPaths = (
          Object.entries(parentPackagePolicy.builtin)
          // grab all allowed builtin paths that match this package
            .filter(([packagePath, allowed]) => allowed === true && moduleId === packagePath.split('.')[0])
          // only include the paths after the packageName
            .map(([packagePath, allowed]) => packagePath.split('.').slice(1).join('.'))
            .sort()
        )
        moduleExports = makeMinimalViewOfRef(moduleExports, builtinPaths)
      }

      return moduleExports
    }

    function prepareModuleInitializer (moduleData, packagePolicy) {
      const { moduleInitializer, precompiledInitializer, package: packageName, id: moduleId, source: moduleSource } = moduleData

      // moduleInitializer may be set by loadModuleData (e.g. builtin + native modules)
      if (moduleInitializer) {
        // if an external moduleInitializer is set, ensure it is allowed
        if (moduleData.type === 'native') {
          // ensure package is allowed to have native modules
          if (packagePolicy.native !== true) {
            throw new Error(`LavaMoat - "native" module type not permitted for package "${packageName}", module "${moduleId}"`)
          }
        } else if (moduleData.type !== 'builtin') {
          // builtin module types dont have policy configurations
          // but the packages that can import them are constrained elsewhere
          // here we just ensure that the module type is the only other type with a external moduleInitializer
          throw new Error(`LavaMoat - invalid external moduleInitializer for module type "${moduleData.type}" in package "${packageName}", module "${moduleId}"`)
        }
        // moduleObj must be from the same Realm as the moduleInitializer (eg dart2js runtime requirement)
        // here we are assuming the provided moduleInitializer is from the same Realm as this kernel
        const moduleObj = { exports: {} }
        return { moduleInitializer, moduleObj }
      }

      // setup initializer from moduleSource and compartment.
      // execute in package compartment with globalThis populated per package policy
      const packageCompartment = getCompartmentForPackage(packageName, packagePolicy)

      try {
        let moduleObj
        let moduleInitializer
        if (runWithPrecompiledModules) {
          if (!precompiledInitializer) {
            throw new Error(`LavaMoat - precompiledInitializer missing for "${moduleId}" from package "${packageName}"`)
          }
          // moduleObj must be from the same Realm as the moduleInitializer (eg dart2js runtime requirement)
          // here we are assuming the provided moduleInitializer is from the same Realm as this kernel
          moduleObj = { exports: {} }
          const { scopeProxy } = packageCompartment.__makeScopeProxy__()
          // this invokes the with-proxy wrapper
          const moduleInitializerFactory = precompiledInitializer.call(scopeProxy)
          // this ensures strict mode
          moduleInitializer = moduleInitializerFactory()
        } else {
          if (typeof moduleSource !== 'string') {
            throw new Error(`LavaMoat - moduleSource not a string for "${moduleId}" from package "${packageName}"`)
          }
          const sourceURL = moduleData.file || `modules/${moduleId}`
          if (sourceURL.includes('\n')) {
            throw new Error(`LavaMoat - Newlines not allowed in filenames: ${JSON.stringify(sourceURL)}`)
          }
          // moduleObj must be from the same Realm as the moduleInitializer (eg dart2js runtime requirement)
          moduleObj = packageCompartment.evaluate('({ exports: {} })')
          // TODO: move all source mutations elsewhere
          moduleInitializer = packageCompartment.evaluate(`${moduleSource}\n//# sourceURL=${sourceURL}`)
        }
        return { moduleInitializer, moduleObj }
      } catch (err) {
        console.warn(`LavaMoat - Error evaluating module "${moduleId}" from package "${packageName}" \n${err.stack}`)
        throw err
      }
    }

    function createRootPackageCompartment (globalRef) {
      if (packageCompartmentCache.has(rootPackageName)) {
        throw new Error('LavaMoat - createRootPackageCompartment called more than once')
      }
      // prepare the root package's SES Compartment
      // endowments:
      // - Math is for untamed Math.random
      // - Date is for untamed Date.now
      const rootPackageCompartment = new Compartment({ Math, Date })
      // find the relevant endowment sources
      const globalProtoChain = getPrototypeChain(globalRef)
      // the index for the common prototypal ancestor, Object.prototype
      // this should always be the last index, but we check just in case
      const commonPrototypeIndex = globalProtoChain.findIndex(globalProtoChainEntry => globalProtoChainEntry === Object.prototype)
      if (commonPrototypeIndex === -1) throw new Error('Lavamoat - unable to find common prototype between Compartment and globalRef')
      // we will copy endowments from all entries in the prototype chain, excluding Object.prototype
      const endowmentSources = globalProtoChain.slice(0, commonPrototypeIndex)

      // call all getters, in case of behavior change (such as with FireFox lazy getters)
      // call on contents of endowmentsSources directly instead of in new array instances. If there is a lazy getter it only changes the original prop desc.
      endowmentSources.forEach(source => {
        const descriptors = Object.getOwnPropertyDescriptors(source)
        Object.values(descriptors).forEach(desc => {
          if ('get' in desc) {
            Reflect.apply(desc.get, globalRef, [])
          }
        })
      })

      const endowmentSourceDescriptors = endowmentSources.map(globalProtoChainEntry => Object.getOwnPropertyDescriptors(globalProtoChainEntry))
      // flatten propDesc collections with precedence for globalThis-end of the prototype chain
      const endowmentDescriptorsFlat = Object.assign(Object.create(null), ...endowmentSourceDescriptors.reverse())
      // expose all own properties of globalRef, including non-enumerable
      Object.entries(endowmentDescriptorsFlat)
        // ignore properties already defined on compartment global
        .filter(([key]) => !(key in rootPackageCompartment.globalThis))
        // ignore circular globalThis refs
        .filter(([key]) => !(globalThisRefs.includes(key)))
        // define property on compartment global
        .forEach(([key, desc]) => {
          // unwrap functions, setters/getters & apply scope proxy workaround
          const wrappedPropDesc = applyEndowmentPropDescTransforms(desc, rootPackageCompartment, globalRef)
          Reflect.defineProperty(rootPackageCompartment.globalThis, key, wrappedPropDesc)
        })
      // global circular references otherwise added by prepareCompartmentGlobalFromConfig
      // Add all circular refs to root package compartment globalThis
      for (const ref of globalThisRefs) {
        if (ref in rootPackageCompartment.globalThis) {
          continue
        }
        rootPackageCompartment.globalThis[ref] = rootPackageCompartment.globalThis
      }

      // save the compartment for use by other modules in the package
      packageCompartmentCache.set(rootPackageName, rootPackageCompartment)

      return rootPackageCompartment
    }

    function getCompartmentForPackage (packageName, packagePolicy) {
      // compartment may have already been created
      let packageCompartment = packageCompartmentCache.get(packageName)
      if (packageCompartment) {
        return packageCompartment
      }

      // prepare Compartment
      if (getExternalCompartment && packagePolicy.env) {
        // external compartment can be provided by the platform (eg lavamoat-node)
        packageCompartment = getExternalCompartment(packageName, packagePolicy)
      } else {
        // prepare the module's SES Compartment
        // endowments:
        // - Math is for untamed Math.random
        // - Date is for untamed Date.now
        packageCompartment = new Compartment({ Math, Date })
      }
      // prepare endowments
      let endowments
      try {
        endowments = getEndowmentsForConfig(
          // source reference
          rootPackageCompartment.globalThis,
          // policy
          packagePolicy,
          // unwrap to
          globalRef,
          // unwrap from
          packageCompartment.globalThis
        )
      } catch (err) {
        const errMsg = `Lavamoat - failed to prepare endowments for package "${packageName}":\n${err.stack}`
        throw new Error(errMsg)
      }

      // transform functions, getters & setters on prop descs. Solves SES scope proxy bug
      Object.entries(Object.getOwnPropertyDescriptors(endowments))
        // ignore non-configurable properties because we are modifying endowments in place
        .filter(([key, propDesc]) => propDesc.configurable)
        .forEach(([key, propDesc]) => {
          const wrappedPropDesc = applyEndowmentPropDescTransforms(propDesc, packageCompartment, rootPackageCompartment.globalThis)
          Reflect.defineProperty(endowments, key, wrappedPropDesc)
        })

      // sets up read/write access as configured
      const globalsConfig = packagePolicy.globals
      prepareCompartmentGlobalFromConfig(packageCompartment, globalsConfig, endowments, globalStore, globalThisRefs)

      // save the compartment for use by other modules in the package
      packageCompartmentCache.set(packageName, packageCompartment)

      return packageCompartment
    }

    // this gets the lavaMoat config for a module by packageName
    // if there were global defaults (e.g. everything gets "console") they could be applied here
    function getPolicyForPackage (config, packageName) {
      const packageConfig = (config.resources || {})[packageName] || {}
      packageConfig.globals = packageConfig.globals || {}
      packageConfig.packages = packageConfig.packages || {}
      packageConfig.builtin = packageConfig.builtin || {}
      return packageConfig
    }

    // util for getting the prototype chain as an array
    // includes the provided value in the result
    function getPrototypeChain (value) {
      const protoChain = []
      let current = value
      while (current && (typeof current === 'object' || typeof current === 'function')) {
        protoChain.push(current)
        current = Reflect.getPrototypeOf(current)
      }
      return protoChain
    }
  }
})()

    const kernel = createKernelCore({
      lavamoatConfig,
      loadModuleData,
      getRelativeModuleId,
      prepareModuleInitializerArgs,
      getExternalCompartment,
      globalRef,
      globalThisRefs,
      debugMode,
      runWithPrecompiledModules
    })
    return kernel
  }
})()

  const { internalRequire } = createKernel({
    runWithPrecompiledModules: true,
    lavamoatConfig: lavamoatPolicy,
    loadModuleData,
    getRelativeModuleId,
    prepareModuleInitializerArgs,
    globalThisRefs: ['window', 'self', 'global', 'globalThis'],
  })

  function loadModuleData (moduleId) {
    if (!moduleRegistry.has(moduleId)) {
      throw new Error(`no module registered for "${moduleId}" (${typeof moduleId})`)
    }
    return moduleRegistry.get(moduleId)
  }

  function getRelativeModuleId (parentModuleId, requestedName) {
    const parentModuleData = loadModuleData(parentModuleId)
    if (!(requestedName in parentModuleData.deps)) {
      console.warn(`missing dep: ${parentModuleData.package} requested ${requestedName}`)
    }
    return parentModuleData.deps[requestedName] || requestedName
  }

  function prepareModuleInitializerArgs (requireRelativeWithContext, moduleObj, moduleData) {
    const require = requireRelativeWithContext
    const module = moduleObj
    const exports = moduleObj.exports
    // bify direct module instantiation disabled ("arguments[4]")
    return [require, module, exports, null, null]
  }

  // create a lavamoat pulic API for loading modules over multiple files
  const LavaPack = Object.freeze({
    loadPolicy: Object.freeze(loadPolicy),
    loadBundle: Object.freeze(loadBundle),
    runModule: Object.freeze(runModule),
  })

  globalThis.LavaPack = LavaPack

  // it is called by the policy loader or modules collection
  function loadPolicy (bundlePolicy) {
    // verify + load config
    Object.entries(bundlePolicy.resources || {}).forEach(([packageName, packageConfig]) => {
      if (packageName in lavamoatPolicy) {
        throw new Error(`LavaMoat - loadBundle encountered redundant config definition for package "${packageName}"`)
      }
      lavamoatPolicy.resources[packageName] = packageConfig
    })
  }

  // it is called by the modules collection
  function loadBundle (newModules, entryPoints, bundlePolicy) {
    // verify + load config
    if (bundlePolicy) loadPolicy(bundlePolicy)
    // verify + load in each module
    for (const [moduleId, moduleDeps, initFn, { package: packageName, type }] of newModules) {
      // verify that module is new
      if (moduleRegistry.has(moduleId)) {
        throw new Error(`LavaMoat - loadBundle encountered redundant module definition for id "${moduleId}"`)
      }
      // add the module
      moduleRegistry.set(moduleId, {
        type: type || 'js',
        id: moduleId,
        deps: moduleDeps,
        // source: `(${initFn})`,
        precompiledInitializer: initFn,
        package: packageName,
      })
    }
    // run each of entryPoints
    const entryExports = Array.prototype.map.call(entryPoints, (entryId) => {
      return runModule(entryId)
    })
    // webpack compat: return the first module's exports
    return entryExports[0]
  }

  function runModule (moduleId) {
    if (!moduleRegistry.has(moduleId)) {
      throw new Error(`no module registered for "${moduleId}" (${typeof moduleId})`)
    }
    return internalRequire(moduleId)
  }

})()
