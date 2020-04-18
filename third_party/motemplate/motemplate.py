# Copyright 2012 Benjamin Kalman
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# TODO: Escaping control characters somehow. e.g. \{{, \{{-.

import json
import re

'''Motemplate templates are data binding templates more-than-loosely inspired by
ctemplate. Use like:

  from motemplate import Motemplate

  template = Motemplate('hello {{#foo bar/}} world')
  input = {
    'foo': [
      { 'bar': 1 },
      { 'bar': 2 },
      { 'bar': 3 }
    ]
  }
  print(template.render(input).text)

Motemplate will use get() on contexts to return values, so to create custom
getters (for example, something that populates values lazily from keys), just
provide an object with a get() method.

  class CustomContext(object):
    def get(self, key):
      return 10
  print(Motemplate('hello {{world}}').render(CustomContext()).text)

will print 'hello 10'.
'''

class ParseException(Exception):
  '''The exception thrown while parsing a template.
  '''
  def __init__(self, error):
    Exception.__init__(self, error)

class RenderResult(object):
  '''The result of a render operation.
  '''
  def __init__(self, text, errors):
    self.text = text;
    self.errors = errors

  def __repr__(self):
    return '%s(text=%s, errors=%s)' % (type(self).__name__,
                                       self.text,
                                       self.errors)

  def __str__(self):
    return repr(self)

class _StringBuilder(object):
  '''Efficiently builds strings.
  '''
  def __init__(self):
    self._buf = []

  def __len__(self):
    self._Collapse()
    return len(self._buf[0])

  def Append(self, string):
    if not isinstance(string, basestring):
      string = str(string)
    self._buf.append(string)

  def ToString(self):
    self._Collapse()
    return self._buf[0]

  def _Collapse(self):
    self._buf = [u''.join(self._buf)]

  def __repr__(self):
    return self.ToString()

  def __str__(self):
    return repr(self)

class _Contexts(object):
  '''Tracks a stack of context objects, providing efficient key/value retrieval.
  '''
  class _Node(object):
    '''A node within the stack. Wraps a real context and maintains the key/value
    pairs seen so far.
    '''
    def __init__(self, value):
      self._value = value
      self._value_has_get = hasattr(value, 'get')
      self._found = {}

    def GetKeys(self):
      '''Returns the list of keys that |_value| contains.
      '''
      return self._found.keys()

    def Get(self, key):
      '''Returns the value for |key|, or None if not found (including if
      |_value| doesn't support key retrieval).
      '''
      if not self._value_has_get:
        return None
      value = self._found.get(key)
      if value is not None:
        return value
      value = self._value.get(key)
      if value is not None:
        self._found[key] = value
      return value

    def __repr__(self):
      return 'Node(value=%s, found=%s)' % (self._value, self._found)

    def __str__(self):
      return repr(self)

  def __init__(self, globals_):
    '''Initializes with the initial global contexts, listed in order from most
    to least important.
    '''
    self._nodes = map(_Contexts._Node, globals_)
    self._first_local = len(self._nodes)
    self._value_info = {}

  def CreateFromGlobals(self):
    new = _Contexts([])
    new._nodes = self._nodes[:self._first_local]
    new._first_local = self._first_local
    return new

  def Push(self, context):
    self._nodes.append(_Contexts._Node(context))

  def Pop(self):
    node = self._nodes.pop()
    assert len(self._nodes) >= self._first_local
    for found_key in node.GetKeys():
      # [0] is the stack of nodes that |found_key| has been found in.
      self._value_info[found_key][0].pop()

  def FirstLocal(self):
    if len(self._nodes) == self._first_local:
      return None
    return self._nodes[-1]._value

  def Resolve(self, path):
    # This method is only efficient at finding |key|; if |tail| has a value (and
    # |key| evaluates to an indexable value) we'll need to descend into that.
    key, tail = path.split('.', 1) if '.' in path else (path, None)
    found = self._FindNodeValue(key)
    if tail is None:
      return found
    for part in tail.split('.'):
      if not hasattr(found, 'get'):
        return None
      found = found.get(part)
    return found

  def Scope(self, context, fn, *args):
    self.Push(context)
    try:
      return fn(*args)
    finally:
      self.Pop()

  def _FindNodeValue(self, key):
    # |found_node_list| will be all the nodes that |key| has been found in.
    # |checked_node_set| are those that have been checked.
    info = self._value_info.get(key)
    if info is None:
      info = ([], set())
      self._value_info[key] = info
    found_node_list, checked_node_set = info

    # Check all the nodes not yet checked for |key|.
    newly_found = []
    for node in reversed(self._nodes):
      if node in checked_node_set:
        break
      value = node.Get(key)
      if value is not None:
        newly_found.append(node)
      checked_node_set.add(node)

    # The nodes will have been found in reverse stack order. After extending
    # the found nodes, the freshest value will be at the tip of the stack.
    found_node_list.extend(reversed(newly_found))
    if not found_node_list:
      return None

    return found_node_list[-1]._value.get(key)

