# GDB pretty printers for STLport.
#
# Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.
# Copyright (C) 2010 Joachim Reichel
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# pylint: disable=C0103,C0111,R0201,R0903


import gdb
import re


# Set the STLport version which is needed for a few features.
#
# - for std::list:
#   STLport older than 5.0?
# - for std::deque, std::stack, and std::queue on 64bit systems:
#   STLport older than 5.2?
stlport_version = 5.2

# Indicates whether std::vector is printed with indices.
print_vector_with_indices = False


def lookup_stlport_type (typename):
    "Look up a type in the public STLport namespace."

    namespaces = ['std::', 'stlpd_std::', 'stlp_std::', '_STL::']
    for namespace in namespaces:
        try:
            return gdb.lookup_type (namespace + typename)
        except RuntimeError:
            pass

def lookup_stlport_priv_type (typename):
    "Look up a type in the private STLport namespace."

    namespaces = ['std::priv::', 'stlpd_std::priv::', 'stlp_priv::', 'stlp_std::priv::',
                  'stlpd_std::', 'stlp_std::', '_STL::']
    for namespace in namespaces:
        try:
            return gdb.lookup_type (namespace + typename)
        except RuntimeError:
            pass


def get_non_debug_impl (value, member = None):
    "Return the non-debug implementation of value or value[member]."
    if member:
        value = value[member]
    try:
        return value['_M_non_dbg_impl']
    except RuntimeError:
        return value


class RbtreeIterator:

    def __init__ (self, rbtree):
        tree = get_non_debug_impl (rbtree , '_M_t')
        self.size = tree['_M_node_count']
        self.node = tree['_M_header']['_M_data']['_M_left']
        self.count = 0

    def __iter__ (self):
        return self

    def __len__ (self):
        return int (self.size)

    def next (self):
        if self.count == self.size:
            raise StopIteration
        result = self.node
        self.count += 1
        if self.count < self.size:
            node = self.node
            # Is there a right child?
            if node.dereference()['_M_right']:
                # Walk down to left-most child in right subtree.
                node = node.dereference()['_M_right']
                while node.dereference()['_M_left']:
                    node = node.dereference()['_M_left']
            else:
                # Walk up to first parent reached via left subtree.
                parent = node.dereference()['_M_parent']
                while node == parent.dereference()['_M_right']:
                    node = parent
                    parent = parent.dereference()['_M_parent']
                node = parent
            self.node = node
        return result


class BitsetPrinter:
    "Pretty printer for std::bitset."

    def __init__(self, typename, val):
        self.typename = typename
        self.val      = val

    def to_string (self):
        # If template_argument handled values, we could print the
        # size.  Or we could use a regexp on the type.
        return '%s' % (self.typename)

    def children (self):
        words = self.val['_M_w']

        # The _M_w member can be either an unsigned long, or an
        # array.  This depends on the template specialization used.
        # If it is a single long, convert to a single element list.
        if words.type.code == gdb.TYPE_CODE_ARRAY:
            word_size = words.type.target ().sizeof
            n_words   = words.type.sizeof / word_size
        else:
            word_size = words.type.sizeof 
            n_words   = 1
            words     = [words]

        result = []
        word = 0
        while word < n_words:
            w = words[word]
            bit = 0
            while w != 0:
                if w & 1:
                    result.append (('[%d]' % (word * word_size * 8 + bit), 1))
                bit += 1
                w = w >> 1
            word += 1
        return result


