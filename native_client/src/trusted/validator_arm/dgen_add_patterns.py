#!/usr/bin/python
#
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#

"""
Checks (rule) patterns associated with rows in tables, and adds an
additional column to each row (in each table) which captures
constraints in rule pattern.
"""

import dgen_core

# If true, print traces of how patterns are added.
# Useful to trace how patterns are generated for one (or more) tables,
# depending on the value of _restrict_to_tables.
_trace = False

# If defined, do a detailed trace of optimizing the given pattern
# Note: This flag is used to discover the cause of a "Row not reachable"
# or a "Table XXX malformed for pattern YYY" exception. It also can be
# used to see how the $pattern test was generated in the generated decoder
# state.
_trace_detailed_pattern = None

# If defined, only optimize patterns only in the given list of table names
_restrict_to_tables = None

def add_rule_pattern_constraints(decoder):
    """Adds an additional column to each table, defining additional
       constraints assumed by rule patterns in rows.
    """
    for table in decoder.tables():
       _add_rule_pattern_constraints_to_table(decoder, table)
    return decoder

def _process_table(table):
  global _restrict_to_tables
  return table.name in _restrict_to_tables  if _restrict_to_tables else True

def _add_rule_pattern_constraints_to_table(decoder, table):
    """Adds an additional column to the given table, defining
       additional constraints assumed by rule patterns in rows.
    """
    global _trace
    if _trace and _process_table(table):
      print "*** processing table: %s ***" % table.name
    constraint_col = len(table.columns())
    table.add_column(dgen_core.BitField('$pattern', 31, 0))
    for row in table.rows():
       _add_rule_pattern_constraints_to_row(
           decoder, table, row, constraint_col)

def _add_rule_pattern_constraints_to_row(decoder, table, row, constraint_col):
    """Adds an additional (constraint) colum to the given row,
       defining additional constraints assumed by the rule
       pattern in the row.
    """
    global _trace
    if _trace and _process_table(table):
      print "consider: %s" % repr(row)
    action = row.action
    if action and action.__class__.__name__ == 'DecoderAction':
        pattern = action.pattern()
        if pattern:
          rule_pattern = table.define_pattern(pattern, constraint_col)
          if _process_table(table):
            # Figure out what bits in the pattern aren't tested when
            # reaching this row, and add a pattern to cover those bits.
            reaching_pattern = RulePatternLookup.reaching_pattern(
                decoder, table, row, pattern, constraint_col)
            row.add_pattern(reaching_pattern)
          else:
            row.add_pattern(table.define_pattern(pattern, constraint_col))
          return
    # If reached, no explicit pattern defined, so add default pattern
    row.add_pattern(table.define_pattern('-', constraint_col))

