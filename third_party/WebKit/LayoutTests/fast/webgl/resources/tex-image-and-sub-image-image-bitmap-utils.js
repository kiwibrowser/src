function checkCanvasRect(buf, x, y, width, height, color, tolerance, bufWidth, retVal)
{
    for (var px = x; px < x + width; px++) {
        for (var py = y; py < y + height; py++) {
            var offset = (py * bufWidth + px) * 4;
            for (var j = 0; j < color.length; j++) {
                if (Math.abs(buf[offset + j] - color[j]) > tolerance) {
                    retVal.testPassed = false;
                    return;
                }
            }
        }
    }
}

function runOneIteration(useTexSubImage2D, bindingTarget, program, bitmap, flipY, premultiplyAlpha, retVal, colorSpace = 'empty')
{
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    // Enable writes to the RGBA channels
    gl.colorMask(1, 1, 1, 0);
    var texture = gl.createTexture();
    // Bind the texture to texture unit 0
    gl.bindTexture(bindingTarget, texture);
    // Set up texture parameters
    gl.texParameteri(bindingTarget, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(bindingTarget, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

    var targets = [gl.TEXTURE_2D];
    if (bindingTarget == gl.TEXTURE_CUBE_MAP) {
        targets = [gl.TEXTURE_CUBE_MAP_POSITIVE_X,
                   gl.TEXTURE_CUBE_MAP_NEGATIVE_X,
                   gl.TEXTURE_CUBE_MAP_POSITIVE_Y,
                   gl.TEXTURE_CUBE_MAP_NEGATIVE_Y,
                   gl.TEXTURE_CUBE_MAP_POSITIVE_Z,
                   gl.TEXTURE_CUBE_MAP_NEGATIVE_Z];
    }
    // Upload the image into the texture
    for (var tt = 0; tt < targets.length; ++tt) {
        if (useTexSubImage2D) {
            // Initialize the texture to black first
            gl.texImage2D(targets[tt], 0, gl[internalFormat], bitmap.width, bitmap.height, 0,
                          gl[pixelFormat], gl[pixelType], null);
            gl.texSubImage2D(targets[tt], 0, 0, 0, gl[pixelFormat], gl[pixelType], bitmap);
        } else {
            gl.texImage2D(targets[tt], 0, gl[internalFormat], gl[pixelFormat], gl[pixelType], bitmap);
        }
    }

    var width = gl.canvas.width;
    var halfWidth = Math.floor(width / 2);
    var quaterWidth = Math.floor(halfWidth / 2);
    var height = gl.canvas.height;
    var halfHeight = Math.floor(height / 2);
    var quaterHeight = Math.floor(halfHeight / 2);

    var top = flipY ? quaterHeight : (height - halfHeight + quaterHeight);
    var bottom = flipY ? (height - halfHeight + quaterHeight) : quaterHeight;

    var tl = redColor;
    var tr = premultiplyAlpha ? ((retVal.alpha == 0.5) ? darkRed : (retVal.alpha == 1) ? redColor : blackColor) : redColor;
    var bl = greenColor;
    var br = premultiplyAlpha ? ((retVal.alpha == 0.5) ? darkGreen : (retVal.alpha == 1) ? greenColor : blackColor) : greenColor;

    var blueColor = [0, 0, 255];
    if (colorSpace == 'none') {
        tl = tr = bl = br = blueColor;
    } else if (colorSpace == 'default' || colorSpace == 'notprovided') {
        tl = tr = bl = br = redColor;
    }

    var loc;
    if (bindingTarget == gl.TEXTURE_CUBE_MAP) {
        loc = gl.getUniformLocation(program, "face");
    }

    var tolerance = (retVal.alpha == 0) ? 0 : 2;
    if (colorSpace == 'default' || colorSpace == 'none' || colorSpace == 'notprovided')
        tolerance = 13; // For linux and win, the tolerance can be 8.
    for (var tt = 0; tt < targets.length; ++tt) {
        if (bindingTarget == gl.TEXTURE_CUBE_MAP) {
            gl.uniform1i(loc, targets[tt]);
        }
        // Draw the triangles
        gl.clearColor(0, 0, 0, 1);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        gl.drawArrays(gl.TRIANGLES, 0, 6);

        // Check the top pixel and bottom pixel and make sure they have
        // the right color.
        var buf = new Uint8Array(width * height * 4);
        gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, buf);
        checkCanvasRect(buf, quaterWidth, bottom, 2, 2, tl, tolerance, width, retVal);
        checkCanvasRect(buf, halfWidth + quaterWidth, bottom, 2, 2, tr, tolerance, width, retVal);
        checkCanvasRect(buf, quaterWidth, top, 2, 2, bl, tolerance, width, retVal);
        checkCanvasRect(buf, halfWidth + quaterWidth, top, 2, 2, br, tolerance, width, retVal);
    }
}

function runTestOnBindingTarget(bindingTarget, program, bitmaps, retVal) {
    var cases = [
        { sub: false, bitmap: bitmaps.defaultOption, flipY: false, premultiply: true, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.defaultOption, flipY: false, premultiply: true, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.noFlipYPremul, flipY: false, premultiply: true, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.noFlipYPremul, flipY: false, premultiply: true, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.noFlipYDefault, flipY: false, premultiply: true, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.noFlipYDefault, flipY: false, premultiply: true, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.noFlipYUnpremul, flipY: false, premultiply: false, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.noFlipYUnpremul, flipY: false, premultiply: false, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.flipYPremul, flipY: true, premultiply: true, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.flipYPremul, flipY: true, premultiply: true, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.flipYDefault, flipY: true, premultiply: true, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.flipYDefault, flipY: true, premultiply: true, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.flipYUnpremul, flipY: true, premultiply: false, colorSpace: 'empty' },
        { sub: true, bitmap: bitmaps.flipYUnpremul, flipY: true, premultiply: false, colorSpace: 'empty' },
        { sub: false, bitmap: bitmaps.colorSpaceDef, flipY: false, premultiply: true, colorSpace: retVal.colorSpaceEffect ? 'notprovided' : 'empty' },
        { sub: true, bitmap: bitmaps.colorSpaceDef, flipY: false, premultiply: true, colorSpace: retVal.colorSpaceEffect ? 'notprovided' : 'empty' },
        { sub: false, bitmap: bitmaps.colorSpaceNone, flipY: false, premultiply: true, colorSpace: retVal.colorSpaceEffect ? 'none' : 'empty' },
        { sub: true, bitmap: bitmaps.colorSpaceNone, flipY: false, premultiply: true, colorSpace: retVal.colorSpaceEffect ? 'none' : 'empty' },
        { sub: false, bitmap: bitmaps.colorSpaceDefault, flipY: false, premultiply: true, colorSpace: retVal.colorSpaceEffect ? 'default' : 'empty' },
        { sub: true, bitmap: bitmaps.colorSpaceDefault, flipY: false, premultiply: true, colorSpace: retVal.colorSpaceEffect ? 'default' : 'empty' },
    ];

    for (var i in cases) {
        runOneIteration(cases[i].sub, bindingTarget, program, cases[i].bitmap, cases[i].flipY,
            cases[i].premultiply, retVal, cases[i].colorSpace);
    }
}

function runTest(bitmaps, alphaVal, colorSpaceEffective)
{
    var retVal = {testPassed: true, alpha: alphaVal, colorSpaceEffect: colorSpaceEffective};
    var program = tiu.setupTexturedQuad(gl, internalFormat);
    runTestOnBindingTarget(gl.TEXTURE_2D, program, bitmaps, retVal);
    program = tiu.setupTexturedQuadWithCubeMap(gl, internalFormat);
    runTestOnBindingTarget(gl.TEXTURE_CUBE_MAP, program, bitmaps, retVal);
    return retVal.testPassed;
}
