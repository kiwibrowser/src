(class DeviceEmulator {
  constructor(testRunner, session) {
    this._testRunner = testRunner;
    this._session = session;
  }

  async emulate(width, height, deviceScaleFactor, insets) {
    this._testRunner.log(`Emulating device: ${width}x${height}x${deviceScaleFactor}`);
    var full = !!width && !!height && !!deviceScaleFactor;
    var params = {
      width,
      height,
      deviceScaleFactor,
      mobile: true,
      fitWindow: false,
      scale: 1,
      screenWidth: width,
      screenHeight: height,
      positionX: 0,
      positionY: 0
    };
    if (insets) {
      params.screenWidth += insets.left + insets.right;
      params.positionX = insets.left;
      params.screenHeight += insets.top + insets.bottom;
      params.positionY = insets.top;
    }
    var response = await this._session.protocol.Emulation.setDeviceMetricsOverride(params);
    if (response.error)
      this._testRunner.log('Error: ' + response.error);
  }

  async clear() {
    await this._session.protocol.Emulation.clearDeviceMetricsOverride();
  }
})
