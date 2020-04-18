# Generic Sensor

`third_party/blink/renderer/modules/sensor` implements the following concrete
sensor intrefaces based on the [Generic Sensor API]
(https://w3c.github.io/sensors):
1. [Ambient Light Sensor] (https://w3c.github.io/ambient-light)
1. [Accelerometer] (https://w3c.github.io/accelerometer)
1. [Gyroscope] (https://w3c.github.io/gyroscope)
1. [Magnetometer] (https://w3c.github.io/magnetometer)
1. [Absolute Orientation Sensor] (https://w3c.github.io/orientation-sensor)

The platform-specific parts of the implementation are located in
`device/generic_sensor`.


## Testing

Sensors layout tests are located in `LayoutTests/sensor`.

Sensors browser tests are located in `content/test/data/generic_sensor`.


## Design Documents

Please refer to the [design documentation](https://docs.google.com/document/d/1Ml65ZdW5AgIsZTszk4mD_ohr40pcrdVFOIf0ZtWxDv0) for more details.
