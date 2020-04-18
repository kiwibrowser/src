#version 100

// Copyright Alastair F. Donaldson and Hugues Evrard, Imperial College London, 2017
// Defect found using GLFuzz - https://www.graphicsfuzz.com/
//
// Causes crash on:
// Samsung Galaxy S7
// Model number: SM-G930P
// GL_VERSION: OpenGL ES 3.2 V@145.0 (GIT@I86b60582e4)
// GL_VENDOR: Qualcomm
// GL_RENDERER: Adreno (TM) 530
// Android version: 7.0

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

vec4 f()
{
    if(injectionSwitch.x < injectionSwitch.y
       ? injectionSwitch.x > injectionSwitch.y
       : false)
        {
            return vec4(1.0);
        }
    return vec4(1.0);
}
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
                    f();
                    a ++;
                    b ++;
                    g ++;
                    r --;
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
