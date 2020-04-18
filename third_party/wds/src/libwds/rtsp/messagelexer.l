%option nodefault nounput nomain
%option bison-bridge reentrant noyywrap
%option never-interactive
%option warn stack noyy_top_state noyy_pop_state noyy_push_state
%option case-insensitive
%option extra-type="bool"
%option outfile="gen/messagescanner.cpp" header-file="gen/messagescanner.h"

%top{
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

#include <stdint.h>

#define YY_NO_UNISTD_H 1
}

%{
#include <string>

#include "parser.h"
#define yyterminate() return(END)
%}

%x MATCH_STRING_STATE
%x MATCH_RESPONSE_CODE_STATE

%s SUPPORTED_METHODS_STATE
%s CONTENT_TYPE_STATE
%s SESSION_STATE
%s MATCH_PRESENTATION_URL
%s NUM_AS_HEX_MODE
%s MATCH_EDID_STATE

DIGIT [0-9]
DIGITS [0-9]+
HEXDIG [0-9a-fA-F]
HEXDIGITS [0-9a-fA-F]+
SP [ \t]
CR [\r]
LF [\n]
CRLF {CR}{LF}
IPADDRESS (([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])
IPPORT [:]({DIGIT}){1,5}

%%

<*>{CRLF} { BEGIN(INITIAL); }
<*>{LF} { BEGIN(INITIAL); }
<*>{CR} { BEGIN(INITIAL); }

 /* Convert these */
"="  { return '='; }
"-"  { return '-'; }
","  { return ','; }
"\*" { return '*'; }
";"  { return ';'; }
":"  { return ':'; }
"/"  { return '/'; }

{SP}+ return WFD_SP;

"none" return WFD_NONE;
"LPCM" return WFD_AUDIO_CODEC_LPCM;
"AAC" return WFD_AUDIO_CODEC_AAC;
"AC3" return WFD_AUDIO_CODEC_AC3;
"HDCP2.0" return WFD_HDCP_SPEC_2_0;
"HDCP2.1" return WFD_HDCP_SPEC_2_1;
"port=" return WFD_IP_PORT;
"RTP/AVP/UDP;unicast" return WFD_STREAM_PROFILE;
"mode=play" return WFD_MODE_PLAY;
"primary" return WFD_ROUTE_PRIMARY;
"secondary" return WFD_ROUTE_SECONDARY;
"input_category_list=" return WFD_INPUT_CATEGORY_LIST;
"GENERIC" return WFD_INPUT_CATEGORY_GENERIC;
"HIDC" return WFD_INPUT_CATEGORY_HIDC;
"generic_cap_list=" return WFD_GENERIC_CAP_LIST;
"Keyboard" return WFD_INPUT_TYPE_KEYBOARD;
"Mouse" return WFD_INPUT_TYPE_MOUSE;
"SingleTouch" return WFD_INPUT_TYPE_SINGLE_TOUCH;
"MultiTouch" return WFD_INPUT_TYPE_MULTI_TOUCH;
"Joystick" return WFD_INPUT_TYPE_JOYSTICK;
"Camera" return WFD_INPUT_TYPE_CAMERA;
"Gesture" return WFD_INPUT_TYPE_GESTURE;
"RemoteControl" return WFD_INPUT_TYPE_REMOTE_CONTROL;
"hidc_cap_list=" return WFD_HIDC_CAP_LIST;
"Infrared" return WFD_INPUT_PATH_INFRARED;
"USB" return WFD_INPUT_PATH_USB;
"BT" return WFD_INPUT_PATH_BT;
"Wi-Fi" return WFD_INPUT_PATH_WIFI;
"Zigbee" return WFD_INPUT_PATH_ZIGBEE;
"No-SP" return WFD_INPUT_PATH_NOSP;
"disable" return WFD_UIBC_SETTING_DISABLE;
"enable" return WFD_UIBC_SETTING_ENABLE;
"supported" return WFD_SUPPORTED;

<NUM_AS_HEX_MODE>{DIGITS} {
    std::string str(yytext, yyleng);
    str += '\0';
    errno = 0;
    yylval->nval = strtoull(str.c_str(), NULL, 16);
    if (errno)
      yyterminate();
    return WFD_NUM;
  }

<NUM_AS_HEX_MODE>{HEXDIGITS} {
    std::string str(yytext, yyleng);
    str += '\0';
    errno = 0;
    yylval->nval = strtoull(str.c_str(), NULL, 16);
    if (errno)
      yyterminate();
    return WFD_NUM;
  }

<MATCH_EDID_STATE>{DIGITS} {
    yylval->sval = new std::string(yytext);
    return WFD_STRING;
  }

<MATCH_EDID_STATE>{HEXDIGITS} {
    yylval->sval = new std::string(yytext);
    return WFD_STRING;
  }

^"wfd_audio_codecs" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_AUDIO_CODECS;
  }

