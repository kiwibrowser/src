# (c) 2005 Clark C. Evans and contributors
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php
# Some of this code was funded by: http://prometheusresearch.com
from time import localtime
from datetime import date
from paste.util.datetimeutil import *

def test_timedelta():
    assert('' == normalize_timedelta(""))
    assert('0.10' == normalize_timedelta("6m"))
    assert('0.50' == normalize_timedelta("30m"))
    assert('0.75' == normalize_timedelta("45m"))
    assert('1.00' == normalize_timedelta("60 min"))
    assert('1.50' == normalize_timedelta("90min"))
    assert('1.50' == normalize_timedelta("1.50"))
    assert('4.50' == normalize_timedelta("4 : 30"))
    assert('1.50' == normalize_timedelta("1h 30m"))
    assert('1.00' == normalize_timedelta("1"))
    assert('1.00' == normalize_timedelta("1 hour"))
    assert('8.00' == normalize_timedelta("480 mins"))
    assert('8.00' == normalize_timedelta("8h"))
    assert('0.50' == normalize_timedelta("0.5"))
    assert('0.10' == normalize_timedelta(".1"))
    assert('0.50' == normalize_timedelta(".50"))
    assert('0.75' == normalize_timedelta("0.75"))

def test_time():
    assert('03:00 PM' == normalize_time("3p", ampm=True))
    assert('03:00 AM' == normalize_time("300", ampm=True))
    assert('03:22 AM' == normalize_time("322", ampm=True))
    assert('01:22 PM' == normalize_time("1322", ampm=True))
    assert('01:00 PM' == normalize_time("13", ampm=True))
    assert('12:00 PM' == normalize_time("noon", ampm=True))
    assert("06:00 PM" == normalize_time("6", ampm=True))
    assert("01:00 PM" == normalize_time("1", ampm=True))
    assert("07:00 AM" == normalize_time("7", ampm=True))
    assert("01:00 PM" == normalize_time("1 pm", ampm=True))
    assert("03:30 PM" == normalize_time("3:30 pm", ampm=True))
    assert("03:30 PM" == normalize_time("3 30 pm", ampm=True))
    assert("03:30 PM" == normalize_time("3 30 P.M.", ampm=True))
    assert("12:00 PM" == normalize_time("0", ampm=True))
    assert("12:00 AM" == normalize_time("1200 AM", ampm=True))

def test_date():
    tm = localtime()
    yr = tm[0]
    mo = tm[1]
    assert(date(yr,4,11)  == parse_date("411"))
    assert(date(yr,4,11)  == parse_date("APR11"))
    assert(date(yr,4,11)  == parse_date("11APR"))
    assert(date(yr,4,11)  == parse_date("4 11"))
    assert(date(yr,4,11)  == parse_date("11 APR"))
    assert(date(yr,4,11)  == parse_date("APR 11"))
    assert(date(yr,mo,11) == parse_date("11"))
    assert(date(yr,4,1)   == parse_date("APR"))
    assert(date(yr,4,11)  == parse_date("4/11"))
    assert(date.today()   == parse_date("today"))
    assert(date.today()   == parse_date("now"))
    assert(None           == parse_date(""))
    assert(''             == normalize_date(None))

    assert('2001-02-03' == normalize_date("20010203"))
    assert('1999-04-11' == normalize_date("1999 4 11"))
    assert('1999-04-11' == normalize_date("1999 APR 11"))
    assert('1999-04-11' == normalize_date("APR 11 1999"))
    assert('1999-04-11' == normalize_date("11 APR 1999"))
    assert('1999-04-11' == normalize_date("4 11 1999"))
    assert('1999-04-01' == normalize_date("1999 APR"))
    assert('1999-04-01' == normalize_date("1999 4"))
    assert('1999-04-01' == normalize_date("4 1999"))
    assert('1999-04-01' == normalize_date("APR 1999"))
    assert('1999-01-01' == normalize_date("1999"))

    assert('1999-04-01' == normalize_date("1APR1999"))
    assert('2001-04-01' == normalize_date("1APR2001"))

    assert('1999-04-18' == normalize_date("1999-04-11+7"))
    assert('1999-04-18' == normalize_date("1999-04-11 7"))
    assert('1999-04-01' == normalize_date("1 apr 1999"))
    assert('1999-04-11' == normalize_date("11 apr 1999"))
    assert('1999-04-11' == normalize_date("11 Apr 1999"))
    assert('1999-04-11' == normalize_date("11-apr-1999"))
    assert('1999-04-11' == normalize_date("11 April 1999"))
    assert('1999-04-11' == normalize_date("11 APRIL 1999"))
    assert('1999-04-11' == normalize_date("11 april 1999"))
    assert('1999-04-11' == normalize_date("11 aprick 1999"))
    assert('1999-04-11' == normalize_date("APR 11, 1999"))
    assert('1999-04-11' == normalize_date("4/11/1999"))
    assert('1999-04-11' == normalize_date("4-11-1999"))
    assert('1999-04-11' == normalize_date("1999-4-11"))
    assert('1999-04-11' == normalize_date("19990411"))

    assert('1999-01-01' == normalize_date("1 Jan 1999"))
    assert('1999-02-01' == normalize_date("1 Feb 1999"))
    assert('1999-03-01' == normalize_date("1 Mar 1999"))
    assert('1999-04-01' == normalize_date("1 Apr 1999"))
    assert('1999-05-01' == normalize_date("1 May 1999"))
    assert('1999-06-01' == normalize_date("1 Jun 1999"))
    assert('1999-07-01' == normalize_date("1 Jul 1999"))
    assert('1999-08-01' == normalize_date("1 Aug 1999"))
    assert('1999-09-01' == normalize_date("1 Sep 1999"))
    assert('1999-10-01' == normalize_date("1 Oct 1999"))
    assert('1999-11-01' == normalize_date("1 Nov 1999"))
    assert('1999-12-01' == normalize_date("1 Dec 1999"))

    assert('1999-04-30' == normalize_date("1999-4-30"))
    assert('2000-02-29' == normalize_date("29 FEB 2000"))
    assert('2001-02-28' == normalize_date("28 FEB 2001"))
    assert('2004-02-29' == normalize_date("29 FEB 2004"))
    assert('2100-02-28' == normalize_date("28 FEB 2100"))
    assert('1900-02-28' == normalize_date("28 FEB 1900"))

    def assertError(val):
        try:
            normalize_date(val)
        except (TypeError,ValueError):
            return
        raise ValueError("type error expected", val)

    assertError("2000-13-11")
    assertError("APR 99")
    assertError("29 FEB 1900")
    assertError("29 FEB 2100")
    assertError("29 FEB 2001")
    assertError("1999-4-31")
    assertError("APR 99")
    assertError("20301")
    assertError("020301")
    assertError("1APR99")
    assertError("1APR01")
    assertError("1 APR 99")
    assertError("1 APR 01")
    assertError("11/5/01")

