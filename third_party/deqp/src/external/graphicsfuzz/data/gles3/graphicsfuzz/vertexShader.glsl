#version 100

// Copyright Alastair F. Donaldson and Hugues Evrard, Imperial College London, 2017

attribute vec2 coord2d;

varying vec2 surfacePosition;

void main(void)
{
    gl_Position = vec4(coord2d, 0.0, 1.0);
    surfacePosition = coord2d;
}
