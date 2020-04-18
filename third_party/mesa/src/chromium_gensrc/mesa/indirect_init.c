/* DO NOT EDIT - This file generated automatically by glX_proto_send.py (from Mesa) script */

/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * \file indirect_init.c
 * Initialize indirect rendering dispatch table.
 *
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Brian Paul <brian@precisioninsight.com>
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "indirect_init.h"
#include "indirect.h"
#include "glapi.h"
#include <assert.h>


/**
 * No-op function used to initialize functions that have no GLX protocol
 * support.
 */
static int NoOp(void)
{
    return 0;
}

/**
 * Create and initialize a new GL dispatch table.  The table is initialized
 * with GLX indirect rendering protocol functions.
 */
struct _glapi_table * __glXNewIndirectAPI( void )
{
    _glapi_proc *table;
    unsigned entries;
    unsigned i;
    int o;

    entries = _glapi_get_dispatch_table_size();
    table = (_glapi_proc *) Xmalloc(entries * sizeof(_glapi_proc));

    /* first, set all entries to point to no-op functions */
    for (i = 0; i < entries; i++) {
       table[i] = (_glapi_proc) NoOp;
    }

    /* now, initialize the entries we understand */

    /* 1.0 */

    table[213] = (_glapi_proc) __indirect_glAccum;
    table[240] = (_glapi_proc) __indirect_glAlphaFunc;
    table[7] = (_glapi_proc) __indirect_glBegin;
    table[8] = (_glapi_proc) __indirect_glBitmap;
    table[241] = (_glapi_proc) __indirect_glBlendFunc;
    table[2] = (_glapi_proc) __indirect_glCallList;
    table[3] = (_glapi_proc) __indirect_glCallLists;
    table[203] = (_glapi_proc) __indirect_glClear;
    table[204] = (_glapi_proc) __indirect_glClearAccum;
    table[206] = (_glapi_proc) __indirect_glClearColor;
    table[208] = (_glapi_proc) __indirect_glClearDepth;
    table[205] = (_glapi_proc) __indirect_glClearIndex;
    table[207] = (_glapi_proc) __indirect_glClearStencil;
    table[150] = (_glapi_proc) __indirect_glClipPlane;
    table[9] = (_glapi_proc) __indirect_glColor3b;
    table[10] = (_glapi_proc) __indirect_glColor3bv;
    table[11] = (_glapi_proc) __indirect_glColor3d;
    table[12] = (_glapi_proc) __indirect_glColor3dv;
    table[13] = (_glapi_proc) __indirect_glColor3f;
    table[14] = (_glapi_proc) __indirect_glColor3fv;
    table[15] = (_glapi_proc) __indirect_glColor3i;
    table[16] = (_glapi_proc) __indirect_glColor3iv;
    table[17] = (_glapi_proc) __indirect_glColor3s;
    table[18] = (_glapi_proc) __indirect_glColor3sv;
    table[19] = (_glapi_proc) __indirect_glColor3ub;
    table[20] = (_glapi_proc) __indirect_glColor3ubv;
    table[21] = (_glapi_proc) __indirect_glColor3ui;
    table[22] = (_glapi_proc) __indirect_glColor3uiv;
    table[23] = (_glapi_proc) __indirect_glColor3us;
    table[24] = (_glapi_proc) __indirect_glColor3usv;
    table[25] = (_glapi_proc) __indirect_glColor4b;
    table[26] = (_glapi_proc) __indirect_glColor4bv;
    table[27] = (_glapi_proc) __indirect_glColor4d;
    table[28] = (_glapi_proc) __indirect_glColor4dv;
    table[29] = (_glapi_proc) __indirect_glColor4f;
    table[30] = (_glapi_proc) __indirect_glColor4fv;
    table[31] = (_glapi_proc) __indirect_glColor4i;
    table[32] = (_glapi_proc) __indirect_glColor4iv;
    table[33] = (_glapi_proc) __indirect_glColor4s;
    table[34] = (_glapi_proc) __indirect_glColor4sv;
    table[35] = (_glapi_proc) __indirect_glColor4ub;
    table[36] = (_glapi_proc) __indirect_glColor4ubv;
    table[37] = (_glapi_proc) __indirect_glColor4ui;
    table[38] = (_glapi_proc) __indirect_glColor4uiv;
    table[39] = (_glapi_proc) __indirect_glColor4us;
    table[40] = (_glapi_proc) __indirect_glColor4usv;
    table[210] = (_glapi_proc) __indirect_glColorMask;
    table[151] = (_glapi_proc) __indirect_glColorMaterial;
    table[255] = (_glapi_proc) __indirect_glCopyPixels;
    table[152] = (_glapi_proc) __indirect_glCullFace;
    table[4] = (_glapi_proc) __indirect_glDeleteLists;
    table[245] = (_glapi_proc) __indirect_glDepthFunc;
    table[211] = (_glapi_proc) __indirect_glDepthMask;
    table[288] = (_glapi_proc) __indirect_glDepthRange;
    table[214] = (_glapi_proc) __indirect_glDisable;
    table[202] = (_glapi_proc) __indirect_glDrawBuffer;
    table[257] = (_glapi_proc) __indirect_glDrawPixels;
    table[41] = (_glapi_proc) __indirect_glEdgeFlag;
    table[42] = (_glapi_proc) __indirect_glEdgeFlagv;
    table[215] = (_glapi_proc) __indirect_glEnable;
    table[43] = (_glapi_proc) __indirect_glEnd;
    table[1] = (_glapi_proc) __indirect_glEndList;
    table[228] = (_glapi_proc) __indirect_glEvalCoord1d;
    table[229] = (_glapi_proc) __indirect_glEvalCoord1dv;
    table[230] = (_glapi_proc) __indirect_glEvalCoord1f;
    table[231] = (_glapi_proc) __indirect_glEvalCoord1fv;
    table[232] = (_glapi_proc) __indirect_glEvalCoord2d;
    table[233] = (_glapi_proc) __indirect_glEvalCoord2dv;
    table[234] = (_glapi_proc) __indirect_glEvalCoord2f;
    table[235] = (_glapi_proc) __indirect_glEvalCoord2fv;
    table[236] = (_glapi_proc) __indirect_glEvalMesh1;
    table[238] = (_glapi_proc) __indirect_glEvalMesh2;
    table[237] = (_glapi_proc) __indirect_glEvalPoint1;
    table[239] = (_glapi_proc) __indirect_glEvalPoint2;
    table[194] = (_glapi_proc) __indirect_glFeedbackBuffer;
    table[216] = (_glapi_proc) __indirect_glFinish;
    table[217] = (_glapi_proc) __indirect_glFlush;
    table[153] = (_glapi_proc) __indirect_glFogf;
    table[154] = (_glapi_proc) __indirect_glFogfv;
    table[155] = (_glapi_proc) __indirect_glFogi;
    table[156] = (_glapi_proc) __indirect_glFogiv;
    table[157] = (_glapi_proc) __indirect_glFrontFace;
    table[289] = (_glapi_proc) __indirect_glFrustum;
    table[5] = (_glapi_proc) __indirect_glGenLists;
    table[258] = (_glapi_proc) __indirect_glGetBooleanv;
    table[259] = (_glapi_proc) __indirect_glGetClipPlane;
    table[260] = (_glapi_proc) __indirect_glGetDoublev;
    table[261] = (_glapi_proc) __indirect_glGetError;
    table[262] = (_glapi_proc) __indirect_glGetFloatv;
    table[263] = (_glapi_proc) __indirect_glGetIntegerv;
    table[264] = (_glapi_proc) __indirect_glGetLightfv;
    table[265] = (_glapi_proc) __indirect_glGetLightiv;
    table[266] = (_glapi_proc) __indirect_glGetMapdv;
    table[267] = (_glapi_proc) __indirect_glGetMapfv;
    table[268] = (_glapi_proc) __indirect_glGetMapiv;
    table[269] = (_glapi_proc) __indirect_glGetMaterialfv;
    table[270] = (_glapi_proc) __indirect_glGetMaterialiv;
    table[271] = (_glapi_proc) __indirect_glGetPixelMapfv;
    table[272] = (_glapi_proc) __indirect_glGetPixelMapuiv;
    table[273] = (_glapi_proc) __indirect_glGetPixelMapusv;
    table[274] = (_glapi_proc) __indirect_glGetPolygonStipple;
    table[275] = (_glapi_proc) __indirect_glGetString;
    table[276] = (_glapi_proc) __indirect_glGetTexEnvfv;
    table[277] = (_glapi_proc) __indirect_glGetTexEnviv;
    table[278] = (_glapi_proc) __indirect_glGetTexGendv;
    table[279] = (_glapi_proc) __indirect_glGetTexGenfv;
    table[280] = (_glapi_proc) __indirect_glGetTexGeniv;
    table[281] = (_glapi_proc) __indirect_glGetTexImage;
    table[284] = (_glapi_proc) __indirect_glGetTexLevelParameterfv;
    table[285] = (_glapi_proc) __indirect_glGetTexLevelParameteriv;
    table[282] = (_glapi_proc) __indirect_glGetTexParameterfv;
    table[283] = (_glapi_proc) __indirect_glGetTexParameteriv;
    table[158] = (_glapi_proc) __indirect_glHint;
    table[212] = (_glapi_proc) __indirect_glIndexMask;
    table[44] = (_glapi_proc) __indirect_glIndexd;
    table[45] = (_glapi_proc) __indirect_glIndexdv;
    table[46] = (_glapi_proc) __indirect_glIndexf;
    table[47] = (_glapi_proc) __indirect_glIndexfv;
    table[48] = (_glapi_proc) __indirect_glIndexi;
    table[49] = (_glapi_proc) __indirect_glIndexiv;
    table[50] = (_glapi_proc) __indirect_glIndexs;
    table[51] = (_glapi_proc) __indirect_glIndexsv;
    table[197] = (_glapi_proc) __indirect_glInitNames;
    table[286] = (_glapi_proc) __indirect_glIsEnabled;
    table[287] = (_glapi_proc) __indirect_glIsList;
    table[163] = (_glapi_proc) __indirect_glLightModelf;
    table[164] = (_glapi_proc) __indirect_glLightModelfv;
    table[165] = (_glapi_proc) __indirect_glLightModeli;
    table[166] = (_glapi_proc) __indirect_glLightModeliv;
    table[159] = (_glapi_proc) __indirect_glLightf;
    table[160] = (_glapi_proc) __indirect_glLightfv;
    table[161] = (_glapi_proc) __indirect_glLighti;
    table[162] = (_glapi_proc) __indirect_glLightiv;
    table[167] = (_glapi_proc) __indirect_glLineStipple;
    table[168] = (_glapi_proc) __indirect_glLineWidth;
    table[6] = (_glapi_proc) __indirect_glListBase;
    table[290] = (_glapi_proc) __indirect_glLoadIdentity;
    table[292] = (_glapi_proc) __indirect_glLoadMatrixd;
    table[291] = (_glapi_proc) __indirect_glLoadMatrixf;
    table[198] = (_glapi_proc) __indirect_glLoadName;
    table[242] = (_glapi_proc) __indirect_glLogicOp;
    table[220] = (_glapi_proc) __indirect_glMap1d;
    table[221] = (_glapi_proc) __indirect_glMap1f;
    table[222] = (_glapi_proc) __indirect_glMap2d;
    table[223] = (_glapi_proc) __indirect_glMap2f;
    table[224] = (_glapi_proc) __indirect_glMapGrid1d;
    table[225] = (_glapi_proc) __indirect_glMapGrid1f;
    table[226] = (_glapi_proc) __indirect_glMapGrid2d;
    table[227] = (_glapi_proc) __indirect_glMapGrid2f;
    table[169] = (_glapi_proc) __indirect_glMaterialf;
    table[170] = (_glapi_proc) __indirect_glMaterialfv;
    table[171] = (_glapi_proc) __indirect_glMateriali;
    table[172] = (_glapi_proc) __indirect_glMaterialiv;
    table[293] = (_glapi_proc) __indirect_glMatrixMode;
    table[295] = (_glapi_proc) __indirect_glMultMatrixd;
    table[294] = (_glapi_proc) __indirect_glMultMatrixf;
    table[0] = (_glapi_proc) __indirect_glNewList;
    table[52] = (_glapi_proc) __indirect_glNormal3b;
    table[53] = (_glapi_proc) __indirect_glNormal3bv;
    table[54] = (_glapi_proc) __indirect_glNormal3d;
    table[55] = (_glapi_proc) __indirect_glNormal3dv;
    table[56] = (_glapi_proc) __indirect_glNormal3f;
    table[57] = (_glapi_proc) __indirect_glNormal3fv;
    table[58] = (_glapi_proc) __indirect_glNormal3i;
    table[59] = (_glapi_proc) __indirect_glNormal3iv;
    table[60] = (_glapi_proc) __indirect_glNormal3s;
    table[61] = (_glapi_proc) __indirect_glNormal3sv;
    table[296] = (_glapi_proc) __indirect_glOrtho;
    table[199] = (_glapi_proc) __indirect_glPassThrough;
    table[251] = (_glapi_proc) __indirect_glPixelMapfv;
    table[252] = (_glapi_proc) __indirect_glPixelMapuiv;
    table[253] = (_glapi_proc) __indirect_glPixelMapusv;
    table[249] = (_glapi_proc) __indirect_glPixelStoref;
    table[250] = (_glapi_proc) __indirect_glPixelStorei;
    table[247] = (_glapi_proc) __indirect_glPixelTransferf;
    table[248] = (_glapi_proc) __indirect_glPixelTransferi;
    table[246] = (_glapi_proc) __indirect_glPixelZoom;
    table[173] = (_glapi_proc) __indirect_glPointSize;
    table[174] = (_glapi_proc) __indirect_glPolygonMode;
    table[175] = (_glapi_proc) __indirect_glPolygonStipple;
    table[218] = (_glapi_proc) __indirect_glPopAttrib;
    table[297] = (_glapi_proc) __indirect_glPopMatrix;
    table[200] = (_glapi_proc) __indirect_glPopName;
    table[219] = (_glapi_proc) __indirect_glPushAttrib;
    table[298] = (_glapi_proc) __indirect_glPushMatrix;
    table[201] = (_glapi_proc) __indirect_glPushName;
    table[62] = (_glapi_proc) __indirect_glRasterPos2d;
    table[63] = (_glapi_proc) __indirect_glRasterPos2dv;
    table[64] = (_glapi_proc) __indirect_glRasterPos2f;
    table[65] = (_glapi_proc) __indirect_glRasterPos2fv;
    table[66] = (_glapi_proc) __indirect_glRasterPos2i;
    table[67] = (_glapi_proc) __indirect_glRasterPos2iv;
    table[68] = (_glapi_proc) __indirect_glRasterPos2s;
    table[69] = (_glapi_proc) __indirect_glRasterPos2sv;
    table[70] = (_glapi_proc) __indirect_glRasterPos3d;
    table[71] = (_glapi_proc) __indirect_glRasterPos3dv;
    table[72] = (_glapi_proc) __indirect_glRasterPos3f;
    table[73] = (_glapi_proc) __indirect_glRasterPos3fv;
    table[74] = (_glapi_proc) __indirect_glRasterPos3i;
    table[75] = (_glapi_proc) __indirect_glRasterPos3iv;
    table[76] = (_glapi_proc) __indirect_glRasterPos3s;
    table[77] = (_glapi_proc) __indirect_glRasterPos3sv;
    table[78] = (_glapi_proc) __indirect_glRasterPos4d;
    table[79] = (_glapi_proc) __indirect_glRasterPos4dv;
    table[80] = (_glapi_proc) __indirect_glRasterPos4f;
    table[81] = (_glapi_proc) __indirect_glRasterPos4fv;
    table[82] = (_glapi_proc) __indirect_glRasterPos4i;
    table[83] = (_glapi_proc) __indirect_glRasterPos4iv;
    table[84] = (_glapi_proc) __indirect_glRasterPos4s;
    table[85] = (_glapi_proc) __indirect_glRasterPos4sv;
    table[254] = (_glapi_proc) __indirect_glReadBuffer;
    table[256] = (_glapi_proc) __indirect_glReadPixels;
    table[86] = (_glapi_proc) __indirect_glRectd;
    table[87] = (_glapi_proc) __indirect_glRectdv;
    table[88] = (_glapi_proc) __indirect_glRectf;
    table[89] = (_glapi_proc) __indirect_glRectfv;
    table[90] = (_glapi_proc) __indirect_glRecti;
    table[91] = (_glapi_proc) __indirect_glRectiv;
    table[92] = (_glapi_proc) __indirect_glRects;
    table[93] = (_glapi_proc) __indirect_glRectsv;
    table[196] = (_glapi_proc) __indirect_glRenderMode;
    table[299] = (_glapi_proc) __indirect_glRotated;
    table[300] = (_glapi_proc) __indirect_glRotatef;
    table[301] = (_glapi_proc) __indirect_glScaled;
    table[302] = (_glapi_proc) __indirect_glScalef;
    table[176] = (_glapi_proc) __indirect_glScissor;
    table[195] = (_glapi_proc) __indirect_glSelectBuffer;
    table[177] = (_glapi_proc) __indirect_glShadeModel;
    table[243] = (_glapi_proc) __indirect_glStencilFunc;
    table[209] = (_glapi_proc) __indirect_glStencilMask;
    table[244] = (_glapi_proc) __indirect_glStencilOp;
    table[94] = (_glapi_proc) __indirect_glTexCoord1d;
    table[95] = (_glapi_proc) __indirect_glTexCoord1dv;
    table[96] = (_glapi_proc) __indirect_glTexCoord1f;
    table[97] = (_glapi_proc) __indirect_glTexCoord1fv;
    table[98] = (_glapi_proc) __indirect_glTexCoord1i;
    table[99] = (_glapi_proc) __indirect_glTexCoord1iv;
    table[100] = (_glapi_proc) __indirect_glTexCoord1s;
    table[101] = (_glapi_proc) __indirect_glTexCoord1sv;
    table[102] = (_glapi_proc) __indirect_glTexCoord2d;
    table[103] = (_glapi_proc) __indirect_glTexCoord2dv;
    table[104] = (_glapi_proc) __indirect_glTexCoord2f;
    table[105] = (_glapi_proc) __indirect_glTexCoord2fv;
    table[106] = (_glapi_proc) __indirect_glTexCoord2i;
    table[107] = (_glapi_proc) __indirect_glTexCoord2iv;
    table[108] = (_glapi_proc) __indirect_glTexCoord2s;
    table[109] = (_glapi_proc) __indirect_glTexCoord2sv;
    table[110] = (_glapi_proc) __indirect_glTexCoord3d;
    table[111] = (_glapi_proc) __indirect_glTexCoord3dv;
    table[112] = (_glapi_proc) __indirect_glTexCoord3f;
    table[113] = (_glapi_proc) __indirect_glTexCoord3fv;
    table[114] = (_glapi_proc) __indirect_glTexCoord3i;
    table[115] = (_glapi_proc) __indirect_glTexCoord3iv;
    table[116] = (_glapi_proc) __indirect_glTexCoord3s;
    table[117] = (_glapi_proc) __indirect_glTexCoord3sv;
    table[118] = (_glapi_proc) __indirect_glTexCoord4d;
    table[119] = (_glapi_proc) __indirect_glTexCoord4dv;
    table[120] = (_glapi_proc) __indirect_glTexCoord4f;
    table[121] = (_glapi_proc) __indirect_glTexCoord4fv;
    table[122] = (_glapi_proc) __indirect_glTexCoord4i;
    table[123] = (_glapi_proc) __indirect_glTexCoord4iv;
    table[124] = (_glapi_proc) __indirect_glTexCoord4s;
    table[125] = (_glapi_proc) __indirect_glTexCoord4sv;
    table[184] = (_glapi_proc) __indirect_glTexEnvf;
    table[185] = (_glapi_proc) __indirect_glTexEnvfv;
    table[186] = (_glapi_proc) __indirect_glTexEnvi;
    table[187] = (_glapi_proc) __indirect_glTexEnviv;
    table[188] = (_glapi_proc) __indirect_glTexGend;
    table[189] = (_glapi_proc) __indirect_glTexGendv;
    table[190] = (_glapi_proc) __indirect_glTexGenf;
    table[191] = (_glapi_proc) __indirect_glTexGenfv;
    table[192] = (_glapi_proc) __indirect_glTexGeni;
    table[193] = (_glapi_proc) __indirect_glTexGeniv;
    table[182] = (_glapi_proc) __indirect_glTexImage1D;
    table[183] = (_glapi_proc) __indirect_glTexImage2D;
    table[178] = (_glapi_proc) __indirect_glTexParameterf;
    table[179] = (_glapi_proc) __indirect_glTexParameterfv;
    table[180] = (_glapi_proc) __indirect_glTexParameteri;
    table[181] = (_glapi_proc) __indirect_glTexParameteriv;
    table[303] = (_glapi_proc) __indirect_glTranslated;
    table[304] = (_glapi_proc) __indirect_glTranslatef;
    table[126] = (_glapi_proc) __indirect_glVertex2d;
    table[127] = (_glapi_proc) __indirect_glVertex2dv;
    table[128] = (_glapi_proc) __indirect_glVertex2f;
    table[129] = (_glapi_proc) __indirect_glVertex2fv;
    table[130] = (_glapi_proc) __indirect_glVertex2i;
    table[131] = (_glapi_proc) __indirect_glVertex2iv;
    table[132] = (_glapi_proc) __indirect_glVertex2s;
    table[133] = (_glapi_proc) __indirect_glVertex2sv;
    table[134] = (_glapi_proc) __indirect_glVertex3d;
    table[135] = (_glapi_proc) __indirect_glVertex3dv;
    table[136] = (_glapi_proc) __indirect_glVertex3f;
    table[137] = (_glapi_proc) __indirect_glVertex3fv;
    table[138] = (_glapi_proc) __indirect_glVertex3i;
    table[139] = (_glapi_proc) __indirect_glVertex3iv;
    table[140] = (_glapi_proc) __indirect_glVertex3s;
    table[141] = (_glapi_proc) __indirect_glVertex3sv;
    table[142] = (_glapi_proc) __indirect_glVertex4d;
    table[143] = (_glapi_proc) __indirect_glVertex4dv;
    table[144] = (_glapi_proc) __indirect_glVertex4f;
    table[145] = (_glapi_proc) __indirect_glVertex4fv;
    table[146] = (_glapi_proc) __indirect_glVertex4i;
    table[147] = (_glapi_proc) __indirect_glVertex4iv;
    table[148] = (_glapi_proc) __indirect_glVertex4s;
    table[149] = (_glapi_proc) __indirect_glVertex4sv;
    table[305] = (_glapi_proc) __indirect_glViewport;

