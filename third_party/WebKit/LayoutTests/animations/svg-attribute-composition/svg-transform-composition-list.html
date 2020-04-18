<!DOCTYPE html>
<html>
<body>
<template id="target-template">
<svg width="90" height="90">
<line x1="1" y1="2" x2="3" y2="4" class="target" />
</svg>
</template>
<script src="../svg-attribute-interpolation/resources/interpolation-test.js"></script>
<script>
'use strict';

// Common list of transforms

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) rotate(10 10 10)',
  from: 'translate(100 10) scale(10 20) rotate(10 20 30)',
  fromComposite: 'add',
  to: 'translate(120 50) scale(20 50) rotate(30 70 150)',
  toComposite: 'add'
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(92 -6) scale(6 8) rotate(2 0 -18)'},
  {at: 0, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(100 10) scale(10 20) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(104 18) scale(12 26) rotate(14 30 54)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(112 34) scale(16 38) rotate(22 50 102)'},
  {at: 1, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(120 50) scale(20 50) rotate(30 70 150)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(128 66) scale(24 62) rotate(38 90 198)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) rotate(10 10 10)',
  from: 'translate(110 20) scale(10 20) rotate(10 20 30)',
  fromComposite: 'replace',
  to: 'translate(120 50) scale(20 50) rotate(30 70 150)',
  toComposite: 'add'
}, [
  {at: -0.4, is: 'translate(110 20) scale(10 20) rotate(10 20 30)'},
  {at: 0, is: 'translate(110 20) scale(10 20) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(110 20) scale(10 20) rotate(10 20 30)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(120 50) scale(20 50) rotate(30 70 150)'},
  {at: 1, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(120 50) scale(20 50) rotate(30 70 150)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) rotate(10 10 10) translate(120 50) scale(20 50) rotate(30 70 150)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(110 20) scale(10 20) rotate(10 20 30)',
  from: neutralKeyframe,
  to: 'translate(130 60) scale(30 60) rotate(40 80 160)',
  toComposite: 'replace'
}, [
  {at: -0.4, is: 'translate(102,4) scale(2 4) rotate(-2 -4 -22)'},
  {at: 0, is: 'translate(110 20) scale(10 20) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(114,28) scale(14 28) rotate(16 32 56)'},
  {at: 0.6, is: 'translate(122,44) scale(22 44) rotate(28 56 108)'},
  {at: 1, is: 'translate(130 60) scale(30 60) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(138,76) scale(38 76) rotate(52 104 212)'}
]);

