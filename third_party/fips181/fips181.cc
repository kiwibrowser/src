/*
** This module uses code from the NIST implementation of  FIPS-181,
** but the algorythm is CHANGED and I think that I CAN
** copyright it. See copiright notes below.
*/

/*
** Copyright (c) 1999, 2000, 2001, 2002, 2003
** Adel I. Mirzazhanov. All rights reserved
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 
**     1.Redistributions of source code must retain the above copyright notice,
**       this list of conditions and the following disclaimer. 
**     2.Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution. 
**     3.The name of the author may not be used to endorse or promote products
**       derived from this software without specific prior written permission. 
** 		  
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND ANY EXPRESS
** OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN  NO  EVENT  SHALL THE AUTHOR BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE
** GOODS OR SERVICES;  LOSS OF USE,  DATA,  OR  PROFITS;  OR BUSINESS
** INTERRUPTION)  HOWEVER  CAUSED  AND  ON  ANY  THEORY OF LIABILITY,
** WHETHER  IN  CONTRACT,   STRICT   LIABILITY,  OR  TORT  (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32) && !defined(__WIN32__)
#include <strings.h> 
#endif
#include <time.h>
#include <sys/types.h>
#include "base/rand_util.h"
#include "fips181.h"
#include "randpass.h"
#include "convert.h"

struct unit
{
    char    unit_code[5];
    unsigned short  flags;
};

static struct unit  rules[] =
{   {"a", VOWEL},
    {"b", NO_SPECIAL_RULE},
    {"c", NO_SPECIAL_RULE},
    {"d", NO_SPECIAL_RULE},
    {"e", NO_FINAL_SPLIT | VOWEL},
    {"f", NO_SPECIAL_RULE},
    {"g", NO_SPECIAL_RULE},
    {"h", NO_SPECIAL_RULE},
    {"i", VOWEL},
    {"j", NO_SPECIAL_RULE},
    {"k", NO_SPECIAL_RULE},
    {"l", NO_SPECIAL_RULE},
    {"m", NO_SPECIAL_RULE},
    {"n", NO_SPECIAL_RULE},
    {"o", VOWEL},
    {"p", NO_SPECIAL_RULE},
    {"r", NO_SPECIAL_RULE},
    {"s", NO_SPECIAL_RULE},
    {"t", NO_SPECIAL_RULE},
    {"u", VOWEL},
    {"v", NO_SPECIAL_RULE},
    {"w", NO_SPECIAL_RULE},
    {"x", NOT_BEGIN_SYLLABLE},
    {"y", ALTERNATE_VOWEL | VOWEL},
    {"z", NO_SPECIAL_RULE},
    {"ch", NO_SPECIAL_RULE},
    {"gh", NO_SPECIAL_RULE},
    {"ph", NO_SPECIAL_RULE},
    {"rh", NO_SPECIAL_RULE},
    {"sh", NO_SPECIAL_RULE},
    {"th", NO_SPECIAL_RULE},
    {"wh", NO_SPECIAL_RULE},
    {"qu", NO_SPECIAL_RULE},
    {"ck", NOT_BEGIN_SYLLABLE}
};

static int  digram[][RULE_SIZE] =
{
    {/* aa */ ILLEGAL_PAIR,
     /* ab */ ANY_COMBINATION,
     /* ac */ ANY_COMBINATION,
     /* ad */ ANY_COMBINATION,
     /* ae */ ILLEGAL_PAIR,
     /* af */ ANY_COMBINATION,
     /* ag */ ANY_COMBINATION,
     /* ah */ NOT_BEGIN | BREAK | NOT_END,
     /* ai */ ANY_COMBINATION,
     /* aj */ ANY_COMBINATION,
     /* ak */ ANY_COMBINATION,
     /* al */ ANY_COMBINATION,
     /* am */ ANY_COMBINATION,
     /* an */ ANY_COMBINATION,
     /* ao */ ILLEGAL_PAIR,
     /* ap */ ANY_COMBINATION,
     /* ar */ ANY_COMBINATION,
     /* as */ ANY_COMBINATION,
     /* at */ ANY_COMBINATION,
     /* au */ ANY_COMBINATION,
     /* av */ ANY_COMBINATION,
     /* aw */ ANY_COMBINATION,
     /* ax */ ANY_COMBINATION,
     /* ay */ ANY_COMBINATION,
     /* az */ ANY_COMBINATION,
     /* ach */ ANY_COMBINATION,
     /* agh */ ILLEGAL_PAIR,
     /* aph */ ANY_COMBINATION,
     /* arh */ ILLEGAL_PAIR,
     /* ash */ ANY_COMBINATION,
     /* ath */ ANY_COMBINATION,
     /* awh */ ILLEGAL_PAIR,
     /* aqu */ BREAK | NOT_END,
     /* ack */ ANY_COMBINATION},
    {/* ba */ ANY_COMBINATION,
     /* bb */ NOT_BEGIN | BREAK | NOT_END,
     /* bc */ NOT_BEGIN | BREAK | NOT_END,
     /* bd */ NOT_BEGIN | BREAK | NOT_END,
     /* be */ ANY_COMBINATION,
     /* bf */ NOT_BEGIN | BREAK | NOT_END,
     /* bg */ NOT_BEGIN | BREAK | NOT_END,
     /* bh */ NOT_BEGIN | BREAK | NOT_END,
     /* bi */ ANY_COMBINATION,
     /* bj */ NOT_BEGIN | BREAK | NOT_END,
     /* bk */ NOT_BEGIN | BREAK | NOT_END,
     /* bl */ BEGIN | SUFFIX | NOT_END,
     /* bm */ NOT_BEGIN | BREAK | NOT_END,
     /* bn */ NOT_BEGIN | BREAK | NOT_END,
     /* bo */ ANY_COMBINATION,
     /* bp */ NOT_BEGIN | BREAK | NOT_END,
     /* br */ BEGIN | END,
     /* bs */ NOT_BEGIN,
     /* bt */ NOT_BEGIN | BREAK | NOT_END,
     /* bu */ ANY_COMBINATION,
     /* bv */ NOT_BEGIN | BREAK | NOT_END,
     /* bw */ NOT_BEGIN | BREAK | NOT_END,
     /* bx */ ILLEGAL_PAIR,
     /* by */ ANY_COMBINATION,
     /* bz */ NOT_BEGIN | BREAK | NOT_END,
     /* bch */ NOT_BEGIN | BREAK | NOT_END,
     /* bgh */ ILLEGAL_PAIR,
     /* bph */ NOT_BEGIN | BREAK | NOT_END,
     /* brh */ ILLEGAL_PAIR,
     /* bsh */ NOT_BEGIN | BREAK | NOT_END,
     /* bth */ NOT_BEGIN | BREAK | NOT_END,
     /* bwh */ ILLEGAL_PAIR,
     /* bqu */ NOT_BEGIN | BREAK | NOT_END,
     /* bck */ ILLEGAL_PAIR },
    {/* ca */ ANY_COMBINATION,
     /* cb */ NOT_BEGIN | BREAK | NOT_END,
     /* cc */ NOT_BEGIN | BREAK | NOT_END,
     /* cd */ NOT_BEGIN | BREAK | NOT_END,
     /* ce */ ANY_COMBINATION,
     /* cf */ NOT_BEGIN | BREAK | NOT_END,
     /* cg */ NOT_BEGIN | BREAK | NOT_END,
     /* ch */ NOT_BEGIN | BREAK | NOT_END,
     /* ci */ ANY_COMBINATION,
     /* cj */ NOT_BEGIN | BREAK | NOT_END,
     /* ck */ NOT_BEGIN | BREAK | NOT_END,
     /* cl */ SUFFIX | NOT_END,
     /* cm */ NOT_BEGIN | BREAK | NOT_END,
     /* cn */ NOT_BEGIN | BREAK | NOT_END,
     /* co */ ANY_COMBINATION,
     /* cp */ NOT_BEGIN | BREAK | NOT_END,
     /* cr */ NOT_END,
     /* cs */ NOT_BEGIN | END,
     /* ct */ NOT_BEGIN | PREFIX,
     /* cu */ ANY_COMBINATION,
     /* cv */ NOT_BEGIN | BREAK | NOT_END,
     /* cw */ NOT_BEGIN | BREAK | NOT_END,
     /* cx */ ILLEGAL_PAIR,
     /* cy */ ANY_COMBINATION,
     /* cz */ NOT_BEGIN | BREAK | NOT_END,
     /* cch */ ILLEGAL_PAIR,
     /* cgh */ ILLEGAL_PAIR,
     /* cph */ NOT_BEGIN | BREAK | NOT_END,
     /* crh */ ILLEGAL_PAIR,
     /* csh */ NOT_BEGIN | BREAK | NOT_END,
     /* cth */ NOT_BEGIN | BREAK | NOT_END,
     /* cwh */ ILLEGAL_PAIR,
     /* cqu */ NOT_BEGIN | SUFFIX | NOT_END,
     /* cck */ ILLEGAL_PAIR},
    {/* da */ ANY_COMBINATION,
     /* db */ NOT_BEGIN | BREAK | NOT_END,
     /* dc */ NOT_BEGIN | BREAK | NOT_END,
     /* dd */ NOT_BEGIN,
     /* de */ ANY_COMBINATION,
     /* df */ NOT_BEGIN | BREAK | NOT_END,
     /* dg */ NOT_BEGIN | BREAK | NOT_END,
     /* dh */ NOT_BEGIN | BREAK | NOT_END,
     /* di */ ANY_COMBINATION,
     /* dj */ NOT_BEGIN | BREAK | NOT_END,
     /* dk */ NOT_BEGIN | BREAK | NOT_END,
     /* dl */ NOT_BEGIN | BREAK | NOT_END,
     /* dm */ NOT_BEGIN | BREAK | NOT_END,
     /* dn */ NOT_BEGIN | BREAK | NOT_END,
     /* do */ ANY_COMBINATION,
     /* dp */ NOT_BEGIN | BREAK | NOT_END,
     /* dr */ BEGIN | NOT_END,
     /* ds */ NOT_BEGIN | END,
     /* dt */ NOT_BEGIN | BREAK | NOT_END,
     /* du */ ANY_COMBINATION,
     /* dv */ NOT_BEGIN | BREAK | NOT_END,
     /* dw */ NOT_BEGIN | BREAK | NOT_END,
     /* dx */ ILLEGAL_PAIR,
     /* dy */ ANY_COMBINATION,
     /* dz */ NOT_BEGIN | BREAK | NOT_END,
     /* dch */ NOT_BEGIN | BREAK | NOT_END,
     /* dgh */ NOT_BEGIN | BREAK | NOT_END,
     /* dph */ NOT_BEGIN | BREAK | NOT_END,
     /* drh */ ILLEGAL_PAIR,
     /* dsh */ NOT_BEGIN | NOT_END,
     /* dth */ NOT_BEGIN | PREFIX,
     /* dwh */ ILLEGAL_PAIR,
     /* dqu */ NOT_BEGIN | BREAK | NOT_END,
     /* dck */ ILLEGAL_PAIR },
    {/* ea */ ANY_COMBINATION,
     /* eb */ ANY_COMBINATION,
     /* ec */ ANY_COMBINATION,
     /* ed */ ANY_COMBINATION,
     /* ee */ ANY_COMBINATION,
     /* ef */ ANY_COMBINATION,
     /* eg */ ANY_COMBINATION,
     /* eh */ NOT_BEGIN | BREAK | NOT_END,
     /* ei */ NOT_END,
     /* ej */ ANY_COMBINATION,
     /* ek */ ANY_COMBINATION,
     /* el */ ANY_COMBINATION,
     /* em */ ANY_COMBINATION,
     /* en */ ANY_COMBINATION,
     /* eo */ BREAK,
     /* ep */ ANY_COMBINATION,
     /* er */ ANY_COMBINATION,
     /* es */ ANY_COMBINATION,
     /* et */ ANY_COMBINATION,
     /* eu */ ANY_COMBINATION,
     /* ev */ ANY_COMBINATION,
     /* ew */ ANY_COMBINATION,
     /* ex */ ANY_COMBINATION,
     /* ey */ ANY_COMBINATION,
     /* ez */ ANY_COMBINATION,
     /* ech */ ANY_COMBINATION,
     /* egh */ NOT_BEGIN | BREAK | NOT_END,
     /* eph */ ANY_COMBINATION,
     /* erh */ ILLEGAL_PAIR,
     /* esh */ ANY_COMBINATION,
     /* eth */ ANY_COMBINATION,
     /* ewh */ ILLEGAL_PAIR,
     /* equ */ BREAK | NOT_END,
     /* eck */ ANY_COMBINATION },
    {/* fa */ ANY_COMBINATION,
     /* fb */ NOT_BEGIN | BREAK | NOT_END,
     /* fc */ NOT_BEGIN | BREAK | NOT_END,
     /* fd */ NOT_BEGIN | BREAK | NOT_END,
     /* fe */ ANY_COMBINATION,
     /* ff */ NOT_BEGIN,
     /* fg */ NOT_BEGIN | BREAK | NOT_END,
     /* fh */ NOT_BEGIN | BREAK | NOT_END,
     /* fi */ ANY_COMBINATION,
     /* fj */ NOT_BEGIN | BREAK | NOT_END,
     /* fk */ NOT_BEGIN | BREAK | NOT_END,
     /* fl */ BEGIN | SUFFIX | NOT_END,
     /* fm */ NOT_BEGIN | BREAK | NOT_END,
     /* fn */ NOT_BEGIN | BREAK | NOT_END,
     /* fo */ ANY_COMBINATION,
     /* fp */ NOT_BEGIN | BREAK | NOT_END,
     /* fr */ BEGIN | NOT_END,
     /* fs */ NOT_BEGIN,
     /* ft */ NOT_BEGIN,
     /* fu */ ANY_COMBINATION,
     /* fv */ NOT_BEGIN | BREAK | NOT_END,
     /* fw */ NOT_BEGIN | BREAK | NOT_END,
     /* fx */ ILLEGAL_PAIR,
     /* fy */ NOT_BEGIN,
     /* fz */ NOT_BEGIN | BREAK | NOT_END,
     /* fch */ NOT_BEGIN | BREAK | NOT_END,
     /* fgh */ NOT_BEGIN | BREAK | NOT_END,
     /* fph */ NOT_BEGIN | BREAK | NOT_END,
     /* frh */ ILLEGAL_PAIR,
     /* fsh */ NOT_BEGIN | BREAK | NOT_END,
     /* fth */ NOT_BEGIN | BREAK | NOT_END,
     /* fwh */ ILLEGAL_PAIR,
     /* fqu */ NOT_BEGIN | BREAK | NOT_END,
     /* fck */ ILLEGAL_PAIR },
    {/* ga */ ANY_COMBINATION,
     /* gb */ NOT_BEGIN | BREAK | NOT_END,
     /* gc */ NOT_BEGIN | BREAK | NOT_END,
     /* gd */ NOT_BEGIN | BREAK | NOT_END,
     /* ge */ ANY_COMBINATION,
     /* gf */ NOT_BEGIN | BREAK | NOT_END,
     /* gg */ NOT_BEGIN,
     /* gh */ NOT_BEGIN | BREAK | NOT_END,
     /* gi */ ANY_COMBINATION,
     /* gj */ NOT_BEGIN | BREAK | NOT_END,
     /* gk */ ILLEGAL_PAIR,
     /* gl */ BEGIN | SUFFIX | NOT_END,
     /* gm */ NOT_BEGIN | BREAK | NOT_END,
     /* gn */ NOT_BEGIN | BREAK | NOT_END,
     /* go */ ANY_COMBINATION,
     /* gp */ NOT_BEGIN | BREAK | NOT_END,
     /* gr */ BEGIN | NOT_END,
     /* gs */ NOT_BEGIN | END,
     /* gt */ NOT_BEGIN | BREAK | NOT_END,
     /* gu */ ANY_COMBINATION,
     /* gv */ NOT_BEGIN | BREAK | NOT_END,
     /* gw */ NOT_BEGIN | BREAK | NOT_END,
     /* gx */ ILLEGAL_PAIR,
     /* gy */ NOT_BEGIN,
     /* gz */ NOT_BEGIN | BREAK | NOT_END,
     /* gch */ NOT_BEGIN | BREAK | NOT_END,
     /* ggh */ ILLEGAL_PAIR,
     /* gph */ NOT_BEGIN | BREAK | NOT_END,
     /* grh */ ILLEGAL_PAIR,
     /* gsh */ NOT_BEGIN,
     /* gth */ NOT_BEGIN,
     /* gwh */ ILLEGAL_PAIR,
     /* gqu */ NOT_BEGIN | BREAK | NOT_END,
     /* gck */ ILLEGAL_PAIR },
    {/* ha */ ANY_COMBINATION,
     /* hb */ NOT_BEGIN | BREAK | NOT_END,
     /* hc */ NOT_BEGIN | BREAK | NOT_END,
     /* hd */ NOT_BEGIN | BREAK | NOT_END,
     /* he */ ANY_COMBINATION,
     /* hf */ NOT_BEGIN | BREAK | NOT_END,
     /* hg */ NOT_BEGIN | BREAK | NOT_END,
     /* hh */ ILLEGAL_PAIR,
     /* hi */ ANY_COMBINATION,
     /* hj */ NOT_BEGIN | BREAK | NOT_END,
     /* hk */ NOT_BEGIN | BREAK | NOT_END,
     /* hl */ NOT_BEGIN | BREAK | NOT_END,
     /* hm */ NOT_BEGIN | BREAK | NOT_END,
     /* hn */ NOT_BEGIN | BREAK | NOT_END,
     /* ho */ ANY_COMBINATION,
     /* hp */ NOT_BEGIN | BREAK | NOT_END,
     /* hr */ NOT_BEGIN | BREAK | NOT_END,
     /* hs */ NOT_BEGIN | BREAK | NOT_END,
     /* ht */ NOT_BEGIN | BREAK | NOT_END,
     /* hu */ ANY_COMBINATION,
     /* hv */ NOT_BEGIN | BREAK | NOT_END,
     /* hw */ NOT_BEGIN | BREAK | NOT_END,
     /* hx */ ILLEGAL_PAIR,
     /* hy */ ANY_COMBINATION,
     /* hz */ NOT_BEGIN | BREAK | NOT_END,
     /* hch */ NOT_BEGIN | BREAK | NOT_END,
     /* hgh */ NOT_BEGIN | BREAK | NOT_END,
     /* hph */ NOT_BEGIN | BREAK | NOT_END,
     /* hrh */ ILLEGAL_PAIR,
     /* hsh */ NOT_BEGIN | BREAK | NOT_END,
     /* hth */ NOT_BEGIN | BREAK | NOT_END,
     /* hwh */ ILLEGAL_PAIR,
     /* hqu */ NOT_BEGIN | BREAK | NOT_END,
     /* hck */ ILLEGAL_PAIR },
    {/* ia */ ANY_COMBINATION,
     /* ib */ ANY_COMBINATION,
     /* ic */ ANY_COMBINATION,
     /* id */ ANY_COMBINATION,
     /* ie */ NOT_BEGIN,
     /* if */ ANY_COMBINATION,
     /* ig */ ANY_COMBINATION,
     /* ih */ NOT_BEGIN | BREAK | NOT_END,
     /* ii */ ILLEGAL_PAIR,
     /* ij */ ANY_COMBINATION,
     /* ik */ ANY_COMBINATION,
     /* il */ ANY_COMBINATION,
     /* im */ ANY_COMBINATION,
     /* in */ ANY_COMBINATION,
     /* io */ BREAK,
     /* ip */ ANY_COMBINATION,
     /* ir */ ANY_COMBINATION,
     /* is */ ANY_COMBINATION,
     /* it */ ANY_COMBINATION,
     /* iu */ NOT_BEGIN | BREAK | NOT_END,
     /* iv */ ANY_COMBINATION,
     /* iw */ NOT_BEGIN | BREAK | NOT_END,
     /* ix */ ANY_COMBINATION,
     /* iy */ NOT_BEGIN | BREAK | NOT_END,
     /* iz */ ANY_COMBINATION,
     /* ich */ ANY_COMBINATION,
     /* igh */ NOT_BEGIN,
     /* iph */ ANY_COMBINATION,
     /* irh */ ILLEGAL_PAIR,
     /* ish */ ANY_COMBINATION,
     /* ith */ ANY_COMBINATION,
     /* iwh */ ILLEGAL_PAIR,
     /* iqu */ BREAK | NOT_END,
     /* ick */ ANY_COMBINATION },
    {/* ja */ ANY_COMBINATION,
     /* jb */ NOT_BEGIN | BREAK | NOT_END,
     /* jc */ NOT_BEGIN | BREAK | NOT_END,
     /* jd */ NOT_BEGIN | BREAK | NOT_END,
     /* je */ ANY_COMBINATION,
     /* jf */ NOT_BEGIN | BREAK | NOT_END,
     /* jg */ ILLEGAL_PAIR,
     /* jh */ NOT_BEGIN | BREAK | NOT_END,
     /* ji */ ANY_COMBINATION,
     /* jj */ ILLEGAL_PAIR,
     /* jk */ NOT_BEGIN | BREAK | NOT_END,
     /* jl */ NOT_BEGIN | BREAK | NOT_END,
     /* jm */ NOT_BEGIN | BREAK | NOT_END,
     /* jn */ NOT_BEGIN | BREAK | NOT_END,
     /* jo */ ANY_COMBINATION,
     /* jp */ NOT_BEGIN | BREAK | NOT_END,
     /* jr */ NOT_BEGIN | BREAK | NOT_END,
     /* js */ NOT_BEGIN | BREAK | NOT_END,
     /* jt */ NOT_BEGIN | BREAK | NOT_END,
     /* ju */ ANY_COMBINATION,
     /* jv */ NOT_BEGIN | BREAK | NOT_END,
     /* jw */ NOT_BEGIN | BREAK | NOT_END,
     /* jx */ ILLEGAL_PAIR,
     /* jy */ NOT_BEGIN,
     /* jz */ NOT_BEGIN | BREAK | NOT_END,
     /* jch */ NOT_BEGIN | BREAK | NOT_END,
     /* jgh */ NOT_BEGIN | BREAK | NOT_END,
     /* jph */ NOT_BEGIN | BREAK | NOT_END,
     /* jrh */ ILLEGAL_PAIR,
     /* jsh */ NOT_BEGIN | BREAK | NOT_END,
     /* jth */ NOT_BEGIN | BREAK | NOT_END,
     /* jwh */ ILLEGAL_PAIR,
     /* jqu */ NOT_BEGIN | BREAK | NOT_END,
     /* jck */ ILLEGAL_PAIR },
    {/* ka */ ANY_COMBINATION,
     /* kb */ NOT_BEGIN | BREAK | NOT_END,
     /* kc */ NOT_BEGIN | BREAK | NOT_END,
     /* kd */ NOT_BEGIN | BREAK | NOT_END,
     /* ke */ ANY_COMBINATION,
     /* kf */ NOT_BEGIN | BREAK | NOT_END,
     /* kg */ NOT_BEGIN | BREAK | NOT_END,
     /* kh */ NOT_BEGIN | BREAK | NOT_END,
     /* ki */ ANY_COMBINATION,
     /* kj */ NOT_BEGIN | BREAK | NOT_END,
     /* kk */ NOT_BEGIN | BREAK | NOT_END,
     /* kl */ SUFFIX | NOT_END,
     /* km */ NOT_BEGIN | BREAK | NOT_END,
     /* kn */ BEGIN | SUFFIX | NOT_END,
     /* ko */ ANY_COMBINATION,
     /* kp */ NOT_BEGIN | BREAK | NOT_END,
     /* kr */ SUFFIX | NOT_END,
     /* ks */ NOT_BEGIN | END,
     /* kt */ NOT_BEGIN | BREAK | NOT_END,
     /* ku */ ANY_COMBINATION,
     /* kv */ NOT_BEGIN | BREAK | NOT_END,
     /* kw */ NOT_BEGIN | BREAK | NOT_END,
     /* kx */ ILLEGAL_PAIR,
     /* ky */ NOT_BEGIN,
     /* kz */ NOT_BEGIN | BREAK | NOT_END,
     /* kch */ NOT_BEGIN | BREAK | NOT_END,
     /* kgh */ NOT_BEGIN | BREAK | NOT_END,
     /* kph */ NOT_BEGIN | PREFIX,
     /* krh */ ILLEGAL_PAIR,
     /* ksh */ NOT_BEGIN,
     /* kth */ NOT_BEGIN | BREAK | NOT_END,
     /* kwh */ ILLEGAL_PAIR,
     /* kqu */ NOT_BEGIN | BREAK | NOT_END,
     /* kck */ ILLEGAL_PAIR },
    {/* la */ ANY_COMBINATION,
     /* lb */ NOT_BEGIN | PREFIX,
     /* lc */ NOT_BEGIN | BREAK | NOT_END,
     /* ld */ NOT_BEGIN | PREFIX,
     /* le */ ANY_COMBINATION,
     /* lf */ NOT_BEGIN | PREFIX,
     /* lg */ NOT_BEGIN | PREFIX,
     /* lh */ NOT_BEGIN | BREAK | NOT_END,
     /* li */ ANY_COMBINATION,
     /* lj */ NOT_BEGIN | PREFIX,
     /* lk */ NOT_BEGIN | PREFIX,
     /* ll */ NOT_BEGIN | PREFIX,
     /* lm */ NOT_BEGIN | PREFIX,
     /* ln */ NOT_BEGIN | BREAK | NOT_END,
     /* lo */ ANY_COMBINATION,
     /* lp */ NOT_BEGIN | PREFIX,
     /* lr */ NOT_BEGIN | BREAK | NOT_END,
     /* ls */ NOT_BEGIN,
     /* lt */ NOT_BEGIN | PREFIX,
     /* lu */ ANY_COMBINATION,
     /* lv */ NOT_BEGIN | PREFIX,
     /* lw */ NOT_BEGIN | BREAK | NOT_END,
     /* lx */ ILLEGAL_PAIR,
     /* ly */ ANY_COMBINATION,
     /* lz */ NOT_BEGIN | BREAK | NOT_END,
     /* lch */ NOT_BEGIN | PREFIX,
     /* lgh */ NOT_BEGIN | BREAK | NOT_END,
     /* lph */ NOT_BEGIN | PREFIX,
     /* lrh */ ILLEGAL_PAIR,
     /* lsh */ NOT_BEGIN | PREFIX,
     /* lth */ NOT_BEGIN | PREFIX,
     /* lwh */ ILLEGAL_PAIR,
     /* lqu */ NOT_BEGIN | BREAK | NOT_END,
     /* lck */ ILLEGAL_PAIR },
    {/* ma */ ANY_COMBINATION,
     /* mb */ NOT_BEGIN | BREAK | NOT_END,
     /* mc */ NOT_BEGIN | BREAK | NOT_END,
     /* md */ NOT_BEGIN | BREAK | NOT_END,
     /* me */ ANY_COMBINATION,
     /* mf */ NOT_BEGIN | BREAK | NOT_END,
     /* mg */ NOT_BEGIN | BREAK | NOT_END,
     /* mh */ NOT_BEGIN | BREAK | NOT_END,
     /* mi */ ANY_COMBINATION,
     /* mj */ NOT_BEGIN | BREAK | NOT_END,
     /* mk */ NOT_BEGIN | BREAK | NOT_END,
     /* ml */ NOT_BEGIN | BREAK | NOT_END,
     /* mm */ NOT_BEGIN,
     /* mn */ NOT_BEGIN | BREAK | NOT_END,
     /* mo */ ANY_COMBINATION,
     /* mp */ NOT_BEGIN,
     /* mr */ NOT_BEGIN | BREAK | NOT_END,
     /* ms */ NOT_BEGIN,
     /* mt */ NOT_BEGIN,
     /* mu */ ANY_COMBINATION,
     /* mv */ NOT_BEGIN | BREAK | NOT_END,
     /* mw */ NOT_BEGIN | BREAK | NOT_END,
     /* mx */ ILLEGAL_PAIR,
     /* my */ ANY_COMBINATION,
     /* mz */ NOT_BEGIN | BREAK | NOT_END,
     /* mch */ NOT_BEGIN | PREFIX,
     /* mgh */ NOT_BEGIN | BREAK | NOT_END,
     /* mph */ NOT_BEGIN,
     /* mrh */ ILLEGAL_PAIR,
     /* msh */ NOT_BEGIN,
     /* mth */ NOT_BEGIN,
     /* mwh */ ILLEGAL_PAIR,
     /* mqu */ NOT_BEGIN | BREAK | NOT_END,
     /* mck */ ILLEGAL_PAIR },
    {/* na */ ANY_COMBINATION,
     /* nb */ NOT_BEGIN | BREAK | NOT_END,
     /* nc */ NOT_BEGIN | BREAK | NOT_END,
     /* nd */ NOT_BEGIN,
     /* ne */ ANY_COMBINATION,
     /* nf */ NOT_BEGIN | BREAK | NOT_END,
     /* ng */ NOT_BEGIN | PREFIX,
     /* nh */ NOT_BEGIN | BREAK | NOT_END,
     /* ni */ ANY_COMBINATION,
     /* nj */ NOT_BEGIN | BREAK | NOT_END,
     /* nk */ NOT_BEGIN | PREFIX,
     /* nl */ NOT_BEGIN | BREAK | NOT_END,
     /* nm */ NOT_BEGIN | BREAK | NOT_END,
     /* nn */ NOT_BEGIN,
     /* no */ ANY_COMBINATION,
     /* np */ NOT_BEGIN | BREAK | NOT_END,
     /* nr */ NOT_BEGIN | BREAK | NOT_END,
     /* ns */ NOT_BEGIN,
     /* nt */ NOT_BEGIN,
     /* nu */ ANY_COMBINATION,
     /* nv */ NOT_BEGIN | BREAK | NOT_END,
     /* nw */ NOT_BEGIN | BREAK | NOT_END,
     /* nx */ ILLEGAL_PAIR,
     /* ny */ NOT_BEGIN,
     /* nz */ NOT_BEGIN | BREAK | NOT_END,
     /* nch */ NOT_BEGIN | PREFIX,
     /* ngh */ NOT_BEGIN | BREAK | NOT_END,
     /* nph */ NOT_BEGIN | PREFIX,
     /* nrh */ ILLEGAL_PAIR,
     /* nsh */ NOT_BEGIN,
     /* nth */ NOT_BEGIN,
     /* nwh */ ILLEGAL_PAIR,
     /* nqu */ NOT_BEGIN | BREAK | NOT_END,
     /* nck */ NOT_BEGIN | PREFIX },
    {/* oa */ ANY_COMBINATION,
     /* ob */ ANY_COMBINATION,
     /* oc */ ANY_COMBINATION,
     /* od */ ANY_COMBINATION,
     /* oe */ ILLEGAL_PAIR,
     /* of */ ANY_COMBINATION,
     /* og */ ANY_COMBINATION,
     /* oh */ NOT_BEGIN | BREAK | NOT_END,
     /* oi */ ANY_COMBINATION,
     /* oj */ ANY_COMBINATION,
     /* ok */ ANY_COMBINATION,
     /* ol */ ANY_COMBINATION,
     /* om */ ANY_COMBINATION,
     /* on */ ANY_COMBINATION,
     /* oo */ ANY_COMBINATION,
     /* op */ ANY_COMBINATION,
     /* or */ ANY_COMBINATION,
     /* os */ ANY_COMBINATION,
     /* ot */ ANY_COMBINATION,
     /* ou */ ANY_COMBINATION,
     /* ov */ ANY_COMBINATION,
     /* ow */ ANY_COMBINATION,
     /* ox */ ANY_COMBINATION,
     /* oy */ ANY_COMBINATION,
     /* oz */ ANY_COMBINATION,
     /* och */ ANY_COMBINATION,
     /* ogh */ NOT_BEGIN,
     /* oph */ ANY_COMBINATION,
     /* orh */ ILLEGAL_PAIR,
     /* osh */ ANY_COMBINATION,
     /* oth */ ANY_COMBINATION,
     /* owh */ ILLEGAL_PAIR,
     /* oqu */ BREAK | NOT_END,
     /* ock */ ANY_COMBINATION },
    {/* pa */ ANY_COMBINATION,
     /* pb */ NOT_BEGIN | BREAK | NOT_END,
     /* pc */ NOT_BEGIN | BREAK | NOT_END,
     /* pd */ NOT_BEGIN | BREAK | NOT_END,
     /* pe */ ANY_COMBINATION,
     /* pf */ NOT_BEGIN | BREAK | NOT_END,
     /* pg */ NOT_BEGIN | BREAK | NOT_END,
     /* ph */ NOT_BEGIN | BREAK | NOT_END,
     /* pi */ ANY_COMBINATION,
     /* pj */ NOT_BEGIN | BREAK | NOT_END,
     /* pk */ NOT_BEGIN | BREAK | NOT_END,
     /* pl */ SUFFIX | NOT_END,
     /* pm */ NOT_BEGIN | BREAK | NOT_END,
     /* pn */ NOT_BEGIN | BREAK | NOT_END,
     /* po */ ANY_COMBINATION,
     /* pp */ NOT_BEGIN | PREFIX,
     /* pr */ NOT_END,
     /* ps */ NOT_BEGIN | END,
     /* pt */ NOT_BEGIN | END,
     /* pu */ NOT_BEGIN | END,
     /* pv */ NOT_BEGIN | BREAK | NOT_END,
     /* pw */ NOT_BEGIN | BREAK | NOT_END,
     /* px */ ILLEGAL_PAIR,
     /* py */ ANY_COMBINATION,
     /* pz */ NOT_BEGIN | BREAK | NOT_END,
     /* pch */ NOT_BEGIN | BREAK | NOT_END,
     /* pgh */ NOT_BEGIN | BREAK | NOT_END,
     /* pph */ NOT_BEGIN | BREAK | NOT_END,
     /* prh */ ILLEGAL_PAIR,
     /* psh */ NOT_BEGIN | BREAK | NOT_END,
     /* pth */ NOT_BEGIN | BREAK | NOT_END,
     /* pwh */ ILLEGAL_PAIR,
     /* pqu */ NOT_BEGIN | BREAK | NOT_END,
     /* pck */ ILLEGAL_PAIR },
    {/* ra */ ANY_COMBINATION,
     /* rb */ NOT_BEGIN | PREFIX,
     /* rc */ NOT_BEGIN | PREFIX,
     /* rd */ NOT_BEGIN | PREFIX,
     /* re */ ANY_COMBINATION,
     /* rf */ NOT_BEGIN | PREFIX,
     /* rg */ NOT_BEGIN | PREFIX,
     /* rh */ NOT_BEGIN | BREAK | NOT_END,
     /* ri */ ANY_COMBINATION,
     /* rj */ NOT_BEGIN | PREFIX,
     /* rk */ NOT_BEGIN | PREFIX,
     /* rl */ NOT_BEGIN | PREFIX,
     /* rm */ NOT_BEGIN | PREFIX,
     /* rn */ NOT_BEGIN | PREFIX,
     /* ro */ ANY_COMBINATION,
     /* rp */ NOT_BEGIN | PREFIX,
     /* rr */ NOT_BEGIN | PREFIX,
     /* rs */ NOT_BEGIN | PREFIX,
     /* rt */ NOT_BEGIN | PREFIX,
     /* ru */ ANY_COMBINATION,
     /* rv */ NOT_BEGIN | PREFIX,
     /* rw */ NOT_BEGIN | BREAK | NOT_END,
     /* rx */ ILLEGAL_PAIR,
     /* ry */ ANY_COMBINATION,
     /* rz */ NOT_BEGIN | PREFIX,
     /* rch */ NOT_BEGIN | PREFIX,
     /* rgh */ NOT_BEGIN | BREAK | NOT_END,
     /* rph */ NOT_BEGIN | PREFIX,
     /* rrh */ ILLEGAL_PAIR,
     /* rsh */ NOT_BEGIN | PREFIX,
     /* rth */ NOT_BEGIN | PREFIX,
     /* rwh */ ILLEGAL_PAIR,
     /* rqu */ NOT_BEGIN | PREFIX | NOT_END,
     /* rck */ NOT_BEGIN | PREFIX },
    {/* sa */ ANY_COMBINATION,
     /* sb */ NOT_BEGIN | BREAK | NOT_END,
     /* sc */ NOT_END,
     /* sd */ NOT_BEGIN | BREAK | NOT_END,
     /* se */ ANY_COMBINATION,
     /* sf */ NOT_BEGIN | BREAK | NOT_END,
     /* sg */ NOT_BEGIN | BREAK | NOT_END,
     /* sh */ NOT_BEGIN | BREAK | NOT_END,
     /* si */ ANY_COMBINATION,
     /* sj */ NOT_BEGIN | BREAK | NOT_END,
     /* sk */ ANY_COMBINATION,
     /* sl */ BEGIN | SUFFIX | NOT_END,
     /* sm */ SUFFIX | NOT_END,
     /* sn */ PREFIX | SUFFIX | NOT_END,
     /* so */ ANY_COMBINATION,
     /* sp */ ANY_COMBINATION,
     /* sr */ NOT_BEGIN | NOT_END,
     /* ss */ NOT_BEGIN | PREFIX,
     /* st */ ANY_COMBINATION,
     /* su */ ANY_COMBINATION,
     /* sv */ NOT_BEGIN | BREAK | NOT_END,
     /* sw */ BEGIN | SUFFIX | NOT_END,
     /* sx */ ILLEGAL_PAIR,
     /* sy */ ANY_COMBINATION,
     /* sz */ NOT_BEGIN | BREAK | NOT_END,
     /* sch */ BEGIN | SUFFIX | NOT_END,
     /* sgh */ NOT_BEGIN | BREAK | NOT_END,
     /* sph */ NOT_BEGIN | BREAK | NOT_END,
     /* srh */ ILLEGAL_PAIR,
     /* ssh */ NOT_BEGIN | BREAK | NOT_END,
     /* sth */ NOT_BEGIN | BREAK | NOT_END,
     /* swh */ ILLEGAL_PAIR,
     /* squ */ SUFFIX | NOT_END,
     /* sck */ NOT_BEGIN },
    {/* ta */ ANY_COMBINATION,
     /* tb */ NOT_BEGIN | BREAK | NOT_END,
     /* tc */ NOT_BEGIN | BREAK | NOT_END,
     /* td */ NOT_BEGIN | BREAK | NOT_END,
     /* te */ ANY_COMBINATION,
     /* tf */ NOT_BEGIN | BREAK | NOT_END,
     /* tg */ NOT_BEGIN | BREAK | NOT_END,
     /* th */ NOT_BEGIN | BREAK | NOT_END,
     /* ti */ ANY_COMBINATION,
     /* tj */ NOT_BEGIN | BREAK | NOT_END,
     /* tk */ NOT_BEGIN | BREAK | NOT_END,
     /* tl */ NOT_BEGIN | BREAK | NOT_END,
     /* tm */ NOT_BEGIN | BREAK | NOT_END,
     /* tn */ NOT_BEGIN | BREAK | NOT_END,
     /* to */ ANY_COMBINATION,
     /* tp */ NOT_BEGIN | BREAK | NOT_END,
     /* tr */ NOT_END,
     /* ts */ NOT_BEGIN | END,
     /* tt */ NOT_BEGIN | PREFIX,
     /* tu */ ANY_COMBINATION,
     /* tv */ NOT_BEGIN | BREAK | NOT_END,
     /* tw */ BEGIN | SUFFIX | NOT_END,
     /* tx */ ILLEGAL_PAIR,
     /* ty */ ANY_COMBINATION,
     /* tz */ NOT_BEGIN | BREAK | NOT_END,
     /* tch */ NOT_BEGIN,
     /* tgh */ NOT_BEGIN | BREAK | NOT_END,
     /* tph */ NOT_BEGIN | END,
     /* trh */ ILLEGAL_PAIR,
     /* tsh */ NOT_BEGIN | END,
     /* tth */ NOT_BEGIN | BREAK | NOT_END,
     /* twh */ ILLEGAL_PAIR,
     /* tqu */ NOT_BEGIN | BREAK | NOT_END,
     /* tck */ ILLEGAL_PAIR },
    {/* ua */ NOT_BEGIN | BREAK | NOT_END,
     /* ub */ ANY_COMBINATION,
     /* uc */ ANY_COMBINATION,
     /* ud */ ANY_COMBINATION,
     /* ue */ NOT_BEGIN,
     /* uf */ ANY_COMBINATION,
     /* ug */ ANY_COMBINATION,
     /* uh */ NOT_BEGIN | BREAK | NOT_END,
     /* ui */ NOT_BEGIN | BREAK | NOT_END,
     /* uj */ ANY_COMBINATION,
     /* uk */ ANY_COMBINATION,
     /* ul */ ANY_COMBINATION,
     /* um */ ANY_COMBINATION,
     /* un */ ANY_COMBINATION,
     /* uo */ NOT_BEGIN | BREAK,
     /* up */ ANY_COMBINATION,
     /* ur */ ANY_COMBINATION,
     /* us */ ANY_COMBINATION,
     /* ut */ ANY_COMBINATION,
     /* uu */ ILLEGAL_PAIR,
     /* uv */ ANY_COMBINATION,
     /* uw */ NOT_BEGIN | BREAK | NOT_END,
     /* ux */ ANY_COMBINATION,
     /* uy */ NOT_BEGIN | BREAK | NOT_END,
     /* uz */ ANY_COMBINATION,
     /* uch */ ANY_COMBINATION,
     /* ugh */ NOT_BEGIN | PREFIX,
     /* uph */ ANY_COMBINATION,
     /* urh */ ILLEGAL_PAIR,
     /* ush */ ANY_COMBINATION,
     /* uth */ ANY_COMBINATION,
     /* uwh */ ILLEGAL_PAIR,
     /* uqu */ BREAK | NOT_END,
     /* uck */ ANY_COMBINATION },
    {/* va */ ANY_COMBINATION,
     /* vb */ NOT_BEGIN | BREAK | NOT_END,
     /* vc */ NOT_BEGIN | BREAK | NOT_END,
     /* vd */ NOT_BEGIN | BREAK | NOT_END,
     /* ve */ ANY_COMBINATION,
     /* vf */ NOT_BEGIN | BREAK | NOT_END,
     /* vg */ NOT_BEGIN | BREAK | NOT_END,
     /* vh */ NOT_BEGIN | BREAK | NOT_END,
     /* vi */ ANY_COMBINATION,
     /* vj */ NOT_BEGIN | BREAK | NOT_END,
     /* vk */ NOT_BEGIN | BREAK | NOT_END,
     /* vl */ NOT_BEGIN | BREAK | NOT_END,
     /* vm */ NOT_BEGIN | BREAK | NOT_END,
     /* vn */ NOT_BEGIN | BREAK | NOT_END,
     /* vo */ ANY_COMBINATION,
     /* vp */ NOT_BEGIN | BREAK | NOT_END,
     /* vr */ NOT_BEGIN | BREAK | NOT_END,
     /* vs */ NOT_BEGIN | BREAK | NOT_END,
     /* vt */ NOT_BEGIN | BREAK | NOT_END,
     /* vu */ ANY_COMBINATION,
     /* vv */ NOT_BEGIN | BREAK | NOT_END,
     /* vw */ NOT_BEGIN | BREAK | NOT_END,
     /* vx */ ILLEGAL_PAIR,
     /* vy */ NOT_BEGIN,
     /* vz */ NOT_BEGIN | BREAK | NOT_END,
     /* vch */ NOT_BEGIN | BREAK | NOT_END,
     /* vgh */ NOT_BEGIN | BREAK | NOT_END,
     /* vph */ NOT_BEGIN | BREAK | NOT_END,
     /* vrh */ ILLEGAL_PAIR,
     /* vsh */ NOT_BEGIN | BREAK | NOT_END,
     /* vth */ NOT_BEGIN | BREAK | NOT_END,
     /* vwh */ ILLEGAL_PAIR,
     /* vqu */ NOT_BEGIN | BREAK | NOT_END,
     /* vck */ ILLEGAL_PAIR },
    {/* wa */ ANY_COMBINATION,
     /* wb */ NOT_BEGIN | PREFIX,
     /* wc */ NOT_BEGIN | BREAK | NOT_END,
     /* wd */ NOT_BEGIN | PREFIX | END,
     /* we */ ANY_COMBINATION,
     /* wf */ NOT_BEGIN | PREFIX,
     /* wg */ NOT_BEGIN | PREFIX | END,
     /* wh */ NOT_BEGIN | BREAK | NOT_END,
     /* wi */ ANY_COMBINATION,
     /* wj */ NOT_BEGIN | BREAK | NOT_END,
     /* wk */ NOT_BEGIN | PREFIX,
     /* wl */ NOT_BEGIN | PREFIX | SUFFIX,
     /* wm */ NOT_BEGIN | PREFIX,
     /* wn */ NOT_BEGIN | PREFIX,
     /* wo */ ANY_COMBINATION,
     /* wp */ NOT_BEGIN | PREFIX,
     /* wr */ BEGIN | SUFFIX | NOT_END,
     /* ws */ NOT_BEGIN | PREFIX,
     /* wt */ NOT_BEGIN | PREFIX,
     /* wu */ ANY_COMBINATION,
     /* wv */ NOT_BEGIN | PREFIX,
     /* ww */ NOT_BEGIN | BREAK | NOT_END,
     /* wx */ NOT_BEGIN | PREFIX,
     /* wy */ ANY_COMBINATION,
     /* wz */ NOT_BEGIN | PREFIX,
     /* wch */ NOT_BEGIN,
     /* wgh */ NOT_BEGIN | BREAK | NOT_END,
     /* wph */ NOT_BEGIN,
     /* wrh */ ILLEGAL_PAIR,
     /* wsh */ NOT_BEGIN,
     /* wth */ NOT_BEGIN,
     /* wwh */ ILLEGAL_PAIR,
     /* wqu */ NOT_BEGIN | BREAK | NOT_END,
     /* wck */ NOT_BEGIN },
    {/* xa */ NOT_BEGIN,
     /* xb */ NOT_BEGIN | BREAK | NOT_END,
     /* xc */ NOT_BEGIN | BREAK | NOT_END,
     /* xd */ NOT_BEGIN | BREAK | NOT_END,
     /* xe */ NOT_BEGIN,
     /* xf */ NOT_BEGIN | BREAK | NOT_END,
     /* xg */ NOT_BEGIN | BREAK | NOT_END,
     /* xh */ NOT_BEGIN | BREAK | NOT_END,
     /* xi */ NOT_BEGIN,
     /* xj */ NOT_BEGIN | BREAK | NOT_END,
     /* xk */ NOT_BEGIN | BREAK | NOT_END,
     /* xl */ NOT_BEGIN | BREAK | NOT_END,
     /* xm */ NOT_BEGIN | BREAK | NOT_END,
     /* xn */ NOT_BEGIN | BREAK | NOT_END,
     /* xo */ NOT_BEGIN,
     /* xp */ NOT_BEGIN | BREAK | NOT_END,
     /* xr */ NOT_BEGIN | BREAK | NOT_END,
     /* xs */ NOT_BEGIN | BREAK | NOT_END,
     /* xt */ NOT_BEGIN | BREAK | NOT_END,
     /* xu */ NOT_BEGIN,
     /* xv */ NOT_BEGIN | BREAK | NOT_END,
     /* xw */ NOT_BEGIN | BREAK | NOT_END,
     /* xx */ ILLEGAL_PAIR,
     /* xy */ NOT_BEGIN,
     /* xz */ NOT_BEGIN | BREAK | NOT_END,
     /* xch */ NOT_BEGIN | BREAK | NOT_END,
     /* xgh */ NOT_BEGIN | BREAK | NOT_END,
     /* xph */ NOT_BEGIN | BREAK | NOT_END,
     /* xrh */ ILLEGAL_PAIR,
     /* xsh */ NOT_BEGIN | BREAK | NOT_END,
     /* xth */ NOT_BEGIN | BREAK | NOT_END,
     /* xwh */ ILLEGAL_PAIR,
     /* xqu */ NOT_BEGIN | BREAK | NOT_END,
     /* xck */ ILLEGAL_PAIR },
    {/* ya */ ANY_COMBINATION,
     /* yb */ NOT_BEGIN,
     /* yc */ NOT_BEGIN | NOT_END,
     /* yd */ NOT_BEGIN,
     /* ye */ ANY_COMBINATION,
     /* yf */ NOT_BEGIN | NOT_END,
     /* yg */ NOT_BEGIN,
     /* yh */ NOT_BEGIN | BREAK | NOT_END,
     /* yi */ BEGIN | NOT_END,
     /* yj */ NOT_BEGIN | NOT_END,
     /* yk */ NOT_BEGIN,
     /* yl */ NOT_BEGIN | NOT_END,
     /* ym */ NOT_BEGIN,
     /* yn */ NOT_BEGIN,
     /* yo */ ANY_COMBINATION,
     /* yp */ NOT_BEGIN,
     /* yr */ NOT_BEGIN | BREAK | NOT_END,
     /* ys */ NOT_BEGIN,
     /* yt */ NOT_BEGIN,
     /* yu */ ANY_COMBINATION,
     /* yv */ NOT_BEGIN | NOT_END,
     /* yw */ NOT_BEGIN | BREAK | NOT_END,
     /* yx */ NOT_BEGIN,
     /* yy */ ILLEGAL_PAIR,
     /* yz */ NOT_BEGIN,
     /* ych */ NOT_BEGIN | BREAK | NOT_END,
     /* ygh */ NOT_BEGIN | BREAK | NOT_END,
     /* yph */ NOT_BEGIN | BREAK | NOT_END,
     /* yrh */ ILLEGAL_PAIR,
     /* ysh */ NOT_BEGIN | BREAK | NOT_END,
     /* yth */ NOT_BEGIN | BREAK | NOT_END,
     /* ywh */ ILLEGAL_PAIR,
     /* yqu */ NOT_BEGIN | BREAK | NOT_END,
     /* yck */ ILLEGAL_PAIR },
    {/* za */ ANY_COMBINATION,
     /* zb */ NOT_BEGIN | BREAK | NOT_END,
     /* zc */ NOT_BEGIN | BREAK | NOT_END,
     /* zd */ NOT_BEGIN | BREAK | NOT_END,
     /* ze */ ANY_COMBINATION,
     /* zf */ NOT_BEGIN | BREAK | NOT_END,
     /* zg */ NOT_BEGIN | BREAK | NOT_END,
     /* zh */ NOT_BEGIN | BREAK | NOT_END,
     /* zi */ ANY_COMBINATION,
     /* zj */ NOT_BEGIN | BREAK | NOT_END,
     /* zk */ NOT_BEGIN | BREAK | NOT_END,
     /* zl */ NOT_BEGIN | BREAK | NOT_END,
     /* zm */ NOT_BEGIN | BREAK | NOT_END,
     /* zn */ NOT_BEGIN | BREAK | NOT_END,
     /* zo */ ANY_COMBINATION,
     /* zp */ NOT_BEGIN | BREAK | NOT_END,
     /* zr */ NOT_BEGIN | NOT_END,
     /* zs */ NOT_BEGIN | BREAK | NOT_END,
     /* zt */ NOT_BEGIN,
     /* zu */ ANY_COMBINATION,
     /* zv */ NOT_BEGIN | BREAK | NOT_END,
     /* zw */ SUFFIX | NOT_END,
     /* zx */ ILLEGAL_PAIR,
     /* zy */ ANY_COMBINATION,
     /* zz */ NOT_BEGIN,
     /* zch */ NOT_BEGIN | BREAK | NOT_END,
     /* zgh */ NOT_BEGIN | BREAK | NOT_END,
     /* zph */ NOT_BEGIN | BREAK | NOT_END,
     /* zrh */ ILLEGAL_PAIR,
     /* zsh */ NOT_BEGIN | BREAK | NOT_END,
     /* zth */ NOT_BEGIN | BREAK | NOT_END,
     /* zwh */ ILLEGAL_PAIR,
     /* zqu */ NOT_BEGIN | BREAK | NOT_END,
     /* zck */ ILLEGAL_PAIR },
    {/* cha */ ANY_COMBINATION,
     /* chb */ NOT_BEGIN | BREAK | NOT_END,
     /* chc */ NOT_BEGIN | BREAK | NOT_END,
     /* chd */ NOT_BEGIN | BREAK | NOT_END,
     /* che */ ANY_COMBINATION,
     /* chf */ NOT_BEGIN | BREAK | NOT_END,
     /* chg */ NOT_BEGIN | BREAK | NOT_END,
     /* chh */ NOT_BEGIN | BREAK | NOT_END,
     /* chi */ ANY_COMBINATION,
     /* chj */ NOT_BEGIN | BREAK | NOT_END,
     /* chk */ NOT_BEGIN | BREAK | NOT_END,
     /* chl */ NOT_BEGIN | BREAK | NOT_END,
     /* chm */ NOT_BEGIN | BREAK | NOT_END,
     /* chn */ NOT_BEGIN | BREAK | NOT_END,
     /* cho */ ANY_COMBINATION,
     /* chp */ NOT_BEGIN | BREAK | NOT_END,
     /* chr */ NOT_END,
     /* chs */ NOT_BEGIN | BREAK | NOT_END,
     /* cht */ NOT_BEGIN | BREAK | NOT_END,
     /* chu */ ANY_COMBINATION,
     /* chv */ NOT_BEGIN | BREAK | NOT_END,
     /* chw */ NOT_BEGIN | NOT_END,
     /* chx */ ILLEGAL_PAIR,
     /* chy */ ANY_COMBINATION,
     /* chz */ NOT_BEGIN | BREAK | NOT_END,
     /* chch */ ILLEGAL_PAIR,
     /* chgh */ NOT_BEGIN | BREAK | NOT_END,
     /* chph */ NOT_BEGIN | BREAK | NOT_END,
     /* chrh */ ILLEGAL_PAIR,
     /* chsh */ NOT_BEGIN | BREAK | NOT_END,
     /* chth */ NOT_BEGIN | BREAK | NOT_END,
     /* chwh */ ILLEGAL_PAIR,
     /* chqu */ NOT_BEGIN | BREAK | NOT_END,
     /* chck */ ILLEGAL_PAIR },
    {/* gha */ ANY_COMBINATION,
     /* ghb */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghc */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghd */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghe */ ANY_COMBINATION,
     /* ghf */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghg */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghh */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghi */ BEGIN | NOT_END,
     /* ghj */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghk */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghl */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghm */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghn */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* gho */ BEGIN | NOT_END,
     /* ghp */ NOT_BEGIN | BREAK | NOT_END,
     /* ghr */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghs */ NOT_BEGIN | PREFIX,
     /* ght */ NOT_BEGIN | PREFIX,
     /* ghu */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghv */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghw */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghx */ ILLEGAL_PAIR,
     /* ghy */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghz */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghch */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghgh */ ILLEGAL_PAIR,
     /* ghph */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghrh */ ILLEGAL_PAIR,
     /* ghsh */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghth */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghwh */ ILLEGAL_PAIR,
     /* ghqu */ NOT_BEGIN | BREAK | PREFIX | NOT_END,
     /* ghck */ ILLEGAL_PAIR },
    {/* pha */ ANY_COMBINATION,
     /* phb */ NOT_BEGIN | BREAK | NOT_END,
     /* phc */ NOT_BEGIN | BREAK | NOT_END,
     /* phd */ NOT_BEGIN | BREAK | NOT_END,
     /* phe */ ANY_COMBINATION,
     /* phf */ NOT_BEGIN | BREAK | NOT_END,
     /* phg */ NOT_BEGIN | BREAK | NOT_END,
     /* phh */ NOT_BEGIN | BREAK | NOT_END,
     /* phi */ ANY_COMBINATION,
     /* phj */ NOT_BEGIN | BREAK | NOT_END,
     /* phk */ NOT_BEGIN | BREAK | NOT_END,
     /* phl */ BEGIN | SUFFIX | NOT_END,
     /* phm */ NOT_BEGIN | BREAK | NOT_END,
     /* phn */ NOT_BEGIN | BREAK | NOT_END,
     /* pho */ ANY_COMBINATION,
     /* php */ NOT_BEGIN | BREAK | NOT_END,
     /* phr */ NOT_END,
     /* phs */ NOT_BEGIN,
     /* pht */ NOT_BEGIN,
     /* phu */ ANY_COMBINATION,
     /* phv */ NOT_BEGIN | NOT_END,
     /* phw */ NOT_BEGIN | NOT_END,
     /* phx */ ILLEGAL_PAIR,
     /* phy */ NOT_BEGIN,
     /* phz */ NOT_BEGIN | BREAK | NOT_END,
     /* phch */ NOT_BEGIN | BREAK | NOT_END,
     /* phgh */ NOT_BEGIN | BREAK | NOT_END,
     /* phph */ ILLEGAL_PAIR,
     /* phrh */ ILLEGAL_PAIR,
     /* phsh */ NOT_BEGIN | BREAK | NOT_END,
     /* phth */ NOT_BEGIN | BREAK | NOT_END,
     /* phwh */ ILLEGAL_PAIR,
     /* phqu */ NOT_BEGIN | BREAK | NOT_END,
     /* phck */ ILLEGAL_PAIR },
    {/* rha */ BEGIN | NOT_END,
     /* rhb */ ILLEGAL_PAIR,
     /* rhc */ ILLEGAL_PAIR,
     /* rhd */ ILLEGAL_PAIR,
     /* rhe */ BEGIN | NOT_END,
     /* rhf */ ILLEGAL_PAIR,
     /* rhg */ ILLEGAL_PAIR,
     /* rhh */ ILLEGAL_PAIR,
     /* rhi */ BEGIN | NOT_END,
     /* rhj */ ILLEGAL_PAIR,
     /* rhk */ ILLEGAL_PAIR,
     /* rhl */ ILLEGAL_PAIR,
     /* rhm */ ILLEGAL_PAIR,
     /* rhn */ ILLEGAL_PAIR,
     /* rho */ BEGIN | NOT_END,
     /* rhp */ ILLEGAL_PAIR,
     /* rhr */ ILLEGAL_PAIR,
     /* rhs */ ILLEGAL_PAIR,
     /* rht */ ILLEGAL_PAIR,
     /* rhu */ BEGIN | NOT_END,
     /* rhv */ ILLEGAL_PAIR,
     /* rhw */ ILLEGAL_PAIR,
     /* rhx */ ILLEGAL_PAIR,
     /* rhy */ BEGIN | NOT_END,
     /* rhz */ ILLEGAL_PAIR,
     /* rhch */ ILLEGAL_PAIR,
     /* rhgh */ ILLEGAL_PAIR,
     /* rhph */ ILLEGAL_PAIR,
     /* rhrh */ ILLEGAL_PAIR,
     /* rhsh */ ILLEGAL_PAIR,
     /* rhth */ ILLEGAL_PAIR,
     /* rhwh */ ILLEGAL_PAIR,
     /* rhqu */ ILLEGAL_PAIR,
     /* rhck */ ILLEGAL_PAIR },
    {/* sha */ ANY_COMBINATION,
     /* shb */ NOT_BEGIN | BREAK | NOT_END,
     /* shc */ NOT_BEGIN | BREAK | NOT_END,
     /* shd */ NOT_BEGIN | BREAK | NOT_END,
     /* she */ ANY_COMBINATION,
     /* shf */ NOT_BEGIN | BREAK | NOT_END,
     /* shg */ NOT_BEGIN | BREAK | NOT_END,
     /* shh */ ILLEGAL_PAIR,
     /* shi */ ANY_COMBINATION,
     /* shj */ NOT_BEGIN | BREAK | NOT_END,
     /* shk */ NOT_BEGIN,
     /* shl */ BEGIN | SUFFIX | NOT_END,
     /* shm */ BEGIN | SUFFIX | NOT_END,
     /* shn */ BEGIN | SUFFIX | NOT_END,
     /* sho */ ANY_COMBINATION,
     /* shp */ NOT_BEGIN,
     /* shr */ BEGIN | SUFFIX | NOT_END,
     /* shs */ NOT_BEGIN | BREAK | NOT_END,
     /* sht */ SUFFIX,
     /* shu */ ANY_COMBINATION,
     /* shv */ NOT_BEGIN | BREAK | NOT_END,
     /* shw */ SUFFIX | NOT_END,
     /* shx */ ILLEGAL_PAIR,
     /* shy */ ANY_COMBINATION,
     /* shz */ NOT_BEGIN | BREAK | NOT_END,
     /* shch */ NOT_BEGIN | BREAK | NOT_END,
     /* shgh */ NOT_BEGIN | BREAK | NOT_END,
     /* shph */ NOT_BEGIN | BREAK | NOT_END,
     /* shrh */ ILLEGAL_PAIR,
     /* shsh */ ILLEGAL_PAIR,
     /* shth */ NOT_BEGIN | BREAK | NOT_END,
     /* shwh */ ILLEGAL_PAIR,
     /* shqu */ NOT_BEGIN | BREAK | NOT_END,
     /* shck */ ILLEGAL_PAIR },
    {/* tha */ ANY_COMBINATION,
     /* thb */ NOT_BEGIN | BREAK | NOT_END,
     /* thc */ NOT_BEGIN | BREAK | NOT_END,
     /* thd */ NOT_BEGIN | BREAK | NOT_END,
     /* the */ ANY_COMBINATION,
     /* thf */ NOT_BEGIN | BREAK | NOT_END,
     /* thg */ NOT_BEGIN | BREAK | NOT_END,
     /* thh */ NOT_BEGIN | BREAK | NOT_END,
     /* thi */ ANY_COMBINATION,
     /* thj */ NOT_BEGIN | BREAK | NOT_END,
     /* thk */ NOT_BEGIN | BREAK | NOT_END,
     /* thl */ NOT_BEGIN | BREAK | NOT_END,
     /* thm */ NOT_BEGIN | BREAK | NOT_END,
     /* thn */ NOT_BEGIN | BREAK | NOT_END,
     /* tho */ ANY_COMBINATION,
     /* thp */ NOT_BEGIN | BREAK | NOT_END,
     /* thr */ NOT_END,
     /* ths */ NOT_BEGIN | END,
     /* tht */ NOT_BEGIN | BREAK | NOT_END,
     /* thu */ ANY_COMBINATION,
     /* thv */ NOT_BEGIN | BREAK | NOT_END,
     /* thw */ SUFFIX | NOT_END,
     /* thx */ ILLEGAL_PAIR,
     /* thy */ ANY_COMBINATION,
     /* thz */ NOT_BEGIN | BREAK | NOT_END,
     /* thch */ NOT_BEGIN | BREAK | NOT_END,
     /* thgh */ NOT_BEGIN | BREAK | NOT_END,
     /* thph */ NOT_BEGIN | BREAK | NOT_END,
     /* thrh */ ILLEGAL_PAIR,
     /* thsh */ NOT_BEGIN | BREAK | NOT_END,
     /* thth */ ILLEGAL_PAIR,
     /* thwh */ ILLEGAL_PAIR,
     /* thqu */ NOT_BEGIN | BREAK | NOT_END,
     /* thck */ ILLEGAL_PAIR },
    {/* wha */ BEGIN | NOT_END,
     /* whb */ ILLEGAL_PAIR,
     /* whc */ ILLEGAL_PAIR,
     /* whd */ ILLEGAL_PAIR,
     /* whe */ BEGIN | NOT_END,
     /* whf */ ILLEGAL_PAIR,
     /* whg */ ILLEGAL_PAIR,
     /* whh */ ILLEGAL_PAIR,
     /* whi */ BEGIN | NOT_END,
     /* whj */ ILLEGAL_PAIR,
     /* whk */ ILLEGAL_PAIR,
     /* whl */ ILLEGAL_PAIR,
     /* whm */ ILLEGAL_PAIR,
     /* whn */ ILLEGAL_PAIR,
     /* who */ BEGIN | NOT_END,
     /* whp */ ILLEGAL_PAIR,
     /* whr */ ILLEGAL_PAIR,
     /* whs */ ILLEGAL_PAIR,
     /* wht */ ILLEGAL_PAIR,
     /* whu */ ILLEGAL_PAIR,
     /* whv */ ILLEGAL_PAIR,
     /* whw */ ILLEGAL_PAIR,
     /* whx */ ILLEGAL_PAIR,
     /* why */ BEGIN | NOT_END,
     /* whz */ ILLEGAL_PAIR,
     /* whch */ ILLEGAL_PAIR,
     /* whgh */ ILLEGAL_PAIR,
     /* whph */ ILLEGAL_PAIR,
     /* whrh */ ILLEGAL_PAIR,
     /* whsh */ ILLEGAL_PAIR,
     /* whth */ ILLEGAL_PAIR,
     /* whwh */ ILLEGAL_PAIR,
     /* whqu */ ILLEGAL_PAIR,
     /* whck */ ILLEGAL_PAIR },
    {/* qua */ ANY_COMBINATION,
     /* qub */ ILLEGAL_PAIR,
     /* quc */ ILLEGAL_PAIR,
     /* qud */ ILLEGAL_PAIR,
     /* que */ ANY_COMBINATION,
     /* quf */ ILLEGAL_PAIR,
     /* qug */ ILLEGAL_PAIR,
     /* quh */ ILLEGAL_PAIR,
     /* qui */ ANY_COMBINATION,
     /* quj */ ILLEGAL_PAIR,
     /* quk */ ILLEGAL_PAIR,
     /* qul */ ILLEGAL_PAIR,
     /* qum */ ILLEGAL_PAIR,
     /* qun */ ILLEGAL_PAIR,
     /* quo */ ANY_COMBINATION,
     /* qup */ ILLEGAL_PAIR,
     /* qur */ ILLEGAL_PAIR,
     /* qus */ ILLEGAL_PAIR,
     /* qut */ ILLEGAL_PAIR,
     /* quu */ ILLEGAL_PAIR,
     /* quv */ ILLEGAL_PAIR,
     /* quw */ ILLEGAL_PAIR,
     /* qux */ ILLEGAL_PAIR,
     /* quy */ ILLEGAL_PAIR,
     /* quz */ ILLEGAL_PAIR,
     /* quch */ ILLEGAL_PAIR,
     /* qugh */ ILLEGAL_PAIR,
     /* quph */ ILLEGAL_PAIR,
     /* qurh */ ILLEGAL_PAIR,
     /* qush */ ILLEGAL_PAIR,
     /* quth */ ILLEGAL_PAIR,
     /* quwh */ ILLEGAL_PAIR,
     /* ququ */ ILLEGAL_PAIR,
     /* quck */ ILLEGAL_PAIR },
    {/* cka */ NOT_BEGIN | BREAK | NOT_END,
     /* ckb */ NOT_BEGIN | BREAK | NOT_END,
     /* ckc */ NOT_BEGIN | BREAK | NOT_END,
     /* ckd */ NOT_BEGIN | BREAK | NOT_END,
     /* cke */ NOT_BEGIN | BREAK | NOT_END,
     /* ckf */ NOT_BEGIN | BREAK | NOT_END,
     /* ckg */ NOT_BEGIN | BREAK | NOT_END,
     /* ckh */ NOT_BEGIN | BREAK | NOT_END,
     /* cki */ NOT_BEGIN | BREAK | NOT_END,
     /* ckj */ NOT_BEGIN | BREAK | NOT_END,
     /* ckk */ NOT_BEGIN | BREAK | NOT_END,
     /* ckl */ NOT_BEGIN | BREAK | NOT_END,
     /* ckm */ NOT_BEGIN | BREAK | NOT_END,
     /* ckn */ NOT_BEGIN | BREAK | NOT_END,
     /* cko */ NOT_BEGIN | BREAK | NOT_END,
     /* ckp */ NOT_BEGIN | BREAK | NOT_END,
     /* ckr */ NOT_BEGIN | BREAK | NOT_END,
     /* cks */ NOT_BEGIN,
     /* ckt */ NOT_BEGIN | BREAK | NOT_END,
     /* cku */ NOT_BEGIN | BREAK | NOT_END,
     /* ckv */ NOT_BEGIN | BREAK | NOT_END,
     /* ckw */ NOT_BEGIN | BREAK | NOT_END,
     /* ckx */ ILLEGAL_PAIR,
     /* cky */ NOT_BEGIN,
     /* ckz */ NOT_BEGIN | BREAK | NOT_END,
     /* ckch */ NOT_BEGIN | BREAK | NOT_END,
     /* ckgh */ NOT_BEGIN | BREAK | NOT_END,
     /* ckph */ NOT_BEGIN | BREAK | NOT_END,
     /* ckrh */ ILLEGAL_PAIR,
     /* cksh */ NOT_BEGIN | BREAK | NOT_END,
     /* ckth */ NOT_BEGIN | BREAK | NOT_END,
     /* ckwh */ ILLEGAL_PAIR,
     /* ckqu */ NOT_BEGIN | BREAK | NOT_END,
     /* ckck */ ILLEGAL_PAIR}
};

