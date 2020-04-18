#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/gles2.h>

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */


int GLAD_GL_ES_VERSION_2_0 = 0;
int GLAD_GL_OES_mapbuffer = 0;
int GLAD_GL_OES_required_internalformat = 0;



PFNGLACTIVETEXTUREPROC glad_glActiveTexture = NULL;
PFNGLATTACHSHADERPROC glad_glAttachShader = NULL;
PFNGLBINDATTRIBLOCATIONPROC glad_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glad_glBindBuffer = NULL;
PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer = NULL;
PFNGLBINDTEXTUREPROC glad_glBindTexture = NULL;
PFNGLBLENDCOLORPROC glad_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glad_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glad_glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glad_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glad_glBlendFuncSeparate = NULL;
PFNGLBUFFERDATAPROC glad_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus = NULL;
PFNGLCLEARPROC glad_glClear = NULL;
PFNGLCLEARCOLORPROC glad_glClearColor = NULL;
PFNGLCLEARDEPTHFPROC glad_glClearDepthf = NULL;
PFNGLCLEARSTENCILPROC glad_glClearStencil = NULL;
PFNGLCOLORMASKPROC glad_glColorMask = NULL;
PFNGLCOMPILESHADERPROC glad_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glad_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glad_glCompressedTexSubImage2D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glad_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glad_glCopyTexSubImage2D = NULL;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram = NULL;
PFNGLCREATESHADERPROC glad_glCreateShader = NULL;
PFNGLCULLFACEPROC glad_glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram = NULL;
PFNGLDELETERENDERBUFFERSPROC glad_glDeleteRenderbuffers = NULL;
PFNGLDELETESHADERPROC glad_glDeleteShader = NULL;
PFNGLDELETETEXTURESPROC glad_glDeleteTextures = NULL;
PFNGLDEPTHFUNCPROC glad_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glad_glDepthMask = NULL;
PFNGLDEPTHRANGEFPROC glad_glDepthRangef = NULL;
PFNGLDETACHSHADERPROC glad_glDetachShader = NULL;
PFNGLDISABLEPROC glad_glDisable = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glad_glDisableVertexAttribArray = NULL;
PFNGLDRAWARRAYSPROC glad_glDrawArrays = NULL;
PFNGLDRAWELEMENTSPROC glad_glDrawElements = NULL;
PFNGLENABLEPROC glad_glEnable = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray = NULL;
PFNGLFINISHPROC glad_glFinish = NULL;
PFNGLFLUSHPROC glad_glFlush = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glad_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D = NULL;
PFNGLFRONTFACEPROC glad_glFrontFace = NULL;
PFNGLGENBUFFERSPROC glad_glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers = NULL;
PFNGLGENRENDERBUFFERSPROC glad_glGenRenderbuffers = NULL;
PFNGLGENTEXTURESPROC glad_glGenTextures = NULL;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = NULL;
PFNGLGETACTIVEATTRIBPROC glad_glGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC glad_glGetActiveUniform = NULL;
PFNGLGETATTACHEDSHADERSPROC glad_glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC glad_glGetAttribLocation = NULL;
PFNGLGETBOOLEANVPROC glad_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glad_glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC glad_glGetBufferPointerv = NULL;
PFNGLGETBUFFERPOINTERVOESPROC glad_glGetBufferPointervOES = NULL;
PFNGLGETERRORPROC glad_glGetError = NULL;
PFNGLGETFLOATVPROC glad_glGetFloatv = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glad_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETINTEGERVPROC glad_glGetIntegerv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glad_glGetRenderbufferParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog = NULL;
PFNGLGETSHADERPRECISIONFORMATPROC glad_glGetShaderPrecisionFormat = NULL;
PFNGLGETSHADERSOURCEPROC glad_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glad_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glad_glGetString = NULL;
PFNGLGETTEXPARAMETERFVPROC glad_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glad_glGetTexParameteriv = NULL;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC glad_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glad_glGetUniformiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC glad_glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBFVPROC glad_glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC glad_glGetVertexAttribiv = NULL;
PFNGLHINTPROC glad_glHint = NULL;
PFNGLISBUFFERPROC glad_glIsBuffer = NULL;
PFNGLISENABLEDPROC glad_glIsEnabled = NULL;
PFNGLISFRAMEBUFFERPROC glad_glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC glad_glIsProgram = NULL;
PFNGLISRENDERBUFFERPROC glad_glIsRenderbuffer = NULL;
PFNGLISSHADERPROC glad_glIsShader = NULL;
PFNGLISTEXTUREPROC glad_glIsTexture = NULL;
PFNGLLINEWIDTHPROC glad_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glad_glLinkProgram = NULL;
PFNGLMAPBUFFERPROC glad_glMapBuffer = NULL;
PFNGLMAPBUFFEROESPROC glad_glMapBufferOES = NULL;
PFNGLPIXELSTOREIPROC glad_glPixelStorei = NULL;
PFNGLPOLYGONOFFSETPROC glad_glPolygonOffset = NULL;
PFNGLREADPIXELSPROC glad_glReadPixels = NULL;
PFNGLRELEASESHADERCOMPILERPROC glad_glReleaseShaderCompiler = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glad_glRenderbufferStorage = NULL;
PFNGLSAMPLECOVERAGEPROC glad_glSampleCoverage = NULL;
PFNGLSCISSORPROC glad_glScissor = NULL;
PFNGLSHADERBINARYPROC glad_glShaderBinary = NULL;
PFNGLSHADERSOURCEPROC glad_glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glad_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glad_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glad_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glad_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glad_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glad_glStencilOpSeparate = NULL;
PFNGLTEXIMAGE2DPROC glad_glTexImage2D = NULL;
PFNGLTEXPARAMETERFPROC glad_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glad_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glad_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glad_glTexParameteriv = NULL;
PFNGLTEXSUBIMAGE2DPROC glad_glTexSubImage2D = NULL;
PFNGLUNIFORM1FPROC glad_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glad_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glad_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glad_glUniform1iv = NULL;
PFNGLUNIFORM2FPROC glad_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glad_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glad_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glad_glUniform2iv = NULL;
PFNGLUNIFORM3FPROC glad_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glad_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glad_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glad_glUniform3iv = NULL;
PFNGLUNIFORM4FPROC glad_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glad_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glad_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glad_glUniform4iv = NULL;
PFNGLUNIFORMMATRIX2FVPROC glad_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glad_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glad_glUniformMatrix4fv = NULL;
PFNGLUNMAPBUFFERPROC glad_glUnmapBuffer = NULL;
PFNGLUNMAPBUFFEROESPROC glad_glUnmapBufferOES = NULL;
PFNGLUSEPROGRAMPROC glad_glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glad_glValidateProgram = NULL;
PFNGLVERTEXATTRIB1FPROC glad_glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC glad_glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB2FPROC glad_glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC glad_glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB3FPROC glad_glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC glad_glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB4FPROC glad_glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC glad_glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer = NULL;
PFNGLVIEWPORTPROC glad_glViewport = NULL;


