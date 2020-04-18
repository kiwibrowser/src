# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import contextlib
import errno
import os
import re
import shutil
import StringIO
import sys
import tempfile
import unittest

import common_util
import task_manager


_GOLDEN_GRAPHVIZ = """digraph graphname {
  n0 [label="0: b", color=black, shape=ellipse];
  n1 [label="1: a", color=black, shape=ellipse];
  n2 [label="2: c", color=black, shape=ellipse];
  n0 -> n2;
  n1 -> n2;
  n3 [label="3: d", color=black, shape=ellipse];
  n2 -> n3;
  n4 [label="4: f", color=black, shape=box];
  n3 -> n4;
  n5 [label="e", color=blue, shape=ellipse];
  n5 -> n4;
}\n"""


@contextlib.contextmanager
def EatStdoutAndStderr():
  """Overrides sys.std{out,err} to intercept write calls."""
  sys.stdout.flush()
  sys.stderr.flush()
  original_stdout = sys.stdout
  original_stderr = sys.stderr
  try:
    sys.stdout = StringIO.StringIO()
    sys.stderr = StringIO.StringIO()
    yield
  finally:
    sys.stdout = original_stdout
    sys.stderr = original_stderr


class TestException(Exception):
  pass


class TaskManagerTestCase(unittest.TestCase):
  def setUp(self):
    self.output_directory = tempfile.mkdtemp()

  def tearDown(self):
    shutil.rmtree(self.output_directory)

  def OutputPath(self, file_path):
    return os.path.join(self.output_directory, file_path)

  def TouchOutputFile(self, file_path):
    with open(self.OutputPath(file_path), 'w') as output:
      output.write(file_path + '\n')


class TaskTest(TaskManagerTestCase):
  def testTaskExecution(self):
    def Recipe():
      Recipe.counter += 1
    Recipe.counter = 0
    task = task_manager.Task('hello.json', 'what/ever/hello.json', [], Recipe)
    self.assertFalse(task._is_done)
    self.assertEqual(0, Recipe.counter)
    task.Execute()
    self.assertEqual(1, Recipe.counter)
    task.Execute()
    self.assertEqual(1, Recipe.counter)

  def testTaskExecutionWithUnexecutedDeps(self):
    def RecipeA():
      self.fail()

    def RecipeB():
      RecipeB.counter += 1
    RecipeB.counter = 0

    a = task_manager.Task('hello.json', 'out/hello.json', [], RecipeA)
    b = task_manager.Task('hello.json', 'out/hello.json', [a], RecipeB)
    self.assertEqual(0, RecipeB.counter)
    b.Execute()
    self.assertEqual(1, RecipeB.counter)