class RulePatternLookup(object):
  """Lookup state for finding what parts of an instruction rule pattern
     survive to the corresponding row of a table. This information is
     use to optimize how rule patterns are added.

     Note: Implements a table stack so that a depth-first
     search can be used. The stack is used to detect cycles,
     and report the problem if detected.

     Note: This data structure also implements a row stack. This
     stack is not really needed. However, when debugging, it can
     be very useful in describing how the current state was reached.
     Hence, it is included for that capability.
  """

  @staticmethod
  def reaching_pattern(decoder, table, row, pattern_text, pattern_column):
    """Given a rule in the given row, of the given table, of the
       given decoder, return the set of bit patterns not already
       handled.
       """

    # Create a look up state and then do a depth-first walk of possible
    # matches, to find possible (unmatched) patterns reaching the
    # given table and row.
    state = RulePatternLookup(decoder, table, row,
                              pattern_text, pattern_column)

    if state._trace_pattern():
      print "*** Tracing pattern: %s   ***" % pattern_text
      print "    table: %s" % table.name
      print "    row: %s" % repr(row)

    # Do a depth-first walk of possible matches, to find
    # possible (unmatched) patterns reaching the given table and
    # row.
    state._visit_table(decoder.primary)

    # Verify that the row can be reached!
    if not state.is_reachable:
      raise Exception("Row not reachable: %s : %s"
                      % (table.name, repr(row)))

    # Return the pattern of significant bits that could not
    # be ruled out by table (parse) patterns.
    return state.reaching_pattern

  def _trace_pattern(self):
    global _trace_detailed_pattern
    if _trace_detailed_pattern:
      return (_trace_detailed_pattern and
              self.pattern_text == _trace_detailed_pattern)

  def __init__(self, decoder, table, row, pattern_text, pattern_column):
    """Create a rule pattern lookup. Arguments are:
       decoder - The decoder being processed.
       table - The table in the decoder the row appears in.
       row - The row we are associating a pattern with.
       pattern - The (rule) pattern associated with a row.

       Uses a depth-first search to find all possible paths
       that can reach the given row in the given table, and
       what bits were already tested in that path.
       """
    self.decoder = decoder
    self.table = table
    self.row = row
    self.pattern_text = pattern_text
    # Define the corresponding pattern for the pattern text.
    self.pattern = table.define_pattern(pattern_text, pattern_column)
    # The following holds the stack of tables visited.
    self.visited_tables = []
    # The following holds the stack of rows (between tables) visited.
    self.visited_rows = []
    # The following holds the significant bits that have been shown
    # as possibly unmatched. Initially, we assume no bits are significant,
    # and let the lookup fill in bits found to be potentially significant.
    self.reaching_pattern = dgen_core.BitPattern.always_matches(
        self.pattern.column)
    # The following holds the part of the current pattern that is still
    # unmatched, or at least only partially matched, and therefore can't
    # be removed.
    self.unmatched_pattern = self.pattern
    # The following defines if the pattern is reachable!
    self.is_reachable = False

  def _visit_table(self, table):
    """Visits the given table, trying to match all rows in the table."""
    if self._trace_pattern():
      print "-> visit %s" % table.name
    if table in self.visited_tables:
      # cycle found, quit.
      raise Exception("Table %s malformed for pattern %s" %
                      (table.name, repr(self.pattern)))
      return
    self.visited_tables.append(table)
    for row in table.rows():
      self._visit_row(row)
    self.visited_tables.pop()

    if self._trace_pattern():
      print "<- visit %s" % table.name

  def _visit_row(self, row):
    """Visits the given row of a table, and updates the reaching pattern
       if there are unmatched bits for the (self) row being processed.
    """
    global _trace
    self.visited_rows.append(row)

    if self._trace_pattern():
      print 'row %s' % row

    # Before processing the row, use a copy of the unmatched pattern so
    # that we don't pollute other path searches through the tables.
    previous_unmatched = self.unmatched_pattern
    self.unmatched_pattern = self.unmatched_pattern.copy()
    matched = True  # Assume true till proven otherwise.

    # Try to match each pattern in the row, removing matched significant
    # bits from the unmatched pattern.
    for row_pattern in row.patterns:
      match = self.unmatched_pattern.categorize_match(row_pattern)
      if self._trace_pattern():
        print ('match %s : %s => %s' %
               (repr(self.unmatched_pattern), repr(row_pattern), match))
      if match == 'match':
        # Matches, i.e. all significant bits were used in the match.
        self.unmatched_pattern = (
            self.unmatched_pattern.remove_overlapping_bits(row_pattern))
        if self._trace_pattern():
          print '  unmatched = %s' % repr(self.unmatched_pattern)
      elif match == 'consistent':
        # Can't draw conclusion if any bits of pattern
        # affect the unmatched pattern. Hence, ignore this
        # pattern and continue matching remaining patterns
        # in the row.
        continue
      elif match == 'conflicts':
        # This row can't be followed because it conflicts with
        # the unmatched pattern. Give up.
        matched = False
        break
      else:
        # This should not happen!
        raise Exception("Error matching %s and %s!"
                        % (repr(row_pattern), repr(self.unmatched_pattern)))

    if matched:
      # Row (may) apply. Continue search for paths that can match
      # the pattern.
      if self._trace_pattern():
        print "row matched!"
        print "row:      %s" % repr(row)
      if row == self.row:
        # We've reached the row in the table that we are trying to
        # reach. Ssignificant bits remaining in unmatched_pattern
        # still need to be tested. Union them into the reaching pattern.
        old_reaching = self.reaching_pattern.copy()
        self.reaching_pattern = self.reaching_pattern.union_mask_and_value(
            self.unmatched_pattern)
        if self._trace_pattern():
          print ("  reaching pattern: %s => %s" %
                 (repr(old_reaching), repr(self.reaching_pattern)))
        self.is_reachable = True
        if _trace:
          print "*** pattern inference ***"
          self._print_trace()
          print ("implies: %s => %s" %
                 (repr(self.pattern), repr(self.unmatched_pattern)))
          print ("resulting in: %s => %s" %
                 (repr(old_reaching), repr(self.reaching_pattern)))
      else:
        # if action is to call another table, continue search with that table.
        if row.action and row.action.__class__.__name__ == 'DecoderMethod':
          tbl = self.decoder.get_table(row.action.name)
          if tbl:
            self._visit_table(tbl)
          else:
            raise Exception("Error: action -> %s used, but not defined" %
                            row.action.name)

    # Restore state back to before matching the row.
    self.visited_rows.pop()
    self.unmatched_pattern = previous_unmatched

  def _print_trace(self):
    for i in range(0, len(self.visited_tables)):
      print "Table %s:" % self.visited_tables[i].name
      if i < len(self.visited_rows):
        print "  %s" % self.visited_rows[i].patterns
