#!/usr/bin/env python
##########################################################################
# 
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
##########################################################################


'''Trace data model.'''


import sys
import string
import format

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO


class Node:
    
    def visit(self, visitor):
        raise NotImplementedError

    def __str__(self):
        stream = StringIO()
        formatter = format.DefaultFormatter(stream)
        pretty_printer = PrettyPrinter(formatter)
        self.visit(pretty_printer)
        return stream.getvalue()


class Literal(Node):
    
    def __init__(self, value):
        self.value = value

    def visit(self, visitor):
        visitor.visit_literal(self)


class NamedConstant(Node):
    
    def __init__(self, name):
        self.name = name

    def visit(self, visitor):
        visitor.visit_named_constant(self)
    

class Array(Node):
    
    def __init__(self, elements):
        self.elements = elements

    def visit(self, visitor):
        visitor.visit_array(self)


class Struct(Node):
    
    def __init__(self, name, members):
        self.name = name
        self.members = members        

    def visit(self, visitor):
        visitor.visit_struct(self)

        
class Pointer(Node):
    
    def __init__(self, address):
        self.address = address

    def visit(self, visitor):
        visitor.visit_pointer(self)


class Call:
    
    def __init__(self, no, klass, method, args, ret):
        self.no = no
        self.klass = klass
        self.method = method
        self.args = args
        self.ret = ret
        
    def visit(self, visitor):
        visitor.visit_call(self)


class Trace:
    
    def __init__(self, calls):
        self.calls = calls
        
    def visit(self, visitor):
        visitor.visit_trace(self)
    
    
class Visitor:
    
    def visit_literal(self, node):
        raise NotImplementedError
    
    def visit_named_constant(self, node):
        raise NotImplementedError
    
    def visit_array(self, node):
        raise NotImplementedError
    
    def visit_struct(self, node):
        raise NotImplementedError
    
    def visit_pointer(self, node):
        raise NotImplementedError
    
    def visit_call(self, node):
        raise NotImplementedError
    
    def visit_trace(self, node):
        raise NotImplementedError


class PrettyPrinter:

    def __init__(self, formatter):
        self.formatter = formatter
    
    def visit_literal(self, node):
        if node.value is None:
            self.formatter.literal('NULL')
            return

        if isinstance(node.value, basestring):
            if len(node.value) >= 4096 or node.value.strip(string.printable):
                self.formatter.address('blob(%u)' % len(node.value))
                #self.formatter.text('...')
                return

            self.formatter.literal('"' + node.value + '"')
            return

        self.formatter.literal(repr(node.value))
    
    def visit_named_constant(self, node):
        self.formatter.literal(node.name)
    
    def visit_array(self, node):
        self.formatter.text('{')
        sep = ''
        for value in node.elements:
            self.formatter.text(sep)
            value.visit(self) 
            sep = ', '
        self.formatter.text('}')
    
    def visit_struct(self, node):
        self.formatter.text('{')
        sep = ''
        for name, value in node.members:
            self.formatter.text(sep)
            self.formatter.variable(name)
            self.formatter.text(' = ')
            value.visit(self) 
            sep = ', '
        self.formatter.text('}')
    
    def visit_pointer(self, node):
        self.formatter.address(node.address)
    
    def visit_call(self, node):
        self.formatter.text('%s ' % node.no)
        if node.klass is not None:
            self.formatter.function(node.klass + '::' + node.method)
        else:
            self.formatter.function(node.method)
        self.formatter.text('(')
        sep = ''
        for name, value in node.args:
            self.formatter.text(sep)
            self.formatter.variable(name)
            self.formatter.text(' = ')
            value.visit(self) 
            sep = ', '
        self.formatter.text(')')
        if node.ret is not None:
            self.formatter.text(' = ')
            node.ret.visit(self)
    
    def visit_trace(self, node):
        for call in node.calls:
            call.visit(self)
            self.formatter.newline()

