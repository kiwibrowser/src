'use strict';

// Run a set of tests for a given |sensorType|. |updateReading| is
// a called by the test to provide the mock values for sensor. |verifyReading|
// is called so that the value read in JavaScript are the values expected (the ones
// sent by |updateReading|).
function runGenericSensorTests(sensorType,
                               mojomSensorType,
                               updateReading,
                               verifyReading,
                               verifyRemappedReading,
                               featurePolicies) {
  sensor_test(sensor => {
    sensor.mockSensorProvider.setGetSensorShouldFail(true);
    let sensorObject = new sensorType;
    sensorObject.start();
    return new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(event => {
        assert_false(sensorObject.activated);
        assert_equals(event.error.name, 'NotReadableError');
        sensorObject.onerror = null;
        resolve();
      }, reject);

      sensorObject.onerror = wrapper.callback;
    });
  }, `${sensorType.name}: Test that onerror is sent when sensor is not supported.`);

  sensor_test(sensor => {
    sensor.mockSensorProvider.setPermissionsDenied(true);
    let sensorObject = new sensorType;
    sensorObject.start();
    return new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(event => {
        assert_false(sensorObject.activated);
        assert_equals(event.error.name, 'NotAllowedError');
        sensorObject.onerror = null;
        resolve();
      }, reject);

      sensorObject.onerror = wrapper.callback;
    });
  }, `${sensorType.name}: Test that onerror is sent when permissions are not granted.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 560});
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    mockSensor.setStartShouldFail(true);
    await mockSensor.addConfigurationCalled();
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(event => {
        assert_false(sensorObject.activated);
        assert_equals(event.error.name, 'NotReadableError');
        sensorObject.onerror = null;
        resolve();
      }, reject);

      sensorObject.onerror = wrapper.callback;
    });
  }, `${sensorType.name}: Test that onerror is send when start() call has failed.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType();
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    mockSensor.setStartShouldFail(true);
    await mockSensor.addConfigurationCalled();
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that no pending configuration left after start() failure.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 560});
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.addConfigurationCalled();
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_less_than_equal(mockSensor.getSamplingFrequency(), 60);
        sensorObject.stop();
        assert_false(sensorObject.activated);
        resolve(mockSensor);
      }, reject);
      sensorObject.onactivate = wrapper.callback;
      sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that frequency is capped to allowed maximum.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType();
    sensorObject.start();
    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.addConfigurationCalled();
    await new Promise((resolve, reject) => {
      sensorObject.onactivate = () => {
        // Now sensor proxy is initialized.
        let anotherSensor = new sensorType({frequency: 21});
        anotherSensor.start();
        anotherSensor.stop();
        resolve(mockSensor);
      }
    });
    await mockSensor.removeConfigurationCalled();
    sensorObject.stop();
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that configuration is removed for a stopped sensor.`);

  sensor_test(async sensor => {
    const maxSupportedFrequency = 5;
    sensor.mockSensorProvider.setMaximumSupportedFrequency(maxSupportedFrequency);
    let sensorObject = new sensorType({frequency: 50});
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.addConfigurationCalled();
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_equals(mockSensor.getSamplingFrequency(), maxSupportedFrequency);
        sensorObject.stop();
        assert_false(sensorObject.activated);
        resolve(mockSensor);
     }, reject);
     sensorObject.onactivate = wrapper.callback;
     sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that frequency is capped to the maximum supported from frequency.`);

  sensor_test(async sensor => {
    const minSupportedFrequency = 2;
    sensor.mockSensorProvider.setMinimumSupportedFrequency(minSupportedFrequency);
    let sensorObject = new sensorType({frequency: -1});
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.addConfigurationCalled();
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_equals(mockSensor.getSamplingFrequency(), minSupportedFrequency);
        sensorObject.stop();
        assert_false(sensorObject.activated);
        resolve(mockSensor);
     }, reject);
     sensorObject.onactivate = wrapper.callback;
     sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that frequency is limited to the minimum supported from frequency.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 60});
    assert_false(sensorObject.activated);
    sensorObject.start();
    assert_false(sensorObject.activated);

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(sensorObject.activated);
        sensorObject.stop();
        assert_false(sensorObject.activated);
        resolve(mockSensor);
      }, reject);
      sensorObject.onactivate = wrapper.callback;
      sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that sensor can be successfully created and its states are correct.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType();
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(sensorObject.activated);
        sensorObject.stop();
        assert_false(sensorObject.activated);
        resolve(mockSensor);
      }, reject);

      sensorObject.onactivate = wrapper.callback;
      sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that sensor can be constructed with default configuration.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 60});
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.addConfigurationCalled();
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(sensorObject.activated);
        sensorObject.stop();
        assert_false(sensorObject.activated);
        resolve(mockSensor);
     }, reject);
     sensorObject.onactivate = wrapper.callback;
     sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that addConfiguration and removeConfiguration is called.`);

  async function checkOnReadingIsCalledAndReadingIsValid(sensor) {
    let sensorObject = new sensorType({frequency: 60});
    sensorObject.start();
    assert_false(sensorObject.hasReading);

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(updateReading);
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(verifyReading(sensorObject));
        assert_true(sensorObject.hasReading);
        sensorObject.stop();
        assert_true(verifyReading(sensorObject, true /*should be null*/));
        assert_false(sensorObject.hasReading);
        resolve(mockSensor);
      }, reject);

      sensorObject.onreading = wrapper.callback;
      sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }

  sensor_test(sensor => checkOnReadingIsCalledAndReadingIsValid(sensor),
  `${sensorType.name}: Test that onreading is called and sensor reading is valid (onchange reporting).`);

  sensor_test(sensor => {
    sensor.mockSensorProvider.setContinuousReportingMode();
    return checkOnReadingIsCalledAndReadingIsValid(sensor);
  }, `${sensorType.name}: Test that onreading is called and sensor reading is valid (continuous reporting).`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 60});
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(updateReading);
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(verifyReading(sensorObject));
        resolve(mockSensor);
      }, reject);

      sensorObject.onreading = wrapper.callback;
      sensorObject.onerror = reject;
    });
    testRunner.setPageVisibility('hidden');
    await mockSensor.suspendCalled();

    testRunner.setPageVisibility('visible');
    await mockSensor.resumeCalled();

    sensorObject.stop();
    await mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that sensor receives suspend / resume notifications when page\
 visibility changes.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 60});
    sensorObject.start();

    // Create a focused editbox inside a cross-origin iframe, sensor notification must suspend.
    const iframeSrc = 'data:text/html;charset=utf-8,<html><body><input type="text" autofocus></body></html>';
    let iframe = document.createElement('iframe');
    iframe.src = encodeURI(iframeSrc);

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(updateReading);

    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(verifyReading(sensorObject));
        resolve(mockSensor);
      }, reject);

      sensorObject.onreading = wrapper.callback;
      sensorObject.onerror = reject;
    });

    document.body.appendChild(iframe);
    await mockSensor.suspendCalled();

    window.focus();
    await mockSensor.resumeCalled();

    sensorObject.stop();
    document.body.removeChild(iframe);
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that sensor receives suspend / resume notifications when\
 cross-origin subframe is focused`);

  sensor_test(async sensor => {
    let sensor1 = new sensorType({frequency: 60});
    sensor1.start();

    let sensor2 = new sensorType({frequency: 20});
    sensor2.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(updateReading);
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        // Reading values are correct for both sensors.
        assert_true(verifyReading(sensor1));
        assert_true(verifyReading(sensor2));

        // After first sensor stops its reading values are null,
        // reading values for the second sensor sensor remain.
        sensor1.stop();
        assert_true(verifyReading(sensor1, true /*should be null*/));
        assert_true(verifyReading(sensor2));

        sensor2.stop();
        assert_true(verifyReading(sensor2, true /*should be null*/));

        resolve(mockSensor);
      }, reject);

      sensor1.onreading = wrapper.callback;
      sensor1.onerror = reject;
      sensor2.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that sensor reading is correct.`);

  async function checkFrequencyHintWorks(sensor) {
    let fastSensor = new sensorType({frequency: 60});
    fastSensor.start();

    let slowSensor;  // To be initialized later.

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(updateReading);
    await new Promise((resolve, reject) => {
      let fastSensorNotifiedCounter = 0;
      let slowSensorNotifiedCounter = 0;

      let slowSensorWrapper = new CallbackWrapper(() => {
        // Skip the initial notification that always comes immediately.
        if (slowSensorNotifiedCounter === 1) {
          assert_true(fastSensorNotifiedCounter > 2,
                      "Fast sensor overtakes the slow one");
          fastSensor.stop();
          slowSensor.stop();
          resolve(mockSensor);
        }

        slowSensorNotifiedCounter++;
      }, reject);

      let fastSensorWrapper = new CallbackWrapper(() => {
        if (fastSensorNotifiedCounter === 0) {
          // For Magnetometer and ALS, the maximum frequency is less than 60Hz
          // we make "slow" sensor 4 times slower than the actual applied
          // frequency, so that the "fast" sensor will immediately overtake it
          // despite the notification adjustments.
          const slowFrequency = mockSensor.getSamplingFrequency() * 0.25;
          slowSensor = new sensorType({frequency: slowFrequency});
          slowSensor.onreading = slowSensorWrapper.callback;
          slowSensor.onerror = reject;
          slowSensor.start();
        }

        fastSensorNotifiedCounter++;
      }, reject);

      fastSensor.onreading = fastSensorWrapper.callback;
      fastSensor.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }

  sensor_test(sensor => checkFrequencyHintWorks(sensor),
  `${sensorType.name}: Test that frequency hint works (onchange reporting).`);

  sensor_test(sensor => {
    sensor.mockSensorProvider.setContinuousReportingMode();
    return checkFrequencyHintWorks(sensor);
  }, `${sensorType.name}: Test that frequency hint works (continuous reporting).`);

  promise_test(() => {
    return new Promise((resolve,reject) => {
      let iframe = document.createElement('iframe');
      iframe.allow = featurePolicies.join(' \'none\'; ') + ' \'none\';';
      iframe.srcdoc = '<script>' +
                      '  window.onmessage = message => {' +
                      '    if (message.data === "LOADED") {' +
                      '      try {' +
                      '        new ' + sensorType.name + '();' +
                      '        parent.postMessage("FAIL", "*");' +
                      '      } catch (e) {' +
                      '        parent.postMessage("PASS", "*");' +
                      '      }' +
                      '    }' +
                      '   };' +
                      '<\/script>';
      iframe.onload = () => iframe.contentWindow.postMessage('LOADED', '*');
      document.body.appendChild(iframe);
      window.onmessage = message => {
        if (message.data == 'PASS') {
          resolve();
        } else if (message.data == 'FAIL') {
          reject();
        }
      }
    });
  }, `${sensorType.name}: Test that sensor cannot be constructed within iframe disallowed to use feature policy.`);

  promise_test(() => {
    return new Promise((resolve,reject) => {
      let iframe = document.createElement('iframe');
      iframe.allow = featurePolicies.join(';') + ';';
      iframe.srcdoc = '<script>' +
                      '  window.onmessage = message => {' +
                      '    if (message.data === "LOADED") {' +
                      '      try {' +
                      '        new ' + sensorType.name + '();' +
                      '        parent.postMessage("PASS", "*");' +
                      '      } catch (e) {' +
                      '        parent.postMessage("FAIL", "*");' +
                      '      }' +
                      '    }' +
                      '   };' +
                      '<\/script>';
      iframe.onload = () => iframe.contentWindow.postMessage('LOADED', '*');
      document.body.appendChild(iframe);
      window.onmessage = message => {
        if (message.data == 'PASS') {
          resolve();
        } else if (message.data == 'FAIL') {
          reject();
        }
      }
    });
  }, `${sensorType.name}: Test that sensor can be constructed within an iframe allowed to use feature policy.`);

  sensor_test(async sensor => {
    let sensorObject = new sensorType({frequency: 60});
    let timestamp = 0;
    sensorObject.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(updateReading);
    await new Promise((resolve, reject) => {
      let wrapper1 = new CallbackWrapper(() => {
        assert_true(sensorObject.hasReading);
        assert_true(verifyReading(sensorObject));
        timestamp = sensorObject.timestamp;
        sensorObject.stop();

        assert_false(sensorObject.hasReading);
        sensorObject.onreading = wrapper2.callback;
        sensorObject.start();
      }, reject);

      let wrapper2 = new CallbackWrapper(() => {
        assert_true(sensorObject.hasReading);
        assert_true(verifyReading(sensorObject));
        // Make sure that 'timestamp' is already initialized.
        assert_greater_than(timestamp, 0);
        // Check that the reading is updated.
        assert_greater_than(sensorObject.timestamp, timestamp);
        sensorObject.stop();
        resolve(mockSensor);
      }, reject);

      sensorObject.onreading = wrapper1.callback;
      sensorObject.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that fresh reading is fetched on start().`);

  sensor_test(async sensor => {
    if (!verifyRemappedReading) {
      // The sensorType does not represent a spatial sensor.
      return;
    }

    let sensor1 = new sensorType({frequency: 60});
    let sensor2 = new sensorType({frequency: 60, referenceFrame: "screen"});

    sensor1.start();
    sensor2.start();

    let mockSensor = await sensor.mockSensorProvider.getCreatedSensor(mojomSensorType);
    await mockSensor.setUpdateSensorReadingFunction(update_sensor_reading);
    await new Promise((resolve, reject) => {
      let wrapper = new CallbackWrapper(() => {
        assert_true(verifyReading(sensor1));
        assert_true(verifyRemappedReading(sensor2));

        sensor1.stop();
        assert_true(verifyReading(sensor1, true /*should be null*/));
        assert_true(verifyRemappedReading(sensor2));

        sensor2.stop();
        assert_true(verifyRemappedReading(sensor2, true /*should be null*/));

        resolve(mockSensor);
      }, reject);

      sensor1.onreading = wrapper.callback;
      sensor1.onerror = reject;
      sensor2.onerror = reject;
    });
    return mockSensor.removeConfigurationCalled();
  }, `${sensorType.name}: Test that reading is mapped to the screen coordinates`);
}
