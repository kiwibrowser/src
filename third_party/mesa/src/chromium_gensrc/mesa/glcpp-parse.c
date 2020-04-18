/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         glcpp_parser_parse
#define yylex           glcpp_parser_lex
#define yyerror         glcpp_parser_error
#define yydebug         glcpp_parser_debug
#define yynerrs         glcpp_parser_nerrs


/* Copy the first part of user declarations.  */
#line 1 "glcpp-parse.y" /* yacc.c:339  */

/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "glcpp.h"
#include "main/core.h" /* for struct gl_extensions */
#include "main/mtypes.h" /* for gl_api enum */

static void
yyerror (YYLTYPE *locp, glcpp_parser_t *parser, const char *error);

static void
_define_object_macro (glcpp_parser_t *parser,
		      YYLTYPE *loc,
		      const char *macro,
		      token_list_t *replacements);

static void
_define_function_macro (glcpp_parser_t *parser,
			YYLTYPE *loc,
			const char *macro,
			string_list_t *parameters,
			token_list_t *replacements);

static string_list_t *
_string_list_create (void *ctx);

static void
_string_list_append_item (string_list_t *list, const char *str);

static int
_string_list_contains (string_list_t *list, const char *member, int *index);

static int
_string_list_length (string_list_t *list);

static int
_string_list_equal (string_list_t *a, string_list_t *b);

static argument_list_t *
_argument_list_create (void *ctx);

static void
_argument_list_append (argument_list_t *list, token_list_t *argument);

static int
_argument_list_length (argument_list_t *list);

static token_list_t *
_argument_list_member_at (argument_list_t *list, int index);

/* Note: This function ralloc_steal()s the str pointer. */
static token_t *
_token_create_str (void *ctx, int type, char *str);

static token_t *
_token_create_ival (void *ctx, int type, int ival);

static token_list_t *
_token_list_create (void *ctx);

static void
_token_list_append (token_list_t *list, token_t *token);

static void
_token_list_append_list (token_list_t *list, token_list_t *tail);

static int
_token_list_equal_ignoring_space (token_list_t *a, token_list_t *b);

static void
_parser_active_list_push (glcpp_parser_t *parser,
			  const char *identifier,
			  token_node_t *marker);

static void
_parser_active_list_pop (glcpp_parser_t *parser);

static int
_parser_active_list_contains (glcpp_parser_t *parser, const char *identifier);

/* Expand list, and begin lexing from the result (after first
 * prefixing a token of type 'head_token_type').
 */
static void
_glcpp_parser_expand_and_lex_from (glcpp_parser_t *parser,
				   int head_token_type,
				   token_list_t *list);

/* Perform macro expansion in-place on the given list. */
static void
_glcpp_parser_expand_token_list (glcpp_parser_t *parser,
				 token_list_t *list);

static void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list);

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, YYLTYPE *loc,
				  int condition);

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, YYLTYPE *loc,
				    const char *type, int condition);

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser, YYLTYPE *loc);

static int
glcpp_parser_lex (YYSTYPE *yylval, YYLTYPE *yylloc, glcpp_parser_t *parser);

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list);

static void
add_builtin_define(glcpp_parser_t *parser, const char *name, int value);


#line 218 "glcpp-parse.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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

/* Copy the second part of user declarations.  */

#line 351 "glcpp-parse.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   670

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  60
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  19
/* YYNRULES -- Number of rules.  */
#define YYNRULES  107
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  172

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   292

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    50,     2,     2,     2,    46,    33,     2,
      48,    49,    44,    42,    52,    43,    57,    45,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    58,
      36,    59,    37,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    53,     2,    54,    32,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    55,    31,    56,    51,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    34,    35,    38,    39,
      40,    41,    47
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   184,   184,   186,   190,   193,   198,   199,   203,   206,
     209,   217,   230,   233,   236,   239,   247,   255,   275,   285,
     290,   295,   315,   330,   330,   333,   333,   336,   357,   361,
     370,   375,   376,   379,   382,   385,   388,   391,   394,   397,
     400,   403,   406,   409,   412,   415,   418,   421,   424,   432,
     440,   443,   446,   449,   452,   455,   461,   466,   474,   475,
     479,   485,   486,   489,   491,   498,   502,   506,   511,   515,
     522,   527,   534,   538,   542,   546,   550,   557,   558,   559,
     560,   561,   562,   563,   564,   565,   566,   567,   568,   569,
     570,   571,   572,   573,   574,   575,   576,   577,   578,   579,
     580,   581,   582,   583,   584,   585,   586,   587
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "COMMA_FINAL", "DEFINED",
  "ELIF_EXPANDED", "HASH", "HASH_DEFINE", "FUNC_IDENTIFIER",
  "OBJ_IDENTIFIER", "HASH_ELIF", "HASH_ELSE", "HASH_ENDIF", "HASH_IF",
  "HASH_IFDEF", "HASH_IFNDEF", "HASH_LINE", "HASH_UNDEF", "HASH_VERSION",
  "IDENTIFIER", "IF_EXPANDED", "INTEGER", "INTEGER_STRING",
  "LINE_EXPANDED", "NEWLINE", "OTHER", "PLACEHOLDER", "SPACE", "PASTE",
  "OR", "AND", "'|'", "'^'", "'&'", "EQUAL", "NOT_EQUAL", "'<'", "'>'",
  "LESS_OR_EQUAL", "GREATER_OR_EQUAL", "LEFT_SHIFT", "RIGHT_SHIFT", "'+'",
  "'-'", "'*'", "'/'", "'%'", "UNARY", "'('", "')'", "'!'", "'~'", "','",
  "'['", "']'", "'{'", "'}'", "'.'", "';'", "'='", "$accept", "input",
  "line", "expanded_line", "control_line", "$@1", "$@2",
  "integer_constant", "expression", "identifier_list", "text_line",
  "non_directive", "replacement_list", "junk", "conditional_token",
  "conditional_tokens", "pp_tokens", "preprocessing_token", "operator", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   124,    94,    38,   286,   287,    60,    62,   288,   289,
     290,   291,    43,    45,    42,    47,    37,   292,    40,    41,
      33,   126,    44,    91,    93,   123,   125,    46,    59,    61
};
# endif

