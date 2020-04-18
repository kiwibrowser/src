void GL_APIENTRY
_es_AlphaFuncx(GLenum func, GLclampx ref);

void GL_APIENTRY
_es_ClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);

void GL_APIENTRY
_es_ClearDepthx(GLclampx depth);

void GL_APIENTRY
_es_ClipPlanef(GLenum plane, const GLfloat *equation);

void GL_APIENTRY
_es_ClipPlanex(GLenum plane, const GLfixed *equation);

void GL_APIENTRY
_es_Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);

void GL_APIENTRY
_es_Color4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);

void GL_APIENTRY
_es_DepthRangex(GLclampx zNear, GLclampx zFar);

void GL_APIENTRY
_es_DrawTexxOES(GLfixed x, GLfixed y, GLfixed z, GLfixed w, GLfixed h);

void GL_APIENTRY
_es_DrawTexxvOES(const GLfixed *coords);

void GL_APIENTRY
_es_Fogx(GLenum pname, GLfixed param);

void GL_APIENTRY
_es_Fogxv(GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_Frustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
             GLfloat zNear, GLfloat zFar);

void GL_APIENTRY
_es_Frustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top,
             GLfixed zNear, GLfixed zFar);

void GL_APIENTRY
_es_GetClipPlanef(GLenum plane, GLfloat *equation);

void GL_APIENTRY
_es_GetClipPlanex(GLenum plane, GLfixed *equation);

void GL_APIENTRY
_es_GetLightxv(GLenum light, GLenum pname, GLfixed *params);

void GL_APIENTRY
_es_GetMaterialxv(GLenum face, GLenum pname, GLfixed *params);

void GL_APIENTRY
_check_GetTexGenivOES(GLenum coord, GLenum pname, GLint *params);

void GL_APIENTRY
_es_GetTexEnvxv(GLenum target, GLenum pname, GLfixed *params);

void GL_APIENTRY
_check_GetTexGenxvOES(GLenum coord, GLenum pname, GLfixed *params);

void GL_APIENTRY
_es_GetTexParameterxv(GLenum target, GLenum pname, GLfixed *params);

void GL_APIENTRY
_es_LightModelx(GLenum pname, GLfixed param);

void GL_APIENTRY
_es_LightModelxv(GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_Lightx(GLenum light, GLenum pname, GLfixed param);

void GL_APIENTRY
_es_Lightxv(GLenum light, GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_LineWidthx(GLfixed width);

void GL_APIENTRY
_es_LoadMatrixx(const GLfixed *m);

void GL_APIENTRY
_es_Materialx(GLenum face, GLenum pname, GLfixed param);

void GL_APIENTRY
_es_Materialxv(GLenum face, GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_MultMatrixx(const GLfixed *m);

void GL_APIENTRY
_es_MultiTexCoord4x(GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q);

void GL_APIENTRY
_es_Normal3x(GLfixed nx, GLfixed ny, GLfixed nz);

void GL_APIENTRY
_es_Orthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
           GLfloat zNear, GLfloat zFar);

void GL_APIENTRY
_es_Orthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top,
           GLfixed zNear, GLfixed zFar);

void GL_APIENTRY
_es_PointParameterx(GLenum pname, GLfixed param);

void GL_APIENTRY
_es_PointParameterxv(GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_PointSizex(GLfixed size);

void GL_APIENTRY
_es_PolygonOffsetx(GLfixed factor, GLfixed units);

void GL_APIENTRY
_es_Rotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);

void GL_APIENTRY
_es_SampleCoveragex(GLclampx value, GLboolean invert);

void GL_APIENTRY
_es_Scalex(GLfixed x, GLfixed y, GLfixed z);

void GL_APIENTRY
_es_TexEnvx(GLenum target, GLenum pname, GLfixed param);

void GL_APIENTRY
_es_TexEnvxv(GLenum target, GLenum pname, const GLfixed *params);

void GL_APIENTRY
_check_TexGeniOES(GLenum coord, GLenum pname, GLint param);

void GL_APIENTRY
_check_TexGenivOES(GLenum coord, GLenum pname, const GLint *params);

void GL_APIENTRY
_check_TexGenxOES(GLenum coord, GLenum pname, GLfixed param);

void GL_APIENTRY
_check_TexGenxvOES(GLenum coord, GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_TexParameterx(GLenum target, GLenum pname, GLfixed param);

void GL_APIENTRY
_es_TexParameterxv(GLenum target, GLenum pname, const GLfixed *params);

void GL_APIENTRY
_es_Translatex(GLfixed x, GLfixed y, GLfixed z);