static void glad_gl_load_GL_ES_VERSION_2_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_ES_VERSION_2_0) return;
    glActiveTexture = (PFNGLACTIVETEXTUREPROC) load("glActiveTexture", userptr);
    glAttachShader = (PFNGLATTACHSHADERPROC) load("glAttachShader", userptr);
    glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) load("glBindAttribLocation", userptr);
    glBindBuffer = (PFNGLBINDBUFFERPROC) load("glBindBuffer", userptr);
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load("glBindFramebuffer", userptr);
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load("glBindRenderbuffer", userptr);
    glBindTexture = (PFNGLBINDTEXTUREPROC) load("glBindTexture", userptr);
    glBlendColor = (PFNGLBLENDCOLORPROC) load("glBlendColor", userptr);
    glBlendEquation = (PFNGLBLENDEQUATIONPROC) load("glBlendEquation", userptr);
    glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC) load("glBlendEquationSeparate", userptr);
    glBlendFunc = (PFNGLBLENDFUNCPROC) load("glBlendFunc", userptr);
    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load("glBlendFuncSeparate", userptr);
    glBufferData = (PFNGLBUFFERDATAPROC) load("glBufferData", userptr);
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC) load("glBufferSubData", userptr);
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load("glCheckFramebufferStatus", userptr);
    glClear = (PFNGLCLEARPROC) load("glClear", userptr);
    glClearColor = (PFNGLCLEARCOLORPROC) load("glClearColor", userptr);
    glClearDepthf = (PFNGLCLEARDEPTHFPROC) load("glClearDepthf", userptr);
    glClearStencil = (PFNGLCLEARSTENCILPROC) load("glClearStencil", userptr);
    glColorMask = (PFNGLCOLORMASKPROC) load("glColorMask", userptr);
    glCompileShader = (PFNGLCOMPILESHADERPROC) load("glCompileShader", userptr);
    glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load("glCompressedTexImage2D", userptr);
    glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load("glCompressedTexSubImage2D", userptr);
    glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load("glCopyTexImage2D", userptr);
    glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load("glCopyTexSubImage2D", userptr);
    glCreateProgram = (PFNGLCREATEPROGRAMPROC) load("glCreateProgram", userptr);
    glCreateShader = (PFNGLCREATESHADERPROC) load("glCreateShader", userptr);
    glCullFace = (PFNGLCULLFACEPROC) load("glCullFace", userptr);
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) load("glDeleteBuffers", userptr);
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load("glDeleteFramebuffers", userptr);
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC) load("glDeleteProgram", userptr);
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load("glDeleteRenderbuffers", userptr);
    glDeleteShader = (PFNGLDELETESHADERPROC) load("glDeleteShader", userptr);
    glDeleteTextures = (PFNGLDELETETEXTURESPROC) load("glDeleteTextures", userptr);
    glDepthFunc = (PFNGLDEPTHFUNCPROC) load("glDepthFunc", userptr);
    glDepthMask = (PFNGLDEPTHMASKPROC) load("glDepthMask", userptr);
    glDepthRangef = (PFNGLDEPTHRANGEFPROC) load("glDepthRangef", userptr);
    glDetachShader = (PFNGLDETACHSHADERPROC) load("glDetachShader", userptr);
    glDisable = (PFNGLDISABLEPROC) load("glDisable", userptr);
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) load("glDisableVertexAttribArray", userptr);
    glDrawArrays = (PFNGLDRAWARRAYSPROC) load("glDrawArrays", userptr);
    glDrawElements = (PFNGLDRAWELEMENTSPROC) load("glDrawElements", userptr);
    glEnable = (PFNGLENABLEPROC) load("glEnable", userptr);
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load("glEnableVertexAttribArray", userptr);
    glFinish = (PFNGLFINISHPROC) load("glFinish", userptr);
    glFlush = (PFNGLFLUSHPROC) load("glFlush", userptr);
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load("glFramebufferRenderbuffer", userptr);
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load("glFramebufferTexture2D", userptr);
    glFrontFace = (PFNGLFRONTFACEPROC) load("glFrontFace", userptr);
    glGenBuffers = (PFNGLGENBUFFERSPROC) load("glGenBuffers", userptr);
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load("glGenFramebuffers", userptr);
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load("glGenRenderbuffers", userptr);
    glGenTextures = (PFNGLGENTEXTURESPROC) load("glGenTextures", userptr);
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load("glGenerateMipmap", userptr);
    glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC) load("glGetActiveAttrib", userptr);
    glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC) load("glGetActiveUniform", userptr);
    glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) load("glGetAttachedShaders", userptr);
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) load("glGetAttribLocation", userptr);
    glGetBooleanv = (PFNGLGETBOOLEANVPROC) load("glGetBooleanv", userptr);
    glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load("glGetBufferParameteriv", userptr);
    glGetError = (PFNGLGETERRORPROC) load("glGetError", userptr);
    glGetFloatv = (PFNGLGETFLOATVPROC) load("glGetFloatv", userptr);
    glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load("glGetFramebufferAttachmentParameteriv", userptr);
    glGetIntegerv = (PFNGLGETINTEGERVPROC) load("glGetIntegerv", userptr);
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) load("glGetProgramInfoLog", userptr);
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC) load("glGetProgramiv", userptr);
    glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load("glGetRenderbufferParameteriv", userptr);
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) load("glGetShaderInfoLog", userptr);
    glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC) load("glGetShaderPrecisionFormat", userptr);
    glGetShaderSource = (PFNGLGETSHADERSOURCEPROC) load("glGetShaderSource", userptr);
    glGetShaderiv = (PFNGLGETSHADERIVPROC) load("glGetShaderiv", userptr);
    glGetString = (PFNGLGETSTRINGPROC) load("glGetString", userptr);
    glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load("glGetTexParameterfv", userptr);
    glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load("glGetTexParameteriv", userptr);
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) load("glGetUniformLocation", userptr);
    glGetUniformfv = (PFNGLGETUNIFORMFVPROC) load("glGetUniformfv", userptr);
    glGetUniformiv = (PFNGLGETUNIFORMIVPROC) load("glGetUniformiv", userptr);
    glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC) load("glGetVertexAttribPointerv", userptr);
    glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC) load("glGetVertexAttribfv", userptr);
    glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC) load("glGetVertexAttribiv", userptr);
    glHint = (PFNGLHINTPROC) load("glHint", userptr);
    glIsBuffer = (PFNGLISBUFFERPROC) load("glIsBuffer", userptr);
    glIsEnabled = (PFNGLISENABLEDPROC) load("glIsEnabled", userptr);
    glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load("glIsFramebuffer", userptr);
    glIsProgram = (PFNGLISPROGRAMPROC) load("glIsProgram", userptr);
    glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load("glIsRenderbuffer", userptr);
    glIsShader = (PFNGLISSHADERPROC) load("glIsShader", userptr);
    glIsTexture = (PFNGLISTEXTUREPROC) load("glIsTexture", userptr);
    glLineWidth = (PFNGLLINEWIDTHPROC) load("glLineWidth", userptr);
    glLinkProgram = (PFNGLLINKPROGRAMPROC) load("glLinkProgram", userptr);
    glPixelStorei = (PFNGLPIXELSTOREIPROC) load("glPixelStorei", userptr);
    glPolygonOffset = (PFNGLPOLYGONOFFSETPROC) load("glPolygonOffset", userptr);
    glReadPixels = (PFNGLREADPIXELSPROC) load("glReadPixels", userptr);
    glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC) load("glReleaseShaderCompiler", userptr);
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load("glRenderbufferStorage", userptr);
    glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load("glSampleCoverage", userptr);
    glScissor = (PFNGLSCISSORPROC) load("glScissor", userptr);
    glShaderBinary = (PFNGLSHADERBINARYPROC) load("glShaderBinary", userptr);
    glShaderSource = (PFNGLSHADERSOURCEPROC) load("glShaderSource", userptr);
    glStencilFunc = (PFNGLSTENCILFUNCPROC) load("glStencilFunc", userptr);
    glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) load("glStencilFuncSeparate", userptr);
    glStencilMask = (PFNGLSTENCILMASKPROC) load("glStencilMask", userptr);
    glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) load("glStencilMaskSeparate", userptr);
    glStencilOp = (PFNGLSTENCILOPPROC) load("glStencilOp", userptr);
    glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) load("glStencilOpSeparate", userptr);
    glTexImage2D = (PFNGLTEXIMAGE2DPROC) load("glTexImage2D", userptr);
    glTexParameterf = (PFNGLTEXPARAMETERFPROC) load("glTexParameterf", userptr);
    glTexParameterfv = (PFNGLTEXPARAMETERFVPROC) load("glTexParameterfv", userptr);
    glTexParameteri = (PFNGLTEXPARAMETERIPROC) load("glTexParameteri", userptr);
    glTexParameteriv = (PFNGLTEXPARAMETERIVPROC) load("glTexParameteriv", userptr);
    glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load("glTexSubImage2D", userptr);
    glUniform1f = (PFNGLUNIFORM1FPROC) load("glUniform1f", userptr);
    glUniform1fv = (PFNGLUNIFORM1FVPROC) load("glUniform1fv", userptr);
    glUniform1i = (PFNGLUNIFORM1IPROC) load("glUniform1i", userptr);
    glUniform1iv = (PFNGLUNIFORM1IVPROC) load("glUniform1iv", userptr);
    glUniform2f = (PFNGLUNIFORM2FPROC) load("glUniform2f", userptr);
    glUniform2fv = (PFNGLUNIFORM2FVPROC) load("glUniform2fv", userptr);
    glUniform2i = (PFNGLUNIFORM2IPROC) load("glUniform2i", userptr);
    glUniform2iv = (PFNGLUNIFORM2IVPROC) load("glUniform2iv", userptr);
    glUniform3f = (PFNGLUNIFORM3FPROC) load("glUniform3f", userptr);
    glUniform3fv = (PFNGLUNIFORM3FVPROC) load("glUniform3fv", userptr);
    glUniform3i = (PFNGLUNIFORM3IPROC) load("glUniform3i", userptr);
    glUniform3iv = (PFNGLUNIFORM3IVPROC) load("glUniform3iv", userptr);
    glUniform4f = (PFNGLUNIFORM4FPROC) load("glUniform4f", userptr);
    glUniform4fv = (PFNGLUNIFORM4FVPROC) load("glUniform4fv", userptr);
    glUniform4i = (PFNGLUNIFORM4IPROC) load("glUniform4i", userptr);
    glUniform4iv = (PFNGLUNIFORM4IVPROC) load("glUniform4iv", userptr);
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC) load("glUniformMatrix2fv", userptr);
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) load("glUniformMatrix3fv", userptr);
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) load("glUniformMatrix4fv", userptr);
    glUseProgram = (PFNGLUSEPROGRAMPROC) load("glUseProgram", userptr);
    glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) load("glValidateProgram", userptr);
    glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) load("glVertexAttrib1f", userptr);
    glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC) load("glVertexAttrib1fv", userptr);
    glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC) load("glVertexAttrib2f", userptr);
    glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC) load("glVertexAttrib2fv", userptr);
    glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC) load("glVertexAttrib3f", userptr);
    glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) load("glVertexAttrib3fv", userptr);
    glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC) load("glVertexAttrib4f", userptr);
    glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC) load("glVertexAttrib4fv", userptr);
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) load("glVertexAttribPointer", userptr);
    glViewport = (PFNGLVIEWPORTPROC) load("glViewport", userptr);
}
static void glad_gl_load_GL_OES_mapbuffer( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_OES_mapbuffer) return;
    glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC) load("glGetBufferPointerv", userptr);
    glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOESPROC) load("glGetBufferPointervOES", userptr);
    glMapBuffer = (PFNGLMAPBUFFERPROC) load("glMapBuffer", userptr);
    glMapBufferOES = (PFNGLMAPBUFFEROESPROC) load("glMapBufferOES", userptr);
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) load("glUnmapBuffer", userptr);
    glUnmapBufferOES = (PFNGLUNMAPBUFFEROESPROC) load("glUnmapBufferOES", userptr);
}



