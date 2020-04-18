# (c) 2010 Ch. Zwerschke and contributors
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php

from paste.util.mimeparse import *

def test_parse_mime_type():
    parse = parse_mime_type
    assert parse('*/*') == ('*', '*', {})
    assert parse('text/html') == ('text', 'html', {})
    assert parse('audio/*; q=0.2') == ('audio', '*', {'q': '0.2'})
    assert parse('text/x-dvi;level=1') == ('text', 'x-dvi', {'level': '1'})
    assert parse('image/gif; level=2; q=0.4') == (
        'image', 'gif', {'level': '2', 'q': '0.4'})
    assert parse('application/xhtml;level=3;q=0.5') == (
        'application', 'xhtml', {'level': '3', 'q': '0.5'})
    assert parse('application/xml') == ('application', 'xml', {})
    assert parse('application/xml;q=1') == ('application', 'xml', {'q': '1'})
    assert parse('application/xml ; q=1;b=other') == (
        'application', 'xml', {'q': '1', 'b': 'other'})
    assert parse('application/xml ; q=2;b=other') == (
        'application', 'xml', {'q': '2', 'b': 'other'})
    assert parse('application/xhtml;q=0.5') == (
        'application', 'xhtml', {'q': '0.5'})
    assert parse('application/xhtml;q=0.5;ver=1.2') == (
        'application', 'xhtml', {'q': '0.5', 'ver': '1.2'})

def test_parse_illformed_mime_type():
    parse = parse_mime_type
    assert parse('*') == ('*', '*', {})
    assert parse('text') == ('text', '*', {})
    assert parse('text/') == ('text', '*', {})
    assert parse('/plain') == ('*', 'plain', {})
    assert parse('/') == ('*', '*', {})
    assert parse('text/plain;') == ('text', 'plain', {})
    assert parse(';q=0.5') == ('*', '*', {'q': '0.5'})
    assert parse('*; q=.2') == ('*', '*', {'q': '.2'})
    assert parse('image; q=.7; level=3') == (
        'image', '*', {'q': '.7', 'level': '3'})
    assert parse('*;q=1') == ('*', '*', {'q': '1'})
    assert parse('*;q=') == ('*', '*', {})
    assert parse('*;=0.5') == ('*', '*', {})
    assert parse('*;q=foobar') == ('*', '*', {'q': 'foobar'})
    assert parse('image/gif; level=2; q=2') == (
        'image', 'gif', {'level': '2', 'q': '2'})
    assert parse('application/xml;q=') == ('application', 'xml', {})
    assert parse('application/xml ;q=') == ('application', 'xml', {})
    assert parse(' *; q =;') == ('*', '*', {})
    assert parse(' *; q=.2') == ('*', '*', {'q': '.2'})

def test_parse_media_range():
    parse = parse_media_range
    assert parse('application/*;q=0.5') == ('application', '*', {'q': '0.5'})
    assert parse('text/plain') == ('text', 'plain', {'q': '1'})
    assert parse('*') == ('*', '*', {'q': '1'})
    assert parse(';q=0.5') == ('*', '*', {'q': '0.5'})
    assert parse('*;q=0.5') == ('*', '*', {'q': '0.5'})
    assert parse('*;q=1') == ('*', '*', {'q': '1'})
    assert parse('*;q=') == ('*', '*', {'q': '1'})
    assert parse('*;q=-1') == ('*', '*', {'q': '1'})
    assert parse('*;q=foobar') == ('*', '*', {'q': '1'})
    assert parse('*;q=0.0001') == ('*', '*', {'q': '0.0001'})
    assert parse('*;q=1000.0') == ('*', '*', {'q': '1'})
    assert parse('*;q=0') == ('*', '*', {'q': '0'})
    assert parse('*;q=0.0000') == ('*', '*', {'q': '0.0000'})
    assert parse('*;q=1.0001') == ('*', '*', {'q': '1'})
    assert parse('*;q=2') == ('*', '*', {'q': '1'})
    assert parse('*;q=1e3') == ('*', '*', {'q': '1'})
    assert parse('image/gif; level=2') == (
        'image', 'gif', {'level': '2', 'q': '1'})
    assert parse('image/gif; level=2; q=0.5') == (
        'image', 'gif', {'level': '2', 'q': '0.5'})
    assert parse('image/gif; level=2; q=2') == (
        'image', 'gif', {'level': '2', 'q': '1'})
    assert parse('application/xml') == ('application', 'xml', {'q': '1'})
    assert parse('application/xml;q=1') == ('application', 'xml', {'q': '1'})
    assert parse('application/xml;q=') == ('application', 'xml', {'q': '1'})
    assert parse('application/xml ;q=') == ('application', 'xml', {'q': '1'})
    assert parse('application/xml ; q=1;b=other') == (
        'application', 'xml', {'q': '1', 'b': 'other'})
    assert parse('application/xml ; q=2;b=other') == (
        'application', 'xml', {'q': '1', 'b': 'other'})
    assert parse(' *; q =;') == ('*', '*', {'q': '1'})
    assert parse(' *; q=.2') == ('*', '*', {'q': '.2'})

