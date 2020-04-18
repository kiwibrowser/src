%option nodefault nounput nomain
%option bison-bridge reentrant noyywrap
%option never-interactive
%option warn stack noyy_top_state noyy_pop_state noyy_push_state
%option case-insensitive
%option extra-type="bool"
%option outfile="gen/headerscanner.cpp" header-file="gen/headerscanner.h"

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

DIGIT [0-9]
DIGITS [0-9]+
HEXDIG [0-9a-fA-F]
HEXDIGITS [0-9a-fA-F]+
SP [ \t]
CR [\r]
LF [\n]
CRLF {CR}{LF}

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

";timeout=" return WFD_TIMEOUT;
";server_port=" return WFD_SERVER_PORT;

^(?-i:"OPTIONS") {
    BEGIN(INITIAL);
    return WFD_OPTIONS;
  }

^(?-i:"SET_PARAMETER") {
    BEGIN(INITIAL);
    return WFD_SET_PARAMETER;
  }

^(?-i:"GET_PARAMETER") {
    BEGIN(INITIAL);
    return WFD_GET_PARAMETER;
  }

^(?-i:"SETUP") {
    BEGIN(INITIAL);
    return WFD_SETUP;
  }

^(?-i:"PLAY") {
    BEGIN(INITIAL);
    return WFD_PLAY;
  }

^(?-i:"TEARDOWN") {
    BEGIN(INITIAL);
    return WFD_TEARDOWN;
  }

^(?-i:"PAUSE") {
    BEGIN(INITIAL);
    return WFD_PAUSE;
  }

  /* RTSP response, get reply code, RTSP/1.0 200 OK */
^"RTSP/"{DIGIT}"."{DIGIT}{SP}+ {
    BEGIN(MATCH_RESPONSE_CODE_STATE);
    return WFD_RESPONSE;
  }

  /* CSeq: i */
^"CSeq:" {
    BEGIN(INITIAL);
    return WFD_CSEQ;
  }

^"Public:" {
    BEGIN(SUPPORTED_METHODS_STATE);
    return WFD_RESPONSE_METHODS;
  }

^"Require: org.wfa.wfd1.0" {
    return WFD_SUPPORT_CHECK;
  }

^"Content-Type:" {
    BEGIN(CONTENT_TYPE_STATE);
    return WFD_CONTENT_TYPE;
  }

^"Content-Length:" {
    BEGIN(INITIAL);
    return WFD_CONTENT_LENGTH;
  }

^"Session:" {
    BEGIN(SESSION_STATE);
    return WFD_SESSION;
  }

^"Transport: RTP/AVP/UDP;unicast;client_port=" {
    return WFD_TRANSPORT;
  }

^[[:alpha:]][[:alnum:]\-\_]*":" {
    BEGIN(MATCH_STRING_STATE);
    yylval->sval = new std::string(yytext, yyleng - 1);
    return WFD_HEADER;
  }

<SESSION_STATE>[^ ;\t\r\n]+ {
    BEGIN(INITIAL);
    yylval->sval = new std::string(yytext, yyleng);
    return WFD_SESSION_ID;
  }

<MATCH_RESPONSE_CODE_STATE>{DIGITS} {
    BEGIN(MATCH_STRING_STATE);
    yylval->nval = atoi(yytext);
    return WFD_RESPONSE_CODE;
  }

<MATCH_STRING_STATE>[^ \r\n][^\r\n]+/"\r\n" {
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

<CONTENT_TYPE_STATE>[-[:alnum:]]+\/[-[:alnum:]]+ {
    BEGIN(INITIAL);
    yylval->sval = new std::string(yytext);
    return WFD_MIME;
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

  /* RTSP request rule,  e.g., OPTIONS * RTSP/1.0 */
"RTSP/"{DIGIT}"."{DIGIT} {
    return WFD_END;
  }

  /* GET_PARAMETER rtsp://localhost/wfd1.0 RTSP/1.0 */
"rtsp://"[^ \t\n]+ {
    yylval->sval = new std::string(yytext);
    return WFD_REQUEST_URI;
  }

 /* all unmatched */
<*>. {}
%%
