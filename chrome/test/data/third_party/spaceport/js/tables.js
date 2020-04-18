define([ ], function () {
    return {
        performance: {
            sprites: {
                $title: 'Source type',
                image: 'Static <img>',
                //spriteSheet: 'Animated sprite sheet',

                $children: {
                    $title: 'Technique',
                    css2dImg: 'CSS3 2D transforms with <img>',
                    css3dImg: 'CSS3 3D transforms with <img>',
                    css2dBackground: 'CSS3 2D transforms with CSS backgrounds',
                    css3dBackground: 'CSS3 3D transforms with CSS backgrounds',
                    //cssMatrixImg: 'CSSMatrix transforms with <img>',

                    canvasDrawImageFullClear: 'Canvas drawImage, full clear',
                    canvasDrawImageFullClearAlign: 'Canvas drawImage, full clear, pixel aligned',
                    canvasDrawImagePartialClear: 'Canvas drawImage, partial clear',
                    canvasDrawImagePartialClearAlign: 'Canvas drawImage, partial clear, pixel aligned',

                    webGLDrawWithUniform: 'WebGL .drawArrays tris with uniforms',
                    webGLBatchDraw: 'WebGL .drawArray tris with buffers',

                    $children: {
                        $title: 'Test type',
                        $errors: true,
                        scale: 'Scale',
                        translate: 'Translate',
                        rotate: 'Rotate',

                        $children: {
                            $mode: 'horizontal',
                            js: 'JS time (ms)',
                            objectCount: 'Objects at 30FPS'
                        }
                    }
                }
            }

            /*
            text: {
                $title: 'Font family',
                sans: 'sans-serif',
                serif: 'serif',
                monospace: 'monospace',

                $children: {
                    $title: 'Font size',
                    '8': '8pt',
                    '10': '10pt',
                    '12': '12pt',
                    '14': '14pt',
                    '16': '16pt',
                    '24': '24pt',

                    $children: {
                        $title: 'Style',
                        outline: 'Solid outline',
                        fill: 'Solid fill',
                        fillOutline: 'Solid fill + outline',

                        $children: {
                            $mode: 'horizontal',
                            score: 'Score'
                        }
                    }
                }
            },

            audioLatency: {
                $title: 'Type',
                coldLatency: 'Cold latency (ms)',
                warmLatency: 'Warm latency (ms)'
            }
            */
        }
    };
});