class _Stack(object):
  class Entry(object):
    def __init__(self, name, id_):
      self.name = name
      self.id_ = id_

  def __init__(self, entries=[]):
    self.entries = entries

  def Descend(self, name, id_):
    descended = list(self.entries)
    descended.append(_Stack.Entry(name, id_))
    return _Stack(entries=descended)

class _InternalContext(object):
  def __init__(self):
    self._render_state = None

  def SetRenderState(self, render_state):
    self._render_state = render_state

  def get(self, key):
    if key == 'errors':
      errors = self._render_state._errors
      return '\n'.join(errors) if errors else None
    return None

class _RenderState(object):
  '''The state of a render call.
  '''
  def __init__(self, name, contexts, _stack=_Stack()):
    self.text = _StringBuilder()
    self.contexts = contexts
    self._name = name
    self._errors = []
    self._stack = _stack

  def AddResolutionError(self, id_, description=None):
    message = id_.CreateResolutionErrorMessage(self._name, stack=self._stack)
    if description is not None:
      message = '%s (%s)' % (message, description)
    self._errors.append(message)

  def Copy(self):
    return _RenderState(
        self._name, self.contexts, _stack=self._stack)

  def ForkPartial(self, custom_name, id_):
    name = custom_name or id_.name
    return _RenderState(name,
                        self.contexts.CreateFromGlobals(),
                        _stack=self._stack.Descend(name, id_))

  def Merge(self, render_state, text_transform=None):
    self._errors.extend(render_state._errors)
    text = render_state.text.ToString()
    if text_transform is not None:
      text = text_transform(text)
    self.text.Append(text)

  def GetResult(self):
    return RenderResult(self.text.ToString(), self._errors);

class _Identifier(object):
  '''An identifier of the form 'foo', 'foo.bar.baz', 'foo-bar.baz', etc.
  '''
  _VALID_ID_MATCHER = re.compile(r'^[a-zA-Z0-9@_/-]+$')

  def __init__(self, name, line, column):
    self.name = name
    self.line = line
    self.column = column
    if name == '':
      raise ParseException('Empty identifier %s' % self.GetDescription())
    for part in name.split('.'):
      if not _Identifier._VALID_ID_MATCHER.match(part):
        raise ParseException('Invalid identifier %s' % self.GetDescription())

  def GetDescription(self):
    return '\'%s\' at line %s column %s' % (self.name, self.line, self.column)

  def CreateResolutionErrorMessage(self, name, stack=None):
    message = _StringBuilder()
    message.Append('Failed to resolve %s in %s\n' % (self.GetDescription(),
                                                     name))
    if stack is not None:
      for entry in reversed(stack.entries):
        message.Append('  included as %s in %s\n' % (entry.id_.GetDescription(),
                                                     entry.name))
    return message.ToString().strip()

  def __repr__(self):
    return self.name

  def __str__(self):
    return repr(self)

class _Node(object): pass

class _LeafNode(_Node):
  def __init__(self, start_line, end_line):
    self._start_line = start_line
    self._end_line = end_line

  def StartsWithNewLine(self):
    return False

  def TrimStartingNewLine(self):
    pass

  def TrimEndingSpaces(self):
    return 0

  def TrimEndingNewLine(self):
    pass

  def EndsWithEmptyLine(self):
    return False

  def GetStartLine(self):
    return self._start_line

  def GetEndLine(self):
    return self._end_line

  def __str__(self):
    return repr(self)

class _DecoratorNode(_Node):
  def __init__(self, content):
    self._content = content

  def StartsWithNewLine(self):
    return self._content.StartsWithNewLine()

  def TrimStartingNewLine(self):
    self._content.TrimStartingNewLine()

  def TrimEndingSpaces(self):
    return self._content.TrimEndingSpaces()

  def TrimEndingNewLine(self):
    self._content.TrimEndingNewLine()

  def EndsWithEmptyLine(self):
    return self._content.EndsWithEmptyLine()

  def GetStartLine(self):
    return self._content.GetStartLine()

  def GetEndLine(self):
    return self._content.GetEndLine()

  def __repr__(self):
    return str(self._content)

  def __str__(self):
    return repr(self)