// Distinct list of transforms

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) skewX(10) skewY(10)',
  from: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)',
  fromComposite: 'add',
  to: 'translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)',
  toComposite: 'add',
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) skewX(10) skewY(10) translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0, is: 'translate(10 10) scale(10 10) skewX(10) skewY(10) translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) skewX(10) skewY(10) translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) skewX(10) skewY(10) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'},
  {at: 1, is: 'translate(10 10) scale(10 10) skewX(10) skewY(10) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) skewX(10) skewY(10) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10)',
  from: 'translate(100 10) scale(0 10) skewX(0) rotate(0 10 20)',
  fromComposite: 'add',
  to: 'translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)',
  toComposite: 'add',
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(100 10) scale(0 10) skewX(0) rotate(0 10 20)'},
  {at: 0, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(100 10) scale(0 10) skewX(0) rotate(0 10 20)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(100 10) scale(0 10) skewX(0) rotate(0 10 20)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'},
  {at: 1, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10)',
  from: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)',
  fromComposite: 'add',
  to: 'translate(120 50) scale(20 50) skewY(-40) rotate(30 70 150)',
  toComposite: 'add',
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10) translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0, is: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10) translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10) translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10) translate(120 50) scale(20 50) skewY(-40) rotate(30 70 150)'},
  {at: 1, is: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10) translate(120 50) scale(20 50) skewY(-40) rotate(30 70 150)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) skewY(10) rotate(10 10 10) translate(120 50) scale(20 50) skewY(-40) rotate(30 70 150)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)',
  from: neutralKeyframe,
  to: 'translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)',
  toComposite: 'add',
}, [
  {at: -0.4, is: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0, is: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30)'},
  {at: 0.6, is: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'},
  {at: 1, is: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(110 20) scale(10 20) skewX(10) rotate(10 20 30) translate(130 60) scale(30 60) skewY(-30) rotate(40 80 160)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10)',
  from: 'translate(110 20) scale(10 20) skewX(10)',
  fromComposite: 'add',
  to: 'translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)',
  toComposite: 'add'
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) translate(110 20) scale(10 20) skewX(10)'},
  {at: 0, is: 'translate(10 10) scale(10 10) translate(110 20) scale(10 20) skewX(10)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) translate(110 20) scale(10 20) skewX(10)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'},
  {at: 1, is: 'translate(10 10) scale(10 10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) skewX(10)',
  from: 'translate(100 10) scale(0 10) skewX(0)',
  fromComposite: 'add',
  to: 'translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)',
  toComposite: 'add'
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) skewX(10) translate(100 10) scale(0 10) skewX(0)'},
  {at: 0, is: 'translate(10 10) scale(10 10) skewX(10) translate(100 10) scale(0 10) skewX(0)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) skewX(10) translate(100 10) scale(0 10) skewX(0)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) skewX(10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'},
  {at: 1, is: 'translate(10 10) scale(10 10) skewX(10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) skewX(10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10)',
  from: 'translate(110 20) scale(10 20) skewX(10)',
  fromComposite: 'add',
  to: 'translate(120 50) scale(20 50) skewX(-40) rotate(30 70 150)',
  toComposite: 'add'
}, [
  {at: -0.4, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(110 20) scale(10 20) skewX(10)'},
  {at: 0, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(110 20) scale(10 20) skewX(10)'},
  {at: 0.2, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(110 20) scale(10 20) skewX(10)'},
  {at: 0.6, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(120 50) scale(20 50) skewX(-40) rotate(30 70 150)'},
  {at: 1, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(120 50) scale(20 50) skewX(-40) rotate(30 70 150)'},
  {at: 1.4, is: 'translate(10 10) scale(10 10) skewX(10) rotate(10 10 10) translate(120 50) scale(20 50) skewX(-40) rotate(30 70 150)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(110 20) scale(10 20) skewX(10)',
  from: neutralKeyframe,
  to: 'translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)',
  toComposite: 'add'
}, [
  {at: -0.4, is: 'translate(110 20) scale(10 20) skewX(10)'},
  {at: 0, is: 'translate(110 20) scale(10 20) skewX(10)'},
  {at: 0.2, is: 'translate(110 20) scale(10 20) skewX(10)'},
  {at: 0.6, is: 'translate(110 20) scale(10 20) skewX(10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'},
  {at: 1, is: 'translate(110 20) scale(10 20) skewX(10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'},
  {at: 1.4, is: 'translate(110 20) scale(10 20) skewX(10) translate(130 60) scale(30 60) skewX(-30) rotate(40 80 160)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10)',
  from: 'rotate(10 20 30)',
  fromComposite: 'add',
  to: 'translate(20 20) rotate(10 10 10)',
  toComposite: 'replace'
}, [
  {at: -0.4, is: 'translate(6 6) rotate(10 24 38)'},
  {at: 0, is: 'translate(10 10) rotate(10 20 30)'},
  {at: 0.2, is: 'translate(12 12) rotate(10 18 26)'},
  {at: 0.6, is: 'translate(16 16) rotate(10 14 18)'},
  {at: 1, is: 'translate(20 20) rotate(10 10 10)'},
  {at: 1.4, is: 'translate(24 24) rotate(10 6 2)'}
]);

assertAttributeInterpolation({
  property: 'transform',
  underlying: 'translate(10 10) scale(10 20)',
  from: 'rotate(10 20 30) translate(5 15)',
  fromComposite: 'add',
  to: 'translate(20 20) scale(20 30) rotate(10 10 10) translate(5 5)',
  toComposite: 'replace'
}, [
  {at: -0.4, is: 'translate(6 6) scale(6 16) rotate(10 24 38) translate(5 19)'},
  {at: 0, is: 'translate(10 10) scale(10 20) rotate(10 20 30) translate(5 15)'},
  {at: 0.2, is: 'translate(12 12) scale(12 22) rotate(10 18 26) translate(5 13)'},
  {at: 0.6, is: 'translate(16 16) scale(16 26) rotate(10 14 18) translate(5 9)'},
  {at: 1, is: 'translate(20 20) scale(20 30) rotate(10 10 10) translate(5 5)'},
  {at: 1.4, is: 'translate(24 24) scale(24 34) rotate(10 6 2) translate(5 1)'}
]);

</script>
</body>
</html>
