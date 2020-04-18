define([ 'util/ensureCallback', 'sprites/canvas', 'sprites/webGL' ], function (ensureCallback, canvas, webGL) {
    function RenderContext(sourceData, frameData) {
        this.sourceData = sourceData;
        this.frameData = frameData;

        this.canvas = canvas();
        var gl = webGL.getContext(this.canvas);
        this.context = gl;

        this.program = webGL.shaders.sprite(gl);

        var buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([
            // Square
            0, 0,
            1, 0,
            0, 1,

            1, 0,
            1, 1,
            0, 1
        ]), gl.STATIC_DRAW);
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

        var transforms = this.frameData[frameIndex];
        var count = transforms.length;
        var i;

        var program = this.program;

        this.clear();

        gl.useProgram(program);
        gl.bindBuffer(gl.ARRAY_BUFFER, this.buffer);
        gl.vertexAttribPointer(program.attr.coord, 2, gl.FLOAT, false, 0, 0);
        gl.enableVertexAttribArray(program.attr.coord);

        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.uniform1i(program.uni.sampler, 0);

        gl.uniform2f(program.uni.size, img.width, img.height);

        var uMatrix = program.uni.matrix;

        for (i = 0; i < count; ++i) {
            var m = transforms[i].matrix;
            gl.uniformMatrix3fv(uMatrix, false, m);
            gl.drawArrays(gl.TRIANGLES, 0, 6);
        }

        // Cleanup
        gl.disableVertexAttribArray(program.attr.coord);
        gl.bindBuffer(gl.ARRAY_BUFFER, null);

        gl.useProgram(null);
    };

    return function (element, frameData) {
        return new RenderContext(element, frameData);
    };
});
