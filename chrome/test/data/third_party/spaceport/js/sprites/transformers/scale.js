define([ 'sprites/Transform' ], function (Transform) {
    return function scale(frameIndex, objectIndex) {
        var scale = Math.sin((objectIndex + frameIndex * (objectIndex + 1)) / 100) / 2 + 1;
        var x = Math.cos(1.2 * objectIndex) * 160 + 176;
        var y = Math.sin(objectIndex) * 160 + 176;

        return new Transform({
            x: x,
            y: y,
            scaleX: scale,
            scaleY: scale
        });
    };
});