class DequePrinter:
    "Pretty printer for std::deque."

    class Iterator:
        def __init__ (self, start_node, start_cur, start_last,
                      finish_cur, buffer_size):
            self.node        = start_node
            self.item        = start_cur
            self.node_last   = start_last
            self.last        = finish_cur
            self.buffer_size = buffer_size
            self.count       = 0

        def __iter__ (self):
            return self

        def next (self):
            if self.item == self.last:
                raise StopIteration
            result = ('[%d]' % self.count, self.item.dereference())
            self.count += 1
            self.item  += 1
            if self.item == self.node_last:
                self.node += 1
                self.item = self.node[0]
                self.node_last = self.item + self.buffer_size
            return result

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = get_non_debug_impl (val)
        size = val.type.template_argument(0).sizeof
        # see MAX_BYTES in stlport/stl/_alloc.h
        if stlport_version < 5.2:
            blocksize = 128
        else:
            blocksize = 32 * gdb.lookup_type ("void").pointer().sizeof
        if size < blocksize:
            self.buffer_size = int (blocksize / size)
        else:
            self.buffer_size = 1

    def to_string (self):
        start   = self.val['_M_start']
        finish  = self.val['_M_finish']
        delta_n = finish['_M_node'] - start['_M_node'] - 1
        delta_s = start['_M_last'] - start['_M_cur']
        delta_f = finish['_M_cur'] - finish['_M_first']
        if delta_n == -1:
            size = delta_f
        else:
            size = self.buffer_size * delta_n + delta_s + delta_f
        ta0 = self.val.type.template_argument (0)
        return '%s<%s> with %d elements' % (self.typename, ta0, int (size))

    def children (self):
        start  = self.val['_M_start']
        finish = self.val['_M_finish']
        return self.Iterator (start['_M_node'], start['_M_cur'],
            start['_M_last'], finish['_M_cur'], self.buffer_size)

    def display_hint (self):
        return 'array'


class ListPrinter:
    "Pretty printer for std::list."

    class Iterator:
        def __init__ (self, node_type, head):
            self.node_type = node_type
            # see empty() in stlport/stl/_list.h
            if stlport_version < 5.0:
                self.sentinel = head
            else:
                self.sentinel = head.address
            self.item  = head['_M_next']
            self.count = 0

        def __iter__ (self):
            return self

        def next (self):
            if self.item == self.sentinel:
                raise StopIteration
            node = self.item.cast (self.node_type).dereference()
            self.item = node['_M_next']
            count = self.count
            self.count += 1
            return ('[%d]' % count, node['_M_data'])

    def __init__(self, typename, val):
        self.typename = typename
        self.val = get_non_debug_impl (val)

    def children (self):
        ta0       = self.val.type.template_argument(0)
        node_type = lookup_stlport_priv_type ('_List_node<%s>' % ta0).pointer()
        return self.Iterator (node_type, self.val['_M_node']['_M_data'])

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        # see empty() in stlport/stl/_list.h
        if stlport_version < 5.0:
            sentinel = self.val['_M_node']['_M_data']
        else:
            sentinel = self.val['_M_node']['_M_data'].address
        if self.val['_M_node']['_M_data']['_M_next'] == sentinel:
            return 'empty %s<%s>' % (self.typename, ta0)
        return '%s<%s>' % (self.typename, ta0)

    def display_hint (self):
        return 'array'


class MapPrinter:
    "Pretty printer for std::map and std::multimap."

    class Iterator:

        def __init__ (self, rbiter, node_type):
            self.rbiter    = rbiter
            self.node_type = node_type
            self.count     = 0

        def __iter__ (self):
            return self

        def next (self):
            if self.count % 2 == 0:
                item = self.rbiter.next().dereference()
                self.pair = (item.cast (self.node_type))['_M_value_field']
                element = self.pair['first']
            else:
                element = self.pair['second']
            count = self.count
            self.count += 1
            return ('[%d]' % count, element)

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = val

    def children (self):
        key_type   = self.val.type.template_argument (0)
        value_type = self.val.type.template_argument (1)
        pair_type  \
            = lookup_stlport_type ('pair<%s const,%s>' % (key_type,value_type))
        node_type  \
            = lookup_stlport_priv_type ('_Rb_tree_node<%s >' % str (pair_type))
        return self.Iterator (RbtreeIterator (self.val), node_type)

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        count = get_non_debug_impl (self.val, '_M_t')['_M_node_count']
        return ('%s<%s> with %d elements' % (self.typename, ta0, count))

    def display_hint (self):
        return 'map'


