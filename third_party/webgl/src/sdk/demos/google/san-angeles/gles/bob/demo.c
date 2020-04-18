/* San Angeles Observation OpenGL ES version example
 * Copyright 2004-2005 Jetro Lauha
 * All rights reserved.
 * Web: http://iki.fi/jetro/
 *
 * This source is free software; you can redistribute it and/or
 * modify it under the terms of EITHER:
 *   (1) The GNU Lesser General Public License as published by the Free
 *       Software Foundation; either version 2.1 of the License, or (at
 *       your option) any later version. The text of the GNU Lesser
 *       General Public License is included with this source in the
 *       file LICENSE-LGPL.txt.
 *   (2) The BSD-style license that is included with this source in
 *       the file LICENSE-BSD.txt.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files
 * LICENSE-LGPL.txt and LICENSE-BSD.txt for more details.
 *
 * $Id: demo.c,v 1.10 2005/02/08 20:54:39 tonic Exp $
 * $Revision: 1.10 $
 */

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <stdio.h>

#include "shapes.h"
#include "cams.h"

#include <stdint.h>


// Total run length is 20 * camera track base unit length (see cams.h).
#define RUN_LENGTH  (20 * CAMTRACK_LEN)
#undef PI
#define PI 3.1415926535897932f
#define RANDOM_UINT_MAX 65535


static uint32_t sRandomSeed = 0;
static uint32_t sRandomCount = 0;

static void seedRandom(unsigned long seed)
{
    sRandomSeed = seed;
    sRandomCount = 0;
    // printf("seed=%d\n", seed);
}

static unsigned long randomUInt()
{
    sRandomSeed = sRandomSeed * 0x343fd + 0x269ec3;
    // printf("%04d: random=%d\n", sRandomCount++, sRandomSeed >> 16);
    return sRandomSeed >> 16;
}


// Capped conversion from float to fixed.
static int32_t floatToFixed(float value)
{
    if (value < -32768) value = -32768;
    if (value > 32767) value = 32767;
    return (int32_t)(value * 65536);
}

#define FIXED(value) floatToFixed(value)

typedef uint32_t GLfixed;
typedef uint8_t GLubyte;
typedef int32_t GLint;
typedef uint32_t GLsizei;
typedef float GLfloat;


// Definition of one GL object in this demo.
typedef struct {
    /* Vertex array and color array are enabled for all objects, so their
     * pointers must always be valid and non-NULL. Normal array is not
     * used by the ground plane, so when its pointer is NULL then normal
     * array usage is disabled.
     *
     * Vertex array is supposed to use GL_FIXED datatype and stride 0
     * (i.e. tightly packed array). Color array is supposed to have 4
     * components per color with GL_UNSIGNED_BYTE datatype and stride 0.
     * Normal array is supposed to use GL_FIXED datatype and stride 0.
     */
    GLfixed *vertexArray;
    GLubyte *colorArray;
    GLfixed *normalArray;
    GLint vertexComponents;
    GLsizei count;
} GLOBJECT;


static long sStartTick = 0;
static long sTick = 0;

static int sCurrentCamTrack = 0;
static long sCurrentCamTrackStartTick = 0;
static long sNextCamTrackStartTick = 0x7fffffff;

static GLOBJECT *sSuperShapeObjects[SUPERSHAPE_COUNT] = { NULL };
static GLOBJECT *sGroundPlane = NULL;


typedef struct {
    float x, y, z;
} VECTOR3;


static void freeGLObject(GLOBJECT *object)
{
    if (object == NULL)
        return;
    free(object->normalArray);
    free(object->colorArray);
    free(object->vertexArray);
    free(object);
}


static GLOBJECT * newGLObject(long vertices, int vertexComponents,
                              int useNormalArray)
{
    GLOBJECT *result;
    result = (GLOBJECT *)malloc(sizeof(GLOBJECT));
    if (result == NULL)
        return NULL;
    result->count = vertices;
    result->vertexComponents = vertexComponents;
    result->vertexArray = (GLfixed *)malloc(vertices * vertexComponents *
                                            sizeof(GLfixed));
    result->colorArray = (GLubyte *)malloc(vertices * 4 * sizeof(GLubyte));
    if (useNormalArray)
    {
        result->normalArray = (GLfixed *)malloc(vertices * 3 *
                                                sizeof(GLfixed));
    }
    else
        result->normalArray = NULL;
    if (result->vertexArray == NULL ||
        result->colorArray == NULL ||
        (useNormalArray && result->normalArray == NULL))
    {
        freeGLObject(result);
        return NULL;
    }
    return result;
}



