function enableRemotePlaybackBackendForTest(t) {
  var remotePlaybackBackendEnabledOldValue =
      internals.runtimeFlags.remotePlaybackBackendEnabled;
  internals.runtimeFlags.remotePlaybackBackendEnabled = true;

  t.add_cleanup(() => {
    internals.runtimeFlags.remotePlaybackBackendEnabled =
        remotePlaybackBackendEnabledOldValue;
  });
}

function setIsLowEndDeviceForTest(t) {
  var isLowEndDeviceOldValue = internals.isLowEndDevice();
  internals.setIsLowEndDevice(true);

  t.add_cleanup(_ => {
    internals.setIsLowEndDevice(isLowEndDeviceOldValue);
  });
}