class SetPrinter:
    "Pretty printer for std::set and std::multiset."

    class Iterator:
        def __init__ (self, rbiter, node_type):
            self.rbiter    = rbiter
            self.node_type = node_type
            self.count     = 0

        def __iter__ (self):
            return self

        def next (self):
            item = self.rbiter.next().dereference()
            element = (item.cast (self.node_type))['_M_value_field']
            count = self.count
            self.count += 1
            return ('[%d]' % count, element)

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = val

    def children (self):
        value_type = self.val.type.template_argument (0)
        node_type  \
            = lookup_stlport_priv_type ('_Rb_tree_node<%s>' % (value_type))
        return self.Iterator (RbtreeIterator (self.val), node_type)

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        count = get_non_debug_impl (self.val, '_M_t')['_M_node_count']
        return ('%s<%s> with %d elements' % (self.typename, ta0, count))

    def display_hint (self):
        return 'array'


class SlistPrinter:
    "Pretty printer for std::slist."

    class Iterator:
        def __init__ (self, node_type, head):
            self.node_type = node_type
            self.item  = head['_M_next']
            self.count = 0

        def __iter__ (self):
            return self

        def next (self):
            if self.item == 0:
                raise StopIteration
            node = self.item.cast (self.node_type).dereference()
            self.item = node['_M_next']
            count = self.count
            self.count += 1
            return ('[%d]' % count, node['_M_data'])

    def __init__(self, typename, val):
        self.typename = typename
        self.val = get_non_debug_impl (val)

    def children (self):
        ta0       = self.val.type.template_argument(0)
        node_type = lookup_stlport_priv_type ('_Slist_node<%s>' % ta0).pointer()
        return self.Iterator (node_type, self.val['_M_head']['_M_data'])

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        if self.val['_M_head']['_M_data']['_M_next'] == 0:
            return 'empty %s<%s>' % (self.typename, ta0)
        return '%s<%s>' % (self.typename, ta0)

    def display_hint (self):
        return 'array'


class StringPrinter:
    "Pretty printer for std::string or std::wstring."

    def __init__ (self, _typename, val):
        self.val = get_non_debug_impl (val)

    def to_string (self):
        try:
            # STLport 5.2 and later
            return self.val['_M_start_of_storage']['_M_data']
        except RuntimeError:
            try:
                # STLport 5.0 and 5.1 with short string optimization
                static_buf = self.val['_M_buffers']['_M_static_buf']
                data       = self.val['_M_end_of_storage']['_M_data']
                if static_buf.address + 1 == data:
                    ta0    = self.val.type.template_argument (0)
                    start  = static_buf.cast (ta0.pointer())
                    finish = self.val['_M_finish']
                    if start == finish:
                        # STLport 5.0 without _STLP_FORCE_STRING_TERMINATION
                        return ""
                    return start
                return self.val['_M_buffers']['_M_dynamic_buf']
            except RuntimeError:
                # STLport 5.0 and 5.1 without short string optimization,
                # and STLport 4.6
                start  = self.val['_M_start']
                finish = self.val['_M_finish']
                if start == finish:
                    # STLport 5.0 without _STLP_FORCE_STRING_TERMINATION
                    return ""
                return start

    def display_hint (self):
        return 'string'