class _InlineNode(_DecoratorNode):
  def __init__(self, content):
    _DecoratorNode.__init__(self, content)

  def Render(self, render_state):
    content_render_state = render_state.Copy()
    self._content.Render(content_render_state)
    render_state.Merge(content_render_state,
                       text_transform=lambda text: text.replace('\n', ''))

class _IndentedNode(_DecoratorNode):
  def __init__(self, content, indentation):
    _DecoratorNode.__init__(self, content)
    self._indent_str = ' ' * indentation

  def Render(self, render_state):
    if isinstance(self._content, _CommentNode):
      return
    def inlinify(text):
      if len(text) == 0:  # avoid rendering a blank line
        return ''
      buf = _StringBuilder()
      buf.Append(self._indent_str)
      buf.Append(text.replace('\n', '\n%s' % self._indent_str))
      if not text.endswith('\n'):  # partials will often already end in a \n
        buf.Append('\n')
      return buf.ToString()
    content_render_state = render_state.Copy()
    self._content.Render(content_render_state)
    render_state.Merge(content_render_state, text_transform=inlinify)

class _BlockNode(_DecoratorNode):
  def __init__(self, content):
    _DecoratorNode.__init__(self, content)
    content.TrimStartingNewLine()
    content.TrimEndingSpaces()

  def Render(self, render_state):
    self._content.Render(render_state)

class _NodeCollection(_Node):
  def __init__(self, nodes):
    assert nodes
    self._nodes = nodes

  def Render(self, render_state):
    for node in self._nodes:
      node.Render(render_state)

  def StartsWithNewLine(self):
    return self._nodes[0].StartsWithNewLine()

  def TrimStartingNewLine(self):
    self._nodes[0].TrimStartingNewLine()

  def TrimEndingSpaces(self):
    return self._nodes[-1].TrimEndingSpaces()

  def TrimEndingNewLine(self):
    self._nodes[-1].TrimEndingNewLine()

  def EndsWithEmptyLine(self):
    return self._nodes[-1].EndsWithEmptyLine()

  def GetStartLine(self):
    return self._nodes[0].GetStartLine()

  def GetEndLine(self):
    return self._nodes[-1].GetEndLine()

  def __repr__(self):
    return ''.join(str(node) for node in self._nodes)

class _StringNode(_Node):
  '''Just a string.
  '''
  def __init__(self, string, start_line, end_line):
    self._string = string
    self._start_line = start_line
    self._end_line = end_line

  def Render(self, render_state):
    render_state.text.Append(self._string)

  def StartsWithNewLine(self):
    return self._string.startswith('\n')

  def TrimStartingNewLine(self):
    if self.StartsWithNewLine():
      self._string = self._string[1:]

  def TrimEndingSpaces(self):
    original_length = len(self._string)
    self._string = self._string[:self._LastIndexOfSpaces()]
    return original_length - len(self._string)

  def TrimEndingNewLine(self):
    if self._string.endswith('\n'):
      self._string = self._string[:len(self._string) - 1]

  def EndsWithEmptyLine(self):
    index = self._LastIndexOfSpaces()
    return index == 0 or self._string[index - 1] == '\n'

  def _LastIndexOfSpaces(self):
    index = len(self._string)
    while index > 0 and self._string[index - 1] == ' ':
      index -= 1
    return index

  def GetStartLine(self):
    return self._start_line

  def GetEndLine(self):
    return self._end_line

  def __repr__(self):
    return self._string

class _EscapedVariableNode(_LeafNode):
  '''{{foo}}
  '''
  def __init__(self, id_):
    _LeafNode.__init__(self, id_.line, id_.line)
    self._id = id_

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if value is None:
      render_state.AddResolutionError(self._id)
      return
    string = value if isinstance(value, basestring) else str(value)
    render_state.text.Append(string.replace('&', '&amp;')
                                   .replace('<', '&lt;')
                                   .replace('>', '&gt;'))

  def __repr__(self):
    return '{{%s}}' % self._id

class _UnescapedVariableNode(_LeafNode):
  '''{{{foo}}}
  '''
  def __init__(self, id_):
    _LeafNode.__init__(self, id_.line, id_.line)
    self._id = id_

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if value is None:
      render_state.AddResolutionError(self._id)
      return
    string = value if isinstance(value, basestring) else str(value)
    render_state.text.Append(string)

  def __repr__(self):
    return '{{{%s}}}' % self._id

