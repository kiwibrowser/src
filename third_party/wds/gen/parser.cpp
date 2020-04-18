/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         wds_parse
#define yylex           wds_lex
#define yyerror         wds_error
#define yydebug         wds_debug
#define yynerrs         wds_nerrs


/* Copy the first part of user declarations.  */



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
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parser.h".  */
#ifndef YY_WDS_GEN_PARSER_H_INCLUDED
# define YY_WDS_GEN_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int wds_debug;
#endif
/* "%code requires" blocks.  */


/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2015 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

   #include <map>
   #include <memory>
   #include "libwds/rtsp/audiocodecs.h"
   #include "libwds/rtsp/contentprotection.h"
   #include "libwds/rtsp/triggermethod.h"
   #include "libwds/rtsp/route.h"
   #include "libwds/rtsp/uibcsetting.h"
   #include "libwds/rtsp/uibccapability.h"
   
   #define YYLEX_PARAM scanner
   
   namespace wds {
      struct AudioCodec;
   namespace rtsp {
      class Driver;
      class Scanner;
      class Message;
      class Header;
      class TransportHeader;
      class Property;
      class PropertyErrors;
      class Payload;
      class VideoFormats;
      struct H264Codec;
      struct H264Codec3d;
   }
   }



/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    END = 0,
    WFD_SP = 258,
    WFD_NUM = 259,
    WFD_OPTIONS = 260,
    WFD_SET_PARAMETER = 261,
    WFD_GET_PARAMETER = 262,
    WFD_SETUP = 263,
    WFD_PLAY = 264,
    WFD_TEARDOWN = 265,
    WFD_PAUSE = 266,
    WFD_END = 267,
    WFD_RESPONSE = 268,
    WFD_RESPONSE_CODE = 269,
    WFD_STRING = 270,
    WFD_GENERIC_PROPERTY = 271,
    WFD_HEADER = 272,
    WFD_CSEQ = 273,
    WFD_RESPONSE_METHODS = 274,
    WFD_TAG = 275,
    WFD_SUPPORT_CHECK = 276,
    WFD_REQUEST_URI = 277,
    WFD_CONTENT_TYPE = 278,
    WFD_MIME = 279,
    WFD_CONTENT_LENGTH = 280,
    WFD_AUDIO_CODECS = 281,
    WFD_VIDEO_FORMATS = 282,
    WFD_3D_FORMATS = 283,
    WFD_CONTENT_PROTECTION = 284,
    WFD_DISPLAY_EDID = 285,
    WFD_COUPLED_SINK = 286,
    WFD_TRIGGER_METHOD = 287,
    WFD_PRESENTATION_URL = 288,
    WFD_CLIENT_RTP_PORTS = 289,
    WFD_ROUTE = 290,
    WFD_I2C = 291,
    WFD_AV_FORMAT_CHANGE_TIMING = 292,
    WFD_PREFERRED_DISPLAY_MODE = 293,
    WFD_UIBC_CAPABILITY = 294,
    WFD_UIBC_SETTING = 295,
    WFD_STANDBY_RESUME_CAPABILITY = 296,
    WFD_STANDBY_IN_REQUEST = 297,
    WFD_STANDBY_IN_RESPONSE = 298,
    WFD_CONNECTOR_TYPE = 299,
    WFD_IDR_REQUEST = 300,
    WFD_AUDIO_CODECS_ERROR = 301,
    WFD_VIDEO_FORMATS_ERROR = 302,
    WFD_3D_FORMATS_ERROR = 303,
    WFD_CONTENT_PROTECTION_ERROR = 304,
    WFD_DISPLAY_EDID_ERROR = 305,
    WFD_COUPLED_SINK_ERROR = 306,
    WFD_TRIGGER_METHOD_ERROR = 307,
    WFD_PRESENTATION_URL_ERROR = 308,
    WFD_CLIENT_RTP_PORTS_ERROR = 309,
    WFD_ROUTE_ERROR = 310,
    WFD_I2C_ERROR = 311,
    WFD_AV_FORMAT_CHANGE_TIMING_ERROR = 312,
    WFD_PREFERRED_DISPLAY_MODE_ERROR = 313,
    WFD_UIBC_CAPABILITY_ERROR = 314,
    WFD_UIBC_SETTING_ERROR = 315,
    WFD_STANDBY_RESUME_CAPABILITY_ERROR = 316,
    WFD_STANDBY_ERROR = 317,
    WFD_CONNECTOR_TYPE_ERROR = 318,
    WFD_IDR_REQUEST_ERROR = 319,
    WFD_GENERIC_PROPERTY_ERROR = 320,
    WFD_NONE = 321,
    WFD_AUDIO_CODEC_LPCM = 322,
    WFD_AUDIO_CODEC_AAC = 323,
    WFD_AUDIO_CODEC_AC3 = 324,
    WFD_HDCP_SPEC_2_0 = 325,
    WFD_HDCP_SPEC_2_1 = 326,
    WFD_IP_PORT = 327,
    WFD_PRESENTATION_URL_0 = 328,
    WFD_PRESENTATION_URL_1 = 329,
    WFD_STREAM_PROFILE = 330,
    WFD_MODE_PLAY = 331,
    WFD_ROUTE_PRIMARY = 332,
    WFD_ROUTE_SECONDARY = 333,
    WFD_INPUT_CATEGORY_LIST = 334,
    WFD_INPUT_CATEGORY_GENERIC = 335,
    WFD_INPUT_CATEGORY_HIDC = 336,
    WFD_GENERIC_CAP_LIST = 337,
    WFD_INPUT_TYPE_KEYBOARD = 338,
    WFD_INPUT_TYPE_MOUSE = 339,
    WFD_INPUT_TYPE_SINGLE_TOUCH = 340,
    WFD_INPUT_TYPE_MULTI_TOUCH = 341,
    WFD_INPUT_TYPE_JOYSTICK = 342,
    WFD_INPUT_TYPE_CAMERA = 343,
    WFD_INPUT_TYPE_GESTURE = 344,
    WFD_INPUT_TYPE_REMOTE_CONTROL = 345,
    WFD_HIDC_CAP_LIST = 346,
    WFD_INPUT_PATH_INFRARED = 347,
    WFD_INPUT_PATH_USB = 348,
    WFD_INPUT_PATH_BT = 349,
    WFD_INPUT_PATH_WIFI = 350,
    WFD_INPUT_PATH_ZIGBEE = 351,
    WFD_INPUT_PATH_NOSP = 352,
    WFD_UIBC_SETTING_ENABLE = 353,
    WFD_UIBC_SETTING_DISABLE = 354,
    WFD_SUPPORTED = 355,
    WFD_SESSION = 356,
    WFD_SESSION_ID = 357,
    WFD_TIMEOUT = 358,
    WFD_TRANSPORT = 359,
    WFD_SERVER_PORT = 360
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{


   std::string* sval;
   unsigned long long int nval;
   bool bool_val;
   std::vector<std::string>* vsval;
   wds::rtsp::Message* message;
   wds::rtsp::Header* header;
   wds::rtsp::Payload* mpayload;
   wds::AudioFormats audio_format;
   wds::rtsp::Property* property;
   std::vector<unsigned short>* error_list;
   wds::rtsp::PropertyErrors* property_errors;
   std::map<wds::rtsp::PropertyType, std::shared_ptr<wds::rtsp::PropertyErrors>>* property_error_map;
   std::vector<wds::rtsp::H264Codec>* codecs;
   wds::rtsp::H264Codec* codec;
   std::vector<wds::rtsp::H264Codec3d>* codecs_3d;
   wds::rtsp::H264Codec3d* codec_3d;
   wds::rtsp::ContentProtection::HDCPSpec hdcp_spec;
   wds::rtsp::TriggerMethod::Method trigger_method;
   wds::rtsp::Route::Destination route_destination;
   bool uibc_setting;
   std::vector<wds::rtsp::UIBCCapability::InputCategory>* input_category_list;
   std::vector<wds::rtsp::UIBCCapability::InputType>* generic_cap_list;
   std::vector<wds::rtsp::UIBCCapability::DetailedCapability>* hidc_cap_list;
   wds::rtsp::UIBCCapability::InputCategory input_category_list_value;
   wds::rtsp::UIBCCapability::InputType generic_cap_list_value;
   wds::rtsp::UIBCCapability::DetailedCapability* hidc_cap_list_value;
   wds::rtsp::UIBCCapability::InputPath input_path;
   wds::rtsp::Method method;
   std::vector<wds::rtsp::Method>* methods;
   wds::rtsp::PropertyType parameter;
   std::vector<wds::rtsp::PropertyType>* parameters;
   std::vector<wds::AudioCodec>* audio_codecs;
   wds::AudioCodec* audio_codec;
   std::pair<std::string, unsigned int>* session_info;
   wds::rtsp::TransportHeader* transport;


};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int wds_parse (void* scanner, std::unique_ptr<wds::rtsp::Message>& message);

