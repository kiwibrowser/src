(async function(testRunner, session) {
  self['Common'] = {};
  self['TextUtils'] = {};
  self['HeapSnapshotModel'] = {};
  self['HeapSnapshotWorker'] = {};

  // This script is supposed to be evaluated in inspector-protocol/heap-profiler tests
  // and the relative paths below are relative to that location.
  await testRunner.loadScript('../../../../blink/renderer/devtools/front_end/platform/utilities.js');
  await testRunner.loadScript('../../../../blink/renderer/devtools/front_end/common/UIString.js');
  await testRunner.loadScript('../../../../blink/renderer/devtools/front_end/heap_snapshot_model/HeapSnapshotModel.js');
  await testRunner.loadScript('../../../../blink/renderer/devtools/front_end/heap_snapshot_worker/HeapSnapshot.js');
  await testRunner.loadScript('../../../../blink/renderer/devtools/front_end/text_utils/TextUtils.js');
  await testRunner.loadScript('../../../../blink/renderer/devtools/front_end/heap_snapshot_worker/HeapSnapshotLoader.js');

  async function takeHeapSnapshotInternal(command) {
    var loader = new HeapSnapshotWorker.HeapSnapshotLoader();
    function onChunk(messageObject) {
      loader.write(messageObject['params']['chunk']);
    }
    session.protocol.HeapProfiler.onAddHeapSnapshotChunk(onChunk);
    await command();
    session.protocol.HeapProfiler.offAddHeapSnapshotChunk(onChunk);
    testRunner.log('Took heap snapshot');
    loader.close();
    var snapshot = loader.buildSnapshot(false);
    testRunner.log('Parsed snapshot');
    return snapshot;
  }

  function firstRetainingPath(node) {
    for (var iter = node.retainers(); iter.hasNext(); iter.next()) {
      var retainingEdge = iter.retainer;
      var retainer = retainingEdge.node();
      if (retainingEdge.isWeak() ||
          retainer.distance() >= node.distance()) continue;
      var path = firstRetainingPath(retainer);
      path.unshift(retainer);
      return path;
    }
    return [];
  }

  return {
    firstRetainingPath: firstRetainingPath,

    takeHeapSnapshot: function() {
      return takeHeapSnapshotInternal(() => session.protocol.HeapProfiler.takeHeapSnapshot());
    },

    stopRecordingHeapTimeline: function() {
      return takeHeapSnapshotInternal(() => session.protocol.HeapProfiler.stopTrackingHeapObjects());
    }
  };
})