static void vector3Sub(VECTOR3 *dest, VECTOR3 *v1, VECTOR3 *v2)
{
    dest->x = v1->x - v2->x;
    dest->y = v1->y - v2->y;
    dest->z = v1->z - v2->z;
}


static void superShapeMap(VECTOR3 *point, float r1, float r2, float t, float p)
{
    // sphere-mapping of supershape parameters
    point->x = (float)(cos(t) * cos(p) / r1 / r2);
    point->y = (float)(sin(t) * cos(p) / r1 / r2);
    point->z = (float)(sin(p) / r2);
}


static float ssFunc(const float t, const float *p)
{
    return (float)(pow(pow(fabs(cos(p[0] * t / 4)) / p[1], p[4]) +
                       pow(fabs(sin(p[0] * t / 4)) / p[2], p[5]), 1 / p[3]));
}


// Creates and returns a supershape object.
// Based on Paul Bourke's POV-Ray implementation.
// http://astronomy.swin.edu.au/~pbourke/povray/supershape/
static GLOBJECT * createSuperShape(const float *params)
{
    const int resol1 = (int)params[SUPERSHAPE_PARAMS - 3];
    const int resol2 = (int)params[SUPERSHAPE_PARAMS - 2];
    // latitude 0 to pi/2 for no mirrored bottom
    // (latitudeBegin==0 for -pi/2 to pi/2 originally)
    const int latitudeBegin = resol2 / 4;
    const int latitudeEnd = resol2 / 2;    // non-inclusive
    const int longitudeCount = resol1;
    const int latitudeCount = latitudeEnd - latitudeBegin;
    const long triangleCount = longitudeCount * latitudeCount * 2;
    const long vertices = triangleCount * 3;
    GLOBJECT *result;
    float baseColor[3];
    int a, longitude, latitude;
    long currentVertex, currentQuad;

    result = newGLObject(vertices, 3, 1);
    if (result == NULL)
        return NULL;

    for (a = 0; a < 3; ++a)
        baseColor[a] = ((randomUInt() % 155) + 100) / 255.f;

    currentQuad = 0;
    currentVertex = 0;

    // longitude -pi to pi
    for (longitude = 0; longitude < longitudeCount; ++longitude)
    {

        // latitude 0 to pi/2
        for (latitude = latitudeBegin; latitude < latitudeEnd; ++latitude)
        {
            float t1 = -PI + longitude * 2 * PI / resol1;
            float t2 = -PI + (longitude + 1) * 2 * PI / resol1;
            float p1 = -PI / 2 + latitude * 2 * PI / resol2;
            float p2 = -PI / 2 + (latitude + 1) * 2 * PI / resol2;
            float r0, r1, r2, r3;

            r0 = ssFunc(t1, params);
            r1 = ssFunc(p1, &params[6]);
            r2 = ssFunc(t2, params);
            r3 = ssFunc(p2, &params[6]);

            if (r0 != 0 && r1 != 0 && r2 != 0 && r3 != 0)
            {
                VECTOR3 pa, pb, pc, pd;
                VECTOR3 v1, v2, n;
                float ca;
                int i;
                //float lenSq, invLenSq;

                superShapeMap(&pa, r0, r1, t1, p1);
                superShapeMap(&pb, r2, r1, t2, p1);
                superShapeMap(&pc, r2, r3, t2, p2);
                superShapeMap(&pd, r0, r3, t1, p2);

                // kludge to set lower edge of the object to fixed level
                if (latitude == latitudeBegin + 1)
                    pa.z = pb.z = 0;

                vector3Sub(&v1, &pb, &pa);
                vector3Sub(&v2, &pd, &pa);

                // Calculate normal with cross product.
                /*   i    j    k      i    j
                 * v1.x v1.y v1.z | v1.x v1.y
                 * v2.x v2.y v2.z | v2.x v2.y
                 */

                n.x = v1.y * v2.z - v1.z * v2.y;
                n.y = v1.z * v2.x - v1.x * v2.z;
                n.z = v1.x * v2.y - v1.y * v2.x;

                /* Pre-normalization of the normals is disabled here because
                 * they will be normalized anyway later due to automatic
                 * normalization (GL_NORMALIZE). It is enabled because the
                 * objects are scaled with glScale.
                 */
                /*
                lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
                invLenSq = (float)(1 / sqrt(lenSq));
                n.x *= invLenSq;
                n.y *= invLenSq;
                n.z *= invLenSq;
                */

                ca = pa.z + 0.5f;

                for (i = currentVertex * 3;
                     i < (currentVertex + 6) * 3;
                     i += 3)
                {
                    result->normalArray[i] = FIXED(n.x);
                    result->normalArray[i + 1] = FIXED(n.y);
                    result->normalArray[i + 2] = FIXED(n.z);
                }
                for (i = currentVertex * 4;
                     i < (currentVertex + 6) * 4;
                     i += 4)
                {
                    int a, color[3];
                    for (a = 0; a < 3; ++a)
                    {
                        color[a] = (int)(ca * baseColor[a] * 255);
                        if (color[a] > 255) color[a] = 255;
                    }
                    result->colorArray[i] = (GLubyte)color[0];
                    result->colorArray[i + 1] = (GLubyte)color[1];
                    result->colorArray[i + 2] = (GLubyte)color[2];
                    result->colorArray[i + 3] = 0;
                }
                result->vertexArray[currentVertex * 3] = FIXED(pa.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pa.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pa.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pb.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pb.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pb.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pd.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pd.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pd.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pb.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pb.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pb.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pc.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pc.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pc.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pd.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pd.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pd.z);
                ++currentVertex;
            } // r0 && r1 && r2 && r3
            ++currentQuad;
        } // latitude
    } // longitude

    // Set number of vertices in object to the actual amount created.
    result->count = currentVertex;

    return result;
}


