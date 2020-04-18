#version 100

// Copyright Alastair F. Donaldson and Hugues Evrard, Imperial College London, 2017
// Defect found using GLFuzz - https://www.graphicsfuzz.com/
//
// Gives compile error on:
// NVIDIA SHIELD Android TV
// Model number: P2571
// GL_VERSION: OpenGL ES 3.2 NVIDIA 361.00
// GL_VENDOR: NVIDIA Corporation
// GL_RENDERER: NVIDIA Tegra
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
    gl_FragColor = vec4(r, g, b, a) * ((false ? 1.0 : 1.0) * vec4(1.0));
}