class _CommentNode(_LeafNode):
  '''{{- This is a comment -}}
  An empty placeholder node for correct indented rendering behaviour.
  '''
  def __init__(self, start_line, end_line):
    _LeafNode.__init__(self, start_line, end_line)

  def Render(self, render_state):
    pass

  def __repr__(self):
    return '<comment>'

class _SectionNode(_DecoratorNode):
  '''{{#var:foo}} ... {{/foo}}
  '''
  def __init__(self, bind_to, id_, content):
    _DecoratorNode.__init__(self, content)
    self._bind_to = bind_to
    self._id = id_

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if isinstance(value, list):
      for item in value:
        if self._bind_to is not None:
          render_state.contexts.Scope({self._bind_to.name: item},
                                      self._content.Render, render_state)
        else:
          self._content.Render(render_state)
    elif hasattr(value, 'get'):
      if self._bind_to is not None:
        render_state.contexts.Scope({self._bind_to.name: value},
                                    self._content.Render, render_state)
      else:
        render_state.contexts.Scope(value, self._content.Render, render_state)
    else:
      render_state.AddResolutionError(self._id)

  def __repr__(self):
    return '{{#%s}}%s{{/%s}}' % (
        self._id, _DecoratorNode.__repr__(self), self._id)

class _VertedSectionNode(_DecoratorNode):
  '''{{?var:foo}} ... {{/foo}}
  '''
  def __init__(self, bind_to, id_, content):
    _DecoratorNode.__init__(self, content)
    self._bind_to = bind_to
    self._id = id_

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if _VertedSectionNode.ShouldRender(value):
      if self._bind_to is not None:
        render_state.contexts.Scope({self._bind_to.name: value},
                                    self._content.Render, render_state)
      else:
        self._content.Render(render_state)

  def __repr__(self):
    return '{{?%s}}%s{{/%s}}' % (
        self._id, _DecoratorNode.__repr__(self), self._id)

  @staticmethod
  def ShouldRender(value):
    if value is None:
      return False
    if isinstance(value, bool):
      return value
    if isinstance(value, list):
      return len(value) > 0
    return True

class _InvertedSectionNode(_DecoratorNode):
  '''{{^foo}} ... {{/foo}}
  '''
  def __init__(self, bind_to, id_, content):
    _DecoratorNode.__init__(self, content)
    if bind_to is not None:
      raise ParseException('{{^%s:%s}} does not support variable binding'
                           % (bind_to, id_))
    self._id = id_

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if not _VertedSectionNode.ShouldRender(value):
      self._content.Render(render_state)

  def __repr__(self):
    return '{{^%s}}%s{{/%s}}' % (
        self._id, _DecoratorNode.__repr__(self), self._id)

class _AssertionNode(_LeafNode):
  '''{{!foo Some comment about foo}}
  '''
  def __init__(self, id_, description):
    _LeafNode.__init__(self, id_.line, id_.line)
    self._id = id_
    self._description = description

  def Render(self, render_state):
    if render_state.contexts.Resolve(self._id.name) is None:
      render_state.AddResolutionError(self._id, description=self._description)

  def __repr__(self):
    return '{{!%s %s}}' % (self._id, self._description)

class _JsonNode(_LeafNode):
  '''{{*foo}}
  '''
  def __init__(self, id_):
    _LeafNode.__init__(self, id_.line, id_.line)
    self._id = id_

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if value is None:
      render_state.AddResolutionError(self._id)
      return
    render_state.text.Append(json.dumps(value, separators=(',',':')))

  def __repr__(self):
    return '{{*%s}}' % self._id

class _PartialNodeWithArguments(_DecoratorNode):
  def __init__(self, partial, args):
    if isinstance(partial, Motemplate):
      # Preserve any get() method that the caller has added.
      if hasattr(partial, 'get'):
        self.get = partial.get
      partial = partial._top_node
    _DecoratorNode.__init__(self, partial)
    self._partial = partial
    self._args = args

  def Render(self, render_state):
    render_state.contexts.Scope(self._args, self._partial.Render, render_state)