/*
** gen_pron_pass will generate a Random word and place it in the
** buffer word.  Also, the hyphenated word will be placed into
** the buffer hyphenated_word.  Both word and hyphenated_word must
** be pre-allocated.  The words generated will have sizes between
** minlen and maxlen.  If restrict is true, words will not be generated that
** appear as login names or as entries in the on-line dictionary.
** This algorithm was initially worded out by Morrie Gasser in 1975.
** Any changes here are minimal so that as many word combinations
** can be produced as possible (and thus keep the words Random).
** The seed is used on first use of the routine.
** The length of the unhyphenated word is returned, or -1 if there
** were an error (length settings are wrong or dictionary checking
** could not be done.
*/
int
gen_pron_pass (char *word, char *hyphenated_word, unsigned short minlen,
               unsigned short maxlen, unsigned int pass_mode)
{

    int     pwlen;

 /* 
  * Check for minlen>maxlen.  This is an error.
  * and a length of 0.
  */
    if (minlen > maxlen || minlen > APG_MAX_PASSWORD_LENGTH ||
        maxlen > APG_MAX_PASSWORD_LENGTH)
      return (-1);
 /* 
  * Check for zero length words.  This is technically not an error,
  * so we take the short cut and return a null word and a length of 0.
  */
    if (maxlen == 0)
    {
     word[0] = '\0';
     hyphenated_word[0] = '\0';
     return (0);
    }

 /* 
  * Find password.
  */
    pwlen = gen_word (word, hyphenated_word, base::RandInt(minlen, maxlen),
                      pass_mode);
    return (pwlen);
}