def test_fitness_and_quality_parsed():
    faq = fitness_and_quality_parsed
    assert faq('*/*;q=0.7', [
        ('foo', 'bar', {'q': '0.5'})]) == (0, 0.5)
    assert faq('foo/*;q=0.7', [
        ('foo', 'bar', {'q': '0.5'})]) == (100, 0.5)
    assert faq('*/bar;q=0.7', [
        ('foo', 'bar', {'q': '0.5'})]) == (10, 0.5)
    assert faq('foo/bar;q=0.7', [
        ('foo', 'bar', {'q': '0.5'})]) == (110, 0.5)
    assert faq('text/html;q=0.7', [
        ('foo', 'bar', {'q': '0.5'})]) == (-1, 0)
    assert faq('text/html;q=0.7', [
        ('text', 'bar', {'q': '0.5'})]) == (-1, 0)
    assert faq('text/html;q=0.7', [
        ('foo', 'html', {'q': '0.5'})]) == (-1, 0)
    assert faq('text/html;q=0.7', [
        ('text', '*', {'q': '0.5'})]) == (100, 0.5)
    assert faq('text/html;q=0.7', [
        ('*', 'html', {'q': '0.5'})]) == (10, 0.5)
    assert faq('text/html;q=0.7', [
        ('*', '*', {'q': '0'}), ('text', 'html', {'q': '0.5'})]) == (110, 0.5)
    assert faq('text/html;q=0.7', [
        ('*', '*', {'q': '0.5'}), ('audio', '*', {'q': '0'})]) == (0, 0.5)
    assert faq('audio/mp3;q=0.7', [
        ('*', '*', {'q': '0'}), ('audio', '*', {'q': '0.5'})]) == (100, 0.5)
    assert faq('*/mp3;q=0.7', [
        ('foo', 'mp3', {'q': '0.5'}), ('audio', '*', {'q': '0'})]) == (10, 0.5)
    assert faq('audio/mp3;q=0.7', [
        ('audio', 'ogg', {'q': '0'}), ('*', 'mp3', {'q': '0.5'})]) == (10, 0.5)
    assert faq('audio/mp3;q=0.7', [
        ('*', 'ogg', {'q': '0'}), ('*', 'mp3', {'q': '0.5'})]) == (10, 0.5)
    assert faq('text/html;q=0.7', [
        ('text', 'plain', {'q': '0'}),
        ('plain', 'html', {'q': '0'}),
        ('text', 'html', {'q': '0.5'}),
        ('html', 'text', {'q': '0'})]) == (110, 0.5)
    assert faq('text/html;q=0.7;level=2', [
        ('plain', 'html', {'q': '0', 'level': '2'}),
        ('text', '*', {'q': '0.5', 'level': '3'}),
        ('*', 'html', {'q': '0.5', 'level': '2'}),
        ('image', 'gif', {'q': '0.5', 'level': '2'})]) == (100, 0.5)
    assert faq('text/html;q=0.7;level=2', [
        ('text', 'plain', {'q': '0'}), ('text', 'html', {'q': '0'}),
        ('text', 'plain', {'q': '0', 'level': '2'}),
        ('text', 'html', {'q': '0.5', 'level': '2'}),
        ('*', '*', {'q': '0', 'level': '2'}),
        ('text', 'html', {'q': '0', 'level': '3'})]) == (111, 0.5)
    assert faq('text/html;q=0.7;level=2;opt=3', [
        ('text', 'html', {'q': '0'}),
        ('text', 'html', {'q': '0', 'level': '2'}),
        ('text', 'html', {'q': '0', 'opt': '3'}),
        ('*', '*', {'q': '0', 'level': '2', 'opt': '3'}),
        ('text', 'html', {'q': '0', 'level': '3', 'opt': '3'}),
        ('text', 'html', {'q': '0.5', 'level': '2', 'opt': '3'}),
        ('*', '*', {'q': '0', 'level': '3', 'opt': '3'})]) == (112, 0.5)