class _PartialNodeInContext(_DecoratorNode):
  def __init__(self, partial, context):
    if isinstance(partial, Motemplate):
      # Preserve any get() method that the caller has added.
      if hasattr(partial, 'get'):
        self.get = partial.get
      partial = partial._top_node
    _DecoratorNode.__init__(self, partial)
    self._partial = partial
    self._context = context

  def Render(self, render_state):
    original_contexts = render_state.contexts
    try:
      render_state.contexts = self._context
      render_state.contexts.Scope(
          # The first local context of |original_contexts| will be the
          # arguments that were passed to the partial, if any.
          original_contexts.FirstLocal() or {},
          self._partial.Render, render_state)
    finally:
      render_state.contexts = original_contexts

class _PartialNode(_LeafNode):
  '''{{+var:foo}} ... {{/foo}}
  '''
  def __init__(self, bind_to, id_, content):
    _LeafNode.__init__(self, id_.line, id_.line)
    self._bind_to = bind_to
    self._id = id_
    self._content = content
    self._args = None
    self._pass_through_id = None

  @classmethod
  def Inline(cls, id_):
    return cls(None, id_, None)

  def Render(self, render_state):
    value = render_state.contexts.Resolve(self._id.name)
    if value is None:
      render_state.AddResolutionError(self._id)
      return
    if not isinstance(value, (Motemplate, _Node)):
      render_state.AddResolutionError(self._id, description='not a partial')
      return

    if isinstance(value, Motemplate):
      node, name = value._top_node, value._name
    else:
      node, name = value, None

    partial_render_state = render_state.ForkPartial(name, self._id)

    arg_context = {}
    if self._pass_through_id is not None:
      context = render_state.contexts.Resolve(self._pass_through_id.name)
      if context is not None:
        arg_context[self._pass_through_id.name] = context
    if self._args is not None:
      def resolve_args(args):
        resolved = {}
        for key, value in args.iteritems():
          if isinstance(value, dict):
            assert len(value.keys()) == 1
            id_of_partial, partial_args = value.items()[0]
            partial = render_state.contexts.Resolve(id_of_partial.name)
            if partial is not None:
              resolved[key] = _PartialNodeWithArguments(
                  partial, resolve_args(partial_args))
          else:
            context = render_state.contexts.Resolve(value.name)
            if context is not None:
              resolved[key] = context
        return resolved
      arg_context.update(resolve_args(self._args))
    if self._bind_to and self._content:
      arg_context[self._bind_to.name] = _PartialNodeInContext(
          self._content, render_state.contexts)
    if arg_context:
      partial_render_state.contexts.Push(arg_context)

    node.Render(partial_render_state)

    render_state.Merge(
        partial_render_state,
        text_transform=lambda text: text[:-1] if text.endswith('\n') else text)

  def SetArguments(self, args):
    self._args = args

  def PassThroughArgument(self, id_):
    self._pass_through_id = id_

  def __repr__(self):
    return '{{+%s}}' % self._id

_TOKENS = {}

class _Token(object):
  '''The tokens that can appear in a template.
  '''
  class Data(object):
    def __init__(self, name, text, clazz):
      self.name = name
      self.text = text
      self.clazz = clazz
      _TOKENS[text] = self

    def ElseNodeClass(self):
      if self.clazz == _VertedSectionNode:
        return _InvertedSectionNode
      if self.clazz == _InvertedSectionNode:
        return _VertedSectionNode
      return None

    def __repr__(self):
      return self.name

    def __str__(self):
      return repr(self)

  OPEN_START_SECTION = Data(
      'OPEN_START_SECTION'         , '{{#', _SectionNode)
  OPEN_START_VERTED_SECTION = Data(
      'OPEN_START_VERTED_SECTION'  , '{{?', _VertedSectionNode)
  OPEN_START_INVERTED_SECTION = Data(
      'OPEN_START_INVERTED_SECTION', '{{^', _InvertedSectionNode)
  OPEN_ASSERTION = Data(
      'OPEN_ASSERTION'             , '{{!', _AssertionNode)
  OPEN_JSON = Data(
      'OPEN_JSON'                  , '{{*', _JsonNode)
  OPEN_PARTIAL = Data(
      'OPEN_PARTIAL'               , '{{+', _PartialNode)
  OPEN_ELSE = Data(
      'OPEN_ELSE'                  , '{{:', None)
  OPEN_END_SECTION = Data(
      'OPEN_END_SECTION'           , '{{/', None)
  INLINE_END_SECTION = Data(
      'INLINE_END_SECTION'         , '/}}', None)
  OPEN_UNESCAPED_VARIABLE = Data(
      'OPEN_UNESCAPED_VARIABLE'    , '{{{', _UnescapedVariableNode)
  CLOSE_MUSTACHE3 = Data(
      'CLOSE_MUSTACHE3'            , '}}}', None)
  OPEN_COMMENT = Data(
      'OPEN_COMMENT'               , '{{-', _CommentNode)
  CLOSE_COMMENT = Data(
      'CLOSE_COMMENT'              , '-}}', None)
  OPEN_VARIABLE = Data(
      'OPEN_VARIABLE'              , '{{' , _EscapedVariableNode)
  CLOSE_MUSTACHE = Data(
      'CLOSE_MUSTACHE'             , '}}' , None)
  CHARACTER = Data(
      'CHARACTER'                  , '.'  , None)

