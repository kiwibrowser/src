/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY__MESA_PROGRAM_PROGRAM_PROGRAM_PARSE_TAB_H_INCLUDED
# define YY__MESA_PROGRAM_PROGRAM_PROGRAM_PARSE_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int _mesa_program_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ARBvp_10 = 258,
    ARBfp_10 = 259,
    ADDRESS = 260,
    ALIAS = 261,
    ATTRIB = 262,
    OPTION = 263,
    OUTPUT = 264,
    PARAM = 265,
    TEMP = 266,
    END = 267,
    BIN_OP = 268,
    BINSC_OP = 269,
    SAMPLE_OP = 270,
    SCALAR_OP = 271,
    TRI_OP = 272,
    VECTOR_OP = 273,
    ARL = 274,
    KIL = 275,
    SWZ = 276,
    TXD_OP = 277,
    INTEGER = 278,
    REAL = 279,
    AMBIENT = 280,
    ATTENUATION = 281,
    BACK = 282,
    CLIP = 283,
    COLOR = 284,
    DEPTH = 285,
    DIFFUSE = 286,
    DIRECTION = 287,
    EMISSION = 288,
    ENV = 289,
    EYE = 290,
    FOG = 291,
    FOGCOORD = 292,
    FRAGMENT = 293,
    FRONT = 294,
    HALF = 295,
    INVERSE = 296,
    INVTRANS = 297,
    LIGHT = 298,
    LIGHTMODEL = 299,
    LIGHTPROD = 300,
    LOCAL = 301,
    MATERIAL = 302,
    MAT_PROGRAM = 303,
    MATRIX = 304,
    MATRIXINDEX = 305,
    MODELVIEW = 306,
    MVP = 307,
    NORMAL = 308,
    OBJECT = 309,
    PALETTE = 310,
    PARAMS = 311,
    PLANE = 312,
    POINT_TOK = 313,
    POINTSIZE = 314,
    POSITION = 315,
    PRIMARY = 316,
    PROGRAM = 317,
    PROJECTION = 318,
    RANGE = 319,
    RESULT = 320,
    ROW = 321,
    SCENECOLOR = 322,
    SECONDARY = 323,
    SHININESS = 324,
    SIZE_TOK = 325,
    SPECULAR = 326,
    SPOT = 327,
    STATE = 328,
    TEXCOORD = 329,
    TEXENV = 330,
    TEXGEN = 331,
    TEXGEN_Q = 332,
    TEXGEN_R = 333,
    TEXGEN_S = 334,
    TEXGEN_T = 335,
    TEXTURE = 336,
    TRANSPOSE = 337,
    TEXTURE_UNIT = 338,
    TEX_1D = 339,
    TEX_2D = 340,
    TEX_3D = 341,
    TEX_CUBE = 342,
    TEX_RECT = 343,
    TEX_SHADOW1D = 344,
    TEX_SHADOW2D = 345,
    TEX_SHADOWRECT = 346,
    TEX_ARRAY1D = 347,
    TEX_ARRAY2D = 348,
    TEX_ARRAYSHADOW1D = 349,
    TEX_ARRAYSHADOW2D = 350,
    VERTEX = 351,
    VTXATTRIB = 352,
    WEIGHT = 353,
    IDENTIFIER = 354,
    USED_IDENTIFIER = 355,
    MASK4 = 356,
    MASK3 = 357,
    MASK2 = 358,
    MASK1 = 359,
    SWIZZLE = 360,
    DOT_DOT = 361,
    DOT = 362
  };
#endif
/* Tokens.  */
#define ARBvp_10 258
#define ARBfp_10 259
#define ADDRESS 260
#define ALIAS 261
#define ATTRIB 262
#define OPTION 263
#define OUTPUT 264
#define PARAM 265
#define TEMP 266
#define END 267
#define BIN_OP 268
#define BINSC_OP 269
#define SAMPLE_OP 270
#define SCALAR_OP 271
#define TRI_OP 272
#define VECTOR_OP 273
#define ARL 274
#define KIL 275
#define SWZ 276
#define TXD_OP 277
#define INTEGER 278
#define REAL 279
#define AMBIENT 280
#define ATTENUATION 281
#define BACK 282
#define CLIP 283
#define COLOR 284
#define DEPTH 285
#define DIFFUSE 286
#define DIRECTION 287
#define EMISSION 288
#define ENV 289
#define EYE 290
#define FOG 291
#define FOGCOORD 292
#define FRAGMENT 293
#define FRONT 294
#define HALF 295
#define INVERSE 296
#define INVTRANS 297
#define LIGHT 298
#define LIGHTMODEL 299
#define LIGHTPROD 300
#define LOCAL 301
#define MATERIAL 302
#define MAT_PROGRAM 303
#define MATRIX 304
#define MATRIXINDEX 305
#define MODELVIEW 306
#define MVP 307
#define NORMAL 308
#define OBJECT 309
#define PALETTE 310
#define PARAMS 311
#define PLANE 312
#define POINT_TOK 313
#define POINTSIZE 314
#define POSITION 315
#define PRIMARY 316
#define PROGRAM 317
#define PROJECTION 318
#define RANGE 319
#define RESULT 320
#define ROW 321
#define SCENECOLOR 322
#define SECONDARY 323
#define SHININESS 324
#define SIZE_TOK 325
#define SPECULAR 326
#define SPOT 327
#define STATE 328
#define TEXCOORD 329
#define TEXENV 330
#define TEXGEN 331
#define TEXGEN_Q 332
#define TEXGEN_R 333
#define TEXGEN_S 334
#define TEXGEN_T 335
#define TEXTURE 336
#define TRANSPOSE 337
#define TEXTURE_UNIT 338
#define TEX_1D 339
#define TEX_2D 340
#define TEX_3D 341
#define TEX_CUBE 342
#define TEX_RECT 343
#define TEX_SHADOW1D 344
#define TEX_SHADOW2D 345
#define TEX_SHADOWRECT 346
#define TEX_ARRAY1D 347
#define TEX_ARRAY2D 348
#define TEX_ARRAYSHADOW1D 349
#define TEX_ARRAYSHADOW2D 350
#define VERTEX 351
#define VTXATTRIB 352
#define WEIGHT 353
#define IDENTIFIER 354
#define USED_IDENTIFIER 355
#define MASK4 356
#define MASK3 357
#define MASK2 358
#define MASK1 359
#define SWIZZLE 360
#define DOT_DOT 361
#define DOT 362

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 126 "program/program_parse.y" /* yacc.c:1909  */

   struct asm_instruction *inst;
   struct asm_symbol *sym;
   struct asm_symbol temp_sym;
   struct asm_swizzle_mask swiz_mask;
   struct asm_src_register src_reg;
   struct prog_dst_register dst_reg;
   struct prog_instruction temp_inst;
   char *string;
   unsigned result;
   unsigned attrib;
   int integer;
   float real;
   gl_state_index state[STATE_LENGTH];
   int negate;
   struct asm_vector vector;
   gl_inst_opcode opcode;

   struct {
      unsigned swz;
      unsigned rgba_valid:1;
      unsigned xyzw_valid:1;
      unsigned negate:1;
   } ext_swizzle;

#line 294 "program/program_parse.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int _mesa_program_parse (struct asm_parser_state *state);

#endif /* !YY__MESA_PROGRAM_PROGRAM_PROGRAM_PARSE_TAB_H_INCLUDED  */
