#version 100

// Copyright Alastair F. Donaldson and Hugues Evrard, Imperial College London, 2017
// Defect found using GLFuzz - https://www.graphicsfuzz.com/
//
// Causes black image to be rendered on:
// ASUS ZenPad 3S 10
// Model number: P027
// GL_VERSION: OpenGL ES 3.1 build 1.5@3830101
// GL_VENDOR: Imagination Technologies
// GL_RENDERER: PowerVR Rogue GX6250
// Android version: 6.0

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
precision highp int;
#else
precision mediump float;
precision mediump int;
#endif
#endif

uniform vec2 injectionSwitch;

void main()
{
    int r;
    int g;
    int b;
    int a;
    r = 100 * int(injectionSwitch.y);
    g = int(injectionSwitch.x) * int(injectionSwitch.y);
    b = 2 * int(injectionSwitch.x);
    a = g - int(injectionSwitch.x);
    for(
        int i = 0;
        i < 10;
        i ++
    )
        {
            r --;
            g ++;
            b ++;
            a ++;
            for(
                int j = 1;
                j < 10;
                j ++
            )
                {
                    if(injectionSwitch.x > injectionSwitch.y)
                        {
                            break;
                        }
                    a ++;
                    if(injectionSwitch.x > injectionSwitch.y)
                        {
                            break;
                        }
                    b ++;
                    g ++;
                    r --;
                    if(injectionSwitch.x > injectionSwitch.y)
                        {
                            return;
                        }
                }
        }
    float fr;
    float fg;
    float fb;
    float fa;
    fr = float(r / 100);
    fg = float(g / 100);
    fb = float(b / 100);
    fa = float(a / 100);
    gl_FragColor = vec4(r, g, b, a);
}