/*
 * This is the routine that returns a Random word -- as
 * yet unchecked against the passwd file or the dictionary.
 * It collects Random syllables until a predetermined
 * word length is found.  If a retry threshold is reached,
 * another word is tried.  Given that the Random number
 * generator is uniformly distributed, eventually a word
 * will be found if the retry limit is adequately large enough.
 */
int
gen_word (char *word, char *hyphenated_word, unsigned short pwlen, unsigned int pass_mode)
{
    unsigned short word_length;
    unsigned short syllable_length;
    char   *new_syllable;
    char   *syllable_for_hyph;
    unsigned short *syllable_units;
    unsigned short word_size;
    unsigned short word_place;
    unsigned short *word_units;
    unsigned short syllable_size;
    unsigned int   tries;
    bool ch_flag = false;
    int dsd = 0;

    /*
     * Keep count of retries.
     */
    tries = 0;

    /*
     * The length of the word in characters.
     */
    word_length = 0;

    /*
     * The length of the word in character units (each of which is one or
     * two characters long.
     */
    word_size = 0;

    /*
     * Initialize the array storing the word units.  Since we know the
     * length of the word, we only need one of that length.  This method is
     * preferable to a static array, since it allows us flexibility in
     * choosing arbitrarily long word lengths.  Since a word can contain one
     * syllable, we should make syllable_units, the array holding the
     * analogous units for an individual syllable, the same length. No
     * explicit rule limits the length of syllables, but digram rules and
     * heuristics do so indirectly.
     */
    if ( (word_units     = (unsigned short *) calloc (sizeof (unsigned short), pwlen+1))==NULL ||
         (syllable_units = (unsigned short *) calloc (sizeof (unsigned short), pwlen+1))==NULL ||
         (new_syllable   = (char *) calloc (sizeof (unsigned short), pwlen+1))  ==NULL ||
	 (syllable_for_hyph = (char *) calloc (sizeof(char), 20))==NULL)
	   return(-1);

    /*
     * Find syllables until the entire word is constructed.
     */
    while (word_length < pwlen)
    {
     /*
      * Get the syllable and find its length.
      */
     (void) gen_syllable (new_syllable, pwlen - word_length, syllable_units, &syllable_size);
     syllable_length = (unsigned short) strlen (new_syllable);
     
     /*
      * Append the syllable units to the word units.
      */
     for (word_place = 0; word_place <= syllable_size; word_place++)
         word_units[word_size + word_place] = syllable_units[word_place];
     word_size += syllable_size + 1;

     /* 
      * If the word has been improperly formed, throw out
      * the syllable.  The checks performed here are those
      * that must be formed on a word basis.  The other
      * tests are performed entirely within the syllable.
      * Otherwise, append the syllable to the word and
      * append the syllable to the hyphenated version of
      * the word.
      */
     if (improper_word (word_units, word_size) ||
        ((word_length == 0) && have_initial_y (syllable_units, syllable_size)) ||
        ((word_length + syllable_length == pwlen) && have_final_split (syllable_units, syllable_size)))
           word_size -= syllable_size + 1;
     else
     {
         if (word_length == 0)
         {
          /*
          ** Modify syllable for numeric or capital symbols required
          ** Should be done after word quality check. 
          */
          dsd = base::RandInt(0, 1);
          if ( ((pass_mode & S_NB) > 0) && (syllable_length == 1) && dsd == 0)
            {
             numerize(new_syllable);
	     ch_flag = true;
            }
          if ( ((pass_mode & S_SS) > 0) && (syllable_length == 1) && (dsd == 1))
            {
	      specialize(new_syllable);
	      ch_flag = true;
	    }
          if ( ( (pass_mode & S_CL) > 0) && (ch_flag != true))
             capitalize(new_syllable);
          ch_flag = false;
          /**/
          (void) strcpy (word, new_syllable);
	  if (syllable_length == 1)
	     {
	      symb2name(new_syllable, syllable_for_hyph);
              (void) strcpy (hyphenated_word, syllable_for_hyph);
	     }
	  else
	     {
              (void) strcpy (hyphenated_word, new_syllable);
	     }
	  (void)memset ( (void *)new_syllable, 0, (size_t)(pwlen * sizeof(unsigned short)+1));
	  (void)memset ( (void *)syllable_for_hyph, 0, 20);
         }
         else
         {
          /*
          ** Modify syllable for numeric or capital symbols required
          ** Should be done after word quality check.
          */
          dsd = base::RandInt(0, 1);
          if ( ((pass_mode & S_NB) > 0) && (syllable_length == 1) && (dsd == 0))
            {
             numerize(new_syllable);
	     ch_flag = true;
            }
          if ( ( (pass_mode & S_SS) > 0) && (syllable_length == 1) && (dsd == 1))
            {
	     specialize(new_syllable);
	     ch_flag = true;
	    }
          if ( ( (pass_mode & S_CL) > 0) && (ch_flag != true))
             capitalize(new_syllable);
          ch_flag = false;
          /**/
          (void) strcat (word, new_syllable);
          (void) strcat (hyphenated_word, "-");
	  if (syllable_length == 1)
	     {
	      symb2name(new_syllable, syllable_for_hyph);
              (void) strcat (hyphenated_word, syllable_for_hyph);
	     }
	  else
	     {
              (void) strcat (hyphenated_word, new_syllable);
	     }
	  (void)memset ( (void *)new_syllable, 0, (size_t)(pwlen * sizeof(unsigned short)+1));
	  (void)memset ( (void *)syllable_for_hyph, 0, 20);
         }
         word_length += syllable_length;
     }

       /* 
        * Keep track of the times we have tried to get
        * syllables.  If we have exceeded the threshold,
        * reinitialize the pwlen and word_size variables, clear
        * out the word arrays, and start from scratch.
        */
     tries++;
     if (tries > MAX_RETRIES)
     {
         word_length = 0;
         word_size = 0;
         tries = 0;
         (void) strcpy (word, "");
         (void) strcpy (hyphenated_word, "");
     }
    }

    /* 
     * The units arrays and syllable storage are internal to this
     * routine.  Since the caller has no need for them, we
     * release the space.
     */
    free ((char *) new_syllable);
    free ((char *) syllable_units);
    free ((char *) word_units);
    free ((char *) syllable_for_hyph);

    return ((int) word_length);
}



