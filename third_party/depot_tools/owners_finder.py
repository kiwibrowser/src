# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Interactive tool for finding reviewers/owners for a change."""

import os
import copy
import owners as owners_module


def first(iterable):
  for element in iterable:
    return element


class OwnersFinder(object):
  COLOR_LINK = '\033[4m'
  COLOR_BOLD = '\033[1;32m'
  COLOR_GREY = '\033[0;37m'
  COLOR_RESET = '\033[0m'

  indentation = 0

  def __init__(self, files, local_root, author, reviewers,
               fopen, os_path,
               email_postfix='@chromium.org',
               disable_color=False,
               override_files=None):
    self.email_postfix = email_postfix

    if os.name == 'nt' or disable_color:
      self.COLOR_LINK = ''
      self.COLOR_BOLD = ''
      self.COLOR_GREY = ''
      self.COLOR_RESET = ''

    self.db = owners_module.Database(local_root, fopen, os_path)
    self.db.override_files = override_files or {}
    self.db.load_data_needed_for(files)

    self.os_path = os_path

    self.author = author

    filtered_files = files

    reviewers = list(reviewers)
    if author:
      reviewers.append(author)

    # Eliminate files that existing reviewers can review.
    filtered_files = list(self.db.files_not_covered_by(
        filtered_files, reviewers))

    # If some files are eliminated.
    if len(filtered_files) != len(files):
      files = filtered_files
      # Reload the database.
      self.db = owners_module.Database(local_root, fopen, os_path)
      self.db.override_files = override_files or {}
      self.db.load_data_needed_for(files)

    self.all_possible_owners = self.db.all_possible_owners(files, None)

    self.owners_to_files = {}
    self._map_owners_to_files(files)

    self.files_to_owners = {}
    self._map_files_to_owners()

    self.owners_score = self.db.total_costs_by_owner(
        self.all_possible_owners, files)

    self.original_files_to_owners = copy.deepcopy(self.files_to_owners)
    self.comments = self.db.comments

    # This is the queue that will be shown in the interactive questions.
    # It is initially sorted by the score in descending order. In the
    # interactive questions a user can choose to "defer" its decision, then the
    # owner will be put to the end of the queue and shown later.
    self.owners_queue = []

    self.unreviewed_files = set()
    self.reviewed_by = {}
    self.selected_owners = set()
    self.deselected_owners = set()
    self.reset()

  def run(self):
    self.reset()
    while self.owners_queue and self.unreviewed_files:
      owner = self.owners_queue[0]

      if (owner in self.selected_owners) or (owner in self.deselected_owners):
        continue

      if not any((file_name in self.unreviewed_files)
                 for file_name in self.owners_to_files[owner]):
        self.deselect_owner(owner)
        continue

      self.print_info(owner)

      while True:
        inp = self.input_command(owner)
        if inp == 'y' or inp == 'yes':
          self.select_owner(owner)
          break
        elif inp == 'n' or inp == 'no':
          self.deselect_owner(owner)
          break
        elif inp == '' or inp == 'd' or inp == 'defer':
          self.owners_queue.append(self.owners_queue.pop(0))
          break
        elif inp == 'f' or inp == 'files':
          self.list_files()
          break
        elif inp == 'o' or inp == 'owners':
          self.list_owners(self.owners_queue)
          break
        elif inp == 'p' or inp == 'pick':
          self.pick_owner(raw_input('Pick an owner: '))
          break
        elif inp.startswith('p ') or inp.startswith('pick '):
          self.pick_owner(inp.split(' ', 2)[1].strip())
          break
        elif inp == 'r' or inp == 'restart':
          self.reset()
          break
        elif inp == 'q' or inp == 'quit':
          # Exit with error
          return 1

    self.print_result()
    return 0

  def _map_owners_to_files(self, files):
    for owner in self.all_possible_owners:
      for dir_name, _ in self.all_possible_owners[owner]:
        for file_name in files:
          if file_name.startswith(dir_name):
            self.owners_to_files.setdefault(owner, set())
            self.owners_to_files[owner].add(file_name)

  def _map_files_to_owners(self):
    for owner in self.owners_to_files:
      for file_name in self.owners_to_files[owner]:
        self.files_to_owners.setdefault(file_name, set())
        self.files_to_owners[file_name].add(owner)

  def reset(self):
    self.files_to_owners = copy.deepcopy(self.original_files_to_owners)
    self.unreviewed_files = set(self.files_to_owners.keys())
    self.reviewed_by = {}
    self.selected_owners = set()
    self.deselected_owners = set()

    # Initialize owners queue, sort it by the score
    self.owners_queue = list(sorted(self.owners_to_files.keys(),
                                    key=lambda owner: self.owners_score[owner]))
    self.find_mandatory_owners()

  def select_owner(self, owner, findMandatoryOwners=True):
    if owner in self.selected_owners or owner in self.deselected_owners\
        or not (owner in self.owners_queue):
      return
    self.writeln('Selected: ' + owner)
    self.owners_queue.remove(owner)
    self.selected_owners.add(owner)
    for file_name in filter(
        lambda file_name: file_name in self.unreviewed_files,
        self.owners_to_files[owner]):
      self.unreviewed_files.remove(file_name)
      self.reviewed_by[file_name] = owner
    if findMandatoryOwners:
      self.find_mandatory_owners()

  def deselect_owner(self, owner, findMandatoryOwners=True):
    if owner in self.selected_owners or owner in self.deselected_owners\
        or not (owner in self.owners_queue):
      return
    self.writeln('Deselected: ' + owner)
    self.owners_queue.remove(owner)
    self.deselected_owners.add(owner)
    for file_name in self.owners_to_files[owner] & self.unreviewed_files:
      self.files_to_owners[file_name].remove(owner)
    if findMandatoryOwners:
      self.find_mandatory_owners()

  def find_mandatory_owners(self):
    continues = True
    for owner in self.owners_queue:
      if owner in self.selected_owners:
        continue
      if owner in self.deselected_owners:
        continue
      if len(self.owners_to_files[owner] & self.unreviewed_files) == 0:
        self.deselect_owner(owner, False)

    while continues:
      continues = False
      for file_name in filter(
          lambda file_name: len(self.files_to_owners[file_name]) == 1,
          self.unreviewed_files):
        owner = first(self.files_to_owners[file_name])
        self.select_owner(owner, False)
        continues = True
        break

  def print_comments(self, owner):
    if owner not in self.comments:
      self.writeln(self.bold_name(owner))
    else:
      self.writeln(self.bold_name(owner) + ' is commented as:')
      self.indent()
      if owners_module.GLOBAL_STATUS in self.comments[owner]:
        self.writeln(
            self.greyed(self.comments[owner][owners_module.GLOBAL_STATUS]) +
            ' (global status)')
        if len(self.comments[owner]) == 1:
          self.unindent()
          return
      for path in self.comments[owner]:
        if path == owners_module.GLOBAL_STATUS:
          continue
        elif len(self.comments[owner][path]) > 0:
          self.writeln(self.greyed(self.comments[owner][path]) +
                       ' (at ' + self.bold(path or '<root>') + ')')
        else:
          self.writeln(self.greyed('[No comment] ') + ' (at ' +
                       self.bold(path or '<root>') + ')')
      self.unindent()

  def print_file_info(self, file_name, except_owner=''):
    if file_name not in self.unreviewed_files:
      self.writeln(self.greyed(file_name +
                               ' (by ' +
                               self.bold_name(self.reviewed_by[file_name]) +
                               ')'))
    else:
      if len(self.files_to_owners[file_name]) <= 3:
        other_owners = []
        for ow in self.files_to_owners[file_name]:
          if ow != except_owner:
            other_owners.append(self.bold_name(ow))
        self.writeln(file_name +
                     ' [' + (', '.join(other_owners)) + ']')
      else:
        self.writeln(file_name + ' [' +
                     self.bold(str(len(self.files_to_owners[file_name]))) +
                     ']')

  def print_file_info_detailed(self, file_name):
    self.writeln(file_name)
    self.indent()
    for ow in sorted(self.files_to_owners[file_name]):
      if ow in self.deselected_owners:
        self.writeln(self.bold_name(self.greyed(ow)))
      elif ow in self.selected_owners:
        self.writeln(self.bold_name(self.greyed(ow)))
      else:
        self.writeln(self.bold_name(ow))
    self.unindent()

  def print_owned_files_for(self, owner):
    # Print owned files
    self.print_comments(owner)
    self.writeln(self.bold_name(owner) + ' owns ' +
                 str(len(self.owners_to_files[owner])) + ' file(s):')
    self.indent()
    for file_name in sorted(self.owners_to_files[owner]):
      self.print_file_info(file_name, owner)
    self.unindent()
    self.writeln()

  def list_owners(self, owners_queue):
    if (len(self.owners_to_files) - len(self.deselected_owners) -
            len(self.selected_owners)) > 3:
      for ow in owners_queue:
        if ow not in self.deselected_owners and ow not in self.selected_owners:
          self.print_comments(ow)
    else:
      for ow in owners_queue:
        if ow not in self.deselected_owners and ow not in self.selected_owners:
          self.writeln()
          self.print_owned_files_for(ow)

  def list_files(self):
    self.indent()
    if len(self.unreviewed_files) > 5:
      for file_name in sorted(self.unreviewed_files):
        self.print_file_info(file_name)
    else:
      for file_name in self.unreviewed_files:
        self.print_file_info_detailed(file_name)
    self.unindent()

  def pick_owner(self, ow):
    # Allowing to omit domain suffixes
    if ow not in self.owners_to_files:
      if ow + self.email_postfix in self.owners_to_files:
        ow += self.email_postfix

    if ow not in self.owners_to_files:
      self.writeln('You cannot pick ' + self.bold_name(ow) + ' manually. ' +
                   'It\'s an invalid name or not related to the change list.')
      return False
    elif ow in self.selected_owners:
      self.writeln('You cannot pick ' + self.bold_name(ow) + ' manually. ' +
                   'It\'s already selected.')
      return False
    elif ow in self.deselected_owners:
      self.writeln('You cannot pick ' + self.bold_name(ow) + ' manually.' +
                   'It\'s already unselected.')
      return False

    self.select_owner(ow)
    return True

  def print_result(self):
    # Print results
    self.writeln()
    self.writeln()
    self.writeln('** You selected these owners **')
    self.writeln()
    for owner in self.selected_owners:
      self.writeln(self.bold_name(owner) + ':')
      self.indent()
      for file_name in sorted(self.owners_to_files[owner]):
        self.writeln(file_name)
      self.unindent()

  def bold(self, text):
    return self.COLOR_BOLD + text + self.COLOR_RESET

  def bold_name(self, name):
    return (self.COLOR_BOLD +
            name.replace(self.email_postfix, '') + self.COLOR_RESET)

  def greyed(self, text):
    return self.COLOR_GREY + text + self.COLOR_RESET

  def indent(self):
    self.indentation += 1

  def unindent(self):
    self.indentation -= 1

  def print_indent(self):
    return '  ' * self.indentation

  def writeln(self, text=''):
    print self.print_indent() + text

  def hr(self):
    self.writeln('=====================')

  def print_info(self, owner):
    self.hr()
    self.writeln(
        self.bold(str(len(self.unreviewed_files))) + ' file(s) left.')
    self.print_owned_files_for(owner)

  def input_command(self, owner):
    self.writeln('Add ' + self.bold_name(owner) + ' as your reviewer? ')
    return raw_input(
        '[yes/no/Defer/pick/files/owners/quit/restart]: ').lower()