class VectorPrinter:
    "Pretty printer for std::vector."

    class Iterator:

        def __init__ (self, start, finish, bit_vector):
            self.bit_vector = bit_vector
            self.count      = 0
            if bit_vector:
                self.item   = start['_M_p']
                self.io     = start['_M_offset']
                self.finish = finish['_M_p']
                self.fo     = finish['_M_offset']
                self.isize  = 8 * self.item.dereference().type.sizeof
            else:
                self.item   = start
                self.finish = finish

        def __iter__ (self):
            return self

        def next (self):
            count = self.count
            self.count += 1
            if self.bit_vector:
                if self.item == self.finish and self.io == self.fo:
                    raise StopIteration
                element = self.item.dereference()
                value = 0
                if element & (1 << self.io):
                    value = 1
                self.io += 1
                if self.io >= self.isize:
                    self.item += 1
                    self.io   =  0
                return ('[%d]' % count, value)
            else:
                if self.item == self.finish:
                    raise StopIteration
                element = self.item.dereference()
                self.item += 1
                return ('[%d]' % count, element)

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = get_non_debug_impl (val)
        self.bit_vector \
            = val.type.template_argument (0).code == gdb.TYPE_CODE_BOOL

    def children (self):
        start  = self.val['_M_start']
        finish = self.val['_M_finish']
        return self.Iterator (start, finish, self.bit_vector)

    def to_string (self):
        if self.bit_vector:
            start    = self.val['_M_start']['_M_p']
            so       = self.val['_M_start']['_M_offset']
            finish   = self.val['_M_finish']['_M_p']
            fo       = self.val['_M_finish']['_M_offset']
            end      = self.val['_M_end_of_storage']['_M_data']
            isize    = 8 * start.dereference().type.sizeof
            length   = (isize - so) + isize * (finish - start - 1) + fo
            capacity = isize * (end - start)
            return ('%s<bool> of length %d, capacity %d'
                % (self.typename, length, capacity))
        else:
            start    = self.val['_M_start']
            finish   = self.val['_M_finish']
            end      = self.val['_M_end_of_storage']['_M_data']
            length   = finish - start
            capacity = end - start
            ta0      = self.val.type.template_argument (0)
            return ('%s<%s> of length %d, capacity %d'
                % (self.typename, ta0, length, capacity))

    def display_hint (self):
        if print_vector_with_indices:
            return None
        else:
            return 'array'


class WrapperPrinter:
    "Pretty printer for std::stack, std::queue, and std::priority_queue."

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = val
        self.visualizer = gdb.default_visualizer (val['c'])

    def children (self):
        return self.visualizer.children()

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        return ('%s<%s>, wrapping %s'
            % (self.typename, ta0, self.visualizer.to_string()))

    def display_hint (self):
        if hasattr (self.visualizer, 'display_hint'):
            return self.visualizer.display_hint()
        return None


class UnorderedMapPrinter:
    """Pretty printer for std::tr1::unordered_map
    and std::tr1::unordered_multimap."""

    class Iterator:
        def __init__ (self, node_type, head):
            self.node_type = node_type
            self.item  = head['_M_next']
            self.count = 0

        def __iter__ (self):
            return self

        def next (self):
            if self.item == 0 and self.count % 2 == 0:
                raise StopIteration
            if self.count % 2 == 0:
                self.pair = self.item.cast (self.node_type).dereference()
                self.item = self.pair['_M_next']
                element = self.pair['_M_data']['first']
            else:
                element = self.pair['_M_data']['second']
            count = self.count
            self.count += 1
            return ('[%d]' % count, element)

    def __init__(self, typename, val):
        self.typename = typename
        self.val = get_non_debug_impl (val)

    def children (self):
        key_type   = self.val.type.template_argument (0)
        value_type = self.val.type.template_argument (1)
        pair_type  \
            = lookup_stlport_type ('pair<%s const,%s>' % (key_type,value_type))
        node_type  \
            = lookup_stlport_priv_type ('_Slist_node<%s >'
                % str (pair_type)).pointer()
        elements = get_non_debug_impl (self.val, '_M_ht')['_M_elems']
        return self.Iterator (node_type, elements['_M_head']['_M_data'])

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        length = get_non_debug_impl (self.val, '_M_ht')['_M_num_elements']
        if length == 0:
            return 'empty %s<%s>' % (self.typename, ta0)
        return '%s<%s> with %d elements' % (self.typename, ta0, length)

    def display_hint (self):
        return 'map'