/*
 * Check that the word does not contain illegal combinations
 * that may span syllables.  Specifically, these are:
 *   1. An illegal pair of units between syllables.
 *   2. Three consecutive vowel units.
 *   3. Three consecutive consonant units.
 * The checks are made against units (1 or 2 letters), not against
 * the individual letters, so three consecutive units can have
 * the length of 6 at most.
 */
bool
improper_word (unsigned short *units, unsigned short word_size)
{
    unsigned short unit_count;
    bool failure;

    failure = false;

    for (unit_count = 0; !failure && (unit_count < word_size);
         unit_count++)
    {
     /* 
      * Check for ILLEGAL_PAIR.  This should have been caught
      * for units within a syllable, but in some cases it
      * would have gone unnoticed for units between syllables
      * (e.g., when saved_unit's in gen_syllable() were not
      * used).
      */
     if ((unit_count != 0) &&
          (digram[units[unit_count - 1]][units[unit_count]] &
              ILLEGAL_PAIR))
         failure = true;

     /* 
      * Check for consecutive vowels or consonants.  Because
      * the initial y of a syllable is treated as a consonant
      * rather than as a vowel, we exclude y from the first
      * vowel in the vowel test.  The only problem comes when
      * y ends a syllable and two other vowels start the next,
      * like fly-oint.  Since such words are still
      * pronounceable, we accept this.
      */
     if (!failure && (unit_count >= 2))
     {
         /*
          * Vowel check.
          */
         if ((((rules[units[unit_count - 2]].flags & VOWEL) &&
                   !(rules[units[unit_count - 2]].flags &
                    ALTERNATE_VOWEL)) &&
               (rules[units[unit_count - 1]].flags & VOWEL) &&
               (rules[units[unit_count]].flags & VOWEL)) ||
         /*
          * Consonant check.
          */
              (!(rules[units[unit_count - 2]].flags & VOWEL) &&
               !(rules[units[unit_count - 1]].flags & VOWEL) &&
               !(rules[units[unit_count]].flags & VOWEL)))
          failure = true;
     }
    }

    return (failure);
}