#endif /* !YY_WDS_GEN_PARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */


/* Unqualified %code blocks.  */


#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include "libwds/rtsp/driver.h"
#include "libwds/rtsp/message.h"
#include "libwds/rtsp/header.h"
#include "libwds/rtsp/transportheader.h"
#include "libwds/rtsp/payload.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/options.h"
#include "libwds/rtsp/getparameter.h"
#include "libwds/rtsp/setparameter.h"
#include "libwds/rtsp/play.h"
#include "libwds/rtsp/teardown.h"
#include "libwds/rtsp/pause.h"
#include "libwds/rtsp/setup.h"
#include "libwds/rtsp/audiocodecs.h"
#include "libwds/rtsp/videoformats.h"
#include "libwds/rtsp/formats3d.h"
#include "libwds/rtsp/contentprotection.h"
#include "libwds/rtsp/displayedid.h"
#include "libwds/rtsp/coupledsink.h"
#include "libwds/rtsp/triggermethod.h"
#include "libwds/rtsp/clientrtpports.h"
#include "libwds/rtsp/i2c.h"
#include "libwds/rtsp/avformatchangetiming.h"
#include "libwds/rtsp/standbyresumecapability.h"
#include "libwds/rtsp/standby.h"
#include "libwds/rtsp/idrrequest.h"
#include "libwds/rtsp/connectortype.h"
#include "libwds/rtsp/preferreddisplaymode.h"
#include "libwds/rtsp/presentationurl.h"
#include "libwds/rtsp/uibccapability.h"

#define UNUSED_TOKEN(T) (void)T
#define DELETE_TOKEN(T) \
    delete T;           \
    T = nullptr



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
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

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
#define YYFINAL  130
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   498

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  112
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  76
/* YYNRULES -- Number of rules.  */
#define YYNRULES  218
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  515

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   360

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,   106,     2,   108,   107,     2,   111,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   109,   110,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   319,   319,   323,   327,   333,   341,   342,   343,   344,
     345,   346,   347,   348,   352,   355,   362,   369,   376,   383,
     390,   397,   404,   411,   414,   415,   416,   420,   421,   425,
     430,   431,   439,   445,   451,   457,   461,   468,   472,   477,
     482,   488,   494,   504,   510,   514,   521,   522,   523,   524,
     525,   526,   527,   528,   531,   533,   536,   539,   540,   541,
     546,   553,   557,   565,   573,   574,   575,   576,   577,   578,
     579,   580,   581,   582,   583,   584,   585,   586,   587,   588,
     589,   590,   594,   598,   603,   607,   611,   615,   619,   623,
     627,   631,   635,   639,   643,   647,   651,   655,   659,   663,
     667,   671,   675,   683,   687,   696,   700,   709,   710,   711,
     712,   713,   714,   715,   716,   717,   718,   719,   720,   721,
     722,   723,   724,   725,   726,   729,   732,   740,   744,   750,
     755,   763,   769,   770,   771,   775,   779,   786,   791,   800,
     806,   811,   819,   825,   828,   832,   835,   840,   844,   850,
     853,   859,   862,   868,   871,   878,   881,   885,   888,   894,
     897,   901,   907,   910,   913,   916,   922,   930,   933,   937,
     940,   944,   949,   955,   958,   964,   970,   973,   977,   984,
     991,   994,  1003,  1009,  1012,  1016,  1022,  1025,  1031,  1037,
    1040,  1044,  1050,  1053,  1056,  1059,  1062,  1065,  1068,  1071,
    1077,  1083,  1086,  1091,  1098,  1105,  1108,  1111,  1114,  1117,
    1120,  1126,  1132,  1135,  1141,  1147,  1150,  1156,  1159
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "END", "error", "$undefined", "WFD_SP", "WFD_NUM", "WFD_OPTIONS",
  "WFD_SET_PARAMETER", "WFD_GET_PARAMETER", "WFD_SETUP", "WFD_PLAY",
  "WFD_TEARDOWN", "WFD_PAUSE", "WFD_END", "WFD_RESPONSE",
  "WFD_RESPONSE_CODE", "WFD_STRING", "WFD_GENERIC_PROPERTY", "WFD_HEADER",
  "WFD_CSEQ", "WFD_RESPONSE_METHODS", "WFD_TAG", "WFD_SUPPORT_CHECK",
  "WFD_REQUEST_URI", "WFD_CONTENT_TYPE", "WFD_MIME", "WFD_CONTENT_LENGTH",
  "WFD_AUDIO_CODECS", "WFD_VIDEO_FORMATS", "WFD_3D_FORMATS",
  "WFD_CONTENT_PROTECTION", "WFD_DISPLAY_EDID", "WFD_COUPLED_SINK",
  "WFD_TRIGGER_METHOD", "WFD_PRESENTATION_URL", "WFD_CLIENT_RTP_PORTS",
  "WFD_ROUTE", "WFD_I2C", "WFD_AV_FORMAT_CHANGE_TIMING",
  "WFD_PREFERRED_DISPLAY_MODE", "WFD_UIBC_CAPABILITY", "WFD_UIBC_SETTING",
  "WFD_STANDBY_RESUME_CAPABILITY", "WFD_STANDBY_IN_REQUEST",
  "WFD_STANDBY_IN_RESPONSE", "WFD_CONNECTOR_TYPE", "WFD_IDR_REQUEST",
  "WFD_AUDIO_CODECS_ERROR", "WFD_VIDEO_FORMATS_ERROR",
  "WFD_3D_FORMATS_ERROR", "WFD_CONTENT_PROTECTION_ERROR",
  "WFD_DISPLAY_EDID_ERROR", "WFD_COUPLED_SINK_ERROR",
  "WFD_TRIGGER_METHOD_ERROR", "WFD_PRESENTATION_URL_ERROR",
  "WFD_CLIENT_RTP_PORTS_ERROR", "WFD_ROUTE_ERROR", "WFD_I2C_ERROR",
  "WFD_AV_FORMAT_CHANGE_TIMING_ERROR", "WFD_PREFERRED_DISPLAY_MODE_ERROR",
  "WFD_UIBC_CAPABILITY_ERROR", "WFD_UIBC_SETTING_ERROR",
  "WFD_STANDBY_RESUME_CAPABILITY_ERROR", "WFD_STANDBY_ERROR",
  "WFD_CONNECTOR_TYPE_ERROR", "WFD_IDR_REQUEST_ERROR",
  "WFD_GENERIC_PROPERTY_ERROR", "WFD_NONE", "WFD_AUDIO_CODEC_LPCM",
  "WFD_AUDIO_CODEC_AAC", "WFD_AUDIO_CODEC_AC3", "WFD_HDCP_SPEC_2_0",
  "WFD_HDCP_SPEC_2_1", "WFD_IP_PORT", "WFD_PRESENTATION_URL_0",
  "WFD_PRESENTATION_URL_1", "WFD_STREAM_PROFILE", "WFD_MODE_PLAY",
  "WFD_ROUTE_PRIMARY", "WFD_ROUTE_SECONDARY", "WFD_INPUT_CATEGORY_LIST",
  "WFD_INPUT_CATEGORY_GENERIC", "WFD_INPUT_CATEGORY_HIDC",
  "WFD_GENERIC_CAP_LIST", "WFD_INPUT_TYPE_KEYBOARD",
  "WFD_INPUT_TYPE_MOUSE", "WFD_INPUT_TYPE_SINGLE_TOUCH",
  "WFD_INPUT_TYPE_MULTI_TOUCH", "WFD_INPUT_TYPE_JOYSTICK",
  "WFD_INPUT_TYPE_CAMERA", "WFD_INPUT_TYPE_GESTURE",
  "WFD_INPUT_TYPE_REMOTE_CONTROL", "WFD_HIDC_CAP_LIST",
  "WFD_INPUT_PATH_INFRARED", "WFD_INPUT_PATH_USB", "WFD_INPUT_PATH_BT",
  "WFD_INPUT_PATH_WIFI", "WFD_INPUT_PATH_ZIGBEE", "WFD_INPUT_PATH_NOSP",
  "WFD_UIBC_SETTING_ENABLE", "WFD_UIBC_SETTING_DISABLE", "WFD_SUPPORTED",
  "WFD_SESSION", "WFD_SESSION_ID", "WFD_TIMEOUT", "WFD_TRANSPORT",
  "WFD_SERVER_PORT", "'*'", "'-'", "','", "':'", "';'", "'/'", "$accept",
  "start", "message", "command", "options", "set_parameter",
  "get_parameter", "setup", "play", "teardown", "pause", "wfd_reply",
  "headers", "wfd_cseq", "wfd_content_type", "wfd_content_length",
  "wfd_session", "wfd_transport", "wfd_supported_methods", "wfd_methods",
  "wfd_method", "wfd_ows", "payload", "wfd_parameter_list",
  "wfd_parameter", "wfd_error_list", "wfd_property_errors",
  "wfd_property_error_map", "wdf_property_map", "wfd_property",
  "wfd_property_audio_codecs", "wfd_audio_codec_list", "wfd_audio_codec",
  "wfd_audio_codec_type", "wfd_property_video_formats", "wfd_h264_codecs",
  "wfd_h264_codec", "wfd_h264_codecs_3d", "wfd_h264_codec_3d",
  "wfd_max_hres", "wfd_max_vres", "wfd_property_3d_formats",
  "wfd_content_protection", "hdcp2_spec", "wfd_display_edid",
  "wfd_edid_payload", "wfd_coupled_sink", "wfd_sink_address",
  "wfd_trigger_method", "wfd_supported_trigger_methods",
  "wfd_presentation_url", "wfd_presentation_url0", "wfd_presentation_url1",
  "wfd_client_rtp_ports", "wfd_route", "wfd_route_destination", "wfd_I2C",
  "wfd_port", "wfd_av_format_change_timing", "wfd_preferred_display_mode",
  "wfd_uibc_capability", "wfd_input_category_list",
  "wfd_input_category_list_values", "wfd_input_category_list_value",
  "wfd_generic_cap_list", "wfd_generic_cap_list_values",
  "wfd_generic_cap_list_value", "wfd_hidc_cap_list",
  "wfd_hidc_cap_list_values", "wfd_hidc_cap_list_value", "wfd_input_path",
  "wfd_uibc_setting", "wfd_uibc_setting_value",
  "wfd_standby_resume_capability", "wfd_standby_resume_capability_value",
  "wfd_connector_type", YY_NULLPTR
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
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,    42,    45,    44,    58,
      59,    47
};
# endif

