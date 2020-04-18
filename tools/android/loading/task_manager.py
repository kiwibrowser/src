# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""API that build and execute recipes wrapped into a task dependency graph.

A Task consists of a 'recipe' (a closure to be executed) and a list of refs to
tasks that should be executed prior to executing this Task (i.e. dependencies).
The responsibility of the recipe of a task is to produce the file with the name
assigned at task creation.

A scenario is a ordered list of tasks to execute such that the dependencies of a
given task are execute before the said task. The scenario is built from a list
of final tasks and a list of frozen tasks:
  - A final task is a task to execute ultimately. Therefore the scenario is
    composed of final tasks and their required intermediary tasks.
  - A frozen task is task to not execute. This is a mechanism to morph a task
    that may have dependencies to a task with no dependency at scenario
    generation time, injecting what the task have already produced before as an
    input of the smaller tasks dependency graph covered by the scenario.

Example:
  # -------------------------------------------------- Build my dependency graph
  builder = Builder('my/output/dir')

  @builder.RegisterTask('out0')
  def BuildOut0():
    Produce(out=BuildOut0.path)

  @builder.RegisterTask('out1')
  def BuildOut1():
    Produce(out=BuildOut1.path)

  @builder.RegisterTask('out2', dependencies=[BuildOut0, BuildOut1])
  def BuildOut2():
    DoStuff(BuildOut0.path, BuildOut1.path, out=BuildOut2.path)

  @builder.RegisterTask('out3', dependencies=[BuildOut0])
  def BuildOut3():
    DoStuff(BuildOut0.path, out=BuildOut3.path)

  # ---------------------------- Case 1: Execute BuildOut3 and its dependencies.
  for task in GenerateScenario(final_tasks=[BuildOut3], frozen_tasks=[])
    task.Execute()

  # ---------- Case 2: Execute BuildOut2 and its dependencies but not BuildOut1.
  # It is required that BuildOut1.path is already existing.
  for task in GenerateScenario(final_tasks=[BuildOut2],
                               frozen_tasks=[BuildOut1])
    task.Execute()