/*
 * Treating y as a vowel is sometimes a problem.  Some words
 * get formed that look irregular.  One special group is when
 * y starts a word and is the only vowel in the first syllable.
 * The word ycl is one example.  We discard words like these.
 */
bool
have_initial_y (unsigned short *units, unsigned short unit_size)
{
    unsigned short unit_count;
    unsigned short vowel_count;
    unsigned short normal_vowel_count;

    vowel_count = 0;
    normal_vowel_count = 0;

    for (unit_count = 0; unit_count <= unit_size; unit_count++)
     /*
      * Count vowels.
      */
     if (rules[units[unit_count]].flags & VOWEL)
     {
         vowel_count++;

         /*
          * Count the vowels that are not: 1. y, 2. at the start of
          * the word.
          */
         if (!(rules[units[unit_count]].flags & ALTERNATE_VOWEL) ||
              (unit_count != 0))
          normal_vowel_count++;
     }

    return ((vowel_count <= 1) && (normal_vowel_count == 0));
}


/*
 * Besides the problem with the letter y, there is one with
 * a silent e at the end of words, like face or nice.  We
 * allow this silent e, but we do not allow it as the only
 * vowel at the end of the word or syllables like ble will
 * be generated.
 */
bool
have_final_split (unsigned short *units, unsigned short unit_size)
{
    unsigned short unit_count;
    unsigned short vowel_count;

    vowel_count = 0;

    /*
     *    Count all the vowels in the word.
     */
    for (unit_count = 0; unit_count <= unit_size; unit_count++)
     if (rules[units[unit_count]].flags & VOWEL)
         vowel_count++;

    /*
     * Return true iff the only vowel was e, found at the end if the
     * word.
     */
    return ((vowel_count == 1) &&
         (rules[units[unit_size]].flags & NO_FINAL_SPLIT));
}


