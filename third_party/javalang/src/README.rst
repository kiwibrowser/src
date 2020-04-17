
========
javalang
========

.. image:: https://travis-ci.org/c2nes/javalang.svg?branch=master
  :target: https://travis-ci.org/c2nes/javalang

.. image:: https://badge.fury.io/py/javalang.svg
    :target: https://badge.fury.io/py/javalang

javalang is a pure Python library for working with Java source
code. javalang provides a lexer and parser targeting Java 8. The
implementation is based on the Java language spec available at
http://docs.oracle.com/javase/specs/jls/se8/html/.

The following gives a very brief introduction to using javalang.

---------------
Getting Started
---------------

.. code-block:: python

    >>> import javalang
    >>> tree = javalang.parse.parse("package javalang.brewtab.com; class Test {}")

This will return a ``CompilationUnit`` instance. This object is the root of a
tree which may be traversed to extract different information about the
compilation unit,

.. code-block:: python

    >>> tree.package.name
    u'javalang.brewtab.com'
    >>> tree.types[0]
    ClassDeclaration
    >>> tree.types[0].name
    u'Test'

The string passed to ``javalang.parse.parse()`` must represent a complete unit
which simply means it should represent a complete, valid Java source file. Other
methods in the ``javalang.parse`` module allow for some smaller code snippets to
be parsed without providing an entire compilation unit.

Working with the syntax tree
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``CompilationUnit`` is a subclass of ``javalang.ast.Node``, as are its
descendants in the tree. The ``javalang.tree`` module defines the different
types of ``Node`` subclasses, each of which represent the different syntaxual
elements you will find in Java code. For more detail on what node types are
available, see the ``javalang/tree.py`` source file until the documentation is
complete.

``Node`` instances support iteration,

.. code-block:: python

    >>> for path, node in tree:
    ...     print path, node
    ... 
    () CompilationUnit
    (CompilationUnit,) PackageDeclaration
    (CompilationUnit, [ClassDeclaration]) ClassDeclaration

This iteration can also be filtered by type,

.. code-block:: python

    >>> for path, node in tree.filter(javalang.tree.ClassDeclaration):
    ...     print path, node
    ... 
    (CompilationUnit, [ClassDeclaration]) ClassDeclaration

---------------
Component Usage
---------------

Internally, the ``javalang.parse.parse`` method is a simple method which creates
a token stream for the input, initializes a new ``javalang.parser.Parser``
instance with the given token stream, and then invokes the parser's ``parse()``
method, returning the resulting ``CompilationUnit``. These components may be
also be used individually.

Tokenizer
^^^^^^^^^

The tokenizer/lexer may be invoked directly be calling ``javalang.tokenizer.tokenize``,

.. code-block:: python

    >>> javalang.tokenizer.tokenize('System.out.println("Hello " + "world");')
    <generator object tokenize at 0x1ce5190>

This returns a generator which provides a stream of ``JavaToken`` objects. Each
token carries position (line, column) and value information,

.. code-block:: python

    >>> tokens = list(javalang.tokenizer.tokenize('System.out.println("Hello " + "world");'))
    >>> tokens[6].value
    u'"Hello "'
    >>> tokens[6].position
    (1, 19)

The tokens are not directly instances of ``JavaToken``, but are instead
instances of subclasses which identify their general type,

.. code-block:: python

    >>> type(tokens[6])
    <class 'javalang.tokenizer.String'>
    >>> type(tokens[7])
    <class 'javalang.tokenizer.Operator'>


**NOTE:** The shift operators ``>>`` and ``>>>`` are represented by multiple
``>`` tokens. This is because multiple ``>`` may appear in a row when closing
nested generic parameter/arguments lists. This abiguity is instead resolved by
the parser.

Parser
^^^^^^

To parse snippets of code, a parser may be used directly,

.. code-block:: python

    >>> tokens = javalang.tokenizer.tokenize('System.out.println("Hello " + "world");')
    >>> parser = javalang.parser.Parser(tokens)
    >>> parser.parse_expression()
    MethodInvocation

The parse methods are designed for incremental parsing so they will not restart
at the beginning of the token stream. Attempting to call a parse method more
than once will result in a ``JavaSyntaxError`` exception.

Invoking the incorrect parse method will also result in a ``JavaSyntaxError``
exception,

.. code-block:: python

    >>> tokens = javalang.tokenizer.tokenize('System.out.println("Hello " + "world");')
    >>> parser = javalang.parser.Parser(tokens)
    >>> parser.parse_type_declaration()
    Traceback (most recent call last):
      File "<stdin>", line 1, in <module>
      File "javalang/parser.py", line 336, in parse_type_declaration
        return self.parse_class_or_interface_declaration()
      File "javalang/parser.py", line 353, in parse_class_or_interface_declaration
        self.illegal("Expected type declaration")
      File "javalang/parser.py", line 122, in illegal
        raise JavaSyntaxError(description, at)
    javalang.parser.JavaSyntaxError

The ``javalang.parse`` module also provides convenience methods for parsing more
common types of code snippets.