#define YYPACT_NINF -149

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-149)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -149,   115,  -149,   492,    -9,   103,   158,  -149,  -149,   201,
      -5,    48,   451,    50,    95,  -149,   492,  -149,    95,  -149,
    -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,
    -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,
    -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,  -149,
    -149,  -149,  -149,  -149,  -149,  -149,  -149,   328,  -149,  -149,
    -149,  -149,  -149,   492,   492,   492,   492,   492,  -149,   515,
    -149,  -149,   369,    23,   451,    38,  -149,  -149,   244,  -149,
     -17,    51,  -149,   287,   451,   451,   410,    55,    63,   538,
      52,  -149,  -149,  -149,  -149,   487,  -149,  -149,  -149,   492,
     492,   492,   492,   492,   492,   492,   492,   492,   492,   492,
     492,   492,   492,   492,   492,   492,   492,  -149,    19,    89,
     451,  -149,    99,  -149,  -149,  -149,  -149,  -149,   100,   451,
     112,  -149,  -149,  -149,  -149,  -149,   117,  -149,   555,   571,
     586,   600,   613,   624,   624,    18,    18,    18,    18,    64,
      64,    37,    37,  -149,  -149,  -149,  -149,   451,   -41,  -149,
      70,  -149,  -149,  -149,   151,   451,   157,  -149,  -149,   154,
    -149,  -149
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,    23,    25,     0,
       0,     0,     0,     0,     0,    72,     0,    73,     0,    58,
      75,    76,   107,   103,   102,   101,   100,    84,    98,    99,
      94,    95,    96,    97,    92,    93,    86,    87,    85,    90,
      91,    79,    80,    89,    88,   105,    77,    78,    81,    82,
      83,   104,   106,     3,     6,     4,     5,     0,    70,    74,
      32,    30,    29,     0,     0,     0,     0,     0,    31,     0,
      28,     7,     0,     0,    61,     0,    22,    68,     0,    67,
       0,     0,    18,     0,    63,    63,     0,     0,     0,     0,
       0,    59,    71,    54,    53,     0,    51,    52,     9,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    60,     0,     0,
      62,    65,     0,    21,    69,    24,    26,    17,     0,    64,
       0,    16,    15,    27,     8,    10,     0,    55,    33,    34,
      35,    36,    37,    39,    38,    43,    42,    41,    40,    45,
      44,    47,    46,    50,    49,    48,    56,    61,     0,    12,
       0,    19,    20,    11,     0,    61,     0,    66,    13,     0,
      57,    14
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -149,  -149,  -149,  -149,  -149,  -149,  -149,   -12,   -11,  -149,
    -149,  -149,  -148,    94,   -13,   172,     0,    -6,  -149
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    53,    54,    55,    80,    81,    68,    69,   158,
      56,    71,   119,   128,    77,    78,   120,    58,    59
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      79,    57,    88,    79,    72,    89,    90,   125,   165,   164,
      15,   166,    86,    17,    84,    70,    20,   169,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,   156,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    92,    93,    94,    95,    96,    97,   121,   110,   111,
     112,   113,   114,   115,   116,   124,    92,    85,   157,    87,
     124,   118,    79,    61,    62,   126,   135,    79,   136,   132,
      92,   114,   115,   116,   129,   129,   122,   133,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   112,   113,   114,   115,
     116,    73,    74,   159,    92,     2,    61,    62,   160,   167,
       3,     4,     5,    92,   161,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,   162,    17,    18,    19,
      20,   163,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    75,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,   168,   170,    15,   171,   130,
      17,    83,    76,    20,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    75,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,     0,     0,
      15,     0,     0,    17,     0,    82,    20,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    75,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,     0,     0,    15,     0,     0,    17,     0,   123,    20,
       0,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    75,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,     0,     0,    15,     0,     0,    17,
       0,   127,    20,     0,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,     0,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    15,     0,     0,
      17,     0,    91,    20,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,     0,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    15,     0,
       0,    17,     0,   117,    20,     0,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,     0,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    15,
       0,     0,    17,     0,   131,    20,     0,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,     0,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      15,     0,     0,    17,     0,     0,    20,     0,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,     0,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    60,     0,    61,    62,     0,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,    63,    64,   137,     0,     0,    98,
      65,     0,    66,    67,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   134,     0,     0,     0,     0,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116
};

static const yytype_int16 yycheck[] =
{
       6,     1,    14,     9,     4,    16,    18,    24,    49,   157,
      19,    52,    12,    22,    19,    24,    25,   165,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    19,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    57,    63,    64,    65,    66,    67,    19,    40,    41,
      42,    43,    44,    45,    46,    78,    72,    19,    49,    19,
      83,    48,    78,    21,    22,    24,    24,    83,    90,    24,
      86,    44,    45,    46,    84,    85,    48,    24,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,    42,    43,    44,    45,
      46,     8,     9,    24,   120,     0,    21,    22,    19,    49,
       5,     6,     7,   129,    24,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    24,    22,    23,    24,
      25,    24,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,     4,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    24,    19,    19,    24,    85,
      22,     9,    24,    25,    -1,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,     4,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    -1,    -1,
      19,    -1,    -1,    22,    -1,    24,    25,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,     4,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    -1,    -1,    19,    -1,    -1,    22,    -1,    24,    25,
      -1,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,     4,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    -1,    -1,    19,    -1,    -1,    22,
      -1,    24,    25,    -1,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    -1,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    19,    -1,    -1,
      22,    -1,    24,    25,    -1,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    19,    -1,
      -1,    22,    -1,    24,    25,    -1,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    -1,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    19,
      -1,    -1,    22,    -1,    24,    25,    -1,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    -1,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      19,    -1,    -1,    22,    -1,    -1,    25,    -1,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    19,    -1,    21,    22,    -1,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    42,    43,    49,    -1,    -1,    24,
      48,    -1,    50,    51,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    24,    -1,    -1,    -1,    -1,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    61,     0,     5,     6,     7,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    22,    23,    24,
      25,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    62,    63,    64,    70,    76,    77,    78,
      19,    21,    22,    42,    43,    48,    50,    51,    67,    68,
      24,    71,    76,     8,     9,     4,    24,    74,    75,    77,
      65,    66,    24,    75,    19,    19,    76,    19,    67,    68,
      67,    24,    77,    68,    68,    68,    68,    68,    24,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    24,    48,    72,
      76,    19,    48,    24,    74,    24,    24,    24,    73,    76,
      73,    24,    24,    24,    24,    24,    67,    49,    68,    68,
      68,    68,    68,    68,    68,    68,    68,    68,    68,    68,
      68,    68,    68,    68,    68,    68,    19,    49,    69,    24,
      19,    24,    24,    24,    72,    49,    52,    49,    24,    72,
      19,    24
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    60,    61,    61,    62,    62,    62,    62,    63,    63,
      63,    63,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    65,    64,    66,    64,    64,    64,    67,
      67,    68,    68,    68,    68,    68,    68,    68,    68,    68,
      68,    68,    68,    68,    68,    68,    68,    68,    68,    68,
      68,    68,    68,    68,    68,    68,    69,    69,    70,    70,
      71,    72,    72,    73,    73,    74,    74,    74,    75,    75,
      76,    76,    77,    77,    77,    77,    77,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     2,     3,     3,
       3,     4,     4,     6,     7,     3,     3,     3,     2,     4,
       4,     3,     2,     0,     3,     0,     3,     3,     2,     1,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     3,     1,     3,     1,     2,
       2,     0,     1,     0,     1,     2,     4,     1,     1,     2,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (&yylloc, parser, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, glcpp_parser_t *parser)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  YYUSE (parser);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, glcpp_parser_t *parser)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, parser);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, glcpp_parser_t *parser)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , parser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, parser); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, glcpp_parser_t *parser)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (glcpp_parser_t *parser)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

/* User initialization code.  */
#line 151 "glcpp-parse.y" /* yacc.c:1429  */
{
	yylloc.first_line = 1;
	yylloc.first_column = 1;
	yylloc.last_line = 1;
	yylloc.last_column = 1;
	yylloc.source = 0;
}

#line 1580 "glcpp-parse.c" /* yacc.c:1429  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, &yylloc, parser);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 190 "glcpp-parse.y" /* yacc.c:1646  */
    {
		ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length, "\n");
	}
#line 1771 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 5:
#line 193 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_glcpp_parser_print_expanded_token_list (parser, (yyvsp[0].token_list));
		ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length, "\n");
		ralloc_free ((yyvsp[0].token_list));
	}
#line 1781 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 8:
#line 203 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[-2]), (yyvsp[-1].ival));
	}
#line 1789 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 9:
#line 206 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_glcpp_parser_skip_stack_change_if (parser, & (yylsp[-2]), "elif", (yyvsp[-1].ival));
	}
#line 1797 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 10:
#line 209 "glcpp-parse.y" /* yacc.c:1646  */
    {
		parser->has_new_line_number = 1;
		parser->new_line_number = (yyvsp[-1].ival);
		ralloc_asprintf_rewrite_tail (&parser->output,
					      &parser->output_length,
					      "#line %" PRIiMAX "\n",
					      (yyvsp[-1].ival));
	}
#line 1810 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 11:
#line 217 "glcpp-parse.y" /* yacc.c:1646  */
    {
		parser->has_new_line_number = 1;
		parser->new_line_number = (yyvsp[-2].ival);
		parser->has_new_source_number = 1;
		parser->new_source_number = (yyvsp[-1].ival);
		ralloc_asprintf_rewrite_tail (&parser->output,
					      &parser->output_length,
					      "#line %" PRIiMAX " %" PRIiMAX "\n",
					      (yyvsp[-2].ival), (yyvsp[-1].ival));
	}
#line 1825 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 12:
#line 230 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_define_object_macro (parser, & (yylsp[-2]), (yyvsp[-2].str), (yyvsp[-1].token_list));
	}
#line 1833 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 13:
#line 233 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_define_function_macro (parser, & (yylsp[-4]), (yyvsp[-4].str), NULL, (yyvsp[-1].token_list));
	}
#line 1841 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 14:
#line 236 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_define_function_macro (parser, & (yylsp[-5]), (yyvsp[-5].str), (yyvsp[-3].string_list), (yyvsp[-1].token_list));
	}
