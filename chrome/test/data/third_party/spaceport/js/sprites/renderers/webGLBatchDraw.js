define([ 'util/ensureCallback', 'sprites/canvas', 'sprites/webGL' ], function (ensureCallback, canvas, webGL) {
    var FLOATS_PER_VERT = 4;
    var VERTS_PER_SPRITE = 6;
    var FLOATS_PER_SPRITE = VERTS_PER_SPRITE * FLOATS_PER_VERT;

    function RenderContext(sourceData, frameData) {
        this.sourceData = sourceData;
        this.frameData = frameData;

        this.canvas = canvas();
        var gl = webGL.getContext(this.canvas);
        this.context = gl;

        this.program = webGL.shaders.batchSprite(gl);

        // Buffer data is interleaved:
        //
        // struct bufferUnit {
        //   vec2 aCoord;
        //   vec2 aTexCoord;  -- Constant
        // };
        //
        // sizeof(bufferUnit) == sizeof(float) * 4 == 16
        //
        // There are six of these structs per sprite
        // (one for each corner of the each triangle,
        // and two triangles per square).

        var maxSprites = Math.max.apply(Math, frameData.map(function (arr) {
            return arr.length;
        }));

        var bufferData = new Float32Array(maxSprites * FLOATS_PER_SPRITE);
        for (var i = 0, j = 0; i < maxSprites; ++i, j += FLOATS_PER_SPRITE) {
            // p0
            bufferData[j + 0*4 + 2] = 0;
            bufferData[j + 0*4 + 3] = 0;

            // p1
            bufferData[j + 1*4 + 2] = 1;
            bufferData[j + 1*4 + 3] = 0;

            // p2
            bufferData[j + 2*4 + 2] = 0;
            bufferData[j + 2*4 + 3] = 1;

            // p3
            bufferData[j + 3*4 + 2] = 1;
            bufferData[j + 3*4 + 3] = 0;

            // p4
            bufferData[j + 4*4 + 2] = 0;
            bufferData[j + 4*4 + 3] = 1;

            // p5
            bufferData[j + 5*4 + 2] = 1;
            bufferData[j + 5*4 + 3] = 1;
        }

        this.bufferData = bufferData;

        var buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, bufferData, gl.DYNAMIC_DRAW);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        this.buffer = buffer;

        this.texture = webGL.makeTexture(gl, this.sourceData.img);

        gl.enable(gl.BLEND);
        gl.blendEquationSeparate(gl.FUNC_ADD, gl.FUNC_ADD);
        gl.blendFuncSeparate(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA, gl.SRC_ALPHA, gl.ONE);
    }

    RenderContext.prototype.load = function load(callback) {
        callback = ensureCallback(callback);

        document.body.appendChild(this.canvas);

        this.clear();

        callback(null);
    };

    RenderContext.prototype.unload = function unload() {
        if (this.canvas.parentNode) {
            this.canvas.parentNode.removeChild(this.canvas);
        }
        
        var gl = this.context;
        gl.deleteTexture(this.texture);
        this.texture = null;
        
        gl.deleteProgram(this.program);
        this.program = null;
        
        gl.deleteBuffer(this.buffer);
        this.buffer = null;
        this.bufferData = null;
    };

    RenderContext.prototype.clear = function clear() {
        var gl = this.context;
        gl.viewport(0, 0, this.canvas.width, this.canvas.height);
        gl.clearColor(255, 255, 255, 255);
        gl.clear(gl.COLOR_BUFFER_BIT);
    };

    RenderContext.prototype.renderFrame = function renderFrame(frameIndex) {
        var gl = this.context;
        var sourceData = this.sourceData;

        var img = sourceData.img;
        var imgWidth = img.width;
        var imgHeight = img.height;

        var transforms = this.frameData[frameIndex];
        var count = transforms.length;

        var bufferData = this.bufferData;

        var i, j;
        for (i = 0, j = 0; i < count; ++i, j += FLOATS_PER_SPRITE) {
            var t = transforms[i];
            t.transformPointInto(0,        0,         bufferData, j + 0);
            t.transformPointInto(imgWidth, 0,         bufferData, j + 4);
            t.transformPointInto(0,        imgHeight, bufferData, j + 8);

            bufferData[j + 12] = bufferData[j + 4];
            bufferData[j + 13] = bufferData[j + 5];

            bufferData[j + 16] = bufferData[j + 8];
            bufferData[j + 17] = bufferData[j + 9];

            t.transformPointInto(imgWidth, imgHeight, bufferData, j + 20);
        }

        this.clear();

        var program = this.program;
        gl.useProgram(program);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.buffer);
        gl.bufferData(gl.ARRAY_BUFFER, bufferData, gl.DYNAMIC_DRAW);

        gl.vertexAttribPointer(program.attr.coord, 2, gl.FLOAT, false, 16, 0);
        gl.vertexAttribPointer(program.attr.texCoord, 2, gl.FLOAT, false, 16, 8);
        gl.enableVertexAttribArray(program.attr.coord);
        gl.enableVertexAttribArray(program.attr.texCoord);

        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.uniform1i(program.uni.sampler, 0);

        gl.drawArrays(gl.TRIANGLES, 0, count * VERTS_PER_SPRITE);

        // Cleanup
        gl.disableVertexAttribArray(program.attr.coord);
        gl.disableVertexAttribArray(program.attr.texCoord);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        gl.useProgram(null);
    };

    return function (element, frameData) {
        return new RenderContext(element, frameData);
    };
});