/*
 * Generate next unit to password, making sure that it follows
 * these rules:
 *   1. Each syllable must contain exactly 1 or 2 consecutive
 *      vowels, where y is considered a vowel.
 *   2. Syllable end is determined as follows:
 *        a. Vowel is generated and previous unit is a
 *           consonant and syllable already has a vowel.  In
 *           this case, new syllable is started and already
 *           contains a vowel.
 *        b. A pair determined to be a "break" pair is encountered.
 *           In this case new syllable is started with second unit
 *           of this pair.
 *        c. End of password is encountered.
 *        d. "begin" pair is encountered legally.  New syllable is
 *           started with this pair.
 *        e. "end" pair is legally encountered.  New syllable has
 *           nothing yet.
 *   3. Try generating another unit if:
 *        a. third consecutive vowel and not y.
 *        b. "break" pair generated but no vowel yet in current
 *           or previous 2 units are "not_end".
 *        c. "begin" pair generated but no vowel in syllable
 *           preceding begin pair, or both previous 2 pairs are
 *          designated "not_end".
 *        d. "end" pair generated but no vowel in current syllable
 *           or in "end" pair.
 *        e. "not_begin" pair generated but new syllable must
 *           begin (because previous syllable ended as defined in
 *           2 above).
 *        f. vowel is generated and 2a is satisfied, but no syllable
 *           break is possible in previous 3 pairs.
 *        g. Second and third units of syllable must begin, and
 *           first unit is "alternate_vowel".
 */