    /* 1.1 */

    table[322] = (_glapi_proc) __indirect_glAreTexturesResident;
    table[306] = (_glapi_proc) __indirect_glArrayElement;
    table[307] = (_glapi_proc) __indirect_glBindTexture;
    table[308] = (_glapi_proc) __indirect_glColorPointer;
    table[323] = (_glapi_proc) __indirect_glCopyTexImage1D;
    table[324] = (_glapi_proc) __indirect_glCopyTexImage2D;
    table[325] = (_glapi_proc) __indirect_glCopyTexSubImage1D;
    table[326] = (_glapi_proc) __indirect_glCopyTexSubImage2D;
    table[327] = (_glapi_proc) __indirect_glDeleteTextures;
    table[309] = (_glapi_proc) __indirect_glDisableClientState;
    table[310] = (_glapi_proc) __indirect_glDrawArrays;
    table[311] = (_glapi_proc) __indirect_glDrawElements;
    table[312] = (_glapi_proc) __indirect_glEdgeFlagPointer;
    table[313] = (_glapi_proc) __indirect_glEnableClientState;
    table[328] = (_glapi_proc) __indirect_glGenTextures;
    table[329] = (_glapi_proc) __indirect_glGetPointerv;
    table[314] = (_glapi_proc) __indirect_glIndexPointer;
    table[315] = (_glapi_proc) __indirect_glIndexub;
    table[316] = (_glapi_proc) __indirect_glIndexubv;
    table[317] = (_glapi_proc) __indirect_glInterleavedArrays;
    table[330] = (_glapi_proc) __indirect_glIsTexture;
    table[318] = (_glapi_proc) __indirect_glNormalPointer;
    table[319] = (_glapi_proc) __indirect_glPolygonOffset;
    table[334] = (_glapi_proc) __indirect_glPopClientAttrib;
    table[331] = (_glapi_proc) __indirect_glPrioritizeTextures;
    table[335] = (_glapi_proc) __indirect_glPushClientAttrib;
    table[320] = (_glapi_proc) __indirect_glTexCoordPointer;
    table[332] = (_glapi_proc) __indirect_glTexSubImage1D;
    table[333] = (_glapi_proc) __indirect_glTexSubImage2D;
    table[321] = (_glapi_proc) __indirect_glVertexPointer;