static GLOBJECT * createGroundPlane()
{
    const int scale = 4;
    const int yBegin = -15, yEnd = 15;    // ends are non-inclusive
    const int xBegin = -15, xEnd = 15;
    const long triangleCount = (yEnd - yBegin) * (xEnd - xBegin) * 2;
    const long vertices = triangleCount * 3;
    GLOBJECT *result;
    int x, y;
    long currentVertex, currentQuad;

    result = newGLObject(vertices, 2, 0);
    if (result == NULL)
        return NULL;

    currentQuad = 0;
    currentVertex = 0;

    for (y = yBegin; y < yEnd; ++y)
    {
        for (x = xBegin; x < xEnd; ++x)
        {
            GLubyte color;
            int i, a;
            color = (GLubyte)((randomUInt() & 0x5f) + 81);  // 101 1111
            for (i = currentVertex * 4; i < (currentVertex + 6) * 4; i += 4)
            {
                result->colorArray[i] = color;
                result->colorArray[i + 1] = color;
                result->colorArray[i + 2] = color;
                result->colorArray[i + 3] = 0;
            }

            // Axis bits for quad triangles:
            // x: 011100 (0x1c), y: 110001 (0x31)  (clockwise)
            // x: 001110 (0x0e), y: 100011 (0x23)  (counter-clockwise)
            for (a = 0; a < 6; ++a)
            {
                const int xm = x + ((0x1c >> a) & 1);
                const int ym = y + ((0x31 >> a) & 1);
                const float m = (float)(cos(xm * 2) * sin(ym * 4) * 0.75f);
                result->vertexArray[currentVertex * 2] =
                    FIXED(xm * scale + m);
                result->vertexArray[currentVertex * 2 + 1] =
                    FIXED(ym * scale + m);
                ++currentVertex;
            }
            ++currentQuad;
        }
    }
    return result;
}


// Called from the app framework.
void appInit()
{
    int a;

    seedRandom(15);

    for (a = 0; a < SUPERSHAPE_COUNT; ++a)
    {
        sSuperShapeObjects[a] = createSuperShape(sSuperShapeParams[a]);
        assert(sSuperShapeObjects[a] != NULL);
    }
    sGroundPlane = createGroundPlane();
    assert(sGroundPlane != NULL);
}


// Called from the app framework.
void appDeinit()
{
    int a;
    for (a = 0; a < SUPERSHAPE_COUNT; ++a)
        freeGLObject(sSuperShapeObjects[a]);
    freeGLObject(sGroundPlane);
}

/* Following gluLookAt implementation is adapted from the
 * Mesa 3D Graphics library. http://www.mesa3d.org
 */
static void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
	              GLfloat centerx, GLfloat centery, GLfloat centerz,
	              GLfloat upx, GLfloat upy, GLfloat upz)
{
    GLfloat m[16];
    GLfloat x[3], y[3], z[3];
    GLfloat mag;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = (float)sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {			/* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }

    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;

    /* X vector = Y cross Z */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];

    /* Recompute Y = Z cross X */
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];

    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */

    mag = (float)sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = (float)sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#define M(row,col)  m[col*4+row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M
    {
        int a;
        GLfixed fixedM[16];
        for (a = 0; a < 16; ++a)
            fixedM[a] = (GLfixed)(m[a] * 65536);
        // glMultMatrixx(fixedM);
    }

#if 0
    /* Translate Eye to Origin */
    glTranslatex((GLfixed)(-eyex * 65536),
                 (GLfixed)(-eyey * 65536),
                 (GLfixed)(-eyez * 65536));
