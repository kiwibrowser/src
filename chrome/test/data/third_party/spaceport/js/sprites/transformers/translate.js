define([ 'sprites/Transform' ], function (Transform) {
    return function translate(frameIndex, objectIndex) {
        var x = Math.cos(1.2 * (objectIndex + frameIndex * (objectIndex + 1)) / 100) * 160 + 176;
        var y = Math.sin((objectIndex + frameIndex * (objectIndex + 1)) / 100) * 160 + 176;

        return new Transform({
            x: x,
            y: y
        });
    };
});