char *
gen_syllable (char *syllable, unsigned short pwlen, unsigned short *units_in_syllable,
              unsigned short *syllable_length)
{
    unsigned short  unit = 0;
    short   current_unit = 0;
    unsigned short  vowel_count = 0;
    bool rule_broken;
    bool want_vowel;
    bool want_another_unit;
    unsigned int    tries = 0;
    unsigned short  last_unit = 0;
    short   length_left = 0;
    unsigned short  hold_saved_unit = 0;
    static  unsigned short saved_unit;
    static  unsigned short saved_pair[2];

    /*
     * This is needed if the saved_unit is tries and the syllable then
     * discarded because of the retry limit. Since the saved_unit is OK and
     * fits in nicely with the preceding syllable, we will always use it.
     */
    hold_saved_unit = saved_unit;

    /*
     * Loop until valid syllable is found.
     */
    do
    {
     /* 
      * Try for a new syllable.  Initialize all pertinent
      * syllable variables.
      */
     tries = 0;
     saved_unit = hold_saved_unit;
     (void) strcpy (syllable, "");
     vowel_count = 0;
     current_unit = 0;
     length_left = (short int) pwlen;
     want_another_unit = true;

     /*
      * This loop finds all the units for the syllable.
      */
     do
     {
         want_vowel = false;

         /*
          * This loop continues until a valid unit is found for the
          * current position within the syllable.
          */
         do
         {
          /* 
           * If there are saved_unit's from the previous
           * syllable, use them up first.
           */
          if (saved_unit != 0)
          {
              /* 
               * If there were two saved units, the first is
               * guaranteed (by checks performed in the previous
               * syllable) to be valid.  We ignore the checks
               * and place it in this syllable manually.
               */
              if (saved_unit == 2)
              {
               units_in_syllable[0] = saved_pair[1];
               if (rules[saved_pair[1]].flags & VOWEL)
                   vowel_count++;
               current_unit++;
               (void) strcpy (syllable, rules[saved_pair[1]].unit_code);
               length_left -= (short) strlen (syllable);
              }

              /* 
               * The unit becomes the last unit checked in the
               * previous syllable.
               */
              unit = saved_pair[0];

              /*
               * The saved units have been used.  Do not try to
               * reuse them in this syllable (unless this particular
               * syllable is rejected at which point we start to rebuild
               * it with these same saved units.
               */
              saved_unit = 0;
          }
          else
              /* 
               * If we don't have to scoff the saved units,
               * we generate a Random one.  If we know it has
               * to be a vowel, we get one rather than looping
               * through until one shows up.
               */
              if (want_vowel)
               unit = random_unit (VOWEL);
              else
               unit = random_unit (NO_SPECIAL_RULE);
          length_left -= (short int) strlen (rules[unit].unit_code);

          /*
           * Prevent having a word longer than expected.
           */
          if (length_left < 0)
              rule_broken = true;
          else
              rule_broken = false;

          /*
           * First unit of syllable.  This is special because the
           * digram tests require 2 units and we don't have that yet.
           * Nevertheless, we can perform some checks.
           */
          if (current_unit == 0)
          {
              /* 
               * If the shouldn't begin a syllable, don't
               * use it.
               */
              if (rules[unit].flags & NOT_BEGIN_SYLLABLE)
               rule_broken = true;
              else
               /* 
                * If this is the last unit of a word,
                * we have a one unit syllable.  Since each
                * syllable must have a vowel, we make sure
                * the unit is a vowel.  Otherwise, we
                * discard it.
                */
               if (length_left == 0)
	          {
                   if (rules[unit].flags & VOWEL)
                    want_another_unit = false;
                   else
                    rule_broken = true;
		  }
          }
          else
          {
              /* 
               * There are some digram tests that are
               * universally true.  We test them out.
               */

              /*
               * Reject ILLEGAL_PAIRS of units.
               */
              if ((ALLOWED (ILLEGAL_PAIR)) ||

              /*
               * Reject units that will be split between syllables
               * when the syllable has no vowels in it.
               */
                   (ALLOWED (BREAK) && (vowel_count == 0)) ||

              /*
               * Reject a unit that will end a syllable when no
               * previous unit was a vowel and neither is this one.
               */
                   (ALLOWED (END) && (vowel_count == 0) &&
                    !(rules[unit].flags & VOWEL)))
               rule_broken = true;

              if (current_unit == 1)
              {
               /*
                * Reject the unit if we are at te starting digram of
                * a syllable and it does not fit.
                */
               if (ALLOWED (NOT_BEGIN))
                   rule_broken = true;
              }
              else
              {
               /* 
                * We are not at the start of a syllable.
                * Save the previous unit for later tests.
                */
               last_unit = units_in_syllable[current_unit - 1];

               /*
                * Do not allow syllables where the first letter is y
                * and the next pair can begin a syllable.  This may
                * lead to splits where y is left alone in a syllable.
                * Also, the combination does not sound to good even
                * if not split.
                */
               if (((current_unit == 2) &&
                        (ALLOWED (BEGIN)) &&
                        (rules[units_in_syllable[0]].flags &
                         ALTERNATE_VOWEL)) ||

                    /*
                     * If this is the last unit of a word, we should
                     * reject any digram that cannot end a syllable.
                     */
                    (ALLOWED (NOT_END) &&
                        (length_left == 0)) ||

                    /*
                     * Reject the unit if the digram it forms wants
                     * to break the syllable, but the resulting
                     * digram that would end the syllable is not
                     * allowed to end a syllable.
                     */
                    (ALLOWED (BREAK) &&
                        (digram[units_in_syllable
                             [current_unit - 2]]
                         [last_unit] &
                         NOT_END)) ||

                    /*
                     * Reject the unit if the digram it forms
                     * expects a vowel preceding it and there is
                     * none.
                     */
                    (ALLOWED (PREFIX) &&
                        !(rules[units_in_syllable
                             [current_unit - 2]].flags &
                         VOWEL)))
                   rule_broken = true;

               /*
                * The following checks occur when the current unit
                * is a vowel and we are not looking at a word ending
                * with an e.
                */
               if (!rule_broken &&
                    (rules[unit].flags & VOWEL) &&
                    ((length_left > 0) ||
                        !(rules[last_unit].flags &
                         NO_FINAL_SPLIT)))
                  {
                   /*
                    * Don't allow 3 consecutive vowels in a
                    * syllable.  Although some words formed like this
                    * are OK, like beau, most are not.
                    */
                   if ((vowel_count > 1) &&
                        (rules[last_unit].flags & VOWEL))
                    rule_broken = true;
                   else
                    /*
                     * Check for the case of
                     * vowels-consonants-vowel, which is only
                     * legal if the last vowel is an e and we are
                     * the end of the word (wich is not
                     * happening here due to a previous check.
                     */
                    if ((vowel_count != 0) &&
                         !(rules[last_unit].flags & VOWEL))
                    {
                        /*
                         * Try to save the vowel for the next
                         * syllable, but if the syllable left here
                         * is not proper (i.e., the resulting last
                         * digram cannot legally end it), just
                         * discard it and try for another.   
                         */
                        if (digram[units_in_syllable
                              [current_unit - 2]]
                             [last_unit] &
                             NOT_END)
                         rule_broken = true;
                        else
                        {
                         saved_unit = 1;
                         saved_pair[0] = unit;
                         want_another_unit = false;
                        }
                    }
		  }
              }

              /*
               * The unit picked and the digram formed are legal.
               * We now determine if we can end the syllable.  It may,
               * in some cases, mean the last unit(s) may be deferred to
               * the next syllable.  We also check here to see if the
               * digram formed expects a vowel to follow.
               */
              if (!rule_broken && want_another_unit)
              {
               /*
                * This word ends in a silent e.
                */
/******/        if (((vowel_count != 0) &&
                     (rules[unit].flags & NO_FINAL_SPLIT) &&
                     (length_left == 0) &&
                    !(rules[last_unit].flags & VOWEL)) ||

                    /*
                     * This syllable ends either because the digram
                     * is an END pair or we would otherwise exceed
                     * the length of the word.
                     */
                    (ALLOWED (END) || (length_left == 0)))
		   {
                   want_another_unit = false;
		   }
	       else
                   /*
                    * Since we have a vowel in the syllable
                    * already, if the digram calls for the end of the
                    * syllable, we can legally split it off. We also
                    * make sure that we are not at the end of the
                    * dangerous because that syllable may not have
                    * vowels, or it may not be a legal syllable end,
                    * and the retrying mechanism will loop infinitely
                    * with the same digram.
                    */
                   if ((vowel_count != 0) && (length_left > 0))
                   {
                    /*
                     * If we must begin a syllable, we do so if
                     * the only vowel in THIS syllable is not part
                     * of the digram we are pushing to the next
                     * syllable.
                     */
                    if (ALLOWED (BEGIN) &&
                         (current_unit > 1) &&
                         !((vowel_count == 1) &&
                         (rules[last_unit].flags & VOWEL)))
                    {
                        saved_unit = 2;
                        saved_pair[0] = unit;
                        saved_pair[1] = last_unit;
                        want_another_unit = false;
                    }
                    else
                        if (ALLOWED (BREAK))
                        {
                         saved_unit = 1;
                         saved_pair[0] = unit;
                         want_another_unit = false;
                        }
                   }
                   else
                    if (ALLOWED (SUFFIX))
		     {
                        want_vowel = true;
		     }
              }
          }
/********/
          tries++;

          /*
           * If this unit was illegal, redetermine the amount of
           * letters left to go in the word.
           */
          if (rule_broken)
              length_left += (short int) strlen (rules[unit].unit_code);
         }
         while (rule_broken && (tries <= MAX_RETRIES));

         /*
          * The unit fit OK.
          */
         if (tries <= MAX_RETRIES)
         {
          /* 
           * If the unit were a vowel, count it in.
           * However, if the unit were a y and appear
           * at the start of the syllable, treat it
           * like a constant (so that words like year can
           * appear and not conflict with the 3 consecutive
           * vowel rule.
           */
          if ((rules[unit].flags & VOWEL) &&
               ((current_unit > 0) ||
                   !(rules[unit].flags & ALTERNATE_VOWEL)))
              vowel_count++;

          /* 
           * If a unit or units were to be saved, we must
           * adjust the syllable formed.  Otherwise, we
           * append the current unit to the syllable.
           */
          switch (saved_unit)
          {
              case 0: 
               units_in_syllable[current_unit] = unit;
               (void) strcat (syllable, rules[unit].unit_code);
               break;
              case 1: 
               current_unit--;
               break;
              case 2: 
               (void) strcpy (&syllable[strlen (syllable) -
                        strlen (rules[last_unit].unit_code)],"");
               length_left += (short int) strlen (rules[last_unit].unit_code);
               current_unit -= 2;
               break;
          }
         }
         else
         /*
          * Whoops!  Too many tries.  We set rule_broken so we can
          * loop in the outer loop and try another syllable.
          */
          rule_broken = true;

         /*
          * ...and the syllable length grows.
          */
         *syllable_length = current_unit;

         current_unit++;
     }
     while ((tries <= MAX_RETRIES) && want_another_unit);
    }
    while (rule_broken ||
           illegal_placement (units_in_syllable, *syllable_length));

    return (syllable);
}


