// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests Geometry utility class\n`);


  TestRunner.runTestSuite([

    function testVectorLength(next) {
      var testVectors =
          [new UI.Geometry.Vector(3, 4, 5), new UI.Geometry.Vector(-1, 0, -1), new UI.Geometry.Vector(6, -2, 3)];
      TestRunner.addResult('Testing vector length');
      for (var i = 0; i < testVectors.length; ++i)
        TestRunner.addResult(String.sprintf('Vector length: %.4f', testVectors[i].length()));

      next();
    },

    function testVectorNormalize(next) {
      var testVectors =
          [new UI.Geometry.Vector(3, 4, 5), new UI.Geometry.Vector(-1, 0, -1), new UI.Geometry.Vector(6, -2, 3)];

      var eps = 1e-05;
      for (var i = 0; i < testVectors.length; ++i) {
        testVectors[i].normalize();
        TestRunner.assertTrue(Math.abs(testVectors[i].length() - 1) <= eps, 'Length of normalized vector is not 1');
      }

      var zeroVector = new UI.Geometry.Vector(0, 0, 0);
      zeroVector.normalize();
      TestRunner.assertTrue(zeroVector.length() <= eps, 'Zero vector after normalization isn\'t zero vector');
      next();
    },

    function testScalarProduct(next) {
      var vectorsU =
          [new UI.Geometry.Vector(3, 4, 5), new UI.Geometry.Vector(-1, 0, -1), new UI.Geometry.Vector(6, -2, 3)];

      var vectorsV =
          [new UI.Geometry.Vector(1, 10, -5), new UI.Geometry.Vector(2, 3, 4), new UI.Geometry.Vector(0, 0, 0)];

      for (var i = 0; i < vectorsU.length; ++i)
        TestRunner.addResult('Scalar Product:' + UI.Geometry.scalarProduct(vectorsU[i], vectorsV[i]));

      next();
    },

    function testCrossProduct(next) {
      var vectorsU =
          [new UI.Geometry.Vector(3, 4, 5), new UI.Geometry.Vector(-1, 0, -1), new UI.Geometry.Vector(6, -2, 3)];

      var vectorsV =
          [new UI.Geometry.Vector(1, 10, -5), new UI.Geometry.Vector(2, 3, 4), new UI.Geometry.Vector(0, 0, 0)];

      for (var i = 0; i < vectorsU.length; ++i) {
        var result = UI.Geometry.crossProduct(vectorsU[i], vectorsV[i]);
        TestRunner.addResult(String.sprintf('Cross Product: [%.4f, %.4f, %.4f]', result.x, result.y, result.z));
      }

      next();
    },

    function testCalculateAngle(next) {
      var vectorsU = [
        new UI.Geometry.Vector(3, 4, 5),
        new UI.Geometry.Vector(-1, 0, -1),
        new UI.Geometry.Vector(1, 1, 0),
        new UI.Geometry.Vector(6, -2, 3),
      ];

      var vectorsV = [
        new UI.Geometry.Vector(-3, -4, -5),
        new UI.Geometry.Vector(2, 3, 4),
        new UI.Geometry.Vector(-1, 1, 0),
        new UI.Geometry.Vector(0, 0, 0),
      ];

      for (var i = 0; i < vectorsU.length; ++i)
        TestRunner.addResult(
            String.sprintf('Calculate angle: %.4f', UI.Geometry.calculateAngle(vectorsU[i], vectorsV[i])));

      next();
    },

    function testRadiansToDegrees(next) {
      var angles = [Math.PI, Math.PI / 4, Math.PI / 6];
      for (var i = 0; i < angles.length; ++i)
        TestRunner.addResult(String.sprintf('deg: %.4f', UI.Geometry.radiansToDegrees(angles[i])));

      next();
    },

    function testDegreesToRadians(next) {
      var angles = [-30, 0, 30, 90, 180];
      for (var i = 0; i < angles.length; ++i)
        TestRunner.addResult(String.sprintf('rad: %.4f', UI.Geometry.degreesToRadians(angles[i])));

      next();
    },

    function testEulerAnglesFromRotationMatrix(next) {
      var rotationMatrices = [
        'matrix3d(0.612, 0.353, -0.707, 0, 0.280, 0.739, 0.612, 0, 0.739, -0.573, 0.353, 0, 0, 0, 0, 1)',

        'matrix3d(0.353, 0.612, -0.707, 0, -0.573, 0.739, 0.353, 0, 0.739, 0.280, 0.612, 0, 0, 0, 0, 1)',

        'matrix3d(0.342, 0, -0.939, 0, 0.321, 0.939, 0.116, 0, 0.883, -0.342, 0.321, 0, 0, 0, 0, 1)'
      ];

      for (var i = 0; i < rotationMatrices.length; ++i) {
        var angles = UI.Geometry.EulerAngles.fromRotationMatrix(new WebKitCSSMatrix(rotationMatrices[i]));
        TestRunner.addResult(String.sprintf('Euler angles: %.4f %.4f %.4f', angles.alpha, angles.beta, angles.gamma));
      }
      next();
    },

    function testEulerAnglesToRotate3DString(next) {
      var angles = [
        new UI.Geometry.EulerAngles(0, 0, 0), new UI.Geometry.EulerAngles(1, 2, 3),
        new UI.Geometry.EulerAngles(-1, -2, 3), new UI.Geometry.EulerAngles(-1, 2, -3),
        new UI.Geometry.EulerAngles(0, 1, 2), new UI.Geometry.EulerAngles(1, 0, 2), new UI.Geometry.EulerAngles(1, 2, 0)
      ];

      for (var i = 0; i < angles.length; ++i) {
        var rotate3DString = angles[i].toRotate3DString();
        TestRunner.addResult(String.sprintf('Transform: %s', rotate3DString));
      }
      next();
    }

  ]);
})();