def test_quality_parsed():
    qp = quality_parsed
    assert qp('image/gif;q=0.7', [('image', 'jpg', {'q': '0.5'})]) == 0
    assert qp('image/gif;q=0.7', [('image', '*', {'q': '0.5'})]) == 0.5
    assert qp('audio/mp3;q=0.7;quality=100', [
        ('*', '*', {'q': '0', 'quality': '100'}),
        ('audio', '*', {'q': '0', 'quality': '100'}),
        ('*', 'mp3', {'q': '0', 'quality': '100'}),
        ('audio', 'mp3', {'q': '0', 'quality': '50'}),
        ('audio', 'mp3', {'q': '0.5', 'quality': '100'}),
        ('audio', 'mp3', {'q': '0.5'})]) == 0.5

def test_quality():
    assert quality('text/html',
        'text/*;q=0.3, text/html;q=0.75, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 0.75
    assert quality('text/html;level=2',
        'text/*;q=0.3, text/html;q=0.7, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 0.4
    assert quality('text/plain',
        'text/*;q=0.25, text/html;q=0.7, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 0.25
    assert quality('plain/text',
        'text/*;q=0.3, text/html;q=0.7, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 0.5
    assert quality('text/html;level=1',
        'text/*;q=0.3, text/html;q=0.7, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 1
    assert quality('image/jpeg',
        'text/*;q=0.3, text/html;q=0.7, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 0.5
    assert quality('text/html;level=2',
        'text/*;q=0.3, text/html;q=0.7, text/html;level=1,'
        ' text/html;level=2;q=0.375, */*;q=0.5') == 0.375
    assert quality('text/html;level=3',
        'text/*;q=0.3, text/html;q=0.75, text/html;level=1,'
        ' text/html;level=2;q=0.4, */*;q=0.5') == 0.75

def test_best_match():
    bm = best_match
    assert bm([], '*/*') == ''
    assert bm(['application/xbel+xml', 'text/xml'],
        'text/*;q=0.5,*/*; q=0.1') == 'text/xml'
    assert bm(['application/xbel+xml', 'audio/mp3'],
        'text/*;q=0.5,*/*; q=0.1') == 'application/xbel+xml'
    assert bm(['application/xbel+xml', 'audio/mp3'],
        'text/*;q=0.5,*/mp3; q=0.1') == 'audio/mp3'
    assert bm(['application/xbel+xml', 'text/plain', 'text/html'],
        'text/*;q=0.5,*/plain; q=0.1') == 'text/plain'
    assert bm(['application/xbel+xml', 'text/html', 'text/xhtml'],
        'text/*;q=0.1,*/xhtml; q=0.5') == 'text/html'
    assert bm(['application/xbel+xml', 'text/html', 'text/xhtml'],
        '*/html;q=0.1,*/xhtml; q=0.5') == 'text/xhtml'
    assert bm(['application/xbel+xml', 'application/xml'],
        'application/xbel+xml') == 'application/xbel+xml'
    assert bm(['application/xbel+xml', 'application/xml'],
        'application/xbel+xml; q=1') == 'application/xbel+xml'
    assert bm(['application/xbel+xml', 'application/xml'],
        'application/xml; q=1') == 'application/xml'
    assert bm(['application/xbel+xml', 'application/xml'],
        'application/*; q=1') == 'application/xbel+xml'
    assert bm(['application/xbel+xml', 'application/xml'],
        '*/*, application/xml') == 'application/xml'
    assert bm(['application/xbel+xml', 'text/xml'],
        'text/*;q=0.5,*/*; q=0.1') == 'text/xml'
    assert bm(['application/xbel+xml', 'text/xml'],
        'text/html,application/atom+xml; q=0.9') == ''
    assert bm(['application/json', 'text/html'],
        'application/json, text/javascript, */*') == 'application/json'
    assert bm(['application/json', 'text/html'],
        'application/json, text/html;q=0.9') == 'application/json'
    assert bm(['image/*', 'application/xml'], 'image/png') == 'image/*'
    assert bm(['image/*', 'application/xml'], 'image/*') == 'image/*'

def test_illformed_best_match():
    bm = best_match
    assert bm(['image/png', 'image/jpeg', 'image/gif', 'text/html'],
        'text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2') == 'image/jpeg'
    assert bm(['image/png', 'image/jpg', 'image/tif', 'text/html'],
        'text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2') == 'text/html'
    assert bm(['image/png', 'image/jpg', 'image/tif', 'audio/mp3'],
        'text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2') == 'image/png'

def test_sorted_match():
    dm = desired_matches
    assert dm(['text/html', 'application/xml'],
        'text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,'
        'text/plain;q=0.8,image/png') == ['text/html', 'application/xml']
    assert dm(['text/html', 'application/xml'],
        'application/xml,application/json') == ['application/xml']
    assert dm(['text/xhtml', 'text/plain', 'application/xhtml'],
        'text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,'
        'text/plain;q=0.8,image/png') == ['text/plain']