#define YYPACT_NINF -432

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-432)))

#define YYTABLE_NINF -57

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      15,  -432,     6,    81,    85,   100,   118,   125,   126,     4,
      92,    67,    74,    75,    83,    84,   110,   130,   155,   156,
     157,   158,   159,   160,   161,   162,   163,  -432,  -432,   164,
    -432,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   178,   180,   181,   182,   183,   184,   185,
     221,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,  -432,   133,  -432,  -432,   177,   107,  -432,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,    -9,   108,   264,   266,   273,
     274,   275,   283,  -432,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
    -432,   -11,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,  -432,    92,    67,    74,    75,    83,    84,   110,   130,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
    -432,   332,   333,   334,   335,   336,   337,   338,   339,  -432,
     113,  -432,    23,    25,    56,    26,    28,   179,    52,   268,
     -38,    29,   340,   341,    38,     7,    16,    30,   342,   342,
     342,   342,   342,   342,   342,   342,   342,   342,   342,   342,
     342,   342,   342,   342,   342,   342,   342,   297,   297,   297,
    -432,   297,   297,   344,   345,  -432,  -432,  -432,  -432,  -432,
    -432,   343,   346,   347,   348,   349,   350,   351,   352,  -432,
    -432,  -432,  -432,   240,  -432,   353,   354,  -432,   362,  -432,
    -432,  -432,  -432,   363,   364,  -432,   365,  -432,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,   366,   367,  -432,  -432,  -432,
    -432,  -432,  -432,   368,   369,  -432,    21,   241,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,   242,   242,   242,
     242,   242,   242,   242,   242,   242,   242,   242,   242,   242,
     242,   242,   242,   242,   242,   242,   358,   370,   104,   355,
     371,   250,   -22,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,   373,   374,   376,   377,   281,    20,    32,   -55,   378,
     379,   380,  -432,  -432,  -432,   246,  -432,   295,   382,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,     0,
    -432,  -432,  -432,   284,   384,   385,   194,   383,   387,   388,
     389,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
     391,  -432,   392,   393,   112,   282,   394,   289,   395,   293,
     356,  -432,   397,   398,   399,  -432,   400,   401,    39,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,   357,  -432,
     359,  -432,   297,  -432,   402,   403,  -432,   405,     0,  -432,
     406,     0,  -432,   407,   408,  -432,   409,   120,   360,   104,
    -432,   361,   410,   372,   411,   375,   381,   412,   128,  -432,
     386,   390,  -432,   404,  -432,   413,   415,   297,   416,   297,
    -432,   417,  -432,    61,   418,    29,  -432,   419,   398,   420,
     399,   421,  -432,  -432,  -432,  -432,  -432,  -432,  -432,   128,
    -432,   423,  -432,   424,  -432,   425,  -432,   426,   427,   428,
     430,   431,   432,   433,   434,   435,   437,   438,   439,   440,
     441,   442,   444,   445,   446,   447,   448,   449,   451,   452,
     453,   454,    33,   455,   457,  -432,  -432,   459,   460,   462,
      34,   463,   461,  -432,  -432,  -432,   466,    33,   467,   469,
     470,    34,   398,  -432,  -432
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     5,     0,     0,     0,     0,     0,     0,     0,     0,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,   124,    81,
     125,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     2,    23,     6,     7,     8,     9,    10,    11,    12,
      13,     4,    57,    61,   103,    59,    58,   105,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,     0,     0,     0,     0,     0,
       0,     0,     0,   126,     0,    54,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       1,     3,    62,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    81,
      60,   104,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     106,     0,     0,     0,     0,     0,     0,     0,     0,    22,
       0,    55,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    54,    54,    54,
      25,    54,    54,     0,     0,    24,    26,    27,    29,    30,
      28,     0,     0,     0,     0,     0,     0,     0,     0,   128,
     132,   133,   134,   127,   129,     0,     0,   135,     0,   148,
     149,   151,   152,     0,     0,   153,     0,   157,   162,   165,
     164,   163,   161,   167,   168,     0,     0,   173,   174,   172,
     177,   176,   175,     0,     0,   180,     0,     0,   212,   213,
     211,   215,   216,   214,   217,   218,    82,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,     0,     0,     0,     0,
       0,     0,    37,    15,    14,    16,    17,    18,    19,    20,
      21,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   183,   186,   187,   182,   184,     0,     0,    31,
      32,    46,    47,    48,    49,    50,    51,    52,    53,    43,
      44,    33,    34,    35,     0,     0,     0,     0,     0,     0,
       0,   156,   155,   154,   160,   159,   158,   169,   170,   166,
       0,   178,     0,     0,     0,     0,     0,     0,     0,    39,
      38,   130,     0,     0,     0,   150,     0,     0,     0,   189,
     192,   193,   194,   195,   196,   197,   198,   199,   188,   190,
       0,    83,    54,    36,     0,     0,   131,     0,   136,   137,
       0,   147,   140,     0,     0,   185,     0,     0,     0,     0,
      41,    40,     0,     0,     0,     0,     0,     0,     0,   201,
       0,   200,   202,     0,    45,     0,     0,    54,     0,    54,
     171,     0,   191,     0,     0,     0,    42,     0,     0,     0,
       0,     0,   205,   206,   207,   209,   208,   210,   204,     0,
     181,     0,   138,     0,   141,     0,   203,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   144,   143,     0,     0,     0,
       0,     0,     0,   146,   145,   142,     0,     0,     0,     0,
       0,     0,     0,   139,   179
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
      -6,  -217,  -432,  -432,   422,    44,   414,  -432,  -432,   429,
    -432,  -432,    66,  -432,  -432,  -432,  -431,  -432,   -21,   -71,
     -68,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,  -432,
    -432,  -432,  -432,  -432,  -432,  -432,  -432,    36,  -432,  -432,
    -432,  -432,  -432,    86,  -432,  -432,  -297,  -432,  -432,    18,
    -432,  -432,  -432,  -432,  -432,  -432
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,   131,   225,   226,   227,   228,   229,   230,   349,
     350,   182,    61,    62,    63,   287,    64,    65,    66,    67,
      68,   243,   244,   245,    69,   408,   409,   411,   412,   497,
     505,    70,    71,   253,    72,   363,    73,   366,    74,   262,
      75,   265,   369,    76,    77,   269,    78,   272,    79,    80,
      81,   277,   335,   336,   375,   398,   430,   418,   431,   432,
     458,    82,   280,    83,   283,    84
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     306,   307,   308,   181,   309,   310,   217,   218,   219,    85,
     220,   367,   221,   171,   222,   -56,     1,   462,    92,   368,
       2,     3,     4,     5,     6,     7,     8,   246,     9,   248,
     254,    10,   256,   270,   284,   361,   364,   495,   503,   267,
     268,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,   399,    47,    48,
      49,   514,   281,   354,    86,   355,   362,   332,    87,   247,
     223,   249,   255,   224,   257,   271,   285,   172,   365,   496,
     504,   333,   334,    88,   275,   278,   279,    93,   -54,   341,
     342,   343,   344,   345,   346,   347,   282,   276,   263,   333,
     334,    89,   250,   152,   348,   264,   251,   252,    90,    91,
     173,   442,   377,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   132,
      28,   169,    30,   452,   453,   454,   455,   456,   457,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,    27,    94,   149,   389,   239,
     240,   241,   242,    95,    96,   419,   429,   258,   259,   260,
     261,   423,    97,    98,   425,   390,   391,   392,   393,   394,
     395,   396,   397,   390,   391,   392,   393,   394,   395,   396,
     397,   390,   391,   392,   393,   394,   395,   396,   397,    99,
     448,   130,   450,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,   100,
      47,    48,    49,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   240,   241,   242,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   174,   123,   175,   124,
     125,   126,   127,   128,   129,   176,   177,   178,   179,   180,
     181,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   231,   232,   233,   234,   235,
     236,   237,   238,   266,   273,   274,   286,   311,   321,   312,
     338,   337,   353,   360,   373,   313,   322,   323,   314,   315,
     316,   317,   318,   319,   320,   324,   325,   326,   327,   328,
     329,   330,   331,   339,   340,   352,   356,   374,   357,   351,
     358,   359,   370,   371,   372,   376,   382,   378,   379,   380,
     383,   384,   400,   385,   386,   387,   388,   402,   401,   403,
     404,   406,   407,   410,   413,   414,   420,   421,   422,   424,
     426,   427,   428,   434,   436,   438,   441,   446,   447,   449,
     451,   459,   381,   461,   463,   465,   467,   468,   469,   464,
     470,   471,   472,   473,   474,   475,   509,   476,   477,   478,
     479,   480,   481,   513,   482,   483,   484,   485,   486,   487,
     417,   488,   489,   490,   491,   492,   493,   440,   494,   498,
     499,   405,   500,   501,   507,   416,   502,   506,   435,   508,
     433,   510,   511,   512,   415,     0,   445,   466,     0,   151,
     437,   460,     0,   439,   150,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   170,     0,   443,   444
};

