%option nodefault nounput nomain
%option bison-bridge reentrant noyywrap
%option never-interactive
%option warn stack noyy_top_state noyy_pop_state noyy_push_state
%option case-insensitive
%option extra-type="bool"
%option outfile="gen/errorscanner.cpp" header-file="gen/errorscanner.h"

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

DIGITS [0-9]+
SP [ \t]
CR [\r]
LF [\n]
CRLF {CR}{LF}

%%
  /* skip these */
{CRLF} { }
{LF} { }
{CR} { }

 /* Convert these */
","  { return ','; }
":"  { return ':'; }

{SP}+ return WFD_SP;

^"wfd_audio_codecs" {
    return WFD_AUDIO_CODECS_ERROR;
  }

^"wfd_video_formats" {
    return WFD_VIDEO_FORMATS_ERROR;
  }

^"wfd_3d_video_formats" {
    return WFD_3D_FORMATS_ERROR;
  }

^"wfd_content_protection" {
    return WFD_CONTENT_PROTECTION_ERROR;
  }

^"wfd_display_edid" {
    return WFD_DISPLAY_EDID_ERROR;
  }

^"wfd_coupled_sink" {
    return WFD_COUPLED_SINK_ERROR;
  }

^"wfd_trigger_method" {
    return WFD_TRIGGER_METHOD_ERROR;
  }

^"wfd_presentation_url" {
    return WFD_PRESENTATION_URL_ERROR;
  }

^"wfd_client_rtp_ports" {
    return WFD_CLIENT_RTP_PORTS_ERROR;
  }

^"wfd_route" {
    return WFD_ROUTE_ERROR;
  }

^"wfd_I2C" {
    return WFD_I2C_ERROR;
  }

^"wfd_av_format_change_timing" {
    return WFD_AV_FORMAT_CHANGE_TIMING_ERROR;
  }

^"wfd_preferred_display_mode" {
    return WFD_PREFERRED_DISPLAY_MODE_ERROR;
  }

^"wfd_uibc_capability" {
    return WFD_UIBC_CAPABILITY_ERROR;
  }

^"wfd_uibc_setting" {
    return WFD_UIBC_SETTING_ERROR;
  }

^"wfd_standby_resume_capability" {
    return WFD_STANDBY_RESUME_CAPABILITY_ERROR;
  }

^"wfd_standby" {
    return WFD_STANDBY_ERROR;
  }

^"wfd_connector_type" {
    return WFD_CONNECTOR_TYPE_ERROR;
  }

^"wfd_idr_request" {
    return WFD_IDR_REQUEST_ERROR;
  }

^[[:alpha:]][[:alnum:]\-\_]* {
    yylval->sval = new std::string(yytext, yyleng);
    return WFD_GENERIC_PROPERTY_ERROR;
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

 /* all unmatched */
<*>. {}
%%