class BuilderTest(TaskManagerTestCase):
  def testRegisterTask(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('hello.txt')
    def TaskA():
      TaskA.executed = True
    TaskA.executed = False
    self.assertEqual(os.path.join(self.output_directory, 'hello.txt'),
                     TaskA.path)
    self.assertFalse(TaskA.executed)
    TaskA.Execute()
    self.assertTrue(TaskA.executed)

  def testRegisterDuplicateTask(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('hello.txt')
    def TaskA():
      pass
    del TaskA # unused
    with self.assertRaises(task_manager.TaskError):
      @builder.RegisterTask('hello.txt')
      def TaskB():
        pass
      del TaskB # unused

  def testTaskMerging(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('hello.txt')
    def TaskA():
      pass
    @builder.RegisterTask('hello.txt', merge=True)
    def TaskB():
      pass
    self.assertEqual(TaskA, TaskB)

  def testOutputSubdirectory(self):
    builder = task_manager.Builder(self.output_directory, 'subdir')

    @builder.RegisterTask('world.txt')
    def TaskA():
      pass
    del TaskA # unused

    self.assertIn('subdir/world.txt', builder._tasks)
    self.assertNotIn('subdir/subdir/world.txt', builder._tasks)
    self.assertNotIn('world.txt', builder._tasks)

    @builder.RegisterTask('subdir/world.txt')
    def TaskB():
      pass
    del TaskB # unused
    self.assertIn('subdir/subdir/world.txt', builder._tasks)
    self.assertNotIn('world.txt', builder._tasks)


class GenerateScenarioTest(TaskManagerTestCase):
  def testParents(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      pass
    @builder.RegisterTask('b', dependencies=[TaskA])
    def TaskB():
      pass
    @builder.RegisterTask('c', dependencies=[TaskB])
    def TaskC():
      pass
    scenario = task_manager.GenerateScenario([TaskA, TaskB, TaskC], set())
    self.assertListEqual([TaskA, TaskB, TaskC], scenario)

    scenario = task_manager.GenerateScenario([TaskB], set())
    self.assertListEqual([TaskA, TaskB], scenario)

    scenario = task_manager.GenerateScenario([TaskC], set())
    self.assertListEqual([TaskA, TaskB, TaskC], scenario)

    scenario = task_manager.GenerateScenario([TaskC, TaskB], set())
    self.assertListEqual([TaskA, TaskB, TaskC], scenario)

  def testFreezing(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      pass
    @builder.RegisterTask('b', dependencies=[TaskA])
    def TaskB():
      pass
    @builder.RegisterTask('c')
    def TaskC():
      pass
    @builder.RegisterTask('d', dependencies=[TaskB, TaskC])
    def TaskD():
      pass

    # assert no exception raised.
    task_manager.GenerateScenario([TaskB], set([TaskC]))

    with self.assertRaises(task_manager.TaskError):
      task_manager.GenerateScenario([TaskD], set([TaskA]))

    self.TouchOutputFile('a')
    scenario = task_manager.GenerateScenario([TaskD], set([TaskA]))
    self.assertListEqual([TaskB, TaskC, TaskD], scenario)

    self.TouchOutputFile('b')
    scenario = task_manager.GenerateScenario([TaskD], set([TaskB]))
    self.assertListEqual([TaskC, TaskD], scenario)

  def testCycleError(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      pass
    @builder.RegisterTask('b', dependencies=[TaskA])
    def TaskB():
      pass
    @builder.RegisterTask('c', dependencies=[TaskB])
    def TaskC():
      pass
    @builder.RegisterTask('d', dependencies=[TaskC])
    def TaskD():
      pass
    TaskA._dependencies.append(TaskC)
    with self.assertRaises(task_manager.TaskError):
      task_manager.GenerateScenario([TaskD], set())

  def testCollisionError(self):
    builder_a = task_manager.Builder(self.output_directory, None)
    builder_b = task_manager.Builder(self.output_directory, None)
    @builder_a.RegisterTask('a')
    def TaskA():
      pass
    @builder_b.RegisterTask('a')
    def TaskB():
      pass
    with self.assertRaises(task_manager.TaskError):
      task_manager.GenerateScenario([TaskA, TaskB], set())

  def testGenerateDependentSetPerTask(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      pass
    @builder.RegisterTask('b')
    def TaskB():
      pass
    @builder.RegisterTask('c', dependencies=[TaskA, TaskB])
    def TaskC():
      pass
    @builder.RegisterTask('d', dependencies=[TaskA])
    def TaskD():
      pass

    def RunSubTest(expected, scenario, task):
      self.assertEqual(
          expected, task_manager.GenerateDependentSetPerTask(scenario)[task])

    RunSubTest(set([]), [TaskA], TaskA)
    RunSubTest(set([]), [TaskA, TaskB], TaskA)
    RunSubTest(set([TaskC]), [TaskA, TaskB, TaskC], TaskA)
    RunSubTest(set([TaskC, TaskD]), [TaskA, TaskB, TaskC, TaskD], TaskA)
    RunSubTest(set([]), [TaskA, TaskD], TaskD)

  def testGraphVizOutput(self):
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      pass
    @builder.RegisterTask('b')
    def TaskB():
      pass
    @builder.RegisterTask('c', dependencies=[TaskB, TaskA])
    def TaskC():
      pass
    @builder.RegisterTask('d', dependencies=[TaskC])
    def TaskD():
      pass
    @builder.RegisterTask('e')
    def TaskE():
      pass
    @builder.RegisterTask('f', dependencies=[TaskD, TaskE])
    def TaskF():
      pass
    self.TouchOutputFile('e')
    scenario = task_manager.GenerateScenario([TaskF], set([TaskE]))
    output = StringIO.StringIO()
    task_manager.OutputGraphViz(scenario, [TaskF], output)
    self.assertEqual(_GOLDEN_GRAPHVIZ, output.getvalue())

  def testListResumingTasksToFreeze(self):
    TaskManagerTestCase.setUp(self)
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      pass
    @builder.RegisterTask('b')
    def TaskB():
      pass
    @builder.RegisterTask('c', dependencies=[TaskA, TaskB])
    def TaskC():
      pass
    @builder.RegisterTask('d', dependencies=[TaskA])
    def TaskD():
      pass
    @builder.RegisterTask('e', dependencies=[TaskC])
    def TaskE():
      pass
    @builder.RegisterTask('f', dependencies=[TaskC])
    def TaskF():
      pass

    for k in 'abcdef':
      self.TouchOutputFile(k)

    def RunSubTest(
        final_tasks, initial_frozen_tasks, skipped_tasks, reference):
      scenario = task_manager.GenerateScenario(
          final_tasks, initial_frozen_tasks)
      resume_frozen_tasks = task_manager.ListResumingTasksToFreeze(
          scenario, final_tasks, skipped_tasks)
      self.assertEqual(reference, resume_frozen_tasks)

      new_scenario = \
          task_manager.GenerateScenario(final_tasks, resume_frozen_tasks)
      self.assertEqual(skipped_tasks, set(new_scenario))

    RunSubTest([TaskA], set([]), set([TaskA]), [])
    RunSubTest([TaskD], set([]), set([TaskA, TaskD]), [])
    RunSubTest([TaskD], set([]), set([TaskD]), [TaskA])
    RunSubTest([TaskE, TaskF], set([TaskA]), set([TaskB, TaskC, TaskE, TaskF]),
               [TaskA])
    RunSubTest([TaskE, TaskF], set([TaskA]), set([TaskC, TaskE, TaskF]),
               [TaskA, TaskB])
    RunSubTest([TaskE, TaskF], set([TaskA]), set([TaskE, TaskF]), [TaskC])
    RunSubTest([TaskE, TaskF], set([TaskA]), set([TaskF]), [TaskE, TaskC])
    RunSubTest([TaskD, TaskE, TaskF], set([]), set([TaskD, TaskF]),
               [TaskA, TaskE, TaskC])


class CommandLineControlledExecutionTest(TaskManagerTestCase):
  def setUp(self):
    TaskManagerTestCase.setUp(self)
    self.with_raise_exception_tasks = False
    self.task_execution_history = None

  def Execute(self, command_line_args):
    self.task_execution_history = []
    builder = task_manager.Builder(self.output_directory, None)
    @builder.RegisterTask('a')
    def TaskA():
      self.task_execution_history.append(TaskA.name)
    @builder.RegisterTask('b')
    def TaskB():
      self.task_execution_history.append(TaskB.name)
    @builder.RegisterTask('c', dependencies=[TaskA, TaskB])
    def TaskC():
      self.task_execution_history.append(TaskC.name)
    @builder.RegisterTask('d', dependencies=[TaskA])
    def TaskD():
      self.task_execution_history.append(TaskD.name)
    @builder.RegisterTask('e', dependencies=[TaskC])
    def TaskE():
      self.task_execution_history.append(TaskE.name)
    @builder.RegisterTask('raise_exception', dependencies=[TaskD])
    def RaiseExceptionTask():
      self.task_execution_history.append(RaiseExceptionTask.name)
      raise TestException('Expected error.')
    @builder.RegisterTask('raise_keyboard_interrupt', dependencies=[TaskD])
    def RaiseKeyboardInterruptTask():
      self.task_execution_history.append(RaiseKeyboardInterruptTask.name)
      raise KeyboardInterrupt
    @builder.RegisterTask('sudden_death', dependencies=[TaskD])
    def SimulateKillTask():
      self.task_execution_history.append(SimulateKillTask.name)
      raise MemoryError
    @builder.RegisterTask('timeout_error', dependencies=[TaskD])
    def SimulateTimeoutError():
      self.task_execution_history.append(SimulateTimeoutError.name)
      raise common_util.TimeoutError
    @builder.RegisterTask('errno_ENOSPC', dependencies=[TaskD])
    def SimulateENOSPC():
      self.task_execution_history.append(SimulateENOSPC.name)
      raise IOError(errno.ENOSPC, os.strerror(errno.ENOSPC))
    @builder.RegisterTask('errno_EPERM', dependencies=[TaskD])
    def SimulateEPERM():
      self.task_execution_history.append(SimulateEPERM.name)
      raise IOError(errno.EPERM, os.strerror(errno.EPERM))

    default_final_tasks = [TaskD, TaskE]
    if self.with_raise_exception_tasks:
      default_final_tasks.extend([
          RaiseExceptionTask,
          RaiseKeyboardInterruptTask,
          SimulateKillTask,
          SimulateTimeoutError,
          SimulateENOSPC,
          SimulateEPERM])
    task_parser = task_manager.CommandLineParser()
    parser = argparse.ArgumentParser(parents=[task_parser],
        fromfile_prefix_chars=task_manager.FROMFILE_PREFIX_CHARS)
    cmd = ['-o', self.output_directory]
    cmd.extend([i for i in command_line_args])
    args = parser.parse_args(cmd)
    with EatStdoutAndStderr():
      return task_manager.ExecuteWithCommandLine(args, default_final_tasks)

  def ResumeFilePath(self):
    return self.OutputPath(task_manager._TASK_RESUME_ARGUMENTS_FILE)

  def ResumeCmd(self):
    return task_manager.FROMFILE_PREFIX_CHARS + self.ResumeFilePath()

  def testSimple(self):
    self.assertEqual(0, self.Execute([]))
    self.assertListEqual(['a', 'd', 'b', 'c', 'e'], self.task_execution_history)

  def testDryRun(self):
    self.assertEqual(0, self.Execute(['-d']))
    self.assertListEqual([], self.task_execution_history)

  def testRegex(self):
    self.assertEqual(0, self.Execute(['-e', 'b', '-e', 'd']))
    self.assertListEqual(['b', 'a', 'd'], self.task_execution_history)
    self.assertEqual(1, self.Execute(['-e', r'\d']))
    self.assertListEqual([], self.task_execution_history)

  def testFreezing(self):
    self.assertEqual(0, self.Execute(['-f', r'\d']))
    self.assertListEqual(['a', 'd', 'b', 'c', 'e'], self.task_execution_history)
    self.TouchOutputFile('c')
    self.assertEqual(0, self.Execute(['-f', 'c']))
    self.assertListEqual(['a', 'd', 'e'], self.task_execution_history)

  def testDontFreezeUnreachableTasks(self):
    self.TouchOutputFile('c')
    self.assertEqual(0, self.Execute(['-e', 'e', '-f', 'c', '-f', 'd']))

  def testAbortOnFirstError(self):
    ARGS = ['-e', 'exception', '-e', r'^b$']
    self.with_raise_exception_tasks = True
    self.assertEqual(1, self.Execute(ARGS))
    self.assertListEqual(
        ['a', 'd', 'raise_exception'], self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual('-f\n^d$', resume_input.read())

    self.TouchOutputFile('d')
    self.assertEqual(1, self.Execute(ARGS + [self.ResumeCmd()]))
    self.assertListEqual(['raise_exception'], self.task_execution_history)

    self.assertEqual(1, self.Execute(ARGS + [self.ResumeCmd()]))
    self.assertListEqual(['raise_exception'], self.task_execution_history)

    self.assertEqual(1, self.Execute(ARGS + [self.ResumeCmd(), '-k']))
    self.assertListEqual(['raise_exception', 'b'], self.task_execution_history)

  def testKeepGoing(self):
    ARGS = ['-k', '-e', 'exception', '-e', r'^b$']
    self.with_raise_exception_tasks = True
    self.assertEqual(1, self.Execute(ARGS))
    self.assertListEqual(
        ['a', 'd', 'raise_exception', 'b'], self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual('-f\n^d$\n-f\n^b$', resume_input.read())

    self.TouchOutputFile('d')
    self.TouchOutputFile('b')
    self.assertEqual(1, self.Execute(ARGS + [self.ResumeCmd()]))
    self.assertListEqual(['raise_exception'], self.task_execution_history)

    self.assertEqual(1, self.Execute(ARGS + [self.ResumeCmd()]))
    self.assertListEqual(['raise_exception'], self.task_execution_history)

  def testKeyboardInterrupt(self):
    self.with_raise_exception_tasks = True
    with self.assertRaises(KeyboardInterrupt):
      self.Execute(
          ['-k', '-e', 'raise_keyboard_interrupt', '-e', r'^b$'])
    self.assertListEqual(['a', 'd', 'raise_keyboard_interrupt'],
                         self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual('-f\n^d$', resume_input.read())

  def testResumeAfterSuddenDeath(self):
    EXPECTED_RESUME_FILE_CONTENT = '-f\n^a$\n-f\n^d$\n'
    ARGS = ['-k', '-e', 'sudden_death', '-e', r'^a$']
    self.with_raise_exception_tasks = True
    with self.assertRaises(MemoryError):
      self.Execute(ARGS)
    self.assertListEqual(
        ['a', 'd', 'sudden_death'], self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual(EXPECTED_RESUME_FILE_CONTENT, resume_input.read())

    self.TouchOutputFile('a')
    self.TouchOutputFile('d')
    with self.assertRaises(MemoryError):
      self.Execute(ARGS + [self.ResumeCmd()])
    self.assertListEqual(['sudden_death'], self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual(EXPECTED_RESUME_FILE_CONTENT, resume_input.read())

    with self.assertRaises(MemoryError):
      self.Execute(ARGS + [self.ResumeCmd()])
    self.assertListEqual(['sudden_death'], self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual(EXPECTED_RESUME_FILE_CONTENT, resume_input.read())

  def testTimeoutError(self):
    self.with_raise_exception_tasks = True
    self.Execute(['-k', '-e', 'timeout_error', '-e', r'^b$'])
    self.assertListEqual(['a', 'd', 'timeout_error', 'b'],
                         self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual('-f\n^d$\n-f\n^b$', resume_input.read())

  def testENOSPC(self):
    self.with_raise_exception_tasks = True
    with self.assertRaises(IOError):
      self.Execute(['-k', '-e', 'errno_ENOSPC', '-e', r'^a$'])
    self.assertListEqual(
        ['a', 'd', 'errno_ENOSPC'], self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual('-f\n^a$\n-f\n^d$\n', resume_input.read())

  def testEPERM(self):
    self.with_raise_exception_tasks = True
    self.Execute(['-k', '-e', 'errno_EPERM', '-e', r'^b$'])
    self.assertListEqual(['a', 'd', 'errno_EPERM', 'b'],
                         self.task_execution_history)
    with open(self.ResumeFilePath()) as resume_input:
      self.assertEqual('-f\n^d$\n-f\n^b$', resume_input.read())

  def testImpossibleTasks(self):
    self.assertEqual(1, self.Execute(['-f', r'^a$', '-e', r'^c$']))
    self.assertListEqual([], self.task_execution_history)

    self.assertEqual(0, self.Execute(
        ['-f', r'^a$', '-e', r'^c$', '-e', r'^b$']))
    self.assertListEqual(['b'], self.task_execution_history)


if __name__ == '__main__':
  unittest.main()
