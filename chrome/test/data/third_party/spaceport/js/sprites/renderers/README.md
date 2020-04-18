# Renderers

#### css2dImg

Renders images using the HTML `<img>` element, and transforms them using the
CSS3 2D `transform` property.

    for (var i = 0; i < sprites.length; ++i) {
        var element = elements[i];
        var sprite = sprites[i];

        element.src = sprite.img.src;
        element.style.transform = sprite.cssTransform2d;
    }

#### css3dImg

Renders images using the HTML `<img>` element, and transforms them using the
CSS3 3D `transform` property.

    for (var i = 0; i < sprites.length; ++i) {
        var element = elements[i];
        var sprite = sprites[i];

        element.src = sprite.img.src;
        element.style.transform = sprite.cssTransform3d;
    }

#### css2dBackground

Renders images using the CSS2 `background-image` property, and transforms them
using the CSS3 2D `transform` property.

    for (var i = 0; i < sprites.length; ++i) {
        var element = elements[i];
        var sprite = sprites[i];

        element.style.backgroundImage = 'url(' + sprite.img.src + ')';
        element.style.transform = sprite.cssTransform2d;
        element.style.width = sprite.width + 'px';
        element.style.height = sprite.height + 'px';
    }

#### css3dBackground

Renders images using the CSS2 `background-image` property, and transforms them
using the CSS3 3D `transform` property.

    for (var i = 0; i < sprites.length; ++i) {
        var element = elements[i];
        var sprite = sprites[i];

        element.style.backgroundImage = 'url(' + sprite.img.src + ')';
        element.style.transform = sprite.cssTransform3d;
        element.style.width = sprite.width + 'px';
        element.style.height = sprite.height + 'px';
    }

#### canvasDrawImageFullClear

Clears the entire canvas, then calls drawImage for every sprite.

    canvas.width = canvas.width;
    for (var i = 0; i < sprites.length; ++i) {
        var m = sprites[i].matrix;
        context.setTransform(m[0], m[1], m[3], m[4], m[2], m[5]);
        context.drawImage(sprites[i].img, 0, 0);
    }

#### canvasDrawImageFullClearAlign

Clears the entire canvas, then calls drawImage for every sprite.  Pixel
alignment is enforced when blitting.

    canvas.width = canvas.width;
    for (var i = 0; i < sprites.length; ++i) {
        var transform = sprites[i].transform;
        context.setTransform(1, 0, 0, 1, Math.floor(transform.x), Math.floor(transform.y));
        context.drawImage(sprites[i].img, 0, 0);
    }

#### canvasDrawImagePartialClear

Clears the parts of the canvas rendered to on the previous frame, then calls
drawImage for every sprite.

    for (var i = 0; i < lastSprites.length; ++i) {
        var m = lastSprites[i].matrix;
        context.setTransform(m[0], m[1], m[3], m[4], m[2], m[5]);
        context.clearRect(-1, -1, lastSprites[i].width + 2, lastSprites[i].height + 2);
    }

    for (var i = 0; i < sprites.length; ++i) {
        var m = sprites[i].transform;
        context.setTransform(m[0], m[1], m[3], m[4], m[2], m[5]);
        context.drawImage(sprite[i].img, 0, 0);
    }

#### canvasDrawImagePartialClearAlign

Clears the parts of the canvas rendered to on the previous frame, then calls
drawImage for every sprite.  Pixel alignment is enforced when blitting.

    for (var i = 0; i < lastSprites.length; ++i) {
        var transform = lastSprites[i].transform;
        context.setTransform(1, 0, 0, 1, Math.floor(transform.x), Math.floor(transform.y));
        context.clearRect(-1, -1, lastSprites[i].width + 2, lastSprites[i].height + 2);
    }

    for (var i = 0; i < sprites.length; ++i) {
        var transform = sprite[i].transform;
        context.setTransform(1, 0, 0, 1, Math.floor(transform.x), Math.floor(transform.y));
        context.drawImage(sprite[i].img, 0, 0);
    }