#line 1849 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 15:
#line 239 "glcpp-parse.y" /* yacc.c:1646  */
    {
		macro_t *macro = hash_table_find (parser->defines, (yyvsp[-1].str));
		if (macro) {
			hash_table_remove (parser->defines, (yyvsp[-1].str));
			ralloc_free (macro);
		}
		ralloc_free ((yyvsp[-1].str));
	}
#line 1862 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 16:
#line 247 "glcpp-parse.y" /* yacc.c:1646  */
    {
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			_glcpp_parser_expand_and_lex_from (parser,
							   LINE_EXPANDED, (yyvsp[-1].token_list));
		}
	}
#line 1875 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 17:
#line 255 "glcpp-parse.y" /* yacc.c:1646  */
    {
		/* Be careful to only evaluate the 'if' expression if
		 * we are not skipping. When we are skipping, we
		 * simply push a new 0-valued 'if' onto the skip
		 * stack.
		 *
		 * This avoids generating diagnostics for invalid
		 * expressions that are being skipped. */
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			_glcpp_parser_expand_and_lex_from (parser,
							   IF_EXPANDED, (yyvsp[-1].token_list));
		}	
		else
		{
			_glcpp_parser_skip_stack_push_if (parser, & (yylsp[-2]), 0);
			parser->skip_stack->type = SKIP_TO_ENDIF;
		}
	}
#line 1900 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 18:
#line 275 "glcpp-parse.y" /* yacc.c:1646  */
    {
		/* #if without an expression is only an error if we
		 *  are not skipping */
		if (parser->skip_stack == NULL ||
		    parser->skip_stack->type == SKIP_NO_SKIP)
		{
			glcpp_error(& (yylsp[-1]), parser, "#if with no expression");
		}	
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[-1]), 0);
	}
#line 1915 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 19:
#line 285 "glcpp-parse.y" /* yacc.c:1646  */
    {
		macro_t *macro = hash_table_find (parser->defines, (yyvsp[-2].str));
		ralloc_free ((yyvsp[-2].str));
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[-3]), macro != NULL);
	}
#line 1925 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 20:
#line 290 "glcpp-parse.y" /* yacc.c:1646  */
    {
		macro_t *macro = hash_table_find (parser->defines, (yyvsp[-2].str));
		ralloc_free ((yyvsp[-2].str));
		_glcpp_parser_skip_stack_push_if (parser, & (yylsp[-3]), macro == NULL);
	}
#line 1935 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 21:
#line 295 "glcpp-parse.y" /* yacc.c:1646  */
    {
		/* Be careful to only evaluate the 'elif' expression
		 * if we are not skipping. When we are skipping, we
		 * simply change to a 0-valued 'elif' on the skip
		 * stack.
		 *
		 * This avoids generating diagnostics for invalid
		 * expressions that are being skipped. */
		if (parser->skip_stack &&
		    parser->skip_stack->type == SKIP_TO_ELSE)
		{
			_glcpp_parser_expand_and_lex_from (parser,
							   ELIF_EXPANDED, (yyvsp[-1].token_list));
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & (yylsp[-2]),
							    "elif", 0);
		}
	}
#line 1960 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 22:
#line 315 "glcpp-parse.y" /* yacc.c:1646  */
    {
		/* #elif without an expression is an error unless we
		 * are skipping. */
		if (parser->skip_stack &&
		    parser->skip_stack->type == SKIP_TO_ELSE)
		{
			glcpp_error(& (yylsp[-1]), parser, "#elif with no expression");
		}
		else
		{
			_glcpp_parser_skip_stack_change_if (parser, & (yylsp[-1]),
							    "elif", 0);
			glcpp_warning(& (yylsp[-1]), parser, "ignoring illegal #elif without expression");
		}
	}
#line 1980 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 23:
#line 330 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_glcpp_parser_skip_stack_change_if (parser, & (yylsp[0]), "else", 1);
	}
#line 1988 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 25:
#line 333 "glcpp-parse.y" /* yacc.c:1646  */
    {
		_glcpp_parser_skip_stack_pop (parser, & (yylsp[0]));
	}
#line 1996 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 27:
#line 336 "glcpp-parse.y" /* yacc.c:1646  */
    {
		macro_t *macro = hash_table_find (parser->defines, "__VERSION__");
		if (macro) {
			hash_table_remove (parser->defines, "__VERSION__");
			ralloc_free (macro);
		}
		add_builtin_define (parser, "__VERSION__", (yyvsp[-1].ival));

		if ((yyvsp[-1].ival) == 100)
			add_builtin_define (parser, "GL_ES", 1);

		/* Currently, all ES2 implementations support highp in the
		 * fragment shader, so we always define this macro in ES2.
		 * If we ever get a driver that doesn't support highp, we'll
		 * need to add a flag to the gl_context and check that here.
		 */
		if ((yyvsp[-1].ival) >= 130 || (yyvsp[-1].ival) == 100)
			add_builtin_define (parser, "GL_FRAGMENT_PRECISION_HIGH", 1);

		ralloc_asprintf_rewrite_tail (&parser->output, &parser->output_length, "#version %" PRIiMAX, (yyvsp[-1].ival));
	}
#line 2022 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 29:
#line 361 "glcpp-parse.y" /* yacc.c:1646  */
    {
		if (strlen ((yyvsp[0].str)) >= 3 && strncmp ((yyvsp[0].str), "0x", 2) == 0) {
			(yyval.ival) = strtoll ((yyvsp[0].str) + 2, NULL, 16);
		} else if ((yyvsp[0].str)[0] == '0') {
			(yyval.ival) = strtoll ((yyvsp[0].str), NULL, 8);
		} else {
			(yyval.ival) = strtoll ((yyvsp[0].str), NULL, 10);
		}
	}
#line 2036 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 30:
#line 370 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[0].ival);
	}
#line 2044 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 32:
#line 376 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = 0;
	}
#line 2052 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 33:
#line 379 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) || (yyvsp[0].ival);
	}
#line 2060 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 34:
#line 382 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) && (yyvsp[0].ival);
	}
#line 2068 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 35:
#line 385 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) | (yyvsp[0].ival);
	}
#line 2076 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 36:
#line 388 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) ^ (yyvsp[0].ival);
	}
#line 2084 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 37:
#line 391 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) & (yyvsp[0].ival);
	}
#line 2092 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 38:
#line 394 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) != (yyvsp[0].ival);
	}
#line 2100 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 39:
#line 397 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) == (yyvsp[0].ival);
	}
#line 2108 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 40:
#line 400 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) >= (yyvsp[0].ival);
	}
#line 2116 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 41:
#line 403 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) <= (yyvsp[0].ival);
	}
#line 2124 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 42:
#line 406 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) > (yyvsp[0].ival);
	}
#line 2132 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 43:
#line 409 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) < (yyvsp[0].ival);
	}
#line 2140 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 44:
#line 412 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) >> (yyvsp[0].ival);
	}
#line 2148 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 45:
#line 415 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) << (yyvsp[0].ival);
	}
#line 2156 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 46:
#line 418 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) - (yyvsp[0].ival);
	}
#line 2164 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 47:
#line 421 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) + (yyvsp[0].ival);
	}
#line 2172 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 48:
#line 424 "glcpp-parse.y" /* yacc.c:1646  */
    {
		if ((yyvsp[0].ival) == 0) {
			yyerror (& (yylsp[-2]), parser,
				 "zero modulus in preprocessor directive");
		} else {
			(yyval.ival) = (yyvsp[-2].ival) % (yyvsp[0].ival);
		}
	}
#line 2185 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 49:
#line 432 "glcpp-parse.y" /* yacc.c:1646  */
    {
		if ((yyvsp[0].ival) == 0) {
			yyerror (& (yylsp[-2]), parser,
				 "division by 0 in preprocessor directive");
		} else {
			(yyval.ival) = (yyvsp[-2].ival) / (yyvsp[0].ival);
		}
	}
#line 2198 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 50:
#line 440 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-2].ival) * (yyvsp[0].ival);
	}
#line 2206 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 51:
#line 443 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = ! (yyvsp[0].ival);
	}