    /* 1.2 */

    table[336] = (_glapi_proc) __indirect_glBlendColor;
    table[337] = (_glapi_proc) __indirect_glBlendEquation;
    table[346] = (_glapi_proc) __indirect_glColorSubTable;
    table[339] = (_glapi_proc) __indirect_glColorTable;
    table[340] = (_glapi_proc) __indirect_glColorTableParameterfv;
    table[341] = (_glapi_proc) __indirect_glColorTableParameteriv;
    table[348] = (_glapi_proc) __indirect_glConvolutionFilter1D;
    table[349] = (_glapi_proc) __indirect_glConvolutionFilter2D;
    table[350] = (_glapi_proc) __indirect_glConvolutionParameterf;
    table[351] = (_glapi_proc) __indirect_glConvolutionParameterfv;
    table[352] = (_glapi_proc) __indirect_glConvolutionParameteri;
    table[353] = (_glapi_proc) __indirect_glConvolutionParameteriv;
    table[347] = (_glapi_proc) __indirect_glCopyColorSubTable;
    table[342] = (_glapi_proc) __indirect_glCopyColorTable;
    table[354] = (_glapi_proc) __indirect_glCopyConvolutionFilter1D;
    table[355] = (_glapi_proc) __indirect_glCopyConvolutionFilter2D;
    table[373] = (_glapi_proc) __indirect_glCopyTexSubImage3D;
    table[338] = (_glapi_proc) __indirect_glDrawRangeElements;
    table[343] = (_glapi_proc) __indirect_glGetColorTable;
    table[344] = (_glapi_proc) __indirect_glGetColorTableParameterfv;
    table[345] = (_glapi_proc) __indirect_glGetColorTableParameteriv;
    table[356] = (_glapi_proc) __indirect_glGetConvolutionFilter;
    table[357] = (_glapi_proc) __indirect_glGetConvolutionParameterfv;
    table[358] = (_glapi_proc) __indirect_glGetConvolutionParameteriv;
    table[361] = (_glapi_proc) __indirect_glGetHistogram;
    table[362] = (_glapi_proc) __indirect_glGetHistogramParameterfv;
    table[363] = (_glapi_proc) __indirect_glGetHistogramParameteriv;
    table[364] = (_glapi_proc) __indirect_glGetMinmax;
    table[365] = (_glapi_proc) __indirect_glGetMinmaxParameterfv;
    table[366] = (_glapi_proc) __indirect_glGetMinmaxParameteriv;
    table[359] = (_glapi_proc) __indirect_glGetSeparableFilter;
    table[367] = (_glapi_proc) __indirect_glHistogram;
    table[368] = (_glapi_proc) __indirect_glMinmax;
    table[369] = (_glapi_proc) __indirect_glResetHistogram;
    table[370] = (_glapi_proc) __indirect_glResetMinmax;
    table[360] = (_glapi_proc) __indirect_glSeparableFilter2D;
    table[371] = (_glapi_proc) __indirect_glTexImage3D;
    table[372] = (_glapi_proc) __indirect_glTexSubImage3D;