#endif
}



static void camTrack()
{
    float lerp[5];
    float eX, eY, eZ, cX, cY, cZ;
    float trackPos;
    CAMTRACK *cam;
    long currentCamTick;
    int a;

    if (sNextCamTrackStartTick <= sTick)
    {
        ++sCurrentCamTrack;
        sCurrentCamTrackStartTick = sNextCamTrackStartTick;
    }
    sNextCamTrackStartTick = sCurrentCamTrackStartTick +
                             sCamTracks[sCurrentCamTrack].len * CAMTRACK_LEN;

    cam = &sCamTracks[sCurrentCamTrack];
    currentCamTick = sTick - sCurrentCamTrackStartTick;
    trackPos = (float)currentCamTick / (CAMTRACK_LEN * cam->len);

    for (a = 0; a < 5; ++a)
        lerp[a] = (cam->src[a] + cam->dest[a] * trackPos) * 0.01f;

    if (cam->dist)
    {
        float dist = cam->dist * 0.1f;
        cX = lerp[0];
        cY = lerp[1];
        cZ = lerp[2];
        eX = cX - (float)cos(lerp[3]) * dist;
        eY = cY - (float)sin(lerp[3]) * dist;
        eZ = cZ - lerp[4];
    }
    else
    {
        eX = lerp[0];
        eY = lerp[1];
        eZ = lerp[2];
        cX = eX + (float)cos(lerp[3]);
        cY = eY + (float)sin(lerp[3]);
        cZ = eZ + lerp[4];
    }
    gluLookAt(eX, eY, eZ, cX, cY, cZ, 0, 0, 1);
}


static void drawModels(float zScale)
{
    const int translationScale = 9;
    int x, y;

    seedRandom(9);

    //glScalex(1 << 16, 1 << 16, (GLfixed)(zScale * 65536));

    for (y = -5; y <= 5; ++y)
    {
        for (x = -5; x <= 5; ++x)
        {
            float buildingScale;
            GLfixed fixedScale;

            int curShape = randomUInt() % SUPERSHAPE_COUNT;
            buildingScale = sSuperShapeParams[curShape][SUPERSHAPE_PARAMS - 1];
            fixedScale = (GLfixed)(buildingScale * 65536);

#if 0
            glPushMatrix();
            glTranslatex((x * translationScale) * 65536,
                         (y * translationScale) * 65536,
                         0);
            glRotatex((GLfixed)((randomUInt() % 360) << 16), 0, 0, 1 << 16);
            glScalex(fixedScale, fixedScale, fixedScale);

            drawGLObject(sSuperShapeObjects[curShape]);
            glPopMatrix();
#endif
        }
    }

    for (x = -2; x <= 2; ++x)
    {
        const int shipScale100 = translationScale * 500;
        const int offs100 = x * shipScale100 + (sTick % shipScale100);
        float offs = offs100 * 0.01f;
        GLfixed fixedOffs = (GLfixed)(offs * 65536);
#if 0
        glPushMatrix();
        glTranslatex(fixedOffs, -4 * 65536, 2 << 16);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        glPopMatrix();
        glPushMatrix();
        glTranslatex(-4 * 65536, fixedOffs, 4 << 16);
        glRotatex(90 << 16, 0, 0, 1 << 16);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        glPopMatrix();
#endif
    }
}
#if 0
// Called from the app framework.
/* The tick is current time in milliseconds, width and height
 * are the image dimensions to be rendered.
 */
void appRender(long tick, int width, int height)
{
    if (sStartTick == 0)
        sStartTick = tick;
    if (!gAppAlive)
        return;

    // Actual tick value is "blurred" a little bit.
    sTick = (sTick + tick - sStartTick) >> 1;

    // Terminate application after running through the demonstration once.
    if (sTick >= RUN_LENGTH)
    {
        gAppAlive = 0;
        return;
    }

    // Prepare OpenGL ES for rendering of the frame.
    prepareFrame(width, height);

    // Update the camera position and set the lookat.
    camTrack();

    // Configure environment.
    configureLightAndMaterial();

    // Draw the reflection by drawing models with negated Z-axis.
    glPushMatrix();
    drawModels(-1);
    glPopMatrix();

    // Blend the ground plane to the window.
    drawGroundPlane();

    // Draw all the models normally.
    drawModels(1);

    // Draw fade quad over whole window (when changing cameras).
    drawFadeQuad();
}
#endif



int main()
{
  int i;
  GLOBJECT * shape;

  appInit();

  sTick = 42000;
  camTrack();
  drawModels(1);

  return 0;
}