#line 2214 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 52:
#line 446 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = ~ (yyvsp[0].ival);
	}
#line 2222 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 53:
#line 449 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = - (yyvsp[0].ival);
	}
#line 2230 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 54:
#line 452 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = + (yyvsp[0].ival);
	}
#line 2238 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 55:
#line 455 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.ival) = (yyvsp[-1].ival);
	}
#line 2246 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 56:
#line 461 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.string_list) = _string_list_create (parser);
		_string_list_append_item ((yyval.string_list), (yyvsp[0].str));
		ralloc_steal ((yyval.string_list), (yyvsp[0].str));
	}
#line 2256 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 57:
#line 466 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.string_list) = (yyvsp[-2].string_list);	
		_string_list_append_item ((yyval.string_list), (yyvsp[0].str));
		ralloc_steal ((yyval.string_list), (yyvsp[0].str));
	}
#line 2266 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 58:
#line 474 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.token_list) = NULL; }
#line 2272 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 60:
#line 479 "glcpp-parse.y" /* yacc.c:1646  */
    {
		yyerror (& (yylsp[-1]), parser, "Invalid tokens after #");
	}
#line 2280 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 61:
#line 485 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.token_list) = NULL; }
#line 2286 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 64:
#line 491 "glcpp-parse.y" /* yacc.c:1646  */
    {
		glcpp_warning(&(yylsp[0]), parser, "extra tokens at end of directive");
	}
#line 2294 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 65:
#line 498 "glcpp-parse.y" /* yacc.c:1646  */
    {
		int v = hash_table_find (parser->defines, (yyvsp[0].str)) ? 1 : 0;
		(yyval.token) = _token_create_ival (parser, INTEGER, v);
	}
#line 2303 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 66:
#line 502 "glcpp-parse.y" /* yacc.c:1646  */
    {
		int v = hash_table_find (parser->defines, (yyvsp[-1].str)) ? 1 : 0;
		(yyval.token) = _token_create_ival (parser, INTEGER, v);
	}
#line 2312 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 68:
#line 511 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token_list) = _token_list_create (parser);
		_token_list_append ((yyval.token_list), (yyvsp[0].token));
	}
#line 2321 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 69:
#line 515 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token_list) = (yyvsp[-1].token_list);
		_token_list_append ((yyval.token_list), (yyvsp[0].token));
	}
#line 2330 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 70:
#line 522 "glcpp-parse.y" /* yacc.c:1646  */
    {
		parser->space_tokens = 1;
		(yyval.token_list) = _token_list_create (parser);
		_token_list_append ((yyval.token_list), (yyvsp[0].token));
	}
#line 2340 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 71:
#line 527 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token_list) = (yyvsp[-1].token_list);
		_token_list_append ((yyval.token_list), (yyvsp[0].token));
	}
#line 2349 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 72:
#line 534 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token) = _token_create_str (parser, IDENTIFIER, (yyvsp[0].str));
		(yyval.token)->location = yylloc;
	}
#line 2358 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 73:
#line 538 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token) = _token_create_str (parser, INTEGER_STRING, (yyvsp[0].str));
		(yyval.token)->location = yylloc;
	}
#line 2367 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 74:
#line 542 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token) = _token_create_ival (parser, (yyvsp[0].ival), (yyvsp[0].ival));
		(yyval.token)->location = yylloc;
	}
#line 2376 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 75:
#line 546 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token) = _token_create_str (parser, OTHER, (yyvsp[0].str));
		(yyval.token)->location = yylloc;
	}
#line 2385 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 76:
#line 550 "glcpp-parse.y" /* yacc.c:1646  */
    {
		(yyval.token) = _token_create_ival (parser, SPACE, SPACE);
		(yyval.token)->location = yylloc;
	}
#line 2394 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 77:
#line 557 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '['; }
#line 2400 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 78:
#line 558 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = ']'; }
#line 2406 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 79:
#line 559 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '('; }
#line 2412 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 80:
#line 560 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = ')'; }
#line 2418 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 81:
#line 561 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '{'; }
#line 2424 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 82:
#line 562 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '}'; }
#line 2430 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 83:
#line 563 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '.'; }
#line 2436 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 84:
#line 564 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '&'; }
#line 2442 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 85:
#line 565 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '*'; }
#line 2448 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 86:
#line 566 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '+'; }
#line 2454 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 87:
#line 567 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '-'; }
#line 2460 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 88:
#line 568 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '~'; }
#line 2466 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 89:
#line 569 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '!'; }
#line 2472 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 90:
#line 570 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '/'; }
#line 2478 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 91:
#line 571 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '%'; }
#line 2484 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 92:
#line 572 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = LEFT_SHIFT; }
#line 2490 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 93:
#line 573 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = RIGHT_SHIFT; }
#line 2496 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 94:
#line 574 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '<'; }
#line 2502 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 95:
#line 575 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '>'; }
#line 2508 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 96:
#line 576 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = LESS_OR_EQUAL; }
#line 2514 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 97:
#line 577 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = GREATER_OR_EQUAL; }
#line 2520 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 98:
#line 578 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = EQUAL; }
#line 2526 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 99:
#line 579 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = NOT_EQUAL; }
#line 2532 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 100:
#line 580 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '^'; }
#line 2538 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 101:
#line 581 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '|'; }
#line 2544 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 102:
#line 582 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = AND; }
#line 2550 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 103:
#line 583 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = OR; }
#line 2556 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 104:
#line 584 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = ';'; }
#line 2562 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 105:
#line 585 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = ','; }
#line 2568 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 106:
#line 586 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = '='; }
#line 2574 "glcpp-parse.c" /* yacc.c:1646  */
    break;

  case 107:
#line 587 "glcpp-parse.y" /* yacc.c:1646  */
    { (yyval.ival) = PASTE; }
#line 2580 "glcpp-parse.c" /* yacc.c:1646  */
    break;


#line 2584 "glcpp-parse.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, parser, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (&yylloc, parser, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, parser);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[1] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, parser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, parser);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, parser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 590 "glcpp-parse.y" /* yacc.c:1906  */


