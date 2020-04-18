/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

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
