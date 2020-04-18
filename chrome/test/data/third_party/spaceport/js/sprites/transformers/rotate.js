define([ 'sprites/Transform' ], function (Transform) {
    return function scale(frameIndex, objectIndex) {
        var s = Math.sin((objectIndex + 1) / 5);
        var rotation = (objectIndex * s / Math.abs(s) + frameIndex * s) * 0.1;
        var x = Math.cos(1.2 * objectIndex) * 160 + 192;
        var y = Math.sin(objectIndex) * 160 + 192;

        return new Transform({
            x: x,
            y: y,
            rotation: rotation
        });
    };
});
