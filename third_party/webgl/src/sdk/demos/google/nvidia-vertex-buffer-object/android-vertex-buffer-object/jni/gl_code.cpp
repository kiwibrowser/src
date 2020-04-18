/*
** Copyright (c) 2012 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

//====================
// OpenGL ES 2.0 code
//====================

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libgl2VBO"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

#define PI 3.1415927f

//===============================================================================
// Everything below this line should essentially be a 1:1 port of the JS version
//===============================================================================

#define SIN_ARRAY_SIZE 1024
#define STRIP_SIZE 144
int TILE_SIZE = 3 * STRIP_SIZE;
int BUFFER_LENGTH = 1000000;

struct SliceInfo
{
    unsigned int vertex_offset;
    unsigned int normal_offset;
};

SliceInfo* slice_info = 0;
int num_slices;

GLfloat* client_array = 0;
int client_array_size;

GLfloat* xy_array = 0;
GLfloat* sin_array = 0;
GLfloat* cos_array = 0;

GLfloat ysinlo[STRIP_SIZE];
GLfloat ycoslo[STRIP_SIZE];
GLfloat ysinhi[STRIP_SIZE];
GLfloat ycoshi[STRIP_SIZE];

float hicoef = .06f;
float locoef = .10f;
float hifreq = 6.1f;
float lofreq = 2.5f;
float phase_rate = .02f;
float phase2_rate = -0.12f;
float phase = 0;
float phase2 = 0;

// Buffers
GLuint bigVBO;
GLuint elementVBO;

// Shader Programs
GLuint gProgram;

// Attributes
GLuint gvPositionHandle;
GLuint gvNormalHandle;

// Uniforms
GLint emissiveColorLoc;
GLint ambientColorLoc;
GLint diffuseColorLoc;
GLint specularColorLoc; 
GLint shininessLoc;
GLint specularFactorLoc;
GLint lightWorldPosLoc;
GLint lightColorLoc;

GLint worldViewProjectionLoc;
GLint worldLoc;
GLint viewInverseLoc;
GLint worldInverseTransposeLoc;

const GLfloat identity_mat[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

const GLfloat view_mat[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 1.0f
};

const GLfloat view_inverse_mat[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 1.0f
};

const GLfloat model_view_projection_mat[] = {
    1.7320508075688776f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.7320508075688776, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0010010010010009f, -1.0f,
    0.0f, 0.0f, 0.9009009009009008f, 1.0f
};

// Per-vertex phong shader
static const char gVertexShader[] = 
"uniform mat4 worldViewProjection;\n"
"uniform vec3 lightWorldPos;\n"
"uniform vec4 lightColor;\n"

"uniform mat4 world;\n"
"uniform mat4 viewInverse;\n"
"uniform mat4 worldInverseTranspose;\n"

"uniform vec4 emissiveColor;\n"
"uniform vec4 ambientColor;\n"
"uniform vec4 diffuseColor;\n"
"uniform vec4 specularColor;\n"
"uniform float shininess;\n"
"uniform float specularFactor;\n"

"attribute vec3 g_Position;\n"
"attribute vec3 g_Normal;\n"

"varying vec4 v_color;\n"

"vec4 lit(float n_dot_l, float n_dot_h, float m) {\n"
"  return vec4(1.,\n"
"              clamp(n_dot_l, 0., 1.),\n"
"              pow(clamp(n_dot_h, 0., 1.), m),\n"
"              1.);\n"
"}\n"

"void main() {\n"
"  vec4 position = vec4(g_Position, 1.);\n"
"  vec4 worldPosition = world * position;\n"
"  vec3 normal = normalize((worldInverseTranspose *\n"
"                           vec4(g_Normal, 0.)).xyz);\n"
"  vec3 surfaceToLight = normalize(lightWorldPos - worldPosition.xyz);\n"
"  vec3 surfaceToView = normalize((viewInverse[3] - worldPosition).xyz);\n"
"  vec3 halfVector = normalize(surfaceToLight + surfaceToView);\n"
"  vec4 litR = lit(dot(normal, surfaceToLight),\n"
"                  dot(normal, halfVector), shininess);\n"
"  v_color = vec4((emissiveColor +\n"
"            lightColor * (ambientColor * litR.x +\n"
"                          diffuseColor * litR.y +\n"
"                          specularColor * litR.z * specularFactor)).rgb,\n"
"           diffuseColor.a);\n"
"  gl_Position = worldViewProjection * position;\n"
"}\n";

static const char gFragmentShader[] = 
"precision mediump float;\n"
"varying vec4 v_color;\n"

"void main() {\n"
"  gl_FragColor = v_color;\n"
"}\n";

// use a fixed-point iterator through a power-of-2-sized array
class PeriodicIterator
{
public:

    PeriodicIterator(unsigned int array_size,
            float period, float initial_offset, float delta) : array_size(array_size)
    {
        float array_delta =  array_size * (delta / period); // floating-point steps-per-increment
        increment = (unsigned int)(array_delta * (1<<16)); // fixed-point steps-per-increment

        float offset = array_size * (initial_offset / period); // floating-point initial index
        init_offset = (unsigned int)(offset * (1<<16));       // fixed-point initial index;
        
        array_size_mask = 0;
        int i = 20; // array should be reasonably sized...
        while((array_size & (1<<i)) == 0) 
            i--;
        array_size_mask = (1<<i)-1;
        index = init_offset;
    }


    int get_index() const { return (index >> 16) & array_size_mask; }
    //int get_index() const { return index % array_size; }

    void incr() { index += increment; }

    void decr() { index -= increment; }

    void reset() { index = init_offset; }

private:
    unsigned int array_size;
    unsigned int array_size_mask;
    // fraction bits == 16
    unsigned int increment;
    unsigned int init_offset;

    unsigned int index;
};

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}



void allocateBigVBO() {
    glGenBuffers(1, &bigVBO);
    glBindBuffer(GL_ARRAY_BUFFER, bigVBO);
    glBufferData(GL_ARRAY_BUFFER, BUFFER_LENGTH * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
}

void computeElements() {
    delete [] xy_array;
    xy_array = new GLfloat[TILE_SIZE];
    for(int i = 0; i < TILE_SIZE; i++) {
        xy_array[i] = (GLfloat)i / (GLfloat)(TILE_SIZE - 1.0f) - 0.5f; 
    }

    GLuint num_elements = (TILE_SIZE - 1) * (2 * STRIP_SIZE);
    unsigned short* elements = new unsigned short[num_elements];

    GLuint idx = 0;
    for (int i = 0; i < TILE_SIZE - 1; i++) {
        for (GLuint j = 0; j < 2 * STRIP_SIZE; j += 2) {
            elements[idx++] = i * STRIP_SIZE + (j / 2);
            elements[idx++] = (i + 1) * STRIP_SIZE + (j / 2);
        }
    }

    glGenBuffers(1, &elementVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_elements * sizeof(unsigned short), elements, GL_STATIC_DRAW);

    delete elements;
}

void buildSinArray() {
    sin_array = new GLfloat[SIN_ARRAY_SIZE];
    cos_array = new GLfloat[SIN_ARRAY_SIZE];
    for (int i = 0; i < SIN_ARRAY_SIZE; i++) {
        sin_array[i] = sin((i*6.283185)/SIN_ARRAY_SIZE);
        cos_array[i] = cos((i*6.283185)/SIN_ARRAY_SIZE);
    }
}

bool init() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    glUseProgram(gProgram);
    gvPositionHandle = glGetAttribLocation(gProgram, "g_Position");
    gvNormalHandle = glGetAttribLocation(gProgram, "g_Normal");

    emissiveColorLoc = glGetUniformLocation(gProgram, "emissiveColor");
    ambientColorLoc = glGetUniformLocation(gProgram, "ambientColor");
    diffuseColorLoc = glGetUniformLocation(gProgram, "diffuseColor");
    specularColorLoc = glGetUniformLocation(gProgram, "specularColor");
    shininessLoc = glGetUniformLocation(gProgram, "shininess");
    specularFactorLoc = glGetUniformLocation(gProgram, "specularFactor");
    lightWorldPosLoc = glGetUniformLocation(gProgram, "lightWorldPos");
    lightColorLoc = glGetUniformLocation(gProgram, "lightColor");

    worldViewProjectionLoc = glGetUniformLocation(gProgram, "worldViewProjection");
    worldLoc = glGetUniformLocation(gProgram, "world");
    viewInverseLoc = glGetUniformLocation(gProgram, "viewInverse");
    worldInverseTransposeLoc = glGetUniformLocation(gProgram, "worldInverseTranspose");

    glUniform4f(emissiveColorLoc, 0.25f, 0.0f, 0.0f, 1.0f);
    glUniform4f(ambientColorLoc, 0.1f, 0.1f, 0.0f, 1.0f);
    glUniform4f(diffuseColorLoc, 0.6f, 0.6f, 0.1f, 1.0f);
    glUniform4f(specularColorLoc, 1.0f, 1.0f, 0.75f, 1.0f);
    glUniform1f(shininessLoc, 128.0f);
    glUniform1f(specularFactorLoc, 1.0f);
    glUniform3f(lightWorldPosLoc, 0.5f, 0.0f, 0.5f);
    glUniform4f(lightColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    glUniformMatrix4fv(worldViewProjectionLoc, 1, false, model_view_projection_mat);
    glUniformMatrix4fv(worldLoc, 1, false, identity_mat);
    glUniformMatrix4fv(viewInverseLoc, 1, false, view_inverse_mat);
    glUniformMatrix4fv(worldInverseTransposeLoc, 1, false, identity_mat);

    allocateBigVBO();
    computeElements();
    buildSinArray();

    return true;
}

void setupSliceInfo() {
    int slice_size = TILE_SIZE * STRIP_SIZE * 6;
    if (client_array == NULL || client_array_size != slice_size * sizeof(GLfloat)) {
        delete [] client_array;
        delete [] slice_info;

        client_array = new GLfloat[slice_size];
        client_array_size = slice_size * sizeof(GLfloat);
        num_slices = int(BUFFER_LENGTH / slice_size) | 0;
        slice_info = new SliceInfo[num_slices];
        for (int i = 0; i < num_slices; i++) {
            int base_offset = i * client_array_size;
            slice_info[i].vertex_offset = base_offset;
            slice_info[i].normal_offset = base_offset + 3 * sizeof(GLfloat);
        }
    }
}

void renderFrame() {

    setupSliceInfo();

    phase += phase_rate;
    phase2 += phase2_rate;

    if (phase > 20 * PI)
        phase = 0;
    if (phase2 < -20 * PI)
        phase2 = 0;

    PeriodicIterator lo_x(SIN_ARRAY_SIZE, 2 * PI, phase, (1.0f / TILE_SIZE) * lofreq * PI);
    PeriodicIterator lo_y(lo_x);
    PeriodicIterator hi_x(SIN_ARRAY_SIZE, 2 * PI, phase2, (1.0f / TILE_SIZE) * hifreq * PI);
    PeriodicIterator hi_y(hi_x);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(gProgram);

    glBindBuffer(GL_ARRAY_BUFFER, bigVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementVBO);

    // Setup vertex attributes
    glEnableVertexAttribArray(gvPositionHandle);
    glEnableVertexAttribArray(gvNormalHandle);

    int cur;
    int num_slabs = TILE_SIZE / STRIP_SIZE;
    unsigned int num_verts_generated_per_frame = 0;
    for (int slab = num_slabs; --slab >= 0; ) {
        // Calculate slabs in here
        unsigned int vertex_index = 0;

        for (int jj = STRIP_SIZE; --jj >= 0; ) {
            ysinlo[jj] = sin_array[lo_y.get_index()];
            ycoslo[jj] = cos_array[lo_y.get_index()]; lo_y.incr();
            ysinhi[jj] = sin_array[hi_y.get_index()];
            ycoshi[jj] = cos_array[hi_y.get_index()]; hi_y.incr();
        }
        lo_y.decr();
        hi_y.decr();

        for (int i = TILE_SIZE; --i >= 0; ) {
            GLfloat x = xy_array[i];
            int lo_x_index = lo_x.get_index();
            int hi_x_index = hi_x.get_index();

            int j_offset = (STRIP_SIZE - 1) * slab;
            GLfloat nx = locoef * -cos_array[lo_x_index] + hicoef * -cos_array[hi_x_index];

            for (int j = STRIP_SIZE; --j >= 0; ) {
                GLfloat y = xy_array[j + j_offset];
                GLfloat ny = locoef * -ycoslo[j] + hicoef * -ycoshi[j];
                client_array[vertex_index + 0] = x;
                client_array[vertex_index + 1] = y;
                client_array[vertex_index + 2] = (locoef * (sin_array[lo_x_index] + ysinlo[j]) +
                                                  hicoef * (sin_array[hi_x_index] + ysinhi[j]));
                client_array[vertex_index + 3] = nx;
                client_array[vertex_index + 4] = ny;
                client_array[vertex_index + 5] = 0.15f;
                vertex_index += 6;
                ++num_verts_generated_per_frame;
            }
            lo_x.incr();
            hi_x.incr();
        }
        lo_x.reset();
        hi_x.reset();

        SliceInfo& slice = slice_info[slab % num_slices];
        glBufferSubData(GL_ARRAY_BUFFER, slice.vertex_offset, client_array_size, client_array);
        glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const void*)slice.vertex_offset);
        glVertexAttribPointer(gvNormalHandle, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (const void*)slice.normal_offset);
        
        int len = TILE_SIZE - 1;
        for (int i = 0; i < len; i++) {
            glDrawElements(GL_TRIANGLE_STRIP, 2 * STRIP_SIZE, GL_UNSIGNED_SHORT,
                (const void*)(i * 2 * STRIP_SIZE * sizeof(short)));
        }
    }

    glDisableVertexAttribArray(gvPositionHandle);
    glDisableVertexAttribArray(gvNormalHandle);
}

//=================================================================================
// This is Java-specific wiring code that is not present in the Javascript version
//=================================================================================

extern "C" {
    JNIEXPORT void JNICALL Java_com_khronos_gl2VBO_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_khronos_gl2VBO_GL2JNILib_step(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_khronos_gl2VBO_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    glViewport(0, 0, width, height);

    init();
}

JNIEXPORT void JNICALL Java_com_khronos_gl2VBO_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}