/*
 * This routine goes through an individual syllable and checks
 * for illegal combinations of letters that go beyond looking
 * at digrams.  We look at things like 3 consecutive vowels or
 * consonants, or syllables with consonants between vowels (unless
 * one of them is the final silent e).
 */
bool
illegal_placement (unsigned short *units, unsigned short pwlen)
{
    unsigned short vowel_count;
    unsigned short unit_count;
    bool failure;

    vowel_count = 0;
    failure = false;

    for (unit_count = 0; !failure && (unit_count <= pwlen);
         unit_count++)
    {
     if (unit_count >= 1)
     {
         /* 
          * Don't allow vowels to be split with consonants in
          * a single syllable.  If we find such a combination
          * (except for the silent e) we have to discard the
          * syllable).
          */
         if ((!(rules[units[unit_count - 1]].flags & VOWEL) &&
               (rules[units[unit_count]].flags & VOWEL) &&
               !((rules[units[unit_count]].flags & NO_FINAL_SPLIT) &&
                   (unit_count == pwlen)) && (vowel_count != 0)) ||
         /*
          * Perform these checks when we have at least 3 units.
          */
              ((unit_count >= 2) &&

                  /*
                   * Disallow 3 consecutive consonants.
                   */
               ((!(rules[units[unit_count - 2]].flags & VOWEL) &&
                    !(rules[units[unit_count - 1]].flags &
                        VOWEL) &&
                    !(rules[units[unit_count]].flags &
                        VOWEL)) ||

                   /*
                    * Disallow 3 consecutive vowels, where the first is
                    * not a y.
                    */
                   (((rules[units[unit_count - 2]].flags &
                         VOWEL) &&
                        !((rules[units[0]].flags &
                             ALTERNATE_VOWEL) &&
                         (unit_count == 2))) &&
                    (rules[units[unit_count - 1]].flags &
                        VOWEL) &&
                    (rules[units[unit_count]].flags &
                        VOWEL)))))
          failure = true;
     }

     /* 
      * Count the vowels in the syllable.  As mentioned somewhere
      * above, exclude the initial y of a syllable.  Instead,
      * treat it as a consonant.
      */
     if ((rules[units[unit_count]].flags & VOWEL) &&
          !((rules[units[0]].flags & ALTERNATE_VOWEL) &&
              (unit_count == 0) && (pwlen != 0)))
         vowel_count++;
    }

    return (failure);
}



/*
 * This is the standard Random unit generating routine for
 * gen_syllable().  It does not reference the digrams, but
 * assumes that it contains 34 units in a particular order.
 * This routine attempts to return unit indexes with a distribution
 * approaching that of the distribution of the 34 units in
 * English.  In order to do this, a Random number (supposedly
 * uniformly distributed) is used to do a table lookup into an
 * array containing unit indices.  There are 211 entries in
 * the array for the random_unit entry point.  The probability
 * of a particular unit being generated is equal to the
 * fraction of those 211 entries that contain that unit index.
 * For example, the letter `a' is unit number 1.  Since unit
 * index 1 appears 10 times in the array, the probability of
 * selecting an `a' is 10/211.
 *
 * Changes may be made to the digram table without affect to this
 * procedure providing the letter-to-number correspondence of
 * the units does not change.  Likewise, the distribution of the
 * 34 units may be altered (and the array size may be changed)
 * in this procedure without affecting the digram table or any other
 * programs using the Random_word subroutine.
 */
static unsigned short numbers[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9, 9, 9, 9,
    10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 12, 12,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    15, 15, 15, 15, 15, 15,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    17, 17, 17, 17, 17, 17, 17, 17,
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    19, 19, 19, 19, 19, 19,
    20, 20, 20, 20, 20, 20, 20, 20,
    21, 21, 21, 21, 21, 21, 21, 21,
    22,
    23, 23, 23, 23, 23, 23, 23, 23,
    24,
    25,
    26,
    27,
    28,
    29, 29,
    30,
    31,
    32,
    33
};


/*
 * This structure has a typical English frequency of vowels.
 * The value of an entry is the vowel position (a=0, e=4, i=8,
 * o=14, u=19, y=23) in the rules array.  The number of times
 * the value appears is the frequency.  Thus, the letter "a"
 * is assumed to appear 2/12 = 1/6 of the time.  This array
 * may be altered if better data is obtained.  The routines that
 * use vowel_numbers will adjust to the size difference
automatically.
 */
static unsigned short vowel_numbers[] =
{
    0, 0, 4, 4, 4, 8, 8, 14, 14, 19, 19, 23
};


/*
 * Select a unit (a letter or a consonant group).  If a vowel is
 * expected, use the vowel_numbers array rather than looping through
 * the numbers array until a vowel is found.
 */
unsigned short
random_unit (unsigned short type)
{
     unsigned short number;

    /*
     * Sometimes, we are asked to explicitly get a vowel (i.e., if
     * a digram pair expects one following it).  This is a shortcut
     * to do that and avoid looping with rejected consonants.
     */
    if (type & VOWEL)
      number = vowel_numbers[
          base::RandInt(0, (sizeof (vowel_numbers) / sizeof (unsigned short))-1)];
    else
     /*
      * Get any letter according to the English distribution.
      */
      number = numbers[
          base::RandInt(0, (sizeof (numbers) / sizeof (unsigned short))-1)];
    return (number);
}