^"wfd_video_formats" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_VIDEO_FORMATS;
  }

^"wfd_3d_video_formats" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_3D_FORMATS;
  }

^"wfd_content_protection" {
    return WFD_CONTENT_PROTECTION;
  }

^"wfd_display_edid" {
    BEGIN(MATCH_EDID_STATE);
    return WFD_DISPLAY_EDID;
  }

^"wfd_coupled_sink" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_COUPLED_SINK;
  }

^"wfd_trigger_method" {
    BEGIN(SUPPORTED_METHODS_STATE);
    return WFD_TRIGGER_METHOD;
  }

^"wfd_presentation_url" {
    BEGIN(MATCH_PRESENTATION_URL);
    return WFD_PRESENTATION_URL;
  }

^"wfd_client_rtp_ports" {
    return WFD_CLIENT_RTP_PORTS;
  }

^"wfd_route" {
    return WFD_ROUTE;
  }

^"wfd_I2C" {
    return WFD_I2C;
  }

^"wfd_av_format_change_timing" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_AV_FORMAT_CHANGE_TIMING;
  }

^"wfd_preferred_display_mode" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_PREFERRED_DISPLAY_MODE;
  }

^"wfd_uibc_capability" {
    return WFD_UIBC_CAPABILITY;
  }

^"wfd_uibc_setting" {
    return WFD_UIBC_SETTING;
  }

^"wfd_standby_resume_capability" {
    return WFD_STANDBY_RESUME_CAPABILITY;
  }

^"wfd_standby" {
    if (yyextra)  // Is reply.
      return WFD_STANDBY_IN_RESPONSE;
    return WFD_STANDBY_IN_REQUEST;
  }

^"wfd_connector_type" {
    BEGIN(NUM_AS_HEX_MODE);
    return WFD_CONNECTOR_TYPE;
  }

^"wfd_idr_request" {
    return WFD_IDR_REQUEST;
  }

^[[:alpha:]][[:alnum:]\-\_]* {
    BEGIN(MATCH_STRING_STATE);
    yylval->sval = new std::string(yytext, yyleng);
    return WFD_GENERIC_PROPERTY;
  }

{DIGITS} {
    std::string str(yytext, yyleng);
    str += '\0';
    errno = 0;
    yylval->nval = strtoull(str.c_str(), NULL, 10);
    if (errno)
      yyterminate();
    return WFD_NUM;
  }

<MATCH_STRING_STATE>[^ :\r\n][^\r\n]+/"\r\n" {
    BEGIN(INITIAL);
    yylval->sval = new std::string(yytext);
    return WFD_STRING;
  }

<SUPPORTED_METHODS_STATE>"OPTIONS" {
    return WFD_OPTIONS;
  }

<SUPPORTED_METHODS_STATE>"SET_PARAMETER" {
    return WFD_SET_PARAMETER;
  }

<SUPPORTED_METHODS_STATE>"GET_PARAMETER" {
    return WFD_GET_PARAMETER;
  }

<SUPPORTED_METHODS_STATE>"SETUP" {
    return WFD_SETUP;
  }

<SUPPORTED_METHODS_STATE>"PLAY" {
    return WFD_PLAY;
  }

<SUPPORTED_METHODS_STATE>"TEARDOWN" {
    return WFD_TEARDOWN;
  }

<SUPPORTED_METHODS_STATE>"PAUSE" {
    return WFD_PAUSE;
  }
     
<SUPPORTED_METHODS_STATE>"org.wfa.wfd1.0" {
  return WFD_TAG;
  }

<MATCH_PRESENTATION_URL>"rtsp://"[^ \t\n]+{IPADDRESS}{IPPORT}?"/wfd1.0/streamid=0" {
    yylval->sval = new std::string(yytext);
    return WFD_PRESENTATION_URL_0;
  }

<MATCH_PRESENTATION_URL>"rtsp://"[^ \t\n]+{IPADDRESS}{IPPORT}?"/wfd1.0/streamid=1" {
    yylval->sval = new std::string(yytext);
    return WFD_PRESENTATION_URL_1;
  }

 /* all unmatched */
<*>. {}
%%
