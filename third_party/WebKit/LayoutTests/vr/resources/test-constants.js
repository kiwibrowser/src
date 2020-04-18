// assert_equals can fail when comparing floats due to precision errors, so
// use assert_approx_equals with this constant instead
var FLOAT_EPSILON = 0.000001;

// A valid VRPose for when we don't care about specific values
var VALID_POSE = {
  position: [1.1, 2.2, 3.3],
  linearVelocity: [0.1, 0.2, 0.3],
  linearAcceleration: [0.0, 0.1, 0.2],
  orientation: [0.1, 0.2, 0.3, 0.4],
  angularVelocity: [1.1, 2.1, 3.1],
  angularAcceleration: [1.0, 2.0, 3.0]
}