class UnorderedSetPrinter:
    """Pretty printer for std::tr1::unordered_set
    and std::tr1::unordered_multiset."""

    class Iterator:
        def __init__ (self, node_type, head):
            self.node_type = node_type
            self.item  = head['_M_next']
            self.count = 0

        def __iter__ (self):
            return self

        def next (self):
            if self.item == 0:
                raise StopIteration
            node = self.item.cast (self.node_type).dereference()
            self.item = node['_M_next']
            count = self.count
            self.count += 1
            return ('[%d]' % count, node['_M_data'])

    def __init__(self, typename, val):
        self.typename = typename
        self.val = get_non_debug_impl (val)

    def children (self):
        ta0 = self.val.type.template_argument(0)
        node_type = lookup_stlport_priv_type ('_Slist_node<%s>' % ta0).pointer()
        elements = get_non_debug_impl (self.val, '_M_ht')['_M_elems']
        return self.Iterator (node_type, elements['_M_head']['_M_data'])

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        length = get_non_debug_impl (self.val, '_M_ht')['_M_num_elements']
        if length == 0:
            return 'empty %s<%s>' % (self.typename, ta0)
        return '%s<%s> with %d elements' % (self.typename, ta0, length)

    def display_hint (self):
        return 'array'


class AutoptrPrinter:
    "Pretty printer for std::auto_ptr."

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = val

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        pointer = self.val['_M_p'].cast (ta0.pointer())
        if pointer == 0:
            return ('%s<%s> (empty)' % (self.typename, ta0))
        else:
            return ('%s<%s>, pointing to %s'
                % (self.typename, ta0, pointer.dereference()))

    def display_hint (self):
        return None


class SharedptrPrinter:
    "Pretty printer for std::shared_ptr and std::weak_ptr."

    def __init__ (self, typename, val):
        self.typename = typename
        self.val = val

    def to_string (self):
        ta0 = self.val.type.template_argument (0)
        pointer = self.val['px'].cast (ta0.pointer())
        if pointer == 0:
            return ('%s<%s> (empty)' % (self.typename, ta0))
        else:
            count = self.val['pn']['pi_']['use_count_']
            return ('%s<%s> (count %d), pointing to %s'
                % (self.typename, ta0, count, pointer.dereference()))

    def display_hint (self):
        return None


def lookup_function (val):
    "Look-up and return a pretty-printer that can print val."

    type = val.type
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target()
    type = type.unqualified().strip_typedefs()

    typename = type.tag
    if typename == None:
        return None

    for function in pretty_printers_dict:
        if function.search (typename):
            return pretty_printers_dict[function] (val)
    return None


def register_stlport_printers (obj):
    "Register STLport pretty-printers with object file obj."

    if obj == None:
        obj = gdb
    obj.pretty_printers.append (lookup_function)



pretty_printers_dict = {}

def add_entry (regex, printer, typename):
    prefix = "^(stlpd?_std|_STL|std)::"
    suffix = "<.*>$"
    if typename != None:
        typename = "std::" + typename
    if regex[0:5] == "boost":
        prefix = ""
    pretty_printers_dict[re.compile (prefix+regex+suffix)] \
        = lambda val: printer (typename, val)

add_entry ("basic_string",   StringPrinter,  None)
add_entry ("bitset",         BitsetPrinter,  "bitset")
add_entry ("deque",          DequePrinter,   "deque")
add_entry ("map",            MapPrinter,     "map")
add_entry ("list",           ListPrinter,    "list")
add_entry ("multimap",       MapPrinter,     "multimap")
add_entry ("multiset",       SetPrinter,     "multiset")
add_entry ("queue",          WrapperPrinter, "queue")
add_entry ("priority_queue", WrapperPrinter, "priority_queue")
add_entry ("set",            SetPrinter,     "set")
add_entry ("slist",          SlistPrinter,   "slist")
add_entry ("stack",          WrapperPrinter, "stack")
add_entry ("vector",         VectorPrinter,  "vector")

add_entry ("tr1::unordered_map",      UnorderedMapPrinter, "tr1::unordered_map")
add_entry ("tr1::unordered_multimap", UnorderedMapPrinter, "tr1::unordered_multimap")
add_entry ("tr1::unordered_set",      UnorderedSetPrinter, "tr1::unordered_set")
add_entry ("tr1::unordered_multiset", UnorderedSetPrinter, "tr1::unordered_multiset")

add_entry ("auto_ptr",          AutoptrPrinter,   "auto_ptr")
add_entry ("boost::shared_ptr", SharedptrPrinter, "tr1::shared_ptr")
add_entry ("boost::weak_ptr",   SharedptrPrinter, "tr1::weak_ptr")