class _TokenStream(object):
  '''Tokeniser for template parsing.
  '''
  def __init__(self, string):
    self.next_token = None
    self.next_line = 1
    self.next_column = 0
    self._string = string
    self._cursor = 0
    self.Advance()

  def HasNext(self):
    return self.next_token is not None

  def NextCharacter(self):
    if self.next_token is _Token.CHARACTER:
      return self._string[self._cursor - 1]
    return None

  def Advance(self):
    if self._cursor > 0 and self._string[self._cursor - 1] == '\n':
      self.next_line += 1
      self.next_column = 0
    elif self.next_token is not None:
      self.next_column += len(self.next_token.text)

    self.next_token = None

    if self._cursor == len(self._string):
      return None
    assert self._cursor < len(self._string)

    if (self._cursor + 1 < len(self._string) and
        self._string[self._cursor + 1] in '{}'):
      self.next_token = (
          _TOKENS.get(self._string[self._cursor:self._cursor+3]) or
          _TOKENS.get(self._string[self._cursor:self._cursor+2]))

    if self.next_token is None:
      self.next_token = _Token.CHARACTER

    self._cursor += len(self.next_token.text)
    return self

  def AdvanceOver(self, token, description=None):
    parse_error = None
    if not self.next_token:
      parse_error = 'Reached EOF but expected %s' % token.name
    elif self.next_token is not token:
      parse_error = 'Expecting token %s but got %s at line %s' % (
                     token.name, self.next_token.name, self.next_line)
    if parse_error:
      parse_error += ' %s' % description or ''
      raise ParseException(parse_error)
    return self.Advance()

  def AdvanceOverSeparator(self, char, description=None):
    self.SkipWhitespace()
    next_char = self.NextCharacter()
    if next_char != char:
      parse_error = 'Expected \'%s\'. got \'%s\'' % (char, next_char)
      if description is not None:
        parse_error += ' (%s)' % description
      raise ParseException(parse_error)
    self.AdvanceOver(_Token.CHARACTER)
    self.SkipWhitespace()

  def AdvanceOverNextString(self, excluded=''):
    start = self._cursor - len(self.next_token.text)
    while (self.next_token is _Token.CHARACTER and
           # Can use -1 here because token length of CHARACTER is 1.
           self._string[self._cursor - 1] not in excluded):
      self.Advance()
    end = self._cursor - (len(self.next_token.text) if self.next_token else 0)
    return self._string[start:end]

  def AdvanceToNextWhitespace(self):
    return self.AdvanceOverNextString(excluded=' \n\r\t')

  def SkipWhitespace(self):
    while (self.next_token is _Token.CHARACTER and
           # Can use -1 here because token length of CHARACTER is 1.
           self._string[self._cursor - 1] in ' \n\r\t'):
      self.Advance()

  def __repr__(self):
    return '%s(next_token=%s, remainder=%s)' % (type(self).__name__,
                                                self.next_token,
                                                self._string[self._cursor:])

  def __str__(self):
    return repr(self)