"""


import argparse
import collections
import datetime
import errno
import logging
import os
import re
import subprocess
import sys

import common_util


_TASK_LOGS_DIR_NAME = 'logs'
_TASK_GRAPH_DOTFILE_NAME = 'tasks_graph.dot'
_TASK_GRAPH_PNG_NAME = 'tasks_graph.png'
_TASK_RESUME_ARGUMENTS_FILE = 'resume.txt'
_TASK_EXECUTION_LOG_NAME_FORMAT = 'task-execution-%Y-%m-%d-%H-%M-%S.log'

FROMFILE_PREFIX_CHARS = '@'


class TaskError(Exception):
  pass


class Task(object):
  """Task with a recipe."""

  def __init__(self, name, path, dependencies, recipe):
    """Constructor.

    Args:
      name: The name of the  task.
      path: Path to the file or directory that this task produces.
      dependencies: List of parent task to execute before.
      recipe: Function to execute.
    """
    self.name = name
    self.path = path
    self._dependencies = dependencies
    self._recipe = recipe
    self._is_done = recipe == None

  def Execute(self):
    """Executes this task."""
    if not self._is_done:
      self._recipe()
    self._is_done = True


class Builder(object):
  """Utilities for creating sub-graphs of tasks with dependencies."""

  def __init__(self, output_directory, output_subdirectory):
    """Constructor.

    Args:
      output_directory: Output directory where the tasks work.
      output_subdirectory: Subdirectory to put all created tasks in or None.
    """
    self.output_directory = output_directory
    self.output_subdirectory = output_subdirectory
    self._tasks = {}

  # Caution:
  #   This decorator may not create a task in the case where merge=True and
  #   another task having the same name have already been created. In this case,
  #   it will just reuse the former task. This is at the user responsibility to
  #   ensure that merged tasks would do the exact same thing.
  #
  #     @builder.RegisterTask('hello')
  #     def TaskA():
  #       my_object.a = 1
  #
  #     @builder.RegisterTask('hello', merge=True)
  #     def TaskB():
  #       # This function won't be executed ever.
  #       my_object.a = 2 # <------- Wrong because different from what TaskA do.
  #
  #     assert TaskA == TaskB
  #     TaskB.Execute() # Sets set my_object.a == 1
  def RegisterTask(self, task_name, dependencies=None, merge=False):
    """Decorator that wraps a function into a task.

    Args:
      task_name: The name of this new task to register.
      dependencies: List of SandwichTarget to build before this task.
      merge: If a task already have this name, don't create a new one and
        reuse the existing one.

    Returns:
      A Task that was created by wrapping the function or an existing registered
      wrapper (that have wrapped a different function).
    """
    rebased_task_name = self._RebaseTaskName(task_name)
    dependencies = dependencies or []
    def InnerAddTaskWithNewPath(recipe):
      if rebased_task_name in self._tasks:
        if not merge:
          raise TaskError('Task {} already exists.'.format(rebased_task_name))
        task = self._tasks[rebased_task_name]
        return task
      task_path = self.RebaseOutputPath(task_name)
      task = Task(rebased_task_name, task_path, dependencies, recipe)
      self._tasks[rebased_task_name] = task
      return task
    return InnerAddTaskWithNewPath

  def RebaseOutputPath(self, builder_relative_path):
    """Rebases buider relative path."""
    return os.path.join(
        self.output_directory, self._RebaseTaskName(builder_relative_path))

  def _RebaseTaskName(self,  task_name):
    if self.output_subdirectory:
      return os.path.join(self.output_subdirectory, task_name)
    return task_name


def GenerateScenario(final_tasks, frozen_tasks):
  """Generates a list of tasks to execute in order of dependencies-first.

  Args:
    final_tasks: The final tasks to generate the scenario from.
    frozen_tasks: Sets of task to freeze.

  Returns:
    [Task]
  """
  scenario = []
  task_paths = {}
  def InternalAppendTarget(task):
    if task in frozen_tasks:
      if not os.path.exists(task.path):
        raise TaskError('Frozen target `{}`\'s path doesn\'t exist.'.format(
            task.name))
      return
    if task.path in task_paths:
      if task_paths[task.path] == None:
        raise TaskError('Target `{}` depends on itself.'.format(task.name))
      if task_paths[task.path] != task:
        raise TaskError(
            'Tasks `{}` and `{}` produce the same file: `{}`.'.format(
                task.name, task_paths[task.path].name, task.path))
      return
    task_paths[task.path] = None
    for dependency in task._dependencies:
      InternalAppendTarget(dependency)
    task_paths[task.path] = task
    scenario.append(task)

  for final_task in final_tasks:
    InternalAppendTarget(final_task)
  return scenario


def GenerateDependentSetPerTask(scenario):
  """Maps direct dependents per tasks of scenario.

  Args:
    scenario: The scenario containing the Tasks to map.

  Returns:
    {Task: set(Task)}
  """
  task_set = set(scenario)
  task_children = collections.defaultdict(set)
  for task in scenario:
    for parent in task._dependencies:
      if parent in task_set:
        task_children[parent].add(task)
  return task_children


def ListResumingTasksToFreeze(scenario, final_tasks, skipped_tasks):
  """Lists the tasks that one needs to freeze to be able to resume the scenario
  after failure.

  Args:
    scenario: The scenario (list of Task) to be resumed.
    final_tasks: The list of final Task used to generate the scenario.
    skipped_tasks: Set of Tasks in the scenario that were skipped.

  Returns:
    [Task]
  """
  scenario_tasks = set(scenario)
  assert skipped_tasks.issubset(scenario_tasks)
  frozen_tasks = []
  frozen_task_set = set()
  walked_tasks = set()

  def InternalWalk(task):
    if task in walked_tasks:
      return
    walked_tasks.add(task)
    if task not in scenario_tasks or task not in skipped_tasks:
      if task not in frozen_task_set:
        frozen_task_set.add(task)
        frozen_tasks.append(task)
    else:
      for dependency in task._dependencies:
        InternalWalk(dependency)

  for final_task in final_tasks:
    InternalWalk(final_task)
  return frozen_tasks


def OutputGraphViz(scenario, final_tasks, output):
  """Outputs the build dependency graph covered by this scenario.

  Args:
    scenario: The generated scenario.
    final_tasks: The final tasks used to generate the scenario.
    output: A file-like output stream to receive the dot file.

  Graph interpretations:
    - Final tasks (the one that where directly appended) are box shaped.
    - Non final tasks are ellipse shaped.
    - Frozen tasks have a blue shape.
  """
  task_execution_ids = {t: i for i, t in enumerate(scenario)}
  tasks_node_ids = dict()

  def GetTaskNodeId(task):
    if task in tasks_node_ids:
      return tasks_node_ids[task]
    node_id = len(tasks_node_ids)
    node_label = task.name
    node_color = 'blue'
    node_shape = 'ellipse'
    if task in task_execution_ids:
      node_color = 'black'
      node_label = str(task_execution_ids[task]) + ': ' + node_label
    if task in final_tasks:
      node_shape = 'box'
    output.write('  n{} [label="{}", color={}, shape={}];\n'.format(
        node_id, node_label, node_color, node_shape))
    tasks_node_ids[task] = node_id
    return node_id

  output.write('digraph graphname {\n')
  for task in scenario:
    task_node_id = GetTaskNodeId(task)
    for dep in task._dependencies:
      dep_node_id = GetTaskNodeId(dep)
      output.write('  n{} -> n{};\n'.format(dep_node_id, task_node_id))
  output.write('}\n')


def CommandLineParser():
  """Creates command line arguments parser meant to be used as a parent parser
  for any entry point that use the ExecuteWithCommandLine() function.

  The root parser must be created with:
    fromfile_prefix_chars=FROMFILE_PREFIX_CHARS.

  Returns:
    The command line arguments parser.
  """
  parser = argparse.ArgumentParser(add_help=False)
  parser.add_argument('-d', '--dry-run', action='store_true',
                      help='Only prints the tasks to build.')
  parser.add_argument('-e', '--to-execute', metavar='REGEX', type=str,
                      action='append', dest='run_regexes', default=[],
                      help='Regex selecting tasks to execute.')
  parser.add_argument('-f', '--to-freeze', metavar='REGEX', type=str,
                      action='append', dest='frozen_regexes', default=[],
                      help='Regex selecting tasks to not execute.')
  parser.add_argument('-k', '--keep-going', action='store_true', default=False,
                      help='Keep going when some targets can\'t be made.')
  parser.add_argument('-o', '--output', type=str, required=True,
                      help='Path of the output directory.')
  parser.add_argument('-v', '--output-graphviz', action='store_true',
      help='Outputs the {} and {} file in the output directory.'
           ''.format(_TASK_GRAPH_DOTFILE_NAME, _TASK_GRAPH_PNG_NAME))
  return parser


def _SelectTasksFromCommandLineRegexes(args, default_final_tasks):
  frozen_regexes = [common_util.VerboseCompileRegexOrAbort(e)
                      for e in args.frozen_regexes]
  run_regexes = [common_util.VerboseCompileRegexOrAbort(e)
                   for e in args.run_regexes]

  # Lists final tasks.
  final_tasks = default_final_tasks
  if run_regexes:
    final_tasks = []
    # Traverse the graph in the normal execution order starting from
    # |default_final_tasks| in case of command line regex selection.
    tasks = GenerateScenario(default_final_tasks, frozen_tasks=set())
    # Order of run regexes prevails on the traversing order of tasks.
    for regex in run_regexes:
      for task in tasks:
        if regex.search(task.name):
          final_tasks.append(task)

  # Lists parents of |final_tasks| to freeze.
  frozen_tasks = set()
  impossible_tasks = set()
  if frozen_regexes:
    complete_scenario = GenerateScenario(final_tasks, frozen_tasks=set())
    dependents_per_task = GenerateDependentSetPerTask(complete_scenario)
    def MarkTaskAsImpossible(task):
      if task in impossible_tasks:
        return
      impossible_tasks.add(task)
      for dependent in dependents_per_task[task]:
        MarkTaskAsImpossible(dependent)

    for task in complete_scenario:
      for regex in frozen_regexes:
        if regex.search(task.name):
          if os.path.exists(task.path):
            frozen_tasks.add(task)
          else:
            MarkTaskAsImpossible(task)
          break

  return [t for t in final_tasks if t not in impossible_tasks], frozen_tasks


class _ResumingFileBuilder(object):
  def __init__(self, args):
    resume_path = os.path.join(args.output, _TASK_RESUME_ARGUMENTS_FILE)
    self._resume_output = open(resume_path, 'w')
    # List initial freezing regexes not to loose track of final targets to
    # freeze in case of severals resume attempts caused by sudden death.
    for regex in args.frozen_regexes:
      self._resume_output.write('-f\n{}\n'.format(regex))

  def __enter__(self):
    return self

  def __exit__(self, exc_type, exc_value, exc_traceback):
    del exc_type, exc_value, exc_traceback # unused
    self._resume_output.close()

  def OnTaskSuccess(self, task):
    # Log the succeed tasks so that they are ensured to be frozen in case
    # of a sudden death.
    self._resume_output.write('-f\n^{}$\n'.format(re.escape(task.name)))
    # Makes sure the task freezing command line make it to the disk.
    self._resume_output.flush()
    os.fsync(self._resume_output.fileno())

  def OnScenarioFinish(
      self, scenario, final_tasks, failed_tasks, skipped_tasks):
    resume_additonal_arguments = []
    for task in ListResumingTasksToFreeze(
        scenario, final_tasks, skipped_tasks):
      resume_additonal_arguments.extend(
          ['-f', '^{}$'.format(re.escape(task.name))])
    self._resume_output.seek(0)
    self._resume_output.truncate()
    self._resume_output.write('\n'.join(resume_additonal_arguments))
    print '# Looks like something went wrong in tasks:'
    for failed_task in failed_tasks:
      print '#   {}'.format(failed_task.name)
    print '#'
    print '# To resume, append the following parameter:'
    print '#   ' + FROMFILE_PREFIX_CHARS + self._resume_output.name


def ExecuteWithCommandLine(args, default_final_tasks):
  """Helper to execute tasks using command line arguments.

  Args:
    args: Command line argument parsed with CommandLineParser().
    default_final_tasks: Default final tasks if there is no -r command
      line arguments.

  Returns:
    0 if success or 1 otherwise
  """
  # Builds the scenario.
  final_tasks, frozen_tasks = _SelectTasksFromCommandLineRegexes(
      args, default_final_tasks)
  scenario = GenerateScenario(final_tasks, frozen_tasks)
  if len(scenario) == 0:
    logging.error('No tasks to build.')
    return 1

  if not os.path.isdir(args.output):
    os.makedirs(args.output)

  # Print the task dependency graph visualization.
  if args.output_graphviz:
    graphviz_path = os.path.join(args.output, _TASK_GRAPH_DOTFILE_NAME)
    png_graph_path = os.path.join(args.output, _TASK_GRAPH_PNG_NAME)
    with open(graphviz_path, 'w') as output:
      OutputGraphViz(scenario, final_tasks, output)
    subprocess.check_call(['dot', '-Tpng', graphviz_path, '-o', png_graph_path])

  # Use the build scenario.
  if args.dry_run:
    for task in scenario:
      print task.name
    return 0

  # Run the Scenario while saving intermediate state to be able to resume later.
  failed_tasks = []
  tasks_to_skip = set()
  dependents_per_task = GenerateDependentSetPerTask(scenario)

  def MarkTaskNotToExecute(task):
    if task not in tasks_to_skip:
      logging.warning('can not execute task: %s', task.name)
      tasks_to_skip.add(task)
      for dependent in dependents_per_task[task]:
        MarkTaskNotToExecute(dependent)

  log_filename = datetime.datetime.now().strftime(
      _TASK_EXECUTION_LOG_NAME_FORMAT)
  log_path = os.path.join(args.output, _TASK_LOGS_DIR_NAME, log_filename)
  if not os.path.isdir(os.path.dirname(log_path)):
    os.makedirs(os.path.dirname(log_path))
  formatter = logging.Formatter('[%(asctime)s] %(levelname)s: %(message)s')
  handler = logging.FileHandler(log_path, mode='a')
  handler.setFormatter(formatter)
  logging.getLogger().addHandler(handler)
  logging.info(
      '%s %s', '-' * 60, common_util.GetCommandLineForLogging(sys.argv))
  try:
    with _ResumingFileBuilder(args) as resume_file_builder:
      for task_execute_id, task in enumerate(scenario):
        if task in tasks_to_skip:
          continue
        logging.info('%s %s', '-' * 60, task.name)
        try:
          task.Execute()
        except (MemoryError, SyntaxError):
          raise
        except BaseException:
          # The resuming file being incrementally generated by
          # resume_file_builder.OnTaskSuccess() is automatically fsynced().
          # But resume_file_builder.OnScenarioFinish() completely rewrite
          # this file with the mininal subset of task to freeze, and in case
          # of an ENOSPC, we don't want to touch the resuming file at all so
          # that it remains uncorrupted.
          if (sys.exc_info()[0] == IOError and
              sys.exc_info()[1].errno == errno.ENOSPC):
            raise
          logging.exception('%s %s failed', '-' * 60, task.name)
          failed_tasks.append(task)
          if args.keep_going and sys.exc_info()[0] != KeyboardInterrupt:
            MarkTaskNotToExecute(task)
          else:
            tasks_to_skip.update(set(scenario[task_execute_id:]))
            break
        else:
          resume_file_builder.OnTaskSuccess(task)
      if tasks_to_skip:
        assert failed_tasks
        resume_file_builder.OnScenarioFinish(
            scenario, final_tasks, failed_tasks, tasks_to_skip)
        if sys.exc_info()[0] == KeyboardInterrupt:
          raise
        return 1
  finally:
    logging.getLogger().removeHandler(handler)
  assert not failed_tasks
  return 0