static const yytype_int16 yycheck[] =
{
     217,   218,   219,     3,   221,   222,    17,    18,    19,     3,
      21,    66,    23,    22,    25,     0,     1,   448,    14,    74,
       5,     6,     7,     8,     9,    10,    11,     4,    13,     4,
       4,    16,     4,     4,     4,    15,     4,     4,     4,    77,
      78,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   374,    63,    64,
      65,   512,    66,   105,     3,   107,    66,    66,     3,    66,
     101,    66,    66,   104,    66,    66,    66,   106,    66,    66,
      66,    80,    81,     3,    66,    98,    99,    15,   108,     5,
       6,     7,     8,     9,    10,    11,   100,    79,    66,    80,
      81,     3,    66,    16,    20,    73,    70,    71,     3,     3,
      22,   428,   349,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    16,
      43,    44,    45,    92,    93,    94,    95,    96,    97,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,   109,    44,    66,    66,
      67,    68,    69,   109,   109,   402,    66,     8,     9,    10,
      11,   408,   109,   109,   411,    83,    84,    85,    86,    87,
      88,    89,    90,    83,    84,    85,    86,    87,    88,    89,
      90,    83,    84,    85,    86,    87,    88,    89,    90,   109,
     437,     0,   439,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,   109,
      63,    64,    65,   199,   200,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,    67,    68,    69,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,    22,   109,    22,   109,
     109,   109,   109,   109,   109,    22,    22,    22,    15,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,    75,     4,     4,     4,     3,   108,     4,
     108,   110,   102,    72,   108,    12,     3,     3,    12,    12,
      12,    12,    12,    12,    12,     3,     3,     3,     3,     3,
       3,     3,     3,    15,     4,     4,     3,    82,     4,    24,
       4,     4,     4,     4,     4,     3,     3,   103,     4,     4,
       3,     3,   110,     4,     3,     3,     3,   108,     4,     4,
     107,     4,     4,     4,     4,     4,     4,     4,     3,     3,
       3,     3,     3,   419,     4,     4,     4,     4,     3,     3,
       3,     3,   356,     4,     4,     4,     3,     3,     3,   450,
       4,     4,     4,     3,     3,     3,   507,     4,     4,     4,
       3,     3,     3,   511,     4,     4,     4,     3,     3,     3,
      91,     4,     4,     4,     3,     3,     3,    76,     4,     4,
       3,   105,     3,     3,     3,   108,     4,     4,   107,     3,
     110,     4,     3,     3,   388,    -1,    72,   459,    -1,    65,
     108,   445,    -1,   108,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    -1,   111,   108
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     5,     6,     7,     8,     9,    10,    11,    13,
      16,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    63,    64,    65,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   134,   135,   136,   138,   139,   140,   141,   142,   146,
     153,   154,   156,   158,   160,   162,   165,   166,   168,   170,
     171,   172,   183,   185,   187,     3,     3,     3,     3,     3,
       3,     3,    14,    15,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
       0,   124,    16,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    44,
     136,   138,    16,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    44,
     141,    22,   106,    22,    22,    22,    22,    22,    22,    15,
       3,     3,   133,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,    17,    18,    19,
      21,    23,    25,   101,   104,   125,   126,   127,   128,   129,
     130,     3,     3,     3,     3,     3,     3,     3,     3,    66,
      67,    68,    69,   143,   144,   145,     4,    66,     4,    66,
      66,    70,    71,   155,     4,    66,     4,    66,     8,     9,
      10,    11,   161,    66,    73,   163,    75,    77,    78,   167,
       4,    66,   169,     4,     4,    66,    79,   173,    98,    99,
     184,    66,   100,   186,     4,    66,     4,   137,   137,   137,
     137,   137,   137,   137,   137,   137,   137,   137,   137,   137,
     137,   137,   137,   137,   137,   137,   133,   133,   133,   133,
     133,     3,     4,    12,    12,    12,    12,    12,    12,    12,
      12,   108,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,    66,    80,    81,   174,   175,   110,   108,    15,
       4,     5,     6,     7,     8,     9,    10,    11,    20,   131,
     132,    24,     4,   102,   105,   107,     3,     4,     4,     4,
      72,    15,    66,   157,     4,    66,   159,    66,    74,   164,
       4,     4,     4,   108,    82,   176,     3,   133,   103,     4,
       4,   144,     3,     3,     3,     4,     3,     3,     3,    66,
      83,    84,    85,    86,    87,    88,    89,    90,   177,   178,
     110,     4,   108,     4,   107,   105,     4,     4,   147,   148,
       4,   149,   150,     4,     4,   175,   108,    91,   179,   133,
       4,     4,     3,   133,     3,   133,     3,     3,     3,    66,
     178,   180,   181,   110,   132,   107,     4,   108,     4,   108,
      76,     4,   178,   111,   108,    72,     4,     3,   133,     3,
     133,     3,    92,    93,    94,    95,    96,    97,   182,     3,
     169,     4,   148,     4,   150,     4,   181,     3,     3,     3,
       4,     4,     4,     3,     3,     3,     4,     4,     4,     3,
       3,     3,     4,     4,     4,     3,     3,     3,     4,     4,
       4,     3,     3,     3,     4,     4,    66,   151,     4,     3,
       3,     3,     4,     4,    66,   152,     4,     3,     3,   151,
       4,     3,     3,   152,   148
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   112,   113,   114,   114,   114,   115,   115,   115,   115,
     115,   115,   115,   115,   116,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   125,   126,   127,   128,   128,   129,   129,   129,
     129,   129,   129,   130,   131,   131,   132,   132,   132,   132,
     132,   132,   132,   132,   133,   133,   134,   134,   134,   134,
     135,   135,   135,   135,   136,   136,   136,   136,   136,   136,
     136,   136,   136,   136,   136,   136,   136,   136,   136,   136,
     136,   136,   137,   137,   138,   138,   138,   138,   138,   138,
     138,   138,   138,   138,   138,   138,   138,   138,   138,   138,
     138,   138,   138,   139,   139,   140,   140,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   142,   142,   143,
     143,   144,   145,   145,   145,   146,   146,   147,   147,   148,
     149,   149,   150,   151,   151,   152,   152,   153,   153,   154,
     154,   155,   155,   156,   156,   157,   157,   158,   158,   159,
     159,   160,   161,   161,   161,   161,   162,   163,   163,   164,
     164,   165,   166,   167,   167,   168,   169,   169,   170,   171,
     172,   172,   173,   174,   174,   174,   175,   175,   176,   177,
     177,   177,   178,   178,   178,   178,   178,   178,   178,   178,
     179,   180,   180,   180,   181,   182,   182,   182,   182,   182,
     182,   183,   184,   184,   185,   186,   186,   187,   187
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     5,     5,     5,     5,     5,     5,
       5,     5,     3,     0,     2,     2,     2,     2,     2,     2,
       2,     4,     3,     3,     3,     3,     5,     2,     4,     4,
       6,     6,     8,     3,     1,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     1,     0,     1,     1,     1,
       2,     1,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     1,     2,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     4,     4,     1,
       4,     5,     1,     1,     1,     4,     8,     1,     5,    21,
       1,     5,    17,     1,     1,     1,     1,     8,     4,     4,
       7,     1,     1,     4,     6,     1,     1,     4,     6,     1,
       1,     4,     1,     1,     1,     1,     6,     1,     1,     1,
       1,    10,     4,     1,     1,     4,     1,     1,     6,    28,
       4,    11,     2,     1,     1,     4,     1,     1,     2,     1,
       1,     4,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     4,     3,     1,     1,     1,     1,     1,
       1,     4,     1,     1,     4,     1,     1,     4,     4
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
      yyerror (scanner, message, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, scanner, message); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void* scanner, std::unique_ptr<wds::rtsp::Message>& message)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (scanner);
  YYUSE (message);
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void* scanner, std::unique_ptr<wds::rtsp::Message>& message)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner, message);
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, void* scanner, std::unique_ptr<wds::rtsp::Message>& message)
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
                                              , scanner, message);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner, message); \
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void* scanner, std::unique_ptr<wds::rtsp::Message>& message)
{
  YYUSE (yyvaluep);
  YYUSE (scanner);
  YYUSE (message);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yytype)
    {
          case 15: /* WFD_STRING  */

      { DELETE_TOKEN(((*yyvaluep).sval)); }

        break;

    case 16: /* WFD_GENERIC_PROPERTY  */

      { DELETE_TOKEN(((*yyvaluep).sval)); }

        break;

    case 22: /* WFD_REQUEST_URI  */

      { DELETE_TOKEN(((*yyvaluep).sval)); }

        break;

    case 24: /* WFD_MIME  */

      { DELETE_TOKEN(((*yyvaluep).sval)); }

        break;

    case 130: /* wfd_supported_methods  */

      { DELETE_TOKEN(((*yyvaluep).methods)); }

        break;

    case 131: /* wfd_methods  */

      { DELETE_TOKEN(((*yyvaluep).methods)); }

        break;

    case 135: /* wfd_parameter_list  */

      { DELETE_TOKEN(((*yyvaluep).mpayload)); }

        break;

    case 143: /* wfd_audio_codec_list  */

      { DELETE_TOKEN(((*yyvaluep).audio_codecs)); }

        break;

    case 144: /* wfd_audio_codec  */

      { DELETE_TOKEN(((*yyvaluep).audio_codec)); }

        break;

    case 147: /* wfd_h264_codecs  */

      { DELETE_TOKEN(((*yyvaluep).codecs)); }

        break;

    case 148: /* wfd_h264_codec  */

      { DELETE_TOKEN(((*yyvaluep).codec)); }

        break;

    case 149: /* wfd_h264_codecs_3d  */

      { DELETE_TOKEN(((*yyvaluep).codecs_3d)); }

        break;

    case 150: /* wfd_h264_codec_3d  */

      { DELETE_TOKEN(((*yyvaluep).codec_3d)); }

        break;

    case 157: /* wfd_edid_payload  */

      { DELETE_TOKEN(((*yyvaluep).sval)); }

        break;

    case 181: /* wfd_hidc_cap_list_value  */

      { DELETE_TOKEN(((*yyvaluep).hidc_cap_list_value)); }

        break;


      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void* scanner, std::unique_ptr<wds::rtsp::Message>& message)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

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

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
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

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

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
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

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
      yychar = yylex (&yylval, scanner, message);
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:

    {
      message.reset((yyvsp[-1].message));
      (yyvsp[-1].message)->set_header(std::unique_ptr<wds::rtsp::Header>((yyvsp[0].header)));
    }

    break;

  case 4:

    {
      if (message && (yyvsp[0].mpayload))
        message->set_payload(std::unique_ptr<wds::rtsp::Payload>((yyvsp[0].mpayload)));
      else
        YYERROR;
    }

    break;

  case 5:

    {
      message.reset();
      std::cerr << "Unknown message" << std::endl;
      YYABORT;
    }

    break;

  case 14:

    {
      (yyval.message) = new wds::rtsp::Options("*");
    }

    break;

  case 15:

    {
      (yyval.message) = new wds::rtsp::Options(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 16:

    {
      (yyval.message) = new wds::rtsp::SetParameter(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 17:

    {
      (yyval.message) = new wds::rtsp::GetParameter(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 18:

    {
      (yyval.message) = new wds::rtsp::Setup(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 19:

    {
      (yyval.message) = new wds::rtsp::Play(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 20:

    {
      (yyval.message) = new wds::rtsp::Teardown(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 21:

    {
      (yyval.message) = new wds::rtsp::Pause(*(yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 22:

    {
      DELETE_TOKEN((yyvsp[0].sval));
      (yyval.message) = new wds::rtsp::Reply((yyvsp[-1].nval));
    }

    break;

  case 23:

    {
      (yyval.header) = new wds::rtsp::Header();
    }

    break;

  case 24:

    { (yyvsp[-1].header)->set_cseq((yyvsp[0].nval)); }

    break;

  case 25:

    { (yyvsp[-1].header)->set_require_wfd_support(true); }

    break;

  case 26:

    {
          (yyvsp[-1].header)->set_content_type(*(yyvsp[0].sval));
          DELETE_TOKEN((yyvsp[0].sval));
      }

    break;

  case 27:

    { (yyvsp[-1].header)->set_content_length((yyvsp[0].nval)); }

    break;

  case 28:

    {
          (yyvsp[-1].header)->set_supported_methods(*(yyvsp[0].methods));
          DELETE_TOKEN((yyvsp[0].methods));
      }

    break;

  case 29:

    {
      (yyvsp[-1].header)->set_session((*(yyvsp[0].session_info)).first);
      (yyvsp[-1].header)->set_timeout((*(yyvsp[0].session_info)).second);
      DELETE_TOKEN((yyvsp[0].session_info));
    }

    break;

  case 30:

    { (yyvsp[-1].header)->set_transport ((yyvsp[0].transport)); }

    break;

  case 31:

    {
          (yyvsp[-3].header)->add_generic_header(*(yyvsp[-2].sval), *(yyvsp[0].sval));
          DELETE_TOKEN((yyvsp[-2].sval));
          DELETE_TOKEN((yyvsp[0].sval));
      }

    break;

  case 32:

    {
      (yyval.nval) = (yyvsp[0].nval);
    }

    break;

  case 33:

    {
      (yyval.sval) = (yyvsp[0].sval);
    }

    break;

  case 34:

    {
      (yyval.nval) = (yyvsp[0].nval);
    }

    break;

  case 35:

    {
      (yyval.session_info) = new std::pair<std::string, unsigned int>(*(yyvsp[0].sval), 0);
      DELETE_TOKEN((yyvsp[0].sval));
    }

    break;

  case 36:

    {
      (yyval.session_info) = new std::pair<std::string, unsigned int>(*(yyvsp[-2].sval), (yyvsp[0].nval));
      DELETE_TOKEN((yyvsp[-2].sval));
    }

    break;

  case 37:

    {
      (yyval.transport) = new wds::rtsp::TransportHeader();
      (yyval.transport)->set_client_port ((yyvsp[0].nval));
    }

    break;

  case 38:

    {
      (yyval.transport) = new wds::rtsp::TransportHeader();
      (yyval.transport)->set_client_port ((yyvsp[-2].nval));
      (yyval.transport)->set_client_supports_rtcp (true);
    }

    break;

  case 39:

    {
      (yyval.transport) = new wds::rtsp::TransportHeader();
      (yyval.transport)->set_client_port ((yyvsp[-2].nval));
      (yyval.transport)->set_server_port ((yyvsp[0].nval));
    }

    break;

  case 40:

    {
      (yyval.transport) = new wds::rtsp::TransportHeader();
      (yyval.transport)->set_client_port ((yyvsp[-4].nval));
      (yyval.transport)->set_client_supports_rtcp (true);
      (yyval.transport)->set_server_port ((yyvsp[0].nval));
    }

    break;

  case 41:

    {
      (yyval.transport) = new wds::rtsp::TransportHeader();
      (yyval.transport)->set_client_port ((yyvsp[-4].nval));
      (yyval.transport)->set_server_port ((yyvsp[-2].nval));
      (yyval.transport)->set_server_supports_rtcp (true);
    }

    break;

  case 42:

    {
      (yyval.transport) = new wds::rtsp::TransportHeader();
      (yyval.transport)->set_client_port ((yyvsp[-6].nval));
      (yyval.transport)->set_client_supports_rtcp (true);
      (yyval.transport)->set_server_port ((yyvsp[-2].nval));
      (yyval.transport)->set_server_supports_rtcp (true);
    }

    break;

  case 43:

    {
     (yyval.methods) = (yyvsp[0].methods);
    }

    break;

  case 44:

    {
      (yyval.methods) = new std::vector<wds::rtsp::Method>();
      (yyval.methods)->push_back((yyvsp[0].method));
    }

    break;

  case 45:

    {
      UNUSED_TOKEN((yyval.methods));
      (yyvsp[-4].methods)->push_back((yyvsp[0].method));
    }

    break;

  case 46:

    { (yyval.method) = wds::rtsp::OPTIONS; }

    break;

  case 47:

    { (yyval.method) = wds::rtsp::SET_PARAMETER; }

    break;

  case 48:

    { (yyval.method) = wds::rtsp::GET_PARAMETER; }

    break;

  case 49:

    { (yyval.method) = wds::rtsp::SETUP; }

    break;

  case 50:

    { (yyval.method) = wds::rtsp::PLAY; }

    break;

  case 51:

    { (yyval.method) = wds::rtsp::TEARDOWN; }

    break;

  case 52:

    { (yyval.method) = wds::rtsp::PAUSE; }

    break;

  case 53:

    { (yyval.method) = wds::rtsp::ORG_WFA_WFD_1_0; }

    break;

  case 56:

    {
    (yyval.mpayload) = 0;
    }

    break;

  case 60:

    {
      UNUSED_TOKEN((yyval.mpayload));
      if (auto payload = ToGetParameterPayload((yyvsp[-1].mpayload)))
        payload->AddRequestProperty((yyvsp[0].parameter));
      else
        YYERROR;
    }

    break;

  case 61:

    {
      (yyval.mpayload) = new wds::rtsp::GetParameterPayload();
      wds::rtsp::ToGetParameterPayload((yyval.mpayload))->AddRequestProperty((yyvsp[0].parameter));
    }

    break;

  case 62:

    {
      UNUSED_TOKEN((yyval.mpayload));
      if (auto payload = ToGetParameterPayload((yyvsp[-1].mpayload)))
        payload->AddRequestProperty(*(yyvsp[0].sval));
      else
        YYERROR;
      DELETE_TOKEN((yyvsp[0].sval));
    }

    break;

  case 63:

    {
      (yyval.mpayload) = new wds::rtsp::GetParameterPayload();
      wds::rtsp::ToGetParameterPayload((yyval.mpayload))->AddRequestProperty(*(yyvsp[0].sval));
      DELETE_TOKEN((yyvsp[0].sval));
    }

    break;

  case 64:

    { (yyval.parameter) = wds::rtsp::AudioCodecsPropertyType; }

    break;

  case 65:

    { (yyval.parameter) = wds::rtsp::VideoFormatsPropertyType; }

    break;

  case 66:

    { (yyval.parameter) = wds::rtsp::Video3DFormatsPropertyType; }

    break;

  case 67:

    { (yyval.parameter) = wds::rtsp::ContentProtectionPropertyType; }

    break;

  case 68:

    { (yyval.parameter) = wds::rtsp::DisplayEdidPropertyType; }

    break;

  case 69:

    { (yyval.parameter) = wds::rtsp::CoupledSinkPropertyType; }

    break;

  case 70:

    { (yyval.parameter) = wds::rtsp::TriggerMethodPropertyType; }

    break;

  case 71:

    { (yyval.parameter) = wds::rtsp::PresentationURLPropertyType; }

    break;

  case 72:

    { (yyval.parameter) = wds::rtsp::ClientRTPPortsPropertyType; }

    break;

  case 73:

    { (yyval.parameter) = wds::rtsp::RoutePropertyType; }

    break;

  case 74:

    { (yyval.parameter) = wds::rtsp::I2CPropertyType; }

    break;

  case 75:

    { (yyval.parameter) = wds::rtsp::AVFormatChangeTimingPropertyType; }

    break;

  case 76:

    { (yyval.parameter) = wds::rtsp::PreferredDisplayModePropertyType; }

    break;

  case 77:

    { (yyval.parameter) = wds::rtsp::UIBCCapabilityPropertyType; }

    break;

  case 78:

    { (yyval.parameter) = wds::rtsp::UIBCSettingPropertyType; }

    break;

  case 79:

    { (yyval.parameter) = wds::rtsp::StandbyResumeCapabilityPropertyType; }

    break;

  case 80:

    { (yyval.parameter) = wds::rtsp::StandbyPropertyType; }

    break;

  case 81:

    { (yyval.parameter) = wds::rtsp::ConnectorTypePropertyType; }

    break;

  case 82:

    {
      (yyval.error_list) = new std::vector<unsigned short>();
      (yyval.error_list)->push_back((yyvsp[0].nval));
    }

    break;

  case 83:

    {
      (yyvsp[-3].error_list)->push_back((yyvsp[0].nval));
    }

    break;

  case 84:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::AudioCodecsPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 85:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::VideoFormatsPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 86:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::Video3DFormatsPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 87:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::ContentProtectionPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 88:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::DisplayEdidPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 89:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::CoupledSinkPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 90:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::TriggerMethodPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 91:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::PresentationURLPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 92:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::ClientRTPPortsPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 93:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::RoutePropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 94:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::I2CPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 95:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::AVFormatChangeTimingPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 96:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::PreferredDisplayModePropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 97:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::UIBCCapabilityPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 98:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::UIBCSettingPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 99:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::StandbyResumeCapabilityPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 100:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::ConnectorTypePropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 101:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(wds::rtsp::IDRRequestPropertyType, *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 102:

    {
      (yyval.property_errors) = new wds::rtsp::PropertyErrors(*(yyvsp[-3].sval), *(yyvsp[0].error_list));
      DELETE_TOKEN((yyvsp[-3].sval));
      DELETE_TOKEN((yyvsp[0].error_list));
    }

    break;

  case 103:

    {
      (yyval.mpayload) = new wds::rtsp::PropertyErrorPayload();
      ToPropertyErrorPayload((yyval.mpayload))->AddPropertyError(std::shared_ptr<wds::rtsp::PropertyErrors>((yyvsp[0].property_errors)));
    }

    break;

  case 104:

    {
      if (auto payload = ToPropertyErrorPayload((yyvsp[-1].mpayload)))
        payload->AddPropertyError(std::shared_ptr<wds::rtsp::PropertyErrors>((yyvsp[0].property_errors)));
      else
        YYERROR;
    }

    break;

  case 105:

    {
      (yyval.mpayload) = new wds::rtsp::PropertyMapPayload();
      ToPropertyMapPayload((yyval.mpayload))->AddProperty(std::shared_ptr<wds::rtsp::Property>((yyvsp[0].property)));
    }

    break;

  case 106:

    {
      if (auto payload = ToPropertyMapPayload((yyvsp[-1].mpayload)))
        payload->AddProperty(std::shared_ptr<wds::rtsp::Property>((yyvsp[0].property)));
      else
        YYERROR;
    }

    break;

  case 124:

    {
      (yyval.property) = new wds::rtsp::Standby();
    }

    break;

  case 125:

    {
      (yyval.property) = new wds::rtsp::IDRRequest();
    }

    break;

  case 126:

    {
      (yyval.property) = new wds::rtsp::GenericProperty(*(yyvsp[-1].sval), *(yyvsp[0].sval));
      DELETE_TOKEN((yyvsp[-1].sval));
      DELETE_TOKEN((yyvsp[0].sval));
    }

    break;

  case 127:

    {
      (yyval.property) = new wds::rtsp::AudioCodecs(*(yyvsp[0].audio_codecs));
      DELETE_TOKEN((yyvsp[0].audio_codecs));
    }

    break;

  case 128:

    {
      (yyval.property) = new wds::rtsp::AudioCodecs();
    }

    break;

  case 129:

    {
      (yyval.audio_codecs) = new std::vector<wds::AudioCodec>();
      (yyval.audio_codecs)->push_back(*(yyvsp[0].audio_codec));
      DELETE_TOKEN((yyvsp[0].audio_codec));
    }

    break;

  case 130:

    {
      UNUSED_TOKEN((yyval.audio_codecs));
      (yyvsp[-3].audio_codecs)->push_back(*(yyvsp[0].audio_codec));
      DELETE_TOKEN((yyvsp[0].audio_codec));
    }

    break;

  case 131:

    {
    (yyval.audio_codec) = new wds::AudioCodec((yyvsp[-4].audio_format), (yyvsp[-2].nval), (yyvsp[0].nval));
  }

    break;

  case 132:

    { (yyval.audio_format) = wds::LPCM; }

    break;

  case 133:

    { (yyval.audio_format) = wds::AAC; }

    break;

  case 134:

    { (yyval.audio_format) = wds::AC3; }

    break;

  case 135:

    {
      (yyval.property) = new wds::rtsp::VideoFormats();
    }

    break;

  case 136:

    {
      (yyval.property) = new wds::rtsp::VideoFormats((yyvsp[-4].nval), (yyvsp[-2].nval), *(yyvsp[0].codecs));
      DELETE_TOKEN((yyvsp[0].codecs));
    }

    break;

  case 137:

    {
      (yyval.codecs) = new wds::rtsp::H264Codecs();
      (yyval.codecs)->push_back(*(yyvsp[0].codec));
      DELETE_TOKEN((yyvsp[0].codec));
    }

    break;

  case 138:

    {
      UNUSED_TOKEN((yyval.codecs));
      (yyvsp[-4].codecs)->push_back(*(yyvsp[0].codec));
      DELETE_TOKEN((yyvsp[0].codec));
    }

    break;

  case 139:

    {
      (yyval.codec) = new wds::rtsp::H264Codec((yyvsp[-20].nval), (yyvsp[-18].nval), (yyvsp[-16].nval), (yyvsp[-14].nval), (yyvsp[-12].nval), (yyvsp[-10].nval), (yyvsp[-8].nval), (yyvsp[-6].nval), (yyvsp[-4].nval), (yyvsp[-2].nval), (yyvsp[0].nval));
    }

    break;

  case 140:

    {
      (yyval.codecs_3d) = new wds::rtsp::H264Codecs3d();
      (yyval.codecs_3d)->push_back(*(yyvsp[0].codec_3d));
      DELETE_TOKEN((yyvsp[0].codec_3d));
    }

    break;

  case 141:

    {
      UNUSED_TOKEN((yyval.codecs_3d));
      (yyvsp[-4].codecs_3d)->push_back(*(yyvsp[0].codec_3d));
      DELETE_TOKEN((yyvsp[0].codec_3d));
    }

    break;

  case 142:

    {
      (yyval.codec_3d) = new wds::rtsp::H264Codec3d((yyvsp[-16].nval), (yyvsp[-14].nval), (yyvsp[-12].nval), (yyvsp[-10].nval), (yyvsp[-8].nval), (yyvsp[-6].nval), (yyvsp[-4].nval), (yyvsp[-2].nval), (yyvsp[0].nval));
    }

    break;

  case 143:

    {
      (yyval.nval) = 0;
    }

    break;

  case 145:

    {
      (yyval.nval) = 0;
    }

    break;

  case 147:

    {
      (yyval.property) = new wds::rtsp::Formats3d((yyvsp[-4].nval), (yyvsp[-2].nval), *(yyvsp[0].codecs_3d));
      DELETE_TOKEN((yyvsp[0].codecs_3d));
    }

    break;

  case 148:

    {
      (yyval.property) = new wds::rtsp::Formats3d();
    }

    break;

  case 149:

    {
      (yyval.property) = new wds::rtsp::ContentProtection();
    }

    break;

  case 150:

    {
      (yyval.property) = new wds::rtsp::ContentProtection((yyvsp[-3].hdcp_spec), (yyvsp[0].nval));
    }

    break;

  case 151:

    {
      (yyval.hdcp_spec) = wds::rtsp::ContentProtection::HDCP_SPEC_2_0;
    }

    break;

  case 152:

    {
      (yyval.hdcp_spec) = wds::rtsp::ContentProtection::HDCP_SPEC_2_1;
    }

    break;

  case 153:

    {
      (yyval.property) = new wds::rtsp::DisplayEdid();
    }

    break;

  case 154:

    {
      (yyval.property) = new wds::rtsp::DisplayEdid((yyvsp[-2].nval), (yyvsp[0].sval) ? *(yyvsp[0].sval) : "");
      DELETE_TOKEN((yyvsp[0].sval));
    }

    break;

  case 155:

    {
      (yyval.sval) = 0;
    }

    break;

  case 157:

    {
      (yyval.property) = new wds::rtsp::CoupledSink();
    }

    break;

  case 158:

    {
      (yyval.property) = new wds::rtsp::CoupledSink((yyvsp[-2].nval), (yyvsp[0].nval));
    }

    break;

  case 159:

    {
     (yyval.nval) = -1;
    }

    break;

  case 161:

    {
      (yyval.property) = new wds::rtsp::TriggerMethod((yyvsp[0].trigger_method));
    }

    break;

  case 162:

    {
      (yyval.trigger_method) = wds::rtsp::TriggerMethod::SETUP;
    }

    break;

  case 163:

    {
      (yyval.trigger_method) = wds::rtsp::TriggerMethod::PAUSE;
    }

    break;

  case 164:

    {
      (yyval.trigger_method) = wds::rtsp::TriggerMethod::TEARDOWN;
    }

    break;

  case 165:

    {
      (yyval.trigger_method) = wds::rtsp::TriggerMethod::PLAY;
    }

    break;

  case 166:

    {
      (yyval.property) = new wds::rtsp::PresentationUrl((yyvsp[-2].sval) ? *(yyvsp[-2].sval) : "", (yyvsp[0].sval) ? *(yyvsp[0].sval) : "");
      DELETE_TOKEN((yyvsp[-2].sval));
      DELETE_TOKEN((yyvsp[0].sval));
    }

    break;

  case 167:

    {
      (yyval.sval) = 0;
    }

    break;

  case 169:

    {
      (yyval.sval) = 0;
    }

    break;

  case 171:

    {
      (yyval.property) = new wds::rtsp::ClientRtpPorts((yyvsp[-4].nval), (yyvsp[-2].nval));
  }

    break;

  case 172:

    {
      (yyval.property) = new wds::rtsp::Route((yyvsp[0].route_destination));
    }

    break;

  case 173:

    {
      (yyval.route_destination) = wds::rtsp::Route::PRIMARY;
    }

    break;

  case 174:

    {
      (yyval.route_destination) = wds::rtsp::Route::SECONDARY;
    }

    break;

  case 175:

    {
      (yyval.property) = new wds::rtsp::I2C((yyvsp[0].nval));
    }

    break;

  case 176:

    {
      (yyval.nval) = -1;
    }

    break;

  case 178:

    {
      (yyval.property) = new wds::rtsp::AVFormatChangeTiming((yyvsp[-2].nval), (yyvsp[0].nval));
    }

    break;

  case 179:

    {
      (yyval.property) = new wds::rtsp::PreferredDisplayMode((yyvsp[-24].nval), (yyvsp[-22].nval), (yyvsp[-20].nval), (yyvsp[-18].nval), (yyvsp[-16].nval), (yyvsp[-14].nval), (yyvsp[-12].nval), (yyvsp[-10].nval), (yyvsp[-8].nval), (yyvsp[-6].nval), (yyvsp[-4].nval), (yyvsp[-2].nval), *(yyvsp[0].codec));
      DELETE_TOKEN((yyvsp[0].codec));
    }

    break;

  case 180:

    {
      (yyval.property) = new wds::rtsp::UIBCCapability();
    }

    break;

  case 181:

    {
      (yyval.property) = new wds::rtsp::UIBCCapability(*(yyvsp[-7].input_category_list), *(yyvsp[-5].generic_cap_list), *(yyvsp[-3].hidc_cap_list), (yyvsp[0].nval));
      DELETE_TOKEN((yyvsp[-7].input_category_list));
      DELETE_TOKEN((yyvsp[-5].generic_cap_list));
      DELETE_TOKEN((yyvsp[-3].hidc_cap_list));
    }

    break;

  case 182:

    {
      (yyval.input_category_list) = (yyvsp[0].input_category_list);
    }

    break;

  case 183:

    {
      (yyval.input_category_list) = new std::vector<wds::rtsp::UIBCCapability::InputCategory>();
    }

    break;

  case 184:

    {
      (yyval.input_category_list) = new std::vector<wds::rtsp::UIBCCapability::InputCategory>();
      (yyval.input_category_list)->push_back((yyvsp[0].input_category_list_value));
    }

    break;

  case 185:

    {
      (yyvsp[-3].input_category_list)->push_back((yyvsp[0].input_category_list_value));
    }

    break;

  case 186:

    {
      (yyval.input_category_list_value) = wds::rtsp::UIBCCapability::GENERIC;
    }

    break;

  case 187:

    {
      (yyval.input_category_list_value) = wds::rtsp::UIBCCapability::HIDC;
    }

    break;

  case 188:

    {
      (yyval.generic_cap_list) = (yyvsp[0].generic_cap_list);
    }

    break;

  case 189:

    {
      (yyval.generic_cap_list) = new std::vector<wds::rtsp::UIBCCapability::InputType>();
    }

    break;

  case 190:

    {
      (yyval.generic_cap_list) = new std::vector<wds::rtsp::UIBCCapability::InputType>();
      (yyval.generic_cap_list)->push_back((yyvsp[0].generic_cap_list_value));
    }

    break;

  case 191:

    {
      (yyvsp[-3].generic_cap_list)->push_back((yyvsp[0].generic_cap_list_value));
    }

    break;

  case 192:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::KEYBOARD;
    }

    break;

  case 193:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::MOUSE;
    }

    break;

  case 194:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::SINGLE_TOUCH;
    }

    break;

  case 195:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::MULTI_TOUCH;
    }

    break;

  case 196:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::JOYSTICK;
    }

    break;

  case 197:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::CAMERA;
    }

    break;

  case 198:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::GESTURE;
    }

    break;

  case 199:

    {
      (yyval.generic_cap_list_value) = wds::rtsp::UIBCCapability::REMOTE_CONTROL;
    }

    break;

  case 200:

    {
      (yyval.hidc_cap_list) = (yyvsp[0].hidc_cap_list);
    }

    break;

  case 201:

    {
      (yyval.hidc_cap_list) = new std::vector<wds::rtsp::UIBCCapability::DetailedCapability>();
    }

    break;

  case 202:

    {
      (yyval.hidc_cap_list) = new std::vector<wds::rtsp::UIBCCapability::DetailedCapability>();
      (yyval.hidc_cap_list)->push_back(*(yyvsp[0].hidc_cap_list_value));
      DELETE_TOKEN((yyvsp[0].hidc_cap_list_value));
    }

    break;

  case 203:

    {
      (yyvsp[-3].hidc_cap_list)->push_back(*(yyvsp[0].hidc_cap_list_value));
      DELETE_TOKEN((yyvsp[0].hidc_cap_list_value));
    }

    break;

  case 204:

    {
      (yyval.hidc_cap_list_value) = new wds::rtsp::UIBCCapability::DetailedCapability((yyvsp[-2].generic_cap_list_value), (yyvsp[0].input_path));
    }

    break;

  case 205:

    {
      (yyval.input_path) = wds::rtsp::UIBCCapability::INFRARED;
    }

    break;

  case 206:

    {
      (yyval.input_path) = wds::rtsp::UIBCCapability::USB;
    }

    break;

  case 207:

    {
      (yyval.input_path) = wds::rtsp::UIBCCapability::BT;
    }

    break;

  case 208:

    {
      (yyval.input_path) = wds::rtsp::UIBCCapability::ZIGBEE;
    }

    break;

  case 209:

    {
      (yyval.input_path) = wds::rtsp::UIBCCapability::WI_FI;
    }

    break;

  case 210:

    {
      (yyval.input_path) = wds::rtsp::UIBCCapability::NO_SP;
    }

    break;

  case 211:

    {
      (yyval.property) = new wds::rtsp::UIBCSetting((yyvsp[0].uibc_setting));
    }

    break;

  case 212:

    {
      (yyval.uibc_setting) = true;
    }

    break;

  case 213:

    {
      (yyval.uibc_setting) = false;
    }

    break;

  case 214:

    {
      (yyval.property) = new wds::rtsp::StandbyResumeCapability((yyvsp[0].bool_val));
    }

    break;

  case 215:

    {
      (yyval.bool_val) = false;
    }

    break;

  case 216:

    {
      (yyval.bool_val) = true;
    }

    break;

  case 217:

    {
      (yyval.property) = new wds::rtsp::ConnectorType((yyvsp[0].nval));
    }

    break;

  case 218:

    {
      (yyval.property) = new wds::rtsp::ConnectorType();
    }

    break;



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
      yyerror (scanner, message, YY_("syntax error"));
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
        yyerror (scanner, message, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



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
                      yytoken, &yylval, scanner, message);
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


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, scanner, message);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


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
  yyerror (scanner, message, YY_("memory exhausted"));
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
                  yytoken, &yylval, scanner, message);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, scanner, message);
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