#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define GLAD_GL_IS_SOME_NEW_VERSION 1
#else
#define GLAD_GL_IS_SOME_NEW_VERSION 0
#endif

static int glad_gl_get_extensions( int version, const char **out_exts, unsigned int *out_num_exts_i, char ***out_exts_i) {
#if GLAD_GL_IS_SOME_NEW_VERSION
    if(GLAD_VERSION_MAJOR(version) < 3) {
#else
    (void) version;
    (void) out_num_exts_i;
    (void) out_exts_i;
#endif
        if (glGetString == NULL) {
            return 0;
        }
        *out_exts = (const char *)glGetString(GL_EXTENSIONS);
#if GLAD_GL_IS_SOME_NEW_VERSION
    } else {
        unsigned int index = 0;
        unsigned int num_exts_i = 0;
        char **exts_i = NULL;
        if (glGetStringi == NULL || glGetIntegerv == NULL) {
            return 0;
        }
        glGetIntegerv(GL_NUM_EXTENSIONS, (int*) &num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char **) malloc(num_exts_i * (sizeof *exts_i));
        }
        if (exts_i == NULL) {
            return 0;
        }
        for(index = 0; index < num_exts_i; index++) {
            const char *gl_str_tmp = (const char*) glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp) + 1;

            char *local_str = (char*) malloc(len * sizeof(char));
            if(local_str != NULL) {
                memcpy(local_str, gl_str_tmp, len * sizeof(char));
            }

            exts_i[index] = local_str;
        }

        *out_num_exts_i = num_exts_i;
        *out_exts_i = exts_i;
    }
