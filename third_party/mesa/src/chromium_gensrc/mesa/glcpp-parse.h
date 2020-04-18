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

#ifndef YY_GLCPP_PARSER_GLCPP_PARSE_H_INCLUDED
# define YY_GLCPP_PARSER_GLCPP_PARSE_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int glcpp_parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    COMMA_FINAL = 258,
    DEFINED = 259,
    ELIF_EXPANDED = 260,
    HASH = 261,
    HASH_DEFINE = 262,
    FUNC_IDENTIFIER = 263,
    OBJ_IDENTIFIER = 264,
    HASH_ELIF = 265,
    HASH_ELSE = 266,
    HASH_ENDIF = 267,
    HASH_IF = 268,
    HASH_IFDEF = 269,
    HASH_IFNDEF = 270,
    HASH_LINE = 271,
    HASH_UNDEF = 272,
    HASH_VERSION = 273,
    IDENTIFIER = 274,
    IF_EXPANDED = 275,
    INTEGER = 276,
    INTEGER_STRING = 277,
    LINE_EXPANDED = 278,
    NEWLINE = 279,
    OTHER = 280,
    PLACEHOLDER = 281,
    SPACE = 282,
    PASTE = 283,
    OR = 284,
    AND = 285,
    EQUAL = 286,
    NOT_EQUAL = 287,
    LESS_OR_EQUAL = 288,
    GREATER_OR_EQUAL = 289,
    LEFT_SHIFT = 290,
    RIGHT_SHIFT = 291,
    UNARY = 292
  };
#endif
/* Tokens.  */
#define COMMA_FINAL 258
#define DEFINED 259
#define ELIF_EXPANDED 260
#define HASH 261
#define HASH_DEFINE 262
#define FUNC_IDENTIFIER 263
#define OBJ_IDENTIFIER 264
#define HASH_ELIF 265
#define HASH_ELSE 266
#define HASH_ENDIF 267
#define HASH_IF 268
#define HASH_IFDEF 269
#define HASH_IFNDEF 270
#define HASH_LINE 271
#define HASH_UNDEF 272
#define HASH_VERSION 273
#define IDENTIFIER 274
#define IF_EXPANDED 275
#define INTEGER 276
#define INTEGER_STRING 277
#define LINE_EXPANDED 278
#define NEWLINE 279
#define OTHER 280
#define PLACEHOLDER 281
#define SPACE 282
#define PASTE 283
#define OR 284
#define AND 285
#define EQUAL 286
#define NOT_EQUAL 287
#define LESS_OR_EQUAL 288
#define GREATER_OR_EQUAL 289
#define LEFT_SHIFT 290
#define RIGHT_SHIFT 291
#define UNARY 292

/* Value type.  */

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



int glcpp_parser_parse (glcpp_parser_t *parser);

#endif /* !YY_GLCPP_PARSER_GLCPP_PARSE_H_INCLUDED  */