    /*   1. GL_ARB_multitexture */

    table[374] = (_glapi_proc) __indirect_glActiveTextureARB;
    table[375] = (_glapi_proc) __indirect_glClientActiveTextureARB;
    table[376] = (_glapi_proc) __indirect_glMultiTexCoord1dARB;
    table[377] = (_glapi_proc) __indirect_glMultiTexCoord1dvARB;
    table[378] = (_glapi_proc) __indirect_glMultiTexCoord1fARB;
    table[379] = (_glapi_proc) __indirect_glMultiTexCoord1fvARB;
    table[380] = (_glapi_proc) __indirect_glMultiTexCoord1iARB;
    table[381] = (_glapi_proc) __indirect_glMultiTexCoord1ivARB;
    table[382] = (_glapi_proc) __indirect_glMultiTexCoord1sARB;
    table[383] = (_glapi_proc) __indirect_glMultiTexCoord1svARB;
    table[384] = (_glapi_proc) __indirect_glMultiTexCoord2dARB;
    table[385] = (_glapi_proc) __indirect_glMultiTexCoord2dvARB;
    table[386] = (_glapi_proc) __indirect_glMultiTexCoord2fARB;
    table[387] = (_glapi_proc) __indirect_glMultiTexCoord2fvARB;
    table[388] = (_glapi_proc) __indirect_glMultiTexCoord2iARB;
    table[389] = (_glapi_proc) __indirect_glMultiTexCoord2ivARB;
    table[390] = (_glapi_proc) __indirect_glMultiTexCoord2sARB;
    table[391] = (_glapi_proc) __indirect_glMultiTexCoord2svARB;
    table[392] = (_glapi_proc) __indirect_glMultiTexCoord3dARB;
    table[393] = (_glapi_proc) __indirect_glMultiTexCoord3dvARB;
    table[394] = (_glapi_proc) __indirect_glMultiTexCoord3fARB;
    table[395] = (_glapi_proc) __indirect_glMultiTexCoord3fvARB;
    table[396] = (_glapi_proc) __indirect_glMultiTexCoord3iARB;
    table[397] = (_glapi_proc) __indirect_glMultiTexCoord3ivARB;
    table[398] = (_glapi_proc) __indirect_glMultiTexCoord3sARB;
    table[399] = (_glapi_proc) __indirect_glMultiTexCoord3svARB;
    table[400] = (_glapi_proc) __indirect_glMultiTexCoord4dARB;
    table[401] = (_glapi_proc) __indirect_glMultiTexCoord4dvARB;
    table[402] = (_glapi_proc) __indirect_glMultiTexCoord4fARB;
    table[403] = (_glapi_proc) __indirect_glMultiTexCoord4fvARB;
    table[404] = (_glapi_proc) __indirect_glMultiTexCoord4iARB;
    table[405] = (_glapi_proc) __indirect_glMultiTexCoord4ivARB;
    table[406] = (_glapi_proc) __indirect_glMultiTexCoord4sARB;
    table[407] = (_glapi_proc) __indirect_glMultiTexCoord4svARB;