#endif
    return 1;
}
static void glad_gl_free_extensions(char **exts_i, unsigned int num_exts_i) {
    if (exts_i != NULL) {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            free((void *) (exts_i[index]));
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}
static int glad_gl_has_extension(int version, const char *exts, unsigned int num_exts_i, char **exts_i, const char *ext) {
    if(GLAD_VERSION_MAJOR(version) < 3 || !GLAD_GL_IS_SOME_NEW_VERSION) {
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }
        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }
            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
    } else {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            const char *e = exts_i[index];
            if(strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static GLADapiproc glad_gl_get_proc_from_userptr(const char* name, void *userptr) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_gl_find_extensions_gles2( int version) {
    const char *exts = NULL;
    unsigned int num_exts_i = 0;
    char **exts_i = NULL;
    if (!glad_gl_get_extensions(version, &exts, &num_exts_i, &exts_i)) return 0;

    GLAD_GL_OES_mapbuffer = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_mapbuffer");
    GLAD_GL_OES_required_internalformat = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_OES_required_internalformat");

    glad_gl_free_extensions(exts_i, num_exts_i);

    return 1;
}

static int glad_gl_find_core_gles2(void) {
    int i, major, minor;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL
    };
    version = (const char*) glGetString(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    GLAD_GL_ES_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLES2UserPtr( GLADuserptrloadfunc load, void *userptr) {
    int version;

    glGetString = (PFNGLGETSTRINGPROC) load("glGetString", userptr);
    if(glGetString == NULL) return 0;
    if(glGetString(GL_VERSION) == NULL) return 0;
    version = glad_gl_find_core_gles2();

    glad_gl_load_GL_ES_VERSION_2_0(load, userptr);

    if (!glad_gl_find_extensions_gles2(version)) return 0;
    glad_gl_load_GL_OES_mapbuffer(load, userptr);



    return version;
}


int gladLoadGLES2( GLADloadfunc load) {
    return gladLoadGLES2UserPtr( glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}




