"""Source List Parser

The syntax of a source list file is a very small subset of GNU Make.  These
features are supported

 operators: =, +=, :=
 line continuation
 non-nested variable expansion
 comment

The goal is to allow Makefile's and SConscript's to share source listing.
"""

class SourceListParser(object):
    def __init__(self):
        self.symbol_table = {}
        self._reset()

    def _reset(self, filename=None):
        self.filename = filename

        self.line_no = 1
        self.line_cont = ''

    def _error(self, msg):
        raise RuntimeError('%s:%d: %s' % (self.filename, self.line_no, msg))

    def _next_dereference(self, val, cur):
        """Locate the next $(...) in value."""
        deref_pos = val.find('$', cur)
        if deref_pos < 0:
            return (-1, -1)
        elif val[deref_pos + 1] != '(':
            self._error('non-variable dereference')

        deref_end = val.find(')', deref_pos + 2)
        if deref_end < 0:
            self._error('unterminated variable dereference')

        return (deref_pos, deref_end + 1)

    def _expand_value(self, val):
        """Perform variable expansion."""
        expanded = ''
        cur = 0
        while True:
            deref_pos, deref_end = self._next_dereference(val, cur)
            if deref_pos < 0:
                expanded += val[cur:]
                break

            sym = val[(deref_pos + 2):(deref_end - 1)]
            expanded += val[cur:deref_pos] + self.symbol_table[sym]
            cur = deref_end

        return expanded

    def _parse_definition(self, line):
        """Parse a variable definition line."""
        op_pos = line.find('=')
        op_end = op_pos + 1
        if op_pos < 0:
            self._error('not a variable definition')

        if op_pos > 0:
            if line[op_pos - 1] in [':', '+', '?']:
                op_pos -= 1
        else:
            self._error('only =, :=, and += are supported')

        # set op, sym, and val
        op = line[op_pos:op_end]
        sym = line[:op_pos].strip()
        val = self._expand_value(line[op_end:].lstrip())

        if op in ('=', ':='):
            self.symbol_table[sym] = val
        elif op == '+=':
            self.symbol_table[sym] += ' ' + val
        elif op == '?=':
            if sym not in self.symbol_table:
                self.symbol_table[sym] = val

    def _parse_line(self, line):
        """Parse a source list line."""
        # more lines to come
        if line and line[-1] == '\\':
            # spaces around "\\\n" are replaced by a single space
            if self.line_cont:
                self.line_cont += line[:-1].strip() + ' '
            else:
                self.line_cont = line[:-1].rstrip() + ' '
            return 0

        # combine with previous lines
        if self.line_cont:
            line = self.line_cont + line.lstrip()
            self.line_cont = ''

        if line:
            begins_with_tab = (line[0] == '\t')

            line = line.lstrip()
            if line[0] != '#':
                if begins_with_tab:
                    self._error('recipe line not supported')
                else:
                    self._parse_definition(line)

        return 1

    def parse(self, filename):
        """Parse a source list file."""
        if self.filename != filename:
            fp = open(filename)
            lines = fp.read().splitlines()
            fp.close()

            try:
                self._reset(filename)
                for line in lines:
                    self.line_no += self._parse_line(line)
            except:
                self._reset()
                raise

        return self.symbol_table

    def add_symbol(self, name, value):
        self.symbol_table[name] = value