    /*   3. GL_ARB_transpose_matrix */

    o = _glapi_get_proc_offset("glLoadTransposeMatrixdARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glLoadTransposeMatrixdARB;
    o = _glapi_get_proc_offset("glLoadTransposeMatrixfARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glLoadTransposeMatrixfARB;
    o = _glapi_get_proc_offset("glMultTransposeMatrixdARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glMultTransposeMatrixdARB;
    o = _glapi_get_proc_offset("glMultTransposeMatrixfARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glMultTransposeMatrixfARB;

    /*   5. GL_ARB_multisample */

    o = _glapi_get_proc_offset("glSampleCoverageARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSampleCoverageARB;

    /*  12. GL_ARB_texture_compression */

    o = _glapi_get_proc_offset("glCompressedTexImage1DARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCompressedTexImage1DARB;
    o = _glapi_get_proc_offset("glCompressedTexImage2DARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCompressedTexImage2DARB;
    o = _glapi_get_proc_offset("glCompressedTexImage3DARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCompressedTexImage3DARB;
    o = _glapi_get_proc_offset("glCompressedTexSubImage1DARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCompressedTexSubImage1DARB;
    o = _glapi_get_proc_offset("glCompressedTexSubImage2DARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCompressedTexSubImage2DARB;
    o = _glapi_get_proc_offset("glCompressedTexSubImage3DARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCompressedTexSubImage3DARB;
    o = _glapi_get_proc_offset("glGetCompressedTexImageARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetCompressedTexImageARB;

    /*  26. GL_ARB_vertex_program */

    o = _glapi_get_proc_offset("glDisableVertexAttribArrayARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glDisableVertexAttribArrayARB;
    o = _glapi_get_proc_offset("glEnableVertexAttribArrayARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glEnableVertexAttribArrayARB;
    o = _glapi_get_proc_offset("glGetProgramEnvParameterdvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramEnvParameterdvARB;
    o = _glapi_get_proc_offset("glGetProgramEnvParameterfvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramEnvParameterfvARB;
    o = _glapi_get_proc_offset("glGetProgramLocalParameterdvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramLocalParameterdvARB;
    o = _glapi_get_proc_offset("glGetProgramLocalParameterfvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramLocalParameterfvARB;
    o = _glapi_get_proc_offset("glGetProgramStringARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramStringARB;
    o = _glapi_get_proc_offset("glGetProgramivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramivARB;
    o = _glapi_get_proc_offset("glGetVertexAttribdvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribdvARB;
    o = _glapi_get_proc_offset("glGetVertexAttribfvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribfvARB;
    o = _glapi_get_proc_offset("glGetVertexAttribivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribivARB;
    o = _glapi_get_proc_offset("glProgramEnvParameter4dARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramEnvParameter4dARB;
    o = _glapi_get_proc_offset("glProgramEnvParameter4dvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramEnvParameter4dvARB;
    o = _glapi_get_proc_offset("glProgramEnvParameter4fARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramEnvParameter4fARB;
    o = _glapi_get_proc_offset("glProgramEnvParameter4fvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramEnvParameter4fvARB;
    o = _glapi_get_proc_offset("glProgramLocalParameter4dARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramLocalParameter4dARB;
    o = _glapi_get_proc_offset("glProgramLocalParameter4dvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramLocalParameter4dvARB;
    o = _glapi_get_proc_offset("glProgramLocalParameter4fARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramLocalParameter4fARB;
    o = _glapi_get_proc_offset("glProgramLocalParameter4fvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramLocalParameter4fvARB;
    o = _glapi_get_proc_offset("glProgramStringARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramStringARB;
    o = _glapi_get_proc_offset("glVertexAttrib1dARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1dARB;
    o = _glapi_get_proc_offset("glVertexAttrib1dvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1dvARB;
    o = _glapi_get_proc_offset("glVertexAttrib1fARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1fARB;
    o = _glapi_get_proc_offset("glVertexAttrib1fvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1fvARB;
    o = _glapi_get_proc_offset("glVertexAttrib1sARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1sARB;
    o = _glapi_get_proc_offset("glVertexAttrib1svARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1svARB;
    o = _glapi_get_proc_offset("glVertexAttrib2dARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2dARB;
    o = _glapi_get_proc_offset("glVertexAttrib2dvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2dvARB;
    o = _glapi_get_proc_offset("glVertexAttrib2fARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2fARB;
    o = _glapi_get_proc_offset("glVertexAttrib2fvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2fvARB;
    o = _glapi_get_proc_offset("glVertexAttrib2sARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2sARB;
    o = _glapi_get_proc_offset("glVertexAttrib2svARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2svARB;
    o = _glapi_get_proc_offset("glVertexAttrib3dARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3dARB;
    o = _glapi_get_proc_offset("glVertexAttrib3dvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3dvARB;
    o = _glapi_get_proc_offset("glVertexAttrib3fARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3fARB;
    o = _glapi_get_proc_offset("glVertexAttrib3fvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3fvARB;
    o = _glapi_get_proc_offset("glVertexAttrib3sARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3sARB;
    o = _glapi_get_proc_offset("glVertexAttrib3svARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3svARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NbvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NbvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NivARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NsvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NsvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NubARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NubARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NubvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NubvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NuivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NuivARB;
    o = _glapi_get_proc_offset("glVertexAttrib4NusvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4NusvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4bvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4bvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4dARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4dARB;
    o = _glapi_get_proc_offset("glVertexAttrib4dvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4dvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4fARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4fARB;
    o = _glapi_get_proc_offset("glVertexAttrib4fvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4fvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4ivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4ivARB;
    o = _glapi_get_proc_offset("glVertexAttrib4sARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4sARB;
    o = _glapi_get_proc_offset("glVertexAttrib4svARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4svARB;
    o = _glapi_get_proc_offset("glVertexAttrib4ubvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4ubvARB;
    o = _glapi_get_proc_offset("glVertexAttrib4uivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4uivARB;
    o = _glapi_get_proc_offset("glVertexAttrib4usvARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4usvARB;
    o = _glapi_get_proc_offset("glVertexAttribPointerARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribPointerARB;

    /*  29. GL_ARB_occlusion_query */

    o = _glapi_get_proc_offset("glBeginQueryARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBeginQueryARB;
    o = _glapi_get_proc_offset("glDeleteQueriesARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glDeleteQueriesARB;
    o = _glapi_get_proc_offset("glEndQueryARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glEndQueryARB;
    o = _glapi_get_proc_offset("glGenQueriesARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGenQueriesARB;
    o = _glapi_get_proc_offset("glGetQueryObjectivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetQueryObjectivARB;
    o = _glapi_get_proc_offset("glGetQueryObjectuivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetQueryObjectuivARB;
    o = _glapi_get_proc_offset("glGetQueryivARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetQueryivARB;
    o = _glapi_get_proc_offset("glIsQueryARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glIsQueryARB;

    /*  37. GL_ARB_draw_buffers */

    o = _glapi_get_proc_offset("glDrawBuffersARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glDrawBuffersARB;

    /*  39. GL_ARB_color_buffer_float */

    o = _glapi_get_proc_offset("glClampColorARB");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glClampColorARB;

    /*  45. GL_ARB_framebuffer_object */

    o = _glapi_get_proc_offset("glRenderbufferStorageMultisample");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glRenderbufferStorageMultisample;

    /*  25. GL_SGIS_multisample */

    o = _glapi_get_proc_offset("glSampleMaskSGIS");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSampleMaskSGIS;
    o = _glapi_get_proc_offset("glSamplePatternSGIS");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSamplePatternSGIS;

    /*  30. GL_EXT_vertex_array */

    o = _glapi_get_proc_offset("glColorPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glColorPointerEXT;
    o = _glapi_get_proc_offset("glEdgeFlagPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glEdgeFlagPointerEXT;
    o = _glapi_get_proc_offset("glIndexPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glIndexPointerEXT;
    o = _glapi_get_proc_offset("glNormalPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glNormalPointerEXT;
    o = _glapi_get_proc_offset("glTexCoordPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glTexCoordPointerEXT;
    o = _glapi_get_proc_offset("glVertexPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexPointerEXT;

    /*  54. GL_EXT_point_parameters */

    o = _glapi_get_proc_offset("glPointParameterfEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glPointParameterfEXT;
    o = _glapi_get_proc_offset("glPointParameterfvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glPointParameterfvEXT;

    /* 145. GL_EXT_secondary_color */

    o = _glapi_get_proc_offset("glSecondaryColor3bEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3bEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3bvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3bvEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3dEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3dEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3dvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3dvEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3fEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3fEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3fvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3fvEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3iEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3iEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3ivEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3ivEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3sEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3sEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3svEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3svEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3ubEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3ubEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3ubvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3ubvEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3uiEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3uiEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3uivEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3uivEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3usEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3usEXT;
    o = _glapi_get_proc_offset("glSecondaryColor3usvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColor3usvEXT;
    o = _glapi_get_proc_offset("glSecondaryColorPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glSecondaryColorPointerEXT;

    /* 148. GL_EXT_multi_draw_arrays */

    o = _glapi_get_proc_offset("glMultiDrawArraysEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glMultiDrawArraysEXT;
    o = _glapi_get_proc_offset("glMultiDrawElementsEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glMultiDrawElementsEXT;

    /* 149. GL_EXT_fog_coord */

    o = _glapi_get_proc_offset("glFogCoordPointerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFogCoordPointerEXT;
    o = _glapi_get_proc_offset("glFogCoorddEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFogCoorddEXT;
    o = _glapi_get_proc_offset("glFogCoorddvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFogCoorddvEXT;
    o = _glapi_get_proc_offset("glFogCoordfEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFogCoordfEXT;
    o = _glapi_get_proc_offset("glFogCoordfvEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFogCoordfvEXT;

    /* 173. GL_EXT_blend_func_separate */

    o = _glapi_get_proc_offset("glBlendFuncSeparateEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBlendFuncSeparateEXT;

    /* 197. GL_MESA_window_pos */

    o = _glapi_get_proc_offset("glWindowPos2dMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2dMESA;
    o = _glapi_get_proc_offset("glWindowPos2dvMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2dvMESA;
    o = _glapi_get_proc_offset("glWindowPos2fMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2fMESA;
    o = _glapi_get_proc_offset("glWindowPos2fvMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2fvMESA;
    o = _glapi_get_proc_offset("glWindowPos2iMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2iMESA;
    o = _glapi_get_proc_offset("glWindowPos2ivMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2ivMESA;
    o = _glapi_get_proc_offset("glWindowPos2sMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2sMESA;
    o = _glapi_get_proc_offset("glWindowPos2svMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos2svMESA;
    o = _glapi_get_proc_offset("glWindowPos3dMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3dMESA;
    o = _glapi_get_proc_offset("glWindowPos3dvMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3dvMESA;
    o = _glapi_get_proc_offset("glWindowPos3fMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3fMESA;
    o = _glapi_get_proc_offset("glWindowPos3fvMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3fvMESA;
    o = _glapi_get_proc_offset("glWindowPos3iMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3iMESA;
    o = _glapi_get_proc_offset("glWindowPos3ivMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3ivMESA;
    o = _glapi_get_proc_offset("glWindowPos3sMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3sMESA;
    o = _glapi_get_proc_offset("glWindowPos3svMESA");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glWindowPos3svMESA;

    /* 233. GL_NV_vertex_program */

    o = _glapi_get_proc_offset("glAreProgramsResidentNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glAreProgramsResidentNV;
    o = _glapi_get_proc_offset("glBindProgramNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBindProgramNV;
    o = _glapi_get_proc_offset("glDeleteProgramsNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glDeleteProgramsNV;
    o = _glapi_get_proc_offset("glExecuteProgramNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glExecuteProgramNV;
    o = _glapi_get_proc_offset("glGenProgramsNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGenProgramsNV;
    o = _glapi_get_proc_offset("glGetProgramParameterdvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramParameterdvNV;
    o = _glapi_get_proc_offset("glGetProgramParameterfvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramParameterfvNV;
    o = _glapi_get_proc_offset("glGetProgramStringNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramStringNV;
    o = _glapi_get_proc_offset("glGetProgramivNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramivNV;
    o = _glapi_get_proc_offset("glGetTrackMatrixivNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetTrackMatrixivNV;
    o = _glapi_get_proc_offset("glGetVertexAttribPointervNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribPointervNV;
    o = _glapi_get_proc_offset("glGetVertexAttribdvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribdvNV;
    o = _glapi_get_proc_offset("glGetVertexAttribfvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribfvNV;
    o = _glapi_get_proc_offset("glGetVertexAttribivNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetVertexAttribivNV;
    o = _glapi_get_proc_offset("glIsProgramNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glIsProgramNV;
    o = _glapi_get_proc_offset("glLoadProgramNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glLoadProgramNV;
    o = _glapi_get_proc_offset("glProgramParameters4dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramParameters4dvNV;
    o = _glapi_get_proc_offset("glProgramParameters4fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramParameters4fvNV;
    o = _glapi_get_proc_offset("glRequestResidentProgramsNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glRequestResidentProgramsNV;
    o = _glapi_get_proc_offset("glTrackMatrixNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glTrackMatrixNV;
    o = _glapi_get_proc_offset("glVertexAttrib1dNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1dNV;
    o = _glapi_get_proc_offset("glVertexAttrib1dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1dvNV;
    o = _glapi_get_proc_offset("glVertexAttrib1fNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1fNV;
    o = _glapi_get_proc_offset("glVertexAttrib1fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1fvNV;
    o = _glapi_get_proc_offset("glVertexAttrib1sNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1sNV;
    o = _glapi_get_proc_offset("glVertexAttrib1svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib1svNV;
    o = _glapi_get_proc_offset("glVertexAttrib2dNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2dNV;
    o = _glapi_get_proc_offset("glVertexAttrib2dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2dvNV;
    o = _glapi_get_proc_offset("glVertexAttrib2fNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2fNV;
    o = _glapi_get_proc_offset("glVertexAttrib2fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2fvNV;
    o = _glapi_get_proc_offset("glVertexAttrib2sNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2sNV;
    o = _glapi_get_proc_offset("glVertexAttrib2svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib2svNV;
    o = _glapi_get_proc_offset("glVertexAttrib3dNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3dNV;
    o = _glapi_get_proc_offset("glVertexAttrib3dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3dvNV;
    o = _glapi_get_proc_offset("glVertexAttrib3fNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3fNV;
    o = _glapi_get_proc_offset("glVertexAttrib3fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3fvNV;
    o = _glapi_get_proc_offset("glVertexAttrib3sNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3sNV;
    o = _glapi_get_proc_offset("glVertexAttrib3svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib3svNV;
    o = _glapi_get_proc_offset("glVertexAttrib4dNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4dNV;
    o = _glapi_get_proc_offset("glVertexAttrib4dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4dvNV;
    o = _glapi_get_proc_offset("glVertexAttrib4fNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4fNV;
    o = _glapi_get_proc_offset("glVertexAttrib4fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4fvNV;
    o = _glapi_get_proc_offset("glVertexAttrib4sNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4sNV;
    o = _glapi_get_proc_offset("glVertexAttrib4svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4svNV;
    o = _glapi_get_proc_offset("glVertexAttrib4ubNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4ubNV;
    o = _glapi_get_proc_offset("glVertexAttrib4ubvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttrib4ubvNV;
    o = _glapi_get_proc_offset("glVertexAttribPointerNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribPointerNV;
    o = _glapi_get_proc_offset("glVertexAttribs1dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs1dvNV;
    o = _glapi_get_proc_offset("glVertexAttribs1fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs1fvNV;
    o = _glapi_get_proc_offset("glVertexAttribs1svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs1svNV;
    o = _glapi_get_proc_offset("glVertexAttribs2dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs2dvNV;
    o = _glapi_get_proc_offset("glVertexAttribs2fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs2fvNV;
    o = _glapi_get_proc_offset("glVertexAttribs2svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs2svNV;
    o = _glapi_get_proc_offset("glVertexAttribs3dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs3dvNV;
    o = _glapi_get_proc_offset("glVertexAttribs3fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs3fvNV;
    o = _glapi_get_proc_offset("glVertexAttribs3svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs3svNV;
    o = _glapi_get_proc_offset("glVertexAttribs4dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs4dvNV;
    o = _glapi_get_proc_offset("glVertexAttribs4fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs4fvNV;
    o = _glapi_get_proc_offset("glVertexAttribs4svNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs4svNV;
    o = _glapi_get_proc_offset("glVertexAttribs4ubvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glVertexAttribs4ubvNV;

    /* 262. GL_NV_point_sprite */

    o = _glapi_get_proc_offset("glPointParameteriNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glPointParameteriNV;
    o = _glapi_get_proc_offset("glPointParameterivNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glPointParameterivNV;

    /* 268. GL_EXT_stencil_two_side */

    o = _glapi_get_proc_offset("glActiveStencilFaceEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glActiveStencilFaceEXT;

    /* 282. GL_NV_fragment_program */

    o = _glapi_get_proc_offset("glGetProgramNamedParameterdvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramNamedParameterdvNV;
    o = _glapi_get_proc_offset("glGetProgramNamedParameterfvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetProgramNamedParameterfvNV;
    o = _glapi_get_proc_offset("glProgramNamedParameter4dNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramNamedParameter4dNV;
    o = _glapi_get_proc_offset("glProgramNamedParameter4dvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramNamedParameter4dvNV;
    o = _glapi_get_proc_offset("glProgramNamedParameter4fNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramNamedParameter4fNV;
    o = _glapi_get_proc_offset("glProgramNamedParameter4fvNV");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glProgramNamedParameter4fvNV;

    /* 299. GL_EXT_blend_equation_separate */

    o = _glapi_get_proc_offset("glBlendEquationSeparateEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBlendEquationSeparateEXT;

    /* 310. GL_EXT_framebuffer_object */

    o = _glapi_get_proc_offset("glBindFramebufferEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBindFramebufferEXT;
    o = _glapi_get_proc_offset("glBindRenderbufferEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBindRenderbufferEXT;
    o = _glapi_get_proc_offset("glCheckFramebufferStatusEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glCheckFramebufferStatusEXT;
    o = _glapi_get_proc_offset("glDeleteFramebuffersEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glDeleteFramebuffersEXT;
    o = _glapi_get_proc_offset("glDeleteRenderbuffersEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glDeleteRenderbuffersEXT;
    o = _glapi_get_proc_offset("glFramebufferRenderbufferEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFramebufferRenderbufferEXT;
    o = _glapi_get_proc_offset("glFramebufferTexture1DEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFramebufferTexture1DEXT;
    o = _glapi_get_proc_offset("glFramebufferTexture2DEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFramebufferTexture2DEXT;
    o = _glapi_get_proc_offset("glFramebufferTexture3DEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFramebufferTexture3DEXT;
    o = _glapi_get_proc_offset("glGenFramebuffersEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGenFramebuffersEXT;
    o = _glapi_get_proc_offset("glGenRenderbuffersEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGenRenderbuffersEXT;
    o = _glapi_get_proc_offset("glGenerateMipmapEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGenerateMipmapEXT;
    o = _glapi_get_proc_offset("glGetFramebufferAttachmentParameterivEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetFramebufferAttachmentParameterivEXT;
    o = _glapi_get_proc_offset("glGetRenderbufferParameterivEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glGetRenderbufferParameterivEXT;
    o = _glapi_get_proc_offset("glIsFramebufferEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glIsFramebufferEXT;
    o = _glapi_get_proc_offset("glIsRenderbufferEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glIsRenderbufferEXT;
    o = _glapi_get_proc_offset("glRenderbufferStorageEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glRenderbufferStorageEXT;

    /* 316. GL_EXT_framebuffer_blit */

    o = _glapi_get_proc_offset("glBlitFramebufferEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glBlitFramebufferEXT;

    /* 329. GL_EXT_texture_array */

    o = _glapi_get_proc_offset("glFramebufferTextureLayerEXT");
    assert(o > 0);
    table[o] = (_glapi_proc) __indirect_glFramebufferTextureLayerEXT;

    return (struct _glapi_table *) table;
}