string_list_t *
_string_list_create (void *ctx)
{
	string_list_t *list;

	list = ralloc (ctx, string_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_string_list_append_item (string_list_t *list, const char *str)
{
	string_node_t *node;

	node = ralloc (list, string_node_t);
	node->str = ralloc_strdup (node, str);

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_string_list_contains (string_list_t *list, const char *member, int *index)
{
	string_node_t *node;
	int i;

	if (list == NULL)
		return 0;

	for (i = 0, node = list->head; node; i++, node = node->next) {
		if (strcmp (node->str, member) == 0) {
			if (index)
				*index = i;
			return 1;
		}
	}

	return 0;
}

int
_string_list_length (string_list_t *list)
{
	int length = 0;
	string_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

int
_string_list_equal (string_list_t *a, string_list_t *b)
{
	string_node_t *node_a, *node_b;

	if (a == NULL && b == NULL)
		return 1;

	if (a == NULL || b == NULL)
		return 0;

	for (node_a = a->head, node_b = b->head;
	     node_a && node_b;
	     node_a = node_a->next, node_b = node_b->next)
	{
		if (strcmp (node_a->str, node_b->str))
			return 0;
	}

	/* Catch the case of lists being different lengths, (which
	 * would cause the loop above to terminate after the shorter
	 * list). */
	return node_a == node_b;
}

argument_list_t *
_argument_list_create (void *ctx)
{
	argument_list_t *list;

	list = ralloc (ctx, argument_list_t);
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void
_argument_list_append (argument_list_t *list, token_list_t *argument)
{
	argument_node_t *node;

	node = ralloc (list, argument_node_t);
	node->argument = argument;

	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
}

int
_argument_list_length (argument_list_t *list)
{
	int length = 0;
	argument_node_t *node;

	if (list == NULL)
		return 0;

	for (node = list->head; node; node = node->next)
		length++;

	return length;
}

token_list_t *
_argument_list_member_at (argument_list_t *list, int index)
{
	argument_node_t *node;
	int i;

	if (list == NULL)
		return NULL;

	node = list->head;
	for (i = 0; i < index; i++) {
		node = node->next;
		if (node == NULL)
			break;
	}

	if (node)
		return node->argument;

	return NULL;
}

/* Note: This function ralloc_steal()s the str pointer. */
token_t *
_token_create_str (void *ctx, int type, char *str)
{
	token_t *token;

	token = ralloc (ctx, token_t);
	token->type = type;
	token->value.str = str;

	ralloc_steal (token, str);

	return token;
}

token_t *
_token_create_ival (void *ctx, int type, int ival)
{
	token_t *token;

	token = ralloc (ctx, token_t);
	token->type = type;
	token->value.ival = ival;

	return token;
}

token_list_t *
_token_list_create (void *ctx)
{
	token_list_t *list;

	list = ralloc (ctx, token_list_t);
	list->head = NULL;
	list->tail = NULL;
	list->non_space_tail = NULL;

	return list;
}

void
_token_list_append (token_list_t *list, token_t *token)
{
	token_node_t *node;

	node = ralloc (list, token_node_t);
	node->token = token;
	node->next = NULL;

	if (list->head == NULL) {
		list->head = node;
	} else {
		list->tail->next = node;
	}

	list->tail = node;
	if (token->type != SPACE)
		list->non_space_tail = node;
}

void
_token_list_append_list (token_list_t *list, token_list_t *tail)
{
	if (tail == NULL || tail->head == NULL)
		return;

	if (list->head == NULL) {
		list->head = tail->head;
	} else {
		list->tail->next = tail->head;
	}

	list->tail = tail->tail;
	list->non_space_tail = tail->non_space_tail;
}

static token_list_t *
_token_list_copy (void *ctx, token_list_t *other)
{
	token_list_t *copy;
	token_node_t *node;

	if (other == NULL)
		return NULL;

	copy = _token_list_create (ctx);
	for (node = other->head; node; node = node->next) {
		token_t *new_token = ralloc (copy, token_t);
		*new_token = *node->token;
		_token_list_append (copy, new_token);
	}

	return copy;
}

static void
_token_list_trim_trailing_space (token_list_t *list)
{
	token_node_t *tail, *next;

	if (list->non_space_tail) {
		tail = list->non_space_tail->next;
		list->non_space_tail->next = NULL;
		list->tail = list->non_space_tail;

		while (tail) {
			next = tail->next;
			ralloc_free (tail);
			tail = next;
		}
	}
}

static int
_token_list_is_empty_ignoring_space (token_list_t *l)
{
	token_node_t *n;

	if (l == NULL)
		return 1;

	n = l->head;
	while (n != NULL && n->token->type == SPACE)
		n = n->next;

	return n == NULL;
}

int
_token_list_equal_ignoring_space (token_list_t *a, token_list_t *b)
{
	token_node_t *node_a, *node_b;

	if (a == NULL || b == NULL) {
		int a_empty = _token_list_is_empty_ignoring_space(a);
		int b_empty = _token_list_is_empty_ignoring_space(b);
		return a_empty == b_empty;
	}

	node_a = a->head;
	node_b = b->head;

	while (1)
	{
		if (node_a == NULL && node_b == NULL)
			break;

		if (node_a == NULL || node_b == NULL)
			return 0;

		if (node_a->token->type == SPACE) {
			node_a = node_a->next;
			continue;
		}

		if (node_b->token->type == SPACE) {
			node_b = node_b->next;
			continue;
		}

		if (node_a->token->type != node_b->token->type)
			return 0;

		switch (node_a->token->type) {
		case INTEGER:
			if (node_a->token->value.ival != 
			    node_b->token->value.ival)
			{
				return 0;
			}
			break;
		case IDENTIFIER:
		case INTEGER_STRING:
		case OTHER:
			if (strcmp (node_a->token->value.str,
				    node_b->token->value.str))
			{
				return 0;
			}
			break;
		}

		node_a = node_a->next;
		node_b = node_b->next;
	}

	return 1;
}

static void
_token_print (char **out, size_t *len, token_t *token)
{
	if (token->type < 256) {
		ralloc_asprintf_rewrite_tail (out, len, "%c", token->type);
		return;
	}

	switch (token->type) {
	case INTEGER:
		ralloc_asprintf_rewrite_tail (out, len, "%" PRIiMAX, token->value.ival);
		break;
	case IDENTIFIER:
	case INTEGER_STRING:
	case OTHER:
		ralloc_asprintf_rewrite_tail (out, len, "%s", token->value.str);
		break;
	case SPACE:
		ralloc_asprintf_rewrite_tail (out, len, " ");
		break;
	case LEFT_SHIFT:
		ralloc_asprintf_rewrite_tail (out, len, "<<");
		break;
	case RIGHT_SHIFT:
		ralloc_asprintf_rewrite_tail (out, len, ">>");
		break;
	case LESS_OR_EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, "<=");
		break;
	case GREATER_OR_EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, ">=");
		break;
	case EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, "==");
		break;
	case NOT_EQUAL:
		ralloc_asprintf_rewrite_tail (out, len, "!=");
		break;
	case AND:
		ralloc_asprintf_rewrite_tail (out, len, "&&");
		break;
	case OR:
		ralloc_asprintf_rewrite_tail (out, len, "||");
		break;
	case PASTE:
		ralloc_asprintf_rewrite_tail (out, len, "##");
		break;
	case COMMA_FINAL:
		ralloc_asprintf_rewrite_tail (out, len, ",");
		break;
	case PLACEHOLDER:
		/* Nothing to print. */
		break;
	default:
		assert(!"Error: Don't know how to print token.");
		break;
	}
}

/* Return a new token (ralloc()ed off of 'token') formed by pasting
 * 'token' and 'other'. Note that this function may return 'token' or
 * 'other' directly rather than allocating anything new.
 *
 * Caution: Only very cursory error-checking is performed to see if
 * the final result is a valid single token. */
static token_t *
_token_paste (glcpp_parser_t *parser, token_t *token, token_t *other)
{
	token_t *combined = NULL;

	/* Pasting a placeholder onto anything makes no change. */
	if (other->type == PLACEHOLDER)
		return token;

	/* When 'token' is a placeholder, just return 'other'. */
	if (token->type == PLACEHOLDER)
		return other;

	/* A very few single-character punctuators can be combined
	 * with another to form a multi-character punctuator. */
	switch (token->type) {
	case '<':
		if (other->type == '<')
			combined = _token_create_ival (token, LEFT_SHIFT, LEFT_SHIFT);
		else if (other->type == '=')
			combined = _token_create_ival (token, LESS_OR_EQUAL, LESS_OR_EQUAL);
		break;
	case '>':
		if (other->type == '>')
			combined = _token_create_ival (token, RIGHT_SHIFT, RIGHT_SHIFT);
		else if (other->type == '=')
			combined = _token_create_ival (token, GREATER_OR_EQUAL, GREATER_OR_EQUAL);
		break;
	case '=':
		if (other->type == '=')
			combined = _token_create_ival (token, EQUAL, EQUAL);
		break;
	case '!':
		if (other->type == '=')
			combined = _token_create_ival (token, NOT_EQUAL, NOT_EQUAL);
		break;
	case '&':
		if (other->type == '&')
			combined = _token_create_ival (token, AND, AND);
		break;
	case '|':
		if (other->type == '|')
			combined = _token_create_ival (token, OR, OR);
		break;
	}

	if (combined != NULL) {
		/* Inherit the location from the first token */
		combined->location = token->location;
		return combined;
	}

	/* Two string-valued tokens can usually just be mashed
	 * together.
	 *
	 * XXX: This isn't actually legitimate. Several things here
	 * should result in a diagnostic since the result cannot be a
	 * valid, single pre-processing token. For example, pasting
	 * "123" and "abc" is not legal, but we don't catch that
	 * here. */
	if ((token->type == IDENTIFIER || token->type == OTHER || token->type == INTEGER_STRING) &&
	    (other->type == IDENTIFIER || other->type == OTHER || other->type == INTEGER_STRING))
	{
		char *str;

		str = ralloc_asprintf (token, "%s%s", token->value.str,
				       other->value.str);
		combined = _token_create_str (token, token->type, str);
		combined->location = token->location;
		return combined;
	}

	glcpp_error (&token->location, parser, "");
	ralloc_asprintf_rewrite_tail (&parser->info_log, &parser->info_log_length, "Pasting \"");
	_token_print (&parser->info_log, &parser->info_log_length, token);
	ralloc_asprintf_rewrite_tail (&parser->info_log, &parser->info_log_length, "\" and \"");
	_token_print (&parser->info_log, &parser->info_log_length, other);
	ralloc_asprintf_rewrite_tail (&parser->info_log, &parser->info_log_length, "\" does not give a valid preprocessing token.\n");

	return token;
}

static void
_token_list_print (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	if (list == NULL)
		return;

	for (node = list->head; node; node = node->next)
		_token_print (&parser->output, &parser->output_length, node->token);
}

void
yyerror (YYLTYPE *locp, glcpp_parser_t *parser, const char *error)
{
	glcpp_error(locp, parser, "%s", error);
}

static void add_builtin_define(glcpp_parser_t *parser,
			       const char *name, int value)
{
   token_t *tok;
   token_list_t *list;

   tok = _token_create_ival (parser, INTEGER, value);

   list = _token_list_create(parser);
   _token_list_append(list, tok);
   _define_object_macro(parser, NULL, name, list);
}

glcpp_parser_t *
glcpp_parser_create (const struct gl_extensions *extensions, int api)
{
	glcpp_parser_t *parser;
	int language_version;

	parser = ralloc (NULL, glcpp_parser_t);

	glcpp_lex_init_extra (parser, &parser->scanner);
	parser->defines = hash_table_ctor (32, hash_table_string_hash,
					   hash_table_string_compare);
	parser->active = NULL;
	parser->lexing_if = 0;
	parser->space_tokens = 1;
	parser->newline_as_space = 0;
	parser->in_control_line = 0;
	parser->paren_count = 0;

	parser->skip_stack = NULL;

	parser->lex_from_list = NULL;
	parser->lex_from_node = NULL;

	parser->output = ralloc_strdup(parser, "");
	parser->output_length = 0;
	parser->info_log = ralloc_strdup(parser, "");
	parser->info_log_length = 0;
	parser->error = 0;

	parser->has_new_line_number = 0;
	parser->new_line_number = 1;
	parser->has_new_source_number = 0;
	parser->new_source_number = 0;

	/* Add pre-defined macros. */
	add_builtin_define(parser, "GL_ARB_draw_buffers", 1);
	add_builtin_define(parser, "GL_ARB_texture_rectangle", 1);

	if (api == API_OPENGLES2)
		add_builtin_define(parser, "GL_ES", 1);

	if (extensions != NULL) {
	   if (extensions->EXT_texture_array) {
	      add_builtin_define(parser, "GL_EXT_texture_array", 1);
	   }

	   if (extensions->ARB_fragment_coord_conventions)
	      add_builtin_define(parser, "GL_ARB_fragment_coord_conventions",
				 1);

	   if (extensions->ARB_explicit_attrib_location)
	      add_builtin_define(parser, "GL_ARB_explicit_attrib_location", 1);

	   if (extensions->ARB_shader_texture_lod)
	      add_builtin_define(parser, "GL_ARB_shader_texture_lod", 1);

	   if (extensions->ARB_draw_instanced)
	      add_builtin_define(parser, "GL_ARB_draw_instanced", 1);

	   if (extensions->ARB_conservative_depth) {
	      add_builtin_define(parser, "GL_AMD_conservative_depth", 1);
	      add_builtin_define(parser, "GL_ARB_conservative_depth", 1);
	   }

	   if (extensions->OES_EGL_image_external)
	      add_builtin_define(parser, "GL_OES_EGL_image_external", 1);

	   if (extensions->ARB_shader_bit_encoding)
	      add_builtin_define(parser, "GL_ARB_shader_bit_encoding", 1);

	   if (extensions->ARB_uniform_buffer_object)
	      add_builtin_define(parser, "GL_ARB_uniform_buffer_object", 1);
	}

	language_version = 110;
	add_builtin_define(parser, "__VERSION__", language_version);

	return parser;
}

void
glcpp_parser_destroy (glcpp_parser_t *parser)
{
	glcpp_lex_destroy (parser->scanner);
	hash_table_dtor (parser->defines);
	ralloc_free (parser);
}

typedef enum function_status
{
	FUNCTION_STATUS_SUCCESS,
	FUNCTION_NOT_A_FUNCTION,
	FUNCTION_UNBALANCED_PARENTHESES
} function_status_t;

/* Find a set of function-like macro arguments by looking for a
 * balanced set of parentheses.
 *
 * When called, 'node' should be the opening-parenthesis token, (or
 * perhaps preceeding SPACE tokens). Upon successful return *last will
 * be the last consumed node, (corresponding to the closing right
 * parenthesis).
 *
 * Return values:
 *
 *   FUNCTION_STATUS_SUCCESS:
 *
 *	Successfully parsed a set of function arguments.	
 *
 *   FUNCTION_NOT_A_FUNCTION:
 *
 *	Macro name not followed by a '('. This is not an error, but
 *	simply that the macro name should be treated as a non-macro.
 *
 *   FUNCTION_UNBALANCED_PARENTHESES
 *
 *	Macro name is not followed by a balanced set of parentheses.
 */
static function_status_t
_arguments_parse (argument_list_t *arguments,
		  token_node_t *node,
		  token_node_t **last)
{
	token_list_t *argument;
	int paren_count;

	node = node->next;

	/* Ignore whitespace before first parenthesis. */
	while (node && node->token->type == SPACE)
		node = node->next;

	if (node == NULL || node->token->type != '(')
		return FUNCTION_NOT_A_FUNCTION;

	node = node->next;

	argument = _token_list_create (arguments);
	_argument_list_append (arguments, argument);

	for (paren_count = 1; node; node = node->next) {
		if (node->token->type == '(')
		{
			paren_count++;
		}
		else if (node->token->type == ')')
		{
			paren_count--;
			if (paren_count == 0)
				break;
		}

		if (node->token->type == ',' &&
			 paren_count == 1)
		{
			_token_list_trim_trailing_space (argument);
			argument = _token_list_create (arguments);
			_argument_list_append (arguments, argument);
		}
		else {
			if (argument->head == NULL) {
				/* Don't treat initial whitespace as
				 * part of the arguement. */
				if (node->token->type == SPACE)
					continue;
			}
			_token_list_append (argument, node->token);
		}
	}

	if (paren_count)
		return FUNCTION_UNBALANCED_PARENTHESES;

	*last = node;

	return FUNCTION_STATUS_SUCCESS;
}

static token_list_t *
_token_list_create_with_one_space (void *ctx)
{
	token_list_t *list;
	token_t *space;

	list = _token_list_create (ctx);
	space = _token_create_ival (list, SPACE, SPACE);
	_token_list_append (list, space);

	return list;
}

/* Perform macro expansion on 'list', placing the resulting tokens
 * into a new list which is initialized with a first token of type
 * 'head_token_type'. Then begin lexing from the resulting list,
 * (return to the current lexing source when this list is exhausted).
 */
static void
_glcpp_parser_expand_and_lex_from (glcpp_parser_t *parser,
				   int head_token_type,
				   token_list_t *list)
{
	token_list_t *expanded;
	token_t *token;

	expanded = _token_list_create (parser);
	token = _token_create_ival (parser, head_token_type, head_token_type);
	_token_list_append (expanded, token);
	_glcpp_parser_expand_token_list (parser, list);
	_token_list_append_list (expanded, list);
	glcpp_parser_lex_from (parser, expanded);
}

static void
_glcpp_parser_apply_pastes (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	node = list->head;
	while (node)
	{
		token_node_t *next_non_space;

		/* Look ahead for a PASTE token, skipping space. */
		next_non_space = node->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL)
			break;

		if (next_non_space->token->type != PASTE) {
			node = next_non_space;
			continue;
		}

		/* Now find the next non-space token after the PASTE. */
		next_non_space = next_non_space->next;
		while (next_non_space && next_non_space->token->type == SPACE)
			next_non_space = next_non_space->next;

		if (next_non_space == NULL) {
			yyerror (&node->token->location, parser, "'##' cannot appear at either end of a macro expansion\n");
			return;
		}

		node->token = _token_paste (parser, node->token, next_non_space->token);
		node->next = next_non_space->next;
		if (next_non_space == list->tail)
			list->tail = node;
	}

	list->non_space_tail = list->tail;
}

/* This is a helper function that's essentially part of the
 * implementation of _glcpp_parser_expand_node. It shouldn't be called
 * except for by that function.
 *
 * Returns NULL if node is a simple token with no expansion, (that is,
 * although 'node' corresponds to an identifier defined as a
 * function-like macro, it is not followed with a parenthesized
 * argument list).
 *
 * Compute the complete expansion of node (which is a function-like
 * macro) and subsequent nodes which are arguments.
 *
 * Returns the token list that results from the expansion and sets
 * *last to the last node in the list that was consumed by the
 * expansion. Specifically, *last will be set as follows: as the
 * token of the closing right parenthesis.
 */
static token_list_t *
_glcpp_parser_expand_function (glcpp_parser_t *parser,
			       token_node_t *node,
			       token_node_t **last)
			       
{
	macro_t *macro;
	const char *identifier;
	argument_list_t *arguments;
	function_status_t status;
	token_list_t *substituted;
	int parameter_index;

	identifier = node->token->value.str;

	macro = hash_table_find (parser->defines, identifier);

	assert (macro->is_function);

	arguments = _argument_list_create (parser);
	status = _arguments_parse (arguments, node, last);

	switch (status) {
	case FUNCTION_STATUS_SUCCESS:
		break;
	case FUNCTION_NOT_A_FUNCTION:
		return NULL;
	case FUNCTION_UNBALANCED_PARENTHESES:
		glcpp_error (&node->token->location, parser, "Macro %s call has unbalanced parentheses\n", identifier);
		return NULL;
	}

	/* Replace a macro defined as empty with a SPACE token. */
	if (macro->replacements == NULL) {
		ralloc_free (arguments);
		return _token_list_create_with_one_space (parser);
	}

	if (! ((_argument_list_length (arguments) == 
		_string_list_length (macro->parameters)) ||
	       (_string_list_length (macro->parameters) == 0 &&
		_argument_list_length (arguments) == 1 &&
		arguments->head->argument->head == NULL)))
	{
		glcpp_error (&node->token->location, parser,
			      "Error: macro %s invoked with %d arguments (expected %d)\n",
			      identifier,
			      _argument_list_length (arguments),
			      _string_list_length (macro->parameters));
		return NULL;
	}

	/* Perform argument substitution on the replacement list. */
	substituted = _token_list_create (arguments);

	for (node = macro->replacements->head; node; node = node->next)
	{
		if (node->token->type == IDENTIFIER &&
		    _string_list_contains (macro->parameters,
					   node->token->value.str,
					   &parameter_index))
		{
			token_list_t *argument;
			argument = _argument_list_member_at (arguments,
							     parameter_index);
			/* Before substituting, we expand the argument
			 * tokens, or append a placeholder token for
			 * an empty argument. */
			if (argument->head) {
				token_list_t *expanded_argument;
				expanded_argument = _token_list_copy (parser,
								      argument);
				_glcpp_parser_expand_token_list (parser,
								 expanded_argument);
				_token_list_append_list (substituted,
							 expanded_argument);
			} else {
				token_t *new_token;

				new_token = _token_create_ival (substituted,
								PLACEHOLDER,
								PLACEHOLDER);
				_token_list_append (substituted, new_token);
			}
		} else {
			_token_list_append (substituted, node->token);
		}
	}

	/* After argument substitution, and before further expansion
	 * below, implement token pasting. */

	_token_list_trim_trailing_space (substituted);

	_glcpp_parser_apply_pastes (parser, substituted);

	return substituted;
}

/* Compute the complete expansion of node, (and subsequent nodes after
 * 'node' in the case that 'node' is a function-like macro and
 * subsequent nodes are arguments).
 *
 * Returns NULL if node is a simple token with no expansion.
 *
 * Otherwise, returns the token list that results from the expansion
 * and sets *last to the last node in the list that was consumed by
 * the expansion. Specifically, *last will be set as follows:
 *
 *	As 'node' in the case of object-like macro expansion.
 *
 *	As the token of the closing right parenthesis in the case of
 *	function-like macro expansion.
 */
static token_list_t *
_glcpp_parser_expand_node (glcpp_parser_t *parser,
			   token_node_t *node,
			   token_node_t **last)
{
	token_t *token = node->token;
	const char *identifier;
	macro_t *macro;

	/* We only expand identifiers */
	if (token->type != IDENTIFIER) {
		/* We change any COMMA into a COMMA_FINAL to prevent
		 * it being mistaken for an argument separator
		 * later. */
		if (token->type == ',') {
			token->type = COMMA_FINAL;
			token->value.ival = COMMA_FINAL;
		}

		return NULL;
	}

	/* Look up this identifier in the hash table. */
	identifier = token->value.str;
	macro = hash_table_find (parser->defines, identifier);

	/* Not a macro, so no expansion needed. */
	if (macro == NULL)
		return NULL;

	/* Finally, don't expand this macro if we're already actively
	 * expanding it, (to avoid infinite recursion). */
	if (_parser_active_list_contains (parser, identifier)) {
		/* We change the token type here from IDENTIFIER to
		 * OTHER to prevent any future expansion of this
		 * unexpanded token. */
		char *str;
		token_list_t *expansion;
		token_t *final;

		str = ralloc_strdup (parser, token->value.str);
		final = _token_create_str (parser, OTHER, str);
		expansion = _token_list_create (parser);
		_token_list_append (expansion, final);
		*last = node;
		return expansion;
	}

	if (! macro->is_function)
	{
		token_list_t *replacement;
		*last = node;

		/* Replace a macro defined as empty with a SPACE token. */
		if (macro->replacements == NULL)
			return _token_list_create_with_one_space (parser);

		replacement = _token_list_copy (parser, macro->replacements);
		_glcpp_parser_apply_pastes (parser, replacement);
		return replacement;
	}

	return _glcpp_parser_expand_function (parser, node, last);
}

/* Push a new identifier onto the parser's active list.
 *
 * Here, 'marker' is the token node that appears in the list after the
 * expansion of 'identifier'. That is, when the list iterator begins
 * examining 'marker', then it is time to pop this node from the
 * active stack.
 */
static void
_parser_active_list_push (glcpp_parser_t *parser,
			  const char *identifier,
			  token_node_t *marker)
{
	active_list_t *node;

	node = ralloc (parser->active, active_list_t);
	node->identifier = ralloc_strdup (node, identifier);
	node->marker = marker;
	node->next = parser->active;

	parser->active = node;
}

static void
_parser_active_list_pop (glcpp_parser_t *parser)
{
	active_list_t *node = parser->active;

	if (node == NULL) {
		parser->active = NULL;
		return;
	}

	node = parser->active->next;
	ralloc_free (parser->active);

	parser->active = node;
}

static int
_parser_active_list_contains (glcpp_parser_t *parser, const char *identifier)
{
	active_list_t *node;

	if (parser->active == NULL)
		return 0;

	for (node = parser->active; node; node = node->next)
		if (strcmp (node->identifier, identifier) == 0)
			return 1;

	return 0;
}

/* Walk over the token list replacing nodes with their expansion.
 * Whenever nodes are expanded the walking will walk over the new
 * nodes, continuing to expand as necessary. The results are placed in
 * 'list' itself;
 */
static void
_glcpp_parser_expand_token_list (glcpp_parser_t *parser,
				 token_list_t *list)
{
	token_node_t *node_prev;
	token_node_t *node, *last = NULL;
	token_list_t *expansion;
	active_list_t *active_initial = parser->active;

	if (list == NULL)
		return;

	_token_list_trim_trailing_space (list);

	node_prev = NULL;
	node = list->head;

	while (node) {

		while (parser->active && parser->active->marker == node)
			_parser_active_list_pop (parser);

		expansion = _glcpp_parser_expand_node (parser, node, &last);
		if (expansion) {
			token_node_t *n;

			for (n = node; n != last->next; n = n->next)
				while (parser->active &&
				       parser->active->marker == n)
				{
					_parser_active_list_pop (parser);
				}

			_parser_active_list_push (parser,
						  node->token->value.str,
						  last->next);
			
			/* Splice expansion into list, supporting a
			 * simple deletion if the expansion is
			 * empty. */
			if (expansion->head) {
				if (node_prev)
					node_prev->next = expansion->head;
				else
					list->head = expansion->head;
				expansion->tail->next = last->next;
				if (last == list->tail)
					list->tail = expansion->tail;
			} else {
				if (node_prev)
					node_prev->next = last->next;
				else
					list->head = last->next;
				if (last == list->tail)
					list->tail = NULL;
			}
		} else {
			node_prev = node;
		}
		node = node_prev ? node_prev->next : list->head;
	}

	/* Remove any lingering effects of this invocation on the
	 * active list. That is, pop until the list looks like it did
	 * at the beginning of this function. */
	while (parser->active && parser->active != active_initial)
		_parser_active_list_pop (parser);

	list->non_space_tail = list->tail;
}

void
_glcpp_parser_print_expanded_token_list (glcpp_parser_t *parser,
					 token_list_t *list)
{
	if (list == NULL)
		return;

	_glcpp_parser_expand_token_list (parser, list);

	_token_list_trim_trailing_space (list);

	_token_list_print (parser, list);
}

static void
_check_for_reserved_macro_name (glcpp_parser_t *parser, YYLTYPE *loc,
				const char *identifier)
{
	/* According to the GLSL specification, macro names starting with "__"
	 * or "GL_" are reserved for future use.  So, don't allow them.
	 */
	if (strstr(identifier, "__")) {
		glcpp_error (loc, parser, "Macro names containing \"__\" are reserved.\n");
	}
	if (strncmp(identifier, "GL_", 3) == 0) {
		glcpp_error (loc, parser, "Macro names starting with \"GL_\" are reserved.\n");
	}
}

static int
_macro_equal (macro_t *a, macro_t *b)
{
	if (a->is_function != b->is_function)
		return 0;

	if (a->is_function) {
		if (! _string_list_equal (a->parameters, b->parameters))
			return 0;
	}

	return _token_list_equal_ignoring_space (a->replacements,
						 b->replacements);
}

void
_define_object_macro (glcpp_parser_t *parser,
		      YYLTYPE *loc,
		      const char *identifier,
		      token_list_t *replacements)
{
	macro_t *macro, *previous;

	if (loc != NULL)
		_check_for_reserved_macro_name(parser, loc, identifier);

	macro = ralloc (parser, macro_t);

	macro->is_function = 0;
	macro->parameters = NULL;
	macro->identifier = ralloc_strdup (macro, identifier);
	macro->replacements = replacements;
	ralloc_steal (macro, replacements);

	previous = hash_table_find (parser->defines, identifier);
	if (previous) {
		if (_macro_equal (macro, previous)) {
			ralloc_free (macro);
			return;
		}
		glcpp_error (loc, parser, "Redefinition of macro %s\n",
			     identifier);
	}

	hash_table_insert (parser->defines, macro, identifier);
}

void
_define_function_macro (glcpp_parser_t *parser,
			YYLTYPE *loc,
			const char *identifier,
			string_list_t *parameters,
			token_list_t *replacements)
{
	macro_t *macro, *previous;

	_check_for_reserved_macro_name(parser, loc, identifier);

	macro = ralloc (parser, macro_t);
	ralloc_steal (macro, parameters);
	ralloc_steal (macro, replacements);

	macro->is_function = 1;
	macro->parameters = parameters;
	macro->identifier = ralloc_strdup (macro, identifier);
	macro->replacements = replacements;
	previous = hash_table_find (parser->defines, identifier);
	if (previous) {
		if (_macro_equal (macro, previous)) {
			ralloc_free (macro);
			return;
		}
		glcpp_error (loc, parser, "Redefinition of macro %s\n",
			     identifier);
	}

	hash_table_insert (parser->defines, macro, identifier);
}

static int
glcpp_parser_lex (YYSTYPE *yylval, YYLTYPE *yylloc, glcpp_parser_t *parser)
{
	token_node_t *node;
	int ret;

	if (parser->lex_from_list == NULL) {
		ret = glcpp_lex (yylval, yylloc, parser->scanner);

		/* XXX: This ugly block of code exists for the sole
		 * purpose of converting a NEWLINE token into a SPACE
		 * token, but only in the case where we have seen a
		 * function-like macro name, but have not yet seen its
		 * closing parenthesis.
		 *
		 * There's perhaps a more compact way to do this with
		 * mid-rule actions in the grammar.
		 *
		 * I'm definitely not pleased with the complexity of
		 * this code here.
		 */
		if (parser->newline_as_space)
		{
			if (ret == '(') {
				parser->paren_count++;
			} else if (ret == ')') {
				parser->paren_count--;
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			} else if (ret == NEWLINE) {
				ret = SPACE;
			} else if (ret != SPACE) {
				if (parser->paren_count == 0)
					parser->newline_as_space = 0;
			}
		}
		else if (parser->in_control_line)
		{
			if (ret == NEWLINE)
				parser->in_control_line = 0;
		}
		else if (ret == HASH_DEFINE ||
			   ret == HASH_UNDEF || ret == HASH_IF ||
			   ret == HASH_IFDEF || ret == HASH_IFNDEF ||
			   ret == HASH_ELIF || ret == HASH_ELSE ||
			   ret == HASH_ENDIF || ret == HASH)
		{
			parser->in_control_line = 1;
		}
		else if (ret == IDENTIFIER)
		{
			macro_t *macro;
			macro = hash_table_find (parser->defines,
						 yylval->str);
			if (macro && macro->is_function) {
				parser->newline_as_space = 1;
				parser->paren_count = 0;
			}
		}

		return ret;
	}

	node = parser->lex_from_node;

	if (node == NULL) {
		ralloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
		return NEWLINE;
	}

	*yylval = node->token->value;
	ret = node->token->type;

	parser->lex_from_node = node->next;

	return ret;
}

static void
glcpp_parser_lex_from (glcpp_parser_t *parser, token_list_t *list)
{
	token_node_t *node;

	assert (parser->lex_from_list == NULL);

	/* Copy list, eliminating any space tokens. */
	parser->lex_from_list = _token_list_create (parser);

	for (node = list->head; node; node = node->next) {
		if (node->token->type == SPACE)
			continue;
		_token_list_append (parser->lex_from_list, node->token);
	}

	ralloc_free (list);

	parser->lex_from_node = parser->lex_from_list->head;

	/* It's possible the list consisted of nothing but whitespace. */
	if (parser->lex_from_node == NULL) {
		ralloc_free (parser->lex_from_list);
		parser->lex_from_list = NULL;
	}
}

static void
_glcpp_parser_skip_stack_push_if (glcpp_parser_t *parser, YYLTYPE *loc,
				  int condition)
{
	skip_type_t current = SKIP_NO_SKIP;
	skip_node_t *node;

	if (parser->skip_stack)
		current = parser->skip_stack->type;

	node = ralloc (parser, skip_node_t);
	node->loc = *loc;

	if (current == SKIP_NO_SKIP) {
		if (condition)
			node->type = SKIP_NO_SKIP;
		else
			node->type = SKIP_TO_ELSE;
	} else {
		node->type = SKIP_TO_ENDIF;
	}

	node->next = parser->skip_stack;
	parser->skip_stack = node;
}

static void
_glcpp_parser_skip_stack_change_if (glcpp_parser_t *parser, YYLTYPE *loc,
				    const char *type, int condition)
{
	if (parser->skip_stack == NULL) {
		glcpp_error (loc, parser, "%s without #if\n", type);
		return;
	}

	if (parser->skip_stack->type == SKIP_TO_ELSE) {
		if (condition)
			parser->skip_stack->type = SKIP_NO_SKIP;
	} else {
		parser->skip_stack->type = SKIP_TO_ENDIF;
	}
}

static void
_glcpp_parser_skip_stack_pop (glcpp_parser_t *parser, YYLTYPE *loc)
{
	skip_node_t *node;

	if (parser->skip_stack == NULL) {
		glcpp_error (loc, parser, "#endif without #if\n");
		return;
	}

	node = parser->skip_stack;
	parser->skip_stack = node->next;
	ralloc_free (node);
}
