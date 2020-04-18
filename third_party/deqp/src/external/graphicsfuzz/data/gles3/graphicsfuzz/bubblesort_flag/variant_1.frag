#version 100

// Copyright Alastair F. Donaldson and Hugues Evrard, Imperial College London, 2017
// Defect found using GLFuzz - https://www.graphicsfuzz.com/
//
// Gives wrong image on:
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

uniform vec2 resolution;

bool checkSwap(float a, float b)
{
    return gl_FragCoord.y < resolution.y / 2.0 ? a > b : a < b;
}
void main()
{
    float data[10];
    for(
        int i = 0;
        i < 10;
        i ++
    )
        {
            data[i] = float(10 - i) * injectionSwitch.y;
        }
    for(
        int i = 0;
        i < 9;
        i ++
    )
        {
            for(
                int j = 0;
                j < 10;
                j ++
            )
                {
                    if(j < i + 1)
                        {
                            if(false)
                                {
                                    continue;
                                }
                            continue;
                            discard;
                        }
                    bool doSwap = checkSwap(data[i], data[j]);
                    if(doSwap)
                        {
                            float temp = data[i];
                            data[i] = data[j];
                            data[j] = temp;
                        }
                }
        }
    if(gl_FragCoord.x < resolution.x / 2.0)
        {
            gl_FragColor = vec4(data[0] / 10.0, data[5] / 10.0, data[9] / 10.0, 1.0);
        }
    else
        {
            gl_FragColor = vec4(data[5] / 10.0, data[9] / 10.0, data[0] / 10.0, 1.0);
        }
}