class Motemplate(object):
  '''A motemplate template.
  '''
  def __init__(self, template, name=None):
    self.source = template
    self._name = name
    tokens = _TokenStream(template)
    self._top_node = self._ParseSection(tokens)
    if not self._top_node:
      raise ParseException('Template is empty')
    if tokens.HasNext():
      raise ParseException('There are still tokens remaining at %s, '
                           'was there an end-section without a start-section?' %
                           tokens.next_line)

  def _ParseSection(self, tokens):
    nodes = []
    while tokens.HasNext():
      if tokens.next_token in (_Token.OPEN_END_SECTION,
                               _Token.OPEN_ELSE):
        # Handled after running parseSection within the SECTION cases, so this
        # is a terminating condition. If there *is* an orphaned
        # OPEN_END_SECTION, it will be caught by noticing that there are
        # leftover tokens after termination.
        break
      elif tokens.next_token in (_Token.CLOSE_MUSTACHE,
                                 _Token.CLOSE_MUSTACHE3):
        raise ParseException('Orphaned %s at line %s' % (tokens.next_token.name,
                                                         tokens.next_line))
      nodes += self._ParseNextOpenToken(tokens)

    for i, node in enumerate(nodes):
      if isinstance(node, _StringNode):
        continue

      previous_node = nodes[i - 1] if i > 0 else None
      next_node = nodes[i + 1] if i < len(nodes) - 1 else None
      rendered_node = None

      if node.GetStartLine() != node.GetEndLine():
        rendered_node = _BlockNode(node)
        if previous_node:
          previous_node.TrimEndingSpaces()
        if next_node:
          next_node.TrimStartingNewLine()
      elif ((not previous_node or previous_node.EndsWithEmptyLine()) and
            (not next_node or next_node.StartsWithNewLine())):
        indentation = 0
        if previous_node:
          indentation = previous_node.TrimEndingSpaces()
        if next_node:
          next_node.TrimStartingNewLine()
        rendered_node = _IndentedNode(node, indentation)
      else:
        rendered_node = _InlineNode(node)

      nodes[i] = rendered_node

    if len(nodes) == 0:
      return None
    if len(nodes) == 1:
      return nodes[0]
    return _NodeCollection(nodes)

  def _ParseNextOpenToken(self, tokens):
    next_token = tokens.next_token

    if next_token is _Token.CHARACTER:
      # Plain strings.
      start_line = tokens.next_line
      string = tokens.AdvanceOverNextString()
      return [_StringNode(string, start_line, tokens.next_line)]
    elif next_token in (_Token.OPEN_VARIABLE,
                        _Token.OPEN_UNESCAPED_VARIABLE,
                        _Token.OPEN_JSON):
      # Inline nodes that don't take arguments.
      tokens.Advance()
      close_token = (_Token.CLOSE_MUSTACHE3
                     if next_token is _Token.OPEN_UNESCAPED_VARIABLE else
                     _Token.CLOSE_MUSTACHE)
      id_ = self._NextIdentifier(tokens)
      tokens.AdvanceOver(close_token)
      return [next_token.clazz(id_)]
    elif next_token is _Token.OPEN_ASSERTION:
      # Inline nodes that take arguments.
      tokens.Advance()
      id_ = self._NextIdentifier(tokens)
      node = next_token.clazz(id_, tokens.AdvanceOverNextString())
      tokens.AdvanceOver(_Token.CLOSE_MUSTACHE)
      return [node]
    elif next_token in (_Token.OPEN_PARTIAL,
                        _Token.OPEN_START_SECTION,
                        _Token.OPEN_START_VERTED_SECTION,
                        _Token.OPEN_START_INVERTED_SECTION):
      # Block nodes, though they may have inline syntax like {{#foo bar /}}.
      tokens.Advance()
      bind_to, id_ = None, self._NextIdentifier(tokens)
      if tokens.NextCharacter() == ':':
        # This section has the format {{#bound:id}} as opposed to just {{id}}.
        # That is, |id_| is actually the identifier to bind what the section
        # is producing, not the identifier of where to find that content.
        tokens.AdvanceOverSeparator(':')
        bind_to, id_ = id_, self._NextIdentifier(tokens)
      partial_args = None
      if next_token is _Token.OPEN_PARTIAL:
        partial_args = self._ParsePartialNodeArgs(tokens)
        if tokens.next_token is not _Token.CLOSE_MUSTACHE:
          # Inline syntax for partial types.
          if bind_to is not None:
            raise ParseException(
                'Cannot bind %s to a self-closing partial' % bind_to)
          tokens.AdvanceOver(_Token.INLINE_END_SECTION)
          partial_node = _PartialNode.Inline(id_)
          partial_node.SetArguments(partial_args)
          return [partial_node]
      elif tokens.next_token is not _Token.CLOSE_MUSTACHE:
        # Inline syntax for non-partial types. Support select node types:
        # variables, partials, JSON.
        line, column = tokens.next_line, (tokens.next_column + 1)
        name = tokens.AdvanceToNextWhitespace()
        clazz = _UnescapedVariableNode
        if name.startswith('*'):
          clazz = _JsonNode
        elif name.startswith('+'):
          clazz = _PartialNode.Inline
        if clazz is not _UnescapedVariableNode:
          name = name[1:]
          column += 1
        inline_node = clazz(_Identifier(name, line, column))
        if isinstance(inline_node, _PartialNode):
          inline_node.SetArguments(self._ParsePartialNodeArgs(tokens))
          if bind_to is not None:
            inline_node.PassThroughArgument(bind_to)
        tokens.SkipWhitespace()
        tokens.AdvanceOver(_Token.INLINE_END_SECTION)
        return [next_token.clazz(bind_to, id_, inline_node)]
      # Block syntax.
      tokens.AdvanceOver(_Token.CLOSE_MUSTACHE)
      section = self._ParseSection(tokens)
      else_node_class = next_token.ElseNodeClass()  # may not have one
      else_section = None
      if (else_node_class is not None and
          tokens.next_token is _Token.OPEN_ELSE):
        self._OpenElse(tokens, id_)
        else_section = self._ParseSection(tokens)
      self._CloseSection(tokens, id_)
      nodes = []
      if section is not None:
        node = next_token.clazz(bind_to, id_, section)
        if partial_args:
          node.SetArguments(partial_args)
        nodes.append(node)
      if else_section is not None:
        nodes.append(else_node_class(bind_to, id_, else_section))
      return nodes
    elif next_token is _Token.OPEN_COMMENT:
      # Comments.
      start_line = tokens.next_line
      self._AdvanceOverComment(tokens)
      return [_CommentNode(start_line, tokens.next_line)]

  def _AdvanceOverComment(self, tokens):
    tokens.AdvanceOver(_Token.OPEN_COMMENT)
    depth = 1
    while tokens.HasNext() and depth > 0:
      if tokens.next_token is _Token.OPEN_COMMENT:
        depth += 1
      elif tokens.next_token is _Token.CLOSE_COMMENT:
        depth -= 1
      tokens.Advance()

  def _CloseSection(self, tokens, id_):
    tokens.AdvanceOver(_Token.OPEN_END_SECTION,
                       description='to match %s' % id_.GetDescription())
    next_string = tokens.AdvanceOverNextString()
    if next_string != '' and next_string != id_.name:
      raise ParseException(
          'Start section %s doesn\'t match end %s' % (id_, next_string))
    tokens.AdvanceOver(_Token.CLOSE_MUSTACHE)

  def _OpenElse(self, tokens, id_):
    tokens.AdvanceOver(_Token.OPEN_ELSE)
    next_string = tokens.AdvanceOverNextString()
    if next_string != '' and next_string != id_.name:
      raise ParseException(
          'Start section %s doesn\'t match else %s' % (id_, next_string))
    tokens.AdvanceOver(_Token.CLOSE_MUSTACHE)

  def _ParsePartialNodeArgs(self, tokens):
    args = {}
    tokens.SkipWhitespace()
    while (tokens.next_token is _Token.CHARACTER and
           tokens.NextCharacter() != ')'):
      key = tokens.AdvanceOverNextString(excluded=':')
      tokens.AdvanceOverSeparator(':')
      if tokens.NextCharacter() == '(':
        tokens.AdvanceOverSeparator('(')
        inner_id = self._NextIdentifier(tokens)
        inner_args = self._ParsePartialNodeArgs(tokens)
        tokens.AdvanceOverSeparator(')')
        args[key] = {inner_id: inner_args}
      else:
        args[key] = self._NextIdentifier(tokens)
    return args or None

  def _NextIdentifier(self, tokens):
    tokens.SkipWhitespace()
    column_start = tokens.next_column + 1
    id_ = _Identifier(tokens.AdvanceOverNextString(excluded=' \n\r\t:()'),
                      tokens.next_line,
                      column_start)
    tokens.SkipWhitespace()
    return id_

  def Render(self, *user_contexts):
    '''Renders this template given a variable number of contexts to read out
    values from (such as those appearing in {{foo}}).
    '''
    internal_context = _InternalContext()
    contexts = list(user_contexts)
    contexts.append({
      '_': internal_context,
      'false': False,
      'true': True,
    })
    render_state = _RenderState(self._name or '<root>', _Contexts(contexts))
    internal_context.SetRenderState(render_state)
    self._top_node.Render(render_state)
    return render_state.GetResult()

  def render(self, *contexts):
    return self.Render(*contexts)

  def __eq__(self, other):
    return self.source == other.source and self._name == other._name

  def __ne__(self, other):
    return not (self == other)

  def __repr__(self):
    return str('%s(%s)' % (type(self).__name__, self._top_node))

  def __str__(self):
    return repr(self)
