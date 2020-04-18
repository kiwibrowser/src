(async function(testRunner) {
  var {page, session, dp} = await testRunner.startURL('../resources/get-layers.html', 'Tests LayerTree domain reporting layers.');

  function layerMutations(oldLayers, newLayers) {
    var oldLayerIds = oldLayers.map(layer => layer.layerId);
    var newLayerIds = newLayers.map(layer => layer.layerId);
    return {
      additions: newLayers.filter(layer => oldLayerIds.indexOf(layer.layerId) === -1),
      removals: oldLayers.filter(layer => newLayerIds.indexOf(layer.layerId) === -1)
    };
  }

  function attributesFromArray(attributes) {
    var map = new Map();
    for (var i = 0, count = attributes.length; i < count; i += 2)
      map.set(attributes[i], attributes[i + 1]);
    return map;
  }

  function dumpLayers(layers) {
    function replacer(key, value) {
      if (['layerId', 'parentLayerId', 'backendNodeId', 'paintCount', 'nearestLayerShiftingContainingBlock'].indexOf(key) >= 0)
        return typeof(value);
      // some values differ based on port, but the ones we most
      // care about will always be less or equal 200.
      if ((key === 'width' || key === 'height') && value > 200)
        return typeof(value);
      return value;
    }

    // Keep 'internal' layers out for better stability.
    layers = layers.filter(layer => !!layer.backendNodeId);
    testRunner.log('\n' + JSON.stringify(layers, replacer, '    '));
  }

  await dp.DOM.getDocument();
  dp.LayerTree.enable();
  var initialLayers = (await dp.LayerTree.onceLayerTreeDidChange()).params.layers;

  dp.Runtime.evaluate({expression: 'addCompositedLayer()'});
  var modifiedLayers = (await dp.LayerTree.onceLayerTreeDidChange()).params.layers;

  var mutations = layerMutations(initialLayers, modifiedLayers);
  var newLayer = mutations.additions[0];

  var nodeResponse = await dp.DOM.pushNodesByBackendIdsToFrontend({backendNodeIds: [newLayer.backendNodeId]});
  var attributesResponse = await dp.DOM.getAttributes({nodeId: nodeResponse.result.nodeIds[0]});
  var attributes = attributesFromArray(attributesResponse.result.attributes);
  if (attributes.get('id') !== 'last-element')
    testRunner.log('FAIL: Did not obtain the expected element for the last inserted layer.');

  dumpLayers(initialLayers);
  dumpLayers(modifiedLayers);
  testRunner.log('DONE!');
  testRunner.completeTest();
})
