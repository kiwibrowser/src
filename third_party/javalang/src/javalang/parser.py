import six

from . import util
from . import tree
from .tokenizer import (
    EndOfInput, Keyword, Modifier, BasicType, Identifier,
    Annotation, Literal, Operator, JavaToken,
    )

ENABLE_DEBUG_SUPPORT = False

def parse_debug(method):
    global ENABLE_DEBUG_SUPPORT

    if ENABLE_DEBUG_SUPPORT:
        def _method(self):
            if not hasattr(self, 'recursion_depth'):
                self.recursion_depth = 0

            if self.debug:
                depth = "%02d" % (self.recursion_depth,)
                token = six.text_type(self.tokens.look())
                start_value = self.tokens.look().value
                name = method.__name__
                sep = ("-" * self.recursion_depth)
                e_message = ""

                print("%s %s> %s(%s)" % (depth, sep, name, token))

                self.recursion_depth += 1

                try:
                    r = method(self)

                except JavaSyntaxError as e:
                    e_message = e.description
                    raise

                except Exception as e:
                    e_message = six.text_type(e)
                    raise

                finally:
                    token = six.text_type(self.tokens.last())
                    print("%s <%s %s(%s, %s) %s" %
                        (depth, sep, name, start_value, token, e_message))
                    self.recursion_depth -= 1
            else:
                self.recursion_depth += 1
                try:
                    r = method(self)
                finally:
                    self.recursion_depth -= 1

            return r

        return _method

    else:
        return method

# ------------------------------------------------------------------------------
# ---- Parsing exception ----

class JavaParserBaseException(Exception):
    def __init__(self, message=''):
        super(JavaParserBaseException, self).__init__(message)

class JavaSyntaxError(JavaParserBaseException):
    def __init__(self, description, at=None):
        super(JavaSyntaxError, self).__init__()

        self.description = description
        self.at = at

class JavaParserError(JavaParserBaseException):
    pass

# ------------------------------------------------------------------------------
# ---- Parser class ----

class Parser(object):
    operator_precedence = [ set(('||',)),
                            set(('&&',)),
                            set(('|',)),
                            set(('^',)),
                            set(('&',)),
                            set(('==', '!=')),
                            set(('<', '>', '>=', '<=', 'instanceof')),
                            set(('<<', '>>', '>>>')),
                            set(('+', '-')),
                            set(('*', '/', '%')) ]

    def __init__(self, tokens):
        self.tokens = util.LookAheadListIterator(tokens)
        self.tokens.set_default(EndOfInput(None))

        self.debug = False

# ------------------------------------------------------------------------------
# ---- Debug control ----

    def set_debug(self, debug=True):
        self.debug = debug

# ------------------------------------------------------------------------------
# ---- Parsing entry point ----

    def parse(self):
        return self.parse_compilation_unit()

# ------------------------------------------------------------------------------
# ---- Helper methods ----

    def illegal(self, description, at=None):
        if not at:
            at = self.tokens.look()

        raise JavaSyntaxError(description, at)

    def accept(self, *accepts):
        last = None

        if len(accepts) == 0:
            raise JavaParserError("Missing acceptable values")

        for accept in accepts:
            token = next(self.tokens)
            if isinstance(accept, six.string_types) and (
                    not token.value == accept):
                self.illegal("Expected '%s'" % (accept,))
            elif isinstance(accept, type) and not isinstance(token, accept):
                self.illegal("Expected %s" % (accept.__name__,))

            last = token

        return last.value

    def would_accept(self, *accepts):
        if len(accepts) == 0:
            raise JavaParserError("Missing acceptable values")

        for i, accept in enumerate(accepts):
            token = self.tokens.look(i)

            if isinstance(accept, six.string_types) and (
                    not token.value == accept):
                return False
            elif isinstance(accept, type) and not isinstance(token, accept):
                return False

        return True

    def try_accept(self, *accepts):
        if len(accepts) == 0:
            raise JavaParserError("Missing acceptable values")

        for i, accept in enumerate(accepts):
            token = self.tokens.look(i)

            if isinstance(accept, six.string_types) and (
                    not token.value == accept):
                return False
            elif isinstance(accept, type) and not isinstance(token, accept):
                return False

        for i in range(0, len(accepts)):
            next(self.tokens)

        return True

    def build_binary_operation(self, parts, start_level=0):
        if len(parts) == 1:
            return parts[0]

        operands = list()
        operators = list()

        i = 0

        for level in range(start_level, len(self.operator_precedence)):
            for j in range(1, len(parts) - 1, 2):
                if parts[j] in self.operator_precedence[level]:
                    operand = self.build_binary_operation(parts[i:j], level + 1)
                    operator = parts[j]
                    i = j + 1

                    operands.append(operand)
                    operators.append(operator)

            if operands:
                break

        operand = self.build_binary_operation(parts[i:], level + 1)
        operands.append(operand)

        operation = operands[0]

        for operator, operandr in zip(operators, operands[1:]):
            operation = tree.BinaryOperation(operandl=operation)
            operation.operator = operator
            operation.operandr = operandr

        return operation

    def is_annotation(self, i=0):
        """ Returns true if the position is the start of an annotation application
        (as opposed to an annotation declaration)

        """

        return (isinstance(self.tokens.look(i), Annotation)
                and not self.tokens.look(i + 1).value == 'interface')

    def is_annotation_declaration(self, i=0):
        """ Returns true if the position is the start of an annotation application
        (as opposed to an annotation declaration)

        """

        return (isinstance(self.tokens.look(i), Annotation)
                and self.tokens.look(i + 1).value == 'interface')

# ------------------------------------------------------------------------------
# ---- Parsing methods ----

# ------------------------------------------------------------------------------
# -- Identifiers --

    @parse_debug
    def parse_identifier(self):
        return self.accept(Identifier)

    @parse_debug
    def parse_qualified_identifier(self):
        qualified_identifier = list()

        while True:
            identifier = self.parse_identifier()
            qualified_identifier.append(identifier)

            if not self.try_accept('.'):
                break

        return '.'.join(qualified_identifier)

    @parse_debug
    def parse_qualified_identifier_list(self):
        qualified_identifiers = list()

        while True:
            qualified_identifier = self.parse_qualified_identifier()
            qualified_identifiers.append(qualified_identifier)

            if not self.try_accept(','):
                break

        return qualified_identifiers

# ------------------------------------------------------------------------------
# -- Top level units --

    @parse_debug
    def parse_compilation_unit(self):
        package = None
        package_annotations = None
        javadoc = None
        import_declarations = list()
        type_declarations = list()

        self.tokens.push_marker()
        next_token = self.tokens.look()
        if next_token:
            javadoc = next_token.javadoc

        if self.is_annotation():
            package_annotations = self.parse_annotations()

        if self.try_accept('package'):
            self.tokens.pop_marker(False)
            
            token = self.tokens.look()
            package_name = self.parse_qualified_identifier()
            package = tree.PackageDeclaration(annotations=package_annotations,
                                              name=package_name,
                                              documentation=javadoc)
            package._position = token.position
            
            self.accept(';')
        else:
            self.tokens.pop_marker(True)
            package_annotations = None

        while self.would_accept('import'):
            token = self.tokens.look()
            import_declaration = self.parse_import_declaration()
            import_declaration._position = token.position
            import_declarations.append(import_declaration)

        while not isinstance(self.tokens.look(), EndOfInput):
            try:
                type_declaration = self.parse_type_declaration()
            except StopIteration:
                self.illegal("Unexpected end of input")

            if type_declaration:
                type_declarations.append(type_declaration)

        return tree.CompilationUnit(package=package,
                                    imports=import_declarations,
                                    types=type_declarations)

    @parse_debug
    def parse_import_declaration(self):
        qualified_identifier = list()
        static = False
        import_all = False

        self.accept('import')

        if self.try_accept('static'):
            static = True

        while True:
            identifier = self.parse_identifier()
            qualified_identifier.append(identifier)

            if self.try_accept('.'):
                if self.try_accept('*'):
                    self.accept(';')
                    import_all = True
                    break

            else:
                self.accept(';')
                break

        return tree.Import(path='.'.join(qualified_identifier),
                           static=static,
                           wildcard=import_all)

    @parse_debug
    def parse_type_declaration(self):
        if self.try_accept(';'):
            return None
        else:
            return self.parse_class_or_interface_declaration()

    @parse_debug
    def parse_class_or_interface_declaration(self):
        modifiers, annotations, javadoc = self.parse_modifiers()
        type_declaration = None

        token = self.tokens.look()
        if token.value == 'class':
            type_declaration = self.parse_normal_class_declaration()
        elif token.value == 'enum':
            type_declaration = self.parse_enum_declaration()
        elif token.value == 'interface':
            type_declaration = self.parse_normal_interface_declaration()
        elif self.is_annotation_declaration():
            type_declaration = self.parse_annotation_type_declaration()
        else:
            self.illegal("Expected type declaration")

        type_declaration._position = token.position
        type_declaration.modifiers = modifiers
        type_declaration.annotations = annotations
        type_declaration.documentation = javadoc

        return type_declaration

    @parse_debug
    def parse_normal_class_declaration(self):
        name = None
        type_params = None
        extends = None
        implements = None
        body = None

        self.accept('class')

        name = self.parse_identifier()

        if self.would_accept('<'):
            type_params = self.parse_type_parameters()

        if self.try_accept('extends'):
            extends = self.parse_type()

        if self.try_accept('implements'):
            implements = self.parse_type_list()

        body = self.parse_class_body()

        return tree.ClassDeclaration(name=name,
                                     type_parameters=type_params,
                                     extends=extends,
                                     implements=implements,
                                     body=body)

    @parse_debug
    def parse_enum_declaration(self):
        name = None
        implements = None
        body = None

        self.accept('enum')
        name = self.parse_identifier()

        if self.try_accept('implements'):
            implements = self.parse_type_list()

        body = self.parse_enum_body()

        return tree.EnumDeclaration(name=name,
                                    implements=implements,
                                    body=body)

    @parse_debug
    def parse_normal_interface_declaration(self):
        name = None
        type_parameters = None
        extends = None
        body = None

        self.accept('interface')
        name = self.parse_identifier()

        if self.would_accept('<'):
            type_parameters = self.parse_type_parameters()

        if self.try_accept('extends'):
            extends = self.parse_type_list()

        body = self.parse_interface_body()

        return tree.InterfaceDeclaration(name=name,
                                         type_parameters=type_parameters,
                                         extends=extends,
                                         body=body)

    @parse_debug
    def parse_annotation_type_declaration(self):
        name = None
        body = None

        self.accept('@', 'interface')

        name = self.parse_identifier()
        body = self.parse_annotation_type_body()

        return tree.AnnotationDeclaration(name=name,
                                          body=body)

# ------------------------------------------------------------------------------
# -- Types --

    @parse_debug
    def parse_type(self):
        java_type = None

        if isinstance(self.tokens.look(), BasicType):
            java_type = self.parse_basic_type()
        elif isinstance(self.tokens.look(), Identifier):
            java_type = self.parse_reference_type()
        else:
            self.illegal("Expected type")

        java_type.dimensions = self.parse_array_dimension()

        return java_type

    @parse_debug
    def parse_basic_type(self):
        return tree.BasicType(name=self.accept(BasicType))

    @parse_debug
    def parse_reference_type(self):
        reference_type = tree.ReferenceType()
        tail = reference_type

        while True:
            tail.name = self.parse_identifier()

            if self.would_accept('<'):
                tail.arguments = self.parse_type_arguments()

            if self.try_accept('.'):
                tail.sub_type = tree.ReferenceType()
                tail = tail.sub_type
            else:
                break

        return reference_type

    @parse_debug
    def parse_type_arguments(self):
        type_arguments = list()

        self.accept('<')

        while True:
            type_argument = self.parse_type_argument()
            type_arguments.append(type_argument)

            if self.try_accept('>'):
                break

            self.accept(',')

        return type_arguments

    @parse_debug
    def parse_type_argument(self):
        pattern_type = None
        base_type = None

        if self.try_accept('?'):
            if self.tokens.look().value in ('extends', 'super'):
                pattern_type = self.tokens.next().value
            else:
                return tree.TypeArgument(pattern_type='?')

        if self.would_accept(BasicType):
            base_type = self.parse_basic_type()
            self.accept('[', ']')
            base_type.dimensions = [None]
        else:
            base_type = self.parse_reference_type()
            base_type.dimensions = []

        base_type.dimensions += self.parse_array_dimension()

        return tree.TypeArgument(type=base_type,
                                 pattern_type=pattern_type)

    @parse_debug
    def parse_nonwildcard_type_arguments(self):
        self.accept('<')
        type_arguments = self.parse_type_list()
        self.accept('>')

        return [tree.TypeArgument(type=t) for t in type_arguments]

    @parse_debug
    def parse_type_list(self):
        types = list()

        while True:
            if self.would_accept(BasicType):
                base_type = self.parse_basic_type()
                self.accept('[', ']')
                base_type.dimensions = [None]
            else:
                base_type = self.parse_reference_type()
                base_type.dimensions = []

            base_type.dimensions += self.parse_array_dimension()
            types.append(base_type)

            if not self.try_accept(','):
                break

        return types

    @parse_debug
    def parse_type_arguments_or_diamond(self):
        if self.try_accept('<', '>'):
            return list()
        else:
            return self.parse_type_arguments()

    @parse_debug
    def parse_nonwildcard_type_arguments_or_diamond(self):
        if self.try_accept('<', '>'):
            return list()
        else:
            return self.parse_nonwildcard_type_arguments()

    @parse_debug
    def parse_type_parameters(self):
        type_parameters = list()

        self.accept('<')

        while True:
            type_parameter = self.parse_type_parameter()
            type_parameters.append(type_parameter)

            if self.try_accept('>'):
                break
            else:
                self.accept(',')

        return type_parameters

    @parse_debug
    def parse_type_parameter(self):
        identifier = self.parse_identifier()
        extends = None

        if self.try_accept('extends'):
            extends = list()

            while True:
                reference_type = self.parse_reference_type()
                extends.append(reference_type)

                if not self.try_accept('&'):
                    break

        return tree.TypeParameter(name=identifier,
                                  extends=extends)

    @parse_debug
    def parse_array_dimension(self):
        array_dimension = 0

        while self.try_accept('[', ']'):
            array_dimension += 1

        return [None] * array_dimension

# ------------------------------------------------------------------------------
# -- Annotations and modifiers --

    @parse_debug
    def parse_modifiers(self):
        annotations = list()
        modifiers = set()
        javadoc = None

        next_token = self.tokens.look()
        if next_token:
            javadoc = next_token.javadoc

        while True:
            token = self.tokens.look()
            if self.would_accept(Modifier):
                modifiers.add(self.accept(Modifier))

            elif self.is_annotation():
                annotation = self.parse_annotation()
                annotation._position = token.position
                annotations.append(annotation)

            else:
                break

        return (modifiers, annotations, javadoc)

    @parse_debug
    def parse_annotations(self):
        annotations = list()

        while True:
            token = self.tokens.look()
            
            annotation = self.parse_annotation()
            annotation._position = token.position
            annotations.append(annotation)

            if not self.is_annotation():
                break

        return annotations

    @parse_debug
    def parse_annotation(self):
        qualified_identifier = None
        annotation_element = None

        self.accept('@')
        qualified_identifier = self.parse_qualified_identifier()

        if self.try_accept('('):
            if not self.would_accept(')'):
                annotation_element = self.parse_annotation_element()
            self.accept(')')

        return tree.Annotation(name=qualified_identifier,
                               element=annotation_element)

    @parse_debug
    def parse_annotation_element(self):
        if self.would_accept(Identifier, '='):
            return self.parse_element_value_pairs()
        else:
            return self.parse_element_value()

    @parse_debug
    def parse_element_value_pairs(self):
        pairs = list()

        while True:
            token = self.tokens.look()
            pair = self.parse_element_value_pair()
            pair._position = token.position
            pairs.append(pair)

            if not self.try_accept(','):
                break

        return pairs

    @parse_debug
    def parse_element_value_pair(self):
        identifier = self.parse_identifier()
        self.accept('=')
        value = self.parse_element_value()

        return tree.ElementValuePair(name=identifier,
                                     value=value)

    @parse_debug
    def parse_element_value(self):
        token = self.tokens.look()
        if self.is_annotation():
            annotation = self.parse_annotation()
            annotation._position = token.position
            return annotation

        elif self.would_accept('{'):
            return self.parse_element_value_array_initializer()

        else:
            return self.parse_expressionl()

    @parse_debug
    def parse_element_value_array_initializer(self):
        self.accept('{')

        if self.try_accept('}'):
            return list()

        element_values = self.parse_element_values()
        self.try_accept(',')
        self.accept('}')

        return tree.ElementArrayValue(values=element_values)

    @parse_debug
    def parse_element_values(self):
        element_values = list()

        while True:
            element_value = self.parse_element_value()
            element_values.append(element_value)

            if self.would_accept('}') or self.would_accept(',', '}'):
                break

            self.accept(',')

        return element_values

# ------------------------------------------------------------------------------
# -- Class body --

    @parse_debug
    def parse_class_body(self):
        declarations = list()

        self.accept('{')

        while not self.would_accept('}'):
            declaration = self.parse_class_body_declaration()
            if declaration:
                declarations.append(declaration)

        self.accept('}')

        return declarations

    @parse_debug
    def parse_class_body_declaration(self):
        token = self.tokens.look()

        if self.try_accept(';'):
            return None

        elif self.would_accept('static', '{'):
            self.accept('static')
            return self.parse_block()

        elif self.would_accept('{'):
            return self.parse_block()

        else:
            return self.parse_member_declaration()

    @parse_debug
    def parse_member_declaration(self):
        modifiers, annotations, javadoc = self.parse_modifiers()
        member = None

        token = self.tokens.look()
        if self.try_accept('void'):
            method_name = self.parse_identifier()
            member = self.parse_void_method_declarator_rest()
            member.name = method_name

        elif token.value == '<':
            member = self.parse_generic_method_or_constructor_declaration()

        elif token.value == 'class':
            member = self.parse_normal_class_declaration()

        elif token.value == 'enum':
            member = self.parse_enum_declaration()

        elif token.value == 'interface':
            member = self.parse_normal_interface_declaration()

        elif self.is_annotation_declaration():
            member = self.parse_annotation_type_declaration()

        elif self.would_accept(Identifier, '('):
            constructor_name = self.parse_identifier()
            member = self.parse_constructor_declarator_rest()
            member.name = constructor_name

        else:
            member = self.parse_method_or_field_declaraction()

        member._position = token.position
        member.modifiers = modifiers
        member.annotations = annotations
        member.documentation = javadoc

        return member

    @parse_debug
    def parse_method_or_field_declaraction(self):
        member_type = self.parse_type()
        member_name = self.parse_identifier()

        member = self.parse_method_or_field_rest()

        if isinstance(member, tree.MethodDeclaration):
            member_type.dimensions += member.return_type.dimensions

            member.name = member_name
            member.return_type = member_type
        else:
            member.type = member_type
            member.declarators[0].name = member_name

        return member

    @parse_debug
    def parse_method_or_field_rest(self):
        token = self.tokens.look()
        
        if self.would_accept('('):
            return self.parse_method_declarator_rest()
        else:
            rest = self.parse_field_declarators_rest()
            self.accept(';')
            return rest

    @parse_debug
    def parse_field_declarators_rest(self):
        array_dimension, initializer = self.parse_variable_declarator_rest()
        declarators = [tree.VariableDeclarator(dimensions=array_dimension,
                                               initializer=initializer)]

        while self.try_accept(','):
            declarator = self.parse_variable_declarator()
            declarators.append(declarator)

        return tree.FieldDeclaration(declarators=declarators)

    @parse_debug
    def parse_method_declarator_rest(self):
        formal_parameters = self.parse_formal_parameters()
        additional_dimensions = self.parse_array_dimension()
        throws = None
        body = None

        if self.try_accept('throws'):
            throws = self.parse_qualified_identifier_list()

        if self.would_accept('{'):
            body = self.parse_block()
        else:
            self.accept(';')

        return tree.MethodDeclaration(parameters=formal_parameters,
                                     throws=throws,
                                     body=body,
                                     return_type=tree.Type(dimensions=additional_dimensions))

    @parse_debug
    def parse_void_method_declarator_rest(self):
        formal_parameters = self.parse_formal_parameters()
        throws = None
        body = None

        if self.try_accept('throws'):
            throws = self.parse_qualified_identifier_list()

        if self.would_accept('{'):
            body = self.parse_block()
        else:
            self.accept(';')

        return tree.MethodDeclaration(parameters=formal_parameters,
                                      throws=throws,
                                      body=body)

    @parse_debug
    def parse_constructor_declarator_rest(self):
        formal_parameters = self.parse_formal_parameters()
        throws = None
        body = None

        if self.try_accept('throws'):
            throws = self.parse_qualified_identifier_list()

        body = self.parse_block()

        return tree.ConstructorDeclaration(parameters=formal_parameters,
                                           throws=throws,
                                           body=body)

    @parse_debug
    def parse_generic_method_or_constructor_declaration(self):
        type_parameters = self.parse_type_parameters()
        method = None

        token = self.tokens.look()
        if self.would_accept(Identifier, '('):
            constructor_name = self.parse_identifier()
            method = self.parse_constructor_declarator_rest()
            method.name = constructor_name
        elif self.try_accept('void'):
            method_name = self.parse_identifier()
            method = self.parse_void_method_declarator_rest()
            method.name = method_name

        else:
            method_return_type = self.parse_type()
            method_name = self.parse_identifier()

            method = self.parse_method_declarator_rest()

            method_return_type.dimensions += method.return_type.dimensions
            method.return_type = method_return_type
            method.name = method_name

        method._position = token.position
        method.type_parameters = type_parameters
        return method

# ------------------------------------------------------------------------------
# -- Interface body --

    @parse_debug
    def parse_interface_body(self):
        declarations = list()

        self.accept('{')
        while not self.would_accept('}'):
            declaration = self.parse_interface_body_declaration()

            if declaration:
                declarations.append(declaration)
        self.accept('}')

        return declarations

    @parse_debug
    def parse_interface_body_declaration(self):
        if self.try_accept(';'):
            return None

        modifiers, annotations, javadoc = self.parse_modifiers()

        declaration = self.parse_interface_member_declaration()
        declaration.modifiers = modifiers
        declaration.annotations = annotations
        declaration.documentation = javadoc

        return declaration

    @parse_debug
    def parse_interface_member_declaration(self):
        declaration = None

        token = self.tokens.look()
        if self.would_accept('class'):
            declaration = self.parse_normal_class_declaration()
        elif self.would_accept('interface'):
            declaration = self.parse_normal_interface_declaration()
        elif self.would_accept('enum'):
            declaration = self.parse_enum_declaration()
        elif self.is_annotation_declaration():
            declaration = self.parse_annotation_type_declaration()
        elif self.would_accept('<'):
            declaration = self.parse_interface_generic_method_declarator()
        elif self.try_accept('void'):
            method_name = self.parse_identifier()
            declaration = self.parse_void_interface_method_declarator_rest()
            declaration.name = method_name
        else:
            declaration = self.parse_interface_method_or_field_declaration()

        declaration._position = token.position
        
        return declaration

    @parse_debug
    def parse_interface_method_or_field_declaration(self):
        java_type = self.parse_type()
        name = self.parse_identifier()
        member = self.parse_interface_method_or_field_rest()

        if isinstance(member, tree.MethodDeclaration):
            java_type.dimensions += member.return_type.dimensions
            member.name = name
            member.return_type = java_type
        else:
            member.declarators[0].name = name
            member.type = java_type

        return member

    @parse_debug
    def parse_interface_method_or_field_rest(self):
        rest = None

        if self.would_accept('('):
            rest = self.parse_interface_method_declarator_rest()
        else:
            rest = self.parse_constant_declarators_rest()
            self.accept(';')

        return rest

    @parse_debug
    def parse_constant_declarators_rest(self):
        array_dimension, initializer = self.parse_constant_declarator_rest()
        declarators = [tree.VariableDeclarator(dimensions=array_dimension,
                                               initializer=initializer)]

        while self.try_accept(','):
            declarator = self.parse_constant_declarator()
            declarators.append(declarator)

        return tree.ConstantDeclaration(declarators=declarators)

    @parse_debug
    def parse_constant_declarator_rest(self):
        array_dimension = self.parse_array_dimension()
        self.accept('=')
        initializer = self.parse_variable_initializer()

        return (array_dimension, initializer)

    @parse_debug
    def parse_constant_declarator(self):
        name = self.parse_identifier()
        additional_dimension, initializer = self.parse_constant_declarator_rest()

        return tree.VariableDeclarator(name=name,
                                       dimensions=additional_dimension,
                                       initializer=initializer)

    @parse_debug
    def parse_interface_method_declarator_rest(self):
        parameters = self.parse_formal_parameters()
        array_dimension = self.parse_array_dimension()
        throws = None
        body = None

        if self.try_accept('throws'):
            throws = self.parse_qualified_identifier_list()

        if self.would_accept('{'):
            body = self.parse_block()
        else:
            self.accept(';')

        return tree.MethodDeclaration(parameters=parameters,
                                      throws=throws,
                                      body=body,
                                      return_type=tree.Type(dimensions=array_dimension))

    @parse_debug
    def parse_void_interface_method_declarator_rest(self):
        parameters = self.parse_formal_parameters()
        throws = None
        body = None

        if self.try_accept('throws'):
            throws = self.parse_qualified_identifier_list()

        if self.would_accept('{'):
            body = self.parse_block()
        else:
            self.accept(';')

        return tree.MethodDeclaration(parameters=parameters,
                                      throws=throws,
                                      body=body)

    @parse_debug
    def parse_interface_generic_method_declarator(self):
        type_parameters = self.parse_type_parameters()
        return_type = None
        method_name = None

        if not self.try_accept('void'):
            return_type = self.parse_type()

        method_name = self.parse_identifier()
        method = self.parse_interface_method_declarator_rest()
        method.name = method_name
        method.return_type = return_type
        method.type_parameters = type_parameters

        return method

# ------------------------------------------------------------------------------
# -- Parameters and variables --

    @parse_debug
    def parse_formal_parameters(self):
        formal_parameters = list()

        self.accept('(')

        if self.try_accept(')'):
            return formal_parameters

        while True:
            modifiers, annotations = self.parse_variable_modifiers()
            
            token = self.tokens.look()
            parameter_type = self.parse_type()
            varargs = False

            if self.try_accept('...'):
                varargs = True

            parameter_name = self.parse_identifier()
            parameter_type.dimensions += self.parse_array_dimension()

            parameter = tree.FormalParameter(modifiers=modifiers,
                                             annotations=annotations,
                                             type=parameter_type,
                                             name=parameter_name,
                                             varargs=varargs)

            parameter._position = token.position
            formal_parameters.append(parameter)

            if varargs:
                # varargs parameter must be the last
                break

            if not self.try_accept(','):
                break

        self.accept(')')

        return formal_parameters

    @parse_debug
    def parse_variable_modifiers(self):
        modifiers = set()
        annotations = list()

        while True:
            token = self.tokens.look()
            if self.try_accept('final'):
                modifiers.add('final')
            elif self.is_annotation():
                annotation = self.parse_annotation()
                annotation._position = token.position
                annotations.append(annotation)
            else:
                break

        return modifiers, annotations

    @parse_debug
    def parse_variable_declators(self):
        declarators = list()

        while True:
            declarator = self.parse_variable_declator()
            declarators.append(declarator)

            if not self.try_accept(','):
                break

        return declarators

    @parse_debug
    def parse_variable_declarators(self):
        declarators = list()

        while True:
            declarator = self.parse_variable_declarator()
            declarators.append(declarator)

            if not self.try_accept(','):
                break

        return declarators

    @parse_debug
    def parse_variable_declarator(self):
        identifier = self.parse_identifier()
        array_dimension, initializer = self.parse_variable_declarator_rest()

        return tree.VariableDeclarator(name=identifier,
                                       dimensions=array_dimension,
                                       initializer=initializer)

    @parse_debug
    def parse_variable_declarator_rest(self):
        array_dimension = self.parse_array_dimension()
        initializer = None

        if self.try_accept('='):
            initializer = self.parse_variable_initializer()

        return (array_dimension, initializer)

    @parse_debug
    def parse_variable_initializer(self):
        if self.would_accept('{'):
            return self.parse_array_initializer()
        else:
            return self.parse_expression()

    @parse_debug
    def parse_array_initializer(self):
        array_initializer = tree.ArrayInitializer(initializers=list())

        self.accept('{')

        if self.try_accept(','):
            self.accept('}')
            return array_initializer

        if self.try_accept('}'):
            return array_initializer

        while True:
            initializer = self.parse_variable_initializer()
            array_initializer.initializers.append(initializer)

            if not self.would_accept('}'):
                self.accept(',')

            if self.try_accept('}'):
                return array_initializer

# ------------------------------------------------------------------------------
# -- Blocks and statements --

    @parse_debug
    def parse_block(self):
        statements = list()

        self.accept('{')

        while not self.would_accept('}'):
            statement = self.parse_block_statement()
            statements.append(statement)
        self.accept('}')

        return statements

    @parse_debug
    def parse_block_statement(self):
        if self.would_accept(Identifier, ':'):
            # Labeled statement
            return self.parse_statement()

        if self.would_accept('synchronized'):
            return self.parse_statement()

        token = None
        found_annotations = False
        i = 0

        # Look past annoatations and modifiers. If we find a modifier that is not
        # 'final' then the statement must be a class or interface declaration
        while True:
            token = self.tokens.look(i)

            if isinstance(token, Modifier):
                if not token.value == 'final':
                    return self.parse_class_or_interface_declaration()

            elif self.is_annotation(i):
                found_annotations = True

                i += 2
                while self.tokens.look(i).value == '.':
                    i += 2

                if self.tokens.look(i).value == '(':
                    parens = 1
                    i += 1

                    while parens > 0:
                        token = self.tokens.look(i)
                        if token.value == '(':
                            parens += 1
                        elif token.value == ')':
                            parens -= 1
                        i += 1
                    continue

            else:
                break

            i += 1

        if token.value in ('class', 'enum', 'interface', '@'):
            return self.parse_class_or_interface_declaration()

        if found_annotations or isinstance(token, BasicType):
            statement = self.parse_local_variable_declaration_statement()
            statement._position = token.position
            return statement

        # At this point, if the block statement is a variable definition the next
        # token MUST be an identifier, so if it isn't we can conclude the block
        # statement is a normal statement
        if not isinstance(token, Identifier):
            return self.parse_statement()

        # We can't easily determine the statement type. Try parsing as a variable
        # declaration first and fall back to a statement
        try:
            with self.tokens:
                statement = self.parse_local_variable_declaration_statement()
                statement._position = token.position
                return statement
        except JavaSyntaxError:
            return self.parse_statement()

    @parse_debug
    def parse_local_variable_declaration_statement(self):
        modifiers, annotations = self.parse_variable_modifiers()
        java_type = self.parse_type()
        declarators = self.parse_variable_declarators()
        self.accept(';')

        var = tree.LocalVariableDeclaration(modifiers=modifiers,
                                            annotations=annotations,
                                            type=java_type,
                                            declarators=declarators)
        return var

    @parse_debug
    def parse_statement(self):
        token = self.tokens.look()
        if self.would_accept('{'):
            block = self.parse_block()
            return tree.BlockStatement(statements=block)

        elif self.try_accept(';'):
            return tree.Statement()

        elif self.would_accept(Identifier, ':'):
            identifer = self.parse_identifier()
            self.accept(':')

            statement = self.parse_statement()
            statement.label = identifer

            return statement

        elif self.try_accept('if'):
            condition = self.parse_par_expression()
            then = self.parse_statement()
            else_statement = None

            if self.try_accept('else'):
                else_statement = self.parse_statement()

            return tree.IfStatement(condition=condition,
                                    then_statement=then,
                                    else_statement=else_statement)

        elif self.try_accept('assert'):
            condition = self.parse_expression()
            value = None

            if self.try_accept(':'):
                value = self.parse_expression()

            self.accept(';')

            return tree.AssertStatement(condition=condition,
                                        value=value)

        elif self.try_accept('switch'):
            switch_expression = self.parse_par_expression()
            self.accept('{')
            switch_block = self.parse_switch_block_statement_groups()
            self.accept('}')

            return tree.SwitchStatement(expression=switch_expression,
                                        cases=switch_block)

        elif self.try_accept('while'):
            condition = self.parse_par_expression()
            action = self.parse_statement()

            return tree.WhileStatement(condition=condition,
                                       body=action)

        elif self.try_accept('do'):
            action = self.parse_statement()
            self.accept('while')
            condition = self.parse_par_expression()
            self.accept(';')

            return tree.DoStatement(condition=condition,
                                    body=action)

        elif self.try_accept('for'):
            self.accept('(')
            for_control = self.parse_for_control()
            self.accept(')')
            for_statement = self.parse_statement()

            return tree.ForStatement(control=for_control,
                                     body=for_statement)

        elif self.try_accept('break'):
            label = None

            if self.would_accept(Identifier):
                label = self.parse_identifier()

            self.accept(';')

            return tree.BreakStatement(goto=label)

        elif self.try_accept('continue'):
            label = None

            if self.would_accept(Identifier):
                label = self.parse_identifier()

            self.accept(';')

            return tree.ContinueStatement(goto=label)

        elif self.try_accept('return'):
            value = None

            if not self.would_accept(';'):
                value = self.parse_expression()

            self.accept(';')

            statement = tree.ReturnStatement(expression=value)
            statement._position = token.position
            return statement

        elif self.try_accept('throw'):
            value = self.parse_expression()
            self.accept(';')

            return tree.ThrowStatement(expression=value)

        elif self.try_accept('synchronized'):
            lock = self.parse_par_expression()
            block = self.parse_block()

            return tree.SynchronizedStatement(lock=lock,
                                              block=block)

        elif self.try_accept('try'):
            resource_specification = None
            block = None
            catches = None
            finally_block = None

            if self.would_accept('{'):
                block = self.parse_block()

                if self.would_accept('catch'):
                    catches = self.parse_catches()

                if self.try_accept('finally'):
                    finally_block = self.parse_block()

                if catches == None and finally_block == None:
                    self.illegal("Expected catch/finally block")

            else:
                resource_specification = self.parse_resource_specification()
                block = self.parse_block()

                if self.would_accept('catch'):
                    catches = self.parse_catches()

                if self.try_accept('finally'):
                    finally_block = self.parse_block()

            return tree.TryStatement(resources=resource_specification,
                                     block=block,
                                     catches=catches,
                                     finally_block=finally_block)

        else:
            expression = self.parse_expression()
            self.accept(';')

            return tree.StatementExpression(expression=expression)

# ------------------------------------------------------------------------------
# -- Try / catch --

    @parse_debug
    def parse_catches(self):
        catches = list()

        while True:
            catch = self.parse_catch_clause()
            catches.append(catch)

            if not self.would_accept('catch'):
                break

        return catches

    @parse_debug
    def parse_catch_clause(self):
        self.accept('catch', '(')

        modifiers, annotations = self.parse_variable_modifiers()
        catch_parameter = tree.CatchClauseParameter(types=list())

        while True:
            catch_type = self.parse_qualified_identifier()
            catch_parameter.types.append(catch_type)

            if not self.try_accept('|'):
                break
        catch_parameter.name = self.parse_identifier()

        self.accept(')')
        block = self.parse_block()

        return tree.CatchClause(parameter=catch_parameter,
                                block=block)

    @parse_debug
    def parse_resource_specification(self):
        resources = list()

        self.accept('(')

        while True:
            resource = self.parse_resource()
            resources.append(resource)

            if not self.would_accept(')'):
                self.accept(';')

            if self.try_accept(')'):
                break

        return resources

    @parse_debug
    def parse_resource(self):
        modifiers, annotations = self.parse_variable_modifiers()
        reference_type = self.parse_reference_type()
        reference_type.dimensions = self.parse_array_dimension()
        name = self.parse_identifier()
        reference_type.dimensions += self.parse_array_dimension()
        self.accept('=')
        value = self.parse_expression()

        return tree.TryResource(modifiers=modifiers,
                                annotations=annotations,
                                type=reference_type,
                                name=name,
                                value=value)

# ------------------------------------------------------------------------------
# -- Switch and for statements ---

    @parse_debug
    def parse_switch_block_statement_groups(self):
        statement_groups = list()

        while self.tokens.look().value in ('case', 'default'):
            statement_group = self.parse_switch_block_statement_group()
            statement_groups.append(statement_group)

        return statement_groups

    @parse_debug
    def parse_switch_block_statement_group(self):
        labels = list()
        statements = list()

        while True:
            case_type = self.tokens.next().value
            case_value = None

            if case_type == 'case':
                if self.would_accept(Identifier, ':'):
                    case_value = self.parse_identifier()
                else:
                    case_value = self.parse_expression()

                labels.append(case_value)
            elif not case_type == 'default':
                self.illegal("Expected switch case")

            self.accept(':')

            if self.tokens.look().value not in ('case', 'default'):
                break

        while self.tokens.look().value not in ('case', 'default', '}'):
            statement = self.parse_block_statement()
            statements.append(statement)

        return tree.SwitchStatementCase(case=labels,
                                        statements=statements)

    @parse_debug
    def parse_for_control(self):
        # Try for_var_control and fall back to normal three part for control

        try:
            with self.tokens:
                return self.parse_for_var_control()
        except JavaSyntaxError:
            pass

        init = None
        if not self.would_accept(';'):
            init = self.parse_for_init_or_update()

        self.accept(';')

        condition = None
        if not self.would_accept(';'):
            condition = self.parse_expression()

        self.accept(';')

        update = None
        if not self.would_accept(')'):
            update = self.parse_for_init_or_update()

        return tree.ForControl(init=init,
                               condition=condition,
                               update=update)

    @parse_debug
    def parse_for_var_control(self):
        modifiers, annotations = self.parse_variable_modifiers()
        var_type = self.parse_type()
        var_name = self.parse_identifier()
        var_type.dimensions += self.parse_array_dimension()

        var = tree.VariableDeclaration(modifiers=modifiers,
                                       annotations=annotations,
                                       type=var_type)

        rest = self.parse_for_var_control_rest()

        if isinstance(rest, tree.Expression):
            var.declarators = [tree.VariableDeclarator(name=var_name)]
            return tree.EnhancedForControl(var=var,
                                           iterable=rest)
        else:
            declarators, condition, update = rest
            declarators[0].name = var_name
            var.declarators = declarators
            return tree.ForControl(init=var,
                                   condition=condition,
                                   update=update)

    @parse_debug
    def parse_for_var_control_rest(self):
        if self.try_accept(':'):
            expression = self.parse_expression()
            return expression

        declarators = None
        if not self.would_accept(';'):
            declarators = self.parse_for_variable_declarator_rest()
        else:
            declarators = [tree.VariableDeclarator()]
        self.accept(';')

        condition = None
        if not self.would_accept(';'):
            condition = self.parse_expression()
        self.accept(';')

        update = None
        if not self.would_accept(')'):
            update = self.parse_for_init_or_update()

        return (declarators, condition, update)

    @parse_debug
    def parse_for_variable_declarator_rest(self):
        initializer = None

        if self.try_accept('='):
            initializer = self.parse_variable_initializer()

        declarators = [tree.VariableDeclarator(initializer=initializer)]

        while self.try_accept(','):
            declarator = self.parse_variable_declarator()
            declarators.append(declarator)

        return declarators

    @parse_debug
    def parse_for_init_or_update(self):
        expressions = list()

        while True:
            expression = self.parse_expression()
            expressions.append(expression)

            if not self.try_accept(','):
                break

        return expressions

# ------------------------------------------------------------------------------
# -- Expressions --

    @parse_debug
    def parse_expression(self):
        expressionl = self.parse_expressionl()
        assignment_type = None
        assignment_expression = None

        if self.tokens.look().value in Operator.ASSIGNMENT:
            assignment_type = self.tokens.next().value
            assignment_expression = self.parse_expression()
            return tree.Assignment(expressionl=expressionl,
                                   type=assignment_type,
                                   value=assignment_expression)
        else:
            return expressionl

    @parse_debug
    def parse_expressionl(self):
        expression_2 = self.parse_expression_2()
        true_expression = None
        false_expression = None

        if self.try_accept('?'):
            true_expression = self.parse_expression()
            self.accept(':')
            false_expression = self.parse_expressionl()

            return tree.TernaryExpression(condition=expression_2,
                                          if_true=true_expression,
                                          if_false=false_expression)
        if self.would_accept('->'):
            body = self.parse_lambda_method_body()
            return tree.LambdaExpression(parameters=[expression_2],
                                         body=body)
        if self.try_accept('::'):
            method_reference, type_arguments = self.parse_method_reference()
            return tree.MethodReference(
                expression=expression_2,
                method=method_reference,
                type_arguments=type_arguments)
        return expression_2

    @parse_debug
    def parse_expression_2(self):
        expression_3 = self.parse_expression_3()
        token = self.tokens.look()
        if token.value in Operator.INFIX or token.value == 'instanceof':
            parts = self.parse_expression_2_rest()
            parts.insert(0, expression_3)
            return self.build_binary_operation(parts)

        return expression_3

    @parse_debug
    def parse_expression_2_rest(self):
        parts = list()

        token = self.tokens.look()
        while token.value in Operator.INFIX or token.value == 'instanceof':
            if self.try_accept('instanceof'):
                comparison_type = self.parse_type()
                parts.extend(('instanceof', comparison_type))
            else:
                operator = self.parse_infix_operator()
                expression = self.parse_expression_3()
                parts.extend((operator, expression))

            token = self.tokens.look()

        return parts

# ------------------------------------------------------------------------------
# -- Expression operators --

    @parse_debug
    def parse_expression_3(self):
        prefix_operators = list()
        while self.tokens.look().value in Operator.PREFIX:
            prefix_operators.append(self.tokens.next().value)

        if self.would_accept('('):
            try:
                with self.tokens:
                        lambda_exp = self.parse_lambda_expression()
                        if lambda_exp:
                            return lambda_exp
            except JavaSyntaxError:
                pass
            try:
                with self.tokens:
                    self.accept('(')
                    cast_target = self.parse_type()
                    self.accept(')')
                    expression = self.parse_expression_3()

                    return tree.Cast(type=cast_target,
                                     expression=expression)
            except JavaSyntaxError:
                pass

        primary = self.parse_primary()
        primary.prefix_operators = prefix_operators
        primary.selectors = list()
        primary.postfix_operators = list()

        token = self.tokens.look()
        while token.value in '[.':
            selector = self.parse_selector()
            selector._position = token.position
            primary.selectors.append(selector)

            token = self.tokens.look()

        while token.value in Operator.POSTFIX:
            primary.postfix_operators.append(self.tokens.next().value)
            token = self.tokens.look()

        return primary

    @parse_debug
    def parse_method_reference(self):
        type_arguments = list()
        if self.would_accept('<'):
            type_arguments = self.parse_nonwildcard_type_arguments()
        if self.would_accept('new'):
            method_reference = tree.MemberReference(member=self.accept('new'))
        else:
            method_reference = self.parse_expression()
        return method_reference, type_arguments

    @parse_debug
    def parse_lambda_expression(self):
        lambda_expr = None
        parameters = None
        if self.would_accept('(', Identifier, ','):
            self.accept('(')
            parameters = []
            while not self.would_accept(')'):
                parameters.append(tree.InferredFormalParameter(
                    name=self.parse_identifier()))
                self.try_accept(',')
            self.accept(')')
        else:
            parameters = self.parse_formal_parameters()
        body = self.parse_lambda_method_body()
        return tree.LambdaExpression(parameters=parameters,
                                     body=body)

    @parse_debug
    def parse_lambda_method_body(self):
        if self.accept('->'):
            if self.would_accept('{'):
                return self.parse_block()
            else:
                return self.parse_expression()

    @parse_debug
    def parse_infix_operator(self):
        operator = self.accept(Operator)

        if not operator in Operator.INFIX:
            self.illegal("Expected infix operator")

        if operator == '>' and self.try_accept('>'):
            operator = '>>'

            if self.try_accept('>'):
                operator = '>>>'

        return operator

# ------------------------------------------------------------------------------
# -- Primary expressions --

    @parse_debug
    def parse_primary(self):
        token = self.tokens.look()

        if isinstance(token, Literal):
            literal = self.parse_literal()
            literal._position = token.position
            return literal

        elif token.value == '(':
            return self.parse_par_expression()

        elif self.try_accept('this'):
            arguments = None

            if self.would_accept('('):
                arguments = self.parse_arguments()
                return tree.ExplicitConstructorInvocation(arguments=arguments)

            return tree.This()
        elif self.would_accept('super', '::'):
            self.accept('super')
            return token
        elif self.try_accept('super'):
            super_suffix = self.parse_super_suffix()
            return super_suffix

        elif self.try_accept('new'):
            return self.parse_creator()

        elif token.value == '<':
            type_arguments = self.parse_nonwildcard_type_arguments()

            if self.try_accept('this'):
                arguments = self.parse_arguments()
                return tree.ExplicitConstructorInvocation(type_arguments=type_arguments,
                                                          arguments=arguments)
            else:
                invocation = self.parse_explicit_generic_invocation_suffix()
                invocation._position = token.position
                invocation.type_arguments = type_arguments

                return invocation

        elif isinstance(token, Identifier):
            qualified_identifier = [self.parse_identifier()]

            while self.would_accept('.', Identifier):
                self.accept('.')
                identifier = self.parse_identifier()
                qualified_identifier.append(identifier)

            identifier_suffix = self.parse_identifier_suffix()

            if isinstance(identifier_suffix, (tree.MemberReference, tree.MethodInvocation)):
                # Take the last identifer as the member and leave the rest for the qualifier
                identifier_suffix.member = qualified_identifier.pop()

            elif isinstance(identifier_suffix, tree.ClassReference):
                identifier_suffix.type = tree.ReferenceType(name=qualified_identifier.pop())

            identifier_suffix._position = token.position
            identifier_suffix.qualifier = '.'.join(qualified_identifier)

            return identifier_suffix

        elif isinstance(token, BasicType):
            base_type = self.parse_basic_type()
            base_type.dimensions = self.parse_array_dimension()
            self.accept('.', 'class')

            return tree.ClassReference(type=base_type)

        elif self.try_accept('void'):
            self.accept('.', 'class')
            return tree.VoidClassReference()

        self.illegal("Expected expression")

    @parse_debug
    def parse_literal(self):
        literal = self.accept(Literal)
        return tree.Literal(value=literal)

    @parse_debug
    def parse_par_expression(self):
        self.accept('(')
        expression = self.parse_expression()
        self.accept(')')

        return expression

    @parse_debug
    def parse_arguments(self):
        expressions = list()

        self.accept('(')

        if self.try_accept(')'):
            return expressions

        while True:
            expression = self.parse_expression()
            expressions.append(expression)

            if not self.try_accept(','):
                break

        self.accept(')')

        return expressions

    @parse_debug
    def parse_super_suffix(self):
        identifier = None
        type_arguments = None
        arguments = None

        if self.try_accept('.'):
            if self.would_accept('<'):
                type_arguments = self.parse_nonwildcard_type_arguments()

            identifier = self.parse_identifier()

            if self.would_accept('('):
                arguments = self.parse_arguments()
        else:
            arguments = self.parse_arguments()

        if identifier and arguments is not None:
            return tree.SuperMethodInvocation(member=identifier,
                                              arguments=arguments,
                                              type_arguments=type_arguments)
        elif arguments is not None:
            return tree.SuperConstructorInvocation(arguments=arguments)
        else:
            return tree.SuperMemberReference(member=identifier)

    @parse_debug
    def parse_explicit_generic_invocation_suffix(self):
        identifier = None
        arguments = None
        if self.try_accept('super'):
            return self.parse_super_suffix()
        else:
            identifier = self.parse_identifier()
            arguments = self.parse_arguments()
            return tree.MethodInvocation(member=identifier,
                                         arguments=arguments)

# ------------------------------------------------------------------------------
# -- Creators --

    @parse_debug
    def parse_creator(self):
        constructor_type_arguments = None

        if self.would_accept(BasicType):
            created_name = self.parse_basic_type()
            rest = self.parse_array_creator_rest()
            rest.type = created_name
            return rest

        if self.would_accept('<'):
            constructor_type_arguments = self.parse_nonwildcard_type_arguments()

        created_name = self.parse_created_name()

        if self.would_accept('['):
            if constructor_type_arguments:
                self.illegal("Array creator not allowed with generic constructor type arguments")

            rest = self.parse_array_creator_rest()
            rest.type = created_name
            return rest
        else:
            arguments, body = self.parse_class_creator_rest()
            return tree.ClassCreator(constructor_type_arguments=constructor_type_arguments,
                                     type=created_name,
                                     arguments=arguments,
                                     body=body)

    @parse_debug
    def parse_created_name(self):
        created_name = tree.ReferenceType()
        tail = created_name

        while True:
            tail.name = self.parse_identifier()

            if self.would_accept('<'):
                tail.arguments = self.parse_type_arguments_or_diamond()

            if self.try_accept('.'):
                tail.sub_type = tree.ReferenceType()
                tail = tail.sub_type
            else:
                break

        return created_name

    @parse_debug
    def parse_class_creator_rest(self):
        arguments = self.parse_arguments()
        class_body = None

        if self.would_accept('{'):
            class_body = self.parse_class_body()

        return (arguments, class_body)

    @parse_debug
    def parse_array_creator_rest(self):
        if self.would_accept('[', ']'):
            array_dimension = self.parse_array_dimension()
            array_initializer = self.parse_array_initializer()

            return tree.ArrayCreator(dimensions=array_dimension,
                                     initializer=array_initializer)

        else:
            array_dimensions = list()

            while self.would_accept('[') and not self.would_accept('[', ']'):
                self.accept('[')
                expression = self.parse_expression()
                array_dimensions.append(expression)
                self.accept(']')

            array_dimensions += self.parse_array_dimension()
            return tree.ArrayCreator(dimensions=array_dimensions)

    @parse_debug
    def parse_identifier_suffix(self):
        if self.try_accept('[', ']'):
            array_dimension = [None] + self.parse_array_dimension()
            self.accept('.', 'class')
            return tree.ClassReference(type=tree.Type(dimensions=array_dimension))

        elif self.would_accept('('):
            arguments = self.parse_arguments()
            return tree.MethodInvocation(arguments=arguments)

        elif self.try_accept('.', 'class'):
            return tree.ClassReference()

        elif self.try_accept('.', 'this'):
            return tree.This()

        elif self.would_accept('.', '<'):
            next(self.tokens)
            return self.parse_explicit_generic_invocation()

        elif self.try_accept('.', 'new'):
            type_arguments = None

            if self.would_accept('<'):
                type_arguments = self.parse_nonwildcard_type_arguments()

            inner_creator = self.parse_inner_creator()
            inner_creator.constructor_type_arguments = type_arguments

            return inner_creator

        elif self.would_accept('.', 'super', '('):
            self.accept('.', 'super')
            arguments = self.parse_arguments()
            return tree.SuperConstructorInvocation(arguments=arguments)

        else:
            return tree.MemberReference()

    @parse_debug
    def parse_explicit_generic_invocation(self):
        type_arguments = self.parse_nonwildcard_type_arguments()

        token = self.tokens.look()
        
        invocation = self.parse_explicit_generic_invocation_suffix()
        invocation._position = token.position
        invocation.type_arguments = type_arguments

        return invocation

    @parse_debug
    def parse_inner_creator(self):
        identifier = self.parse_identifier()
        type_arguments = None

        if self.would_accept('<'):
            type_arguments = self.parse_nonwildcard_type_arguments_or_diamond()

        java_type = tree.ReferenceType(name=identifier,
                                       arguments=type_arguments)

        arguments, class_body = self.parse_class_creator_rest()

        return tree.InnerClassCreator(type=java_type,
                                      arguments=arguments,
                                      body=class_body)

    @parse_debug
    def parse_selector(self):
        if self.try_accept('['):
            expression = self.parse_expression()
            self.accept(']')
            return tree.ArraySelector(index=expression)

        elif self.try_accept('.'):

            token = self.tokens.look()
            if isinstance(token, Identifier):
                identifier = self.tokens.next().value
                arguments = None

                if self.would_accept('('):
                    arguments = self.parse_arguments()

                    return tree.MethodInvocation(member=identifier,
                                                 arguments=arguments)
                else:
                    return tree.MemberReference(member=identifier)
            elif self.would_accept('super', '::'):
                self.accept('super')
                return token
            elif self.would_accept('<'):
                return self.parse_explicit_generic_invocation()
            elif self.try_accept('this'):
                return tree.This()
            elif self.try_accept('super'):
                return self.parse_super_suffix()
            elif self.try_accept('new'):
                type_arguments = None

                if self.would_accept('<'):
                    type_arguments = self.parse_nonwildcard_type_arguments()

                inner_creator = self.parse_inner_creator()
                inner_creator.constructor_type_arguments = type_arguments

                return inner_creator

        self.illegal("Expected selector")

# ------------------------------------------------------------------------------
# -- Enum and annotation body --

    @parse_debug
    def parse_enum_body(self):
        constants = list()
        body_declarations = list()

        self.accept('{')

        if not self.try_accept(','):
            while not (self.would_accept(';') or self.would_accept('}')):
                constant = self.parse_enum_constant()
                constants.append(constant)

                if not self.try_accept(','):
                    break

        if self.try_accept(';'):
            while not self.would_accept('}'):
                declaration = self.parse_class_body_declaration()

                if declaration:
                    body_declarations.append(declaration)

        self.accept('}')

        return tree.EnumBody(constants=constants,
                             declarations=body_declarations)

    @parse_debug
    def parse_enum_constant(self):
        annotations = list()
        javadoc = None
        constant_name = None
        arguments = None
        body = None

        next_token = self.tokens.look()
        if next_token:
            javadoc = next_token.javadoc

        if self.would_accept(Annotation):
            annotations = self.parse_annotations()

        constant_name = self.parse_identifier()

        if self.would_accept('('):
            arguments = self.parse_arguments()

        if self.would_accept('{'):
            body = self.parse_class_body()

        return tree.EnumConstantDeclaration(annotations=annotations,
                                            name=constant_name,
                                            arguments=arguments,
                                            body=body,
                                            documentation=javadoc)

    @parse_debug
    def parse_annotation_type_body(self):
        declarations = None

        self.accept('{')
        declarations = self.parse_annotation_type_element_declarations()
        self.accept('}')

        return declarations

    @parse_debug
    def parse_annotation_type_element_declarations(self):
        declarations = list()

        while not self.would_accept('}'):
            declaration = self.parse_annotation_type_element_declaration()
            declarations.append(declaration)

        return declarations

    @parse_debug
    def parse_annotation_type_element_declaration(self):
        modifiers, annotations, javadoc = self.parse_modifiers()
        declaration = None

        token = self.tokens.look()
        if self.would_accept('class'):
            declaration = self.parse_normal_class_declaration()
        elif self.would_accept('interface'):
            declaration = self.parse_normal_interface_declaration()
        elif self.would_accept('enum'):
            declaration = self.parse_enum_declaration()
        elif self.is_annotation_declaration():
            declaration = self.parse_annotation_type_declaration()
        else:
            attribute_type = self.parse_type()
            attribute_name = self.parse_identifier()
            declaration = self.parse_annotation_method_or_constant_rest()
            self.accept(';')

            if isinstance(declaration, tree.AnnotationMethod):
                declaration.name = attribute_name
                declaration.return_type = attribute_type
            else:
                declaration.declarators[0].name = attribute_name
                declaration.type = attribute_type

        declaration._position = token.position
        declaration.modifiers = modifiers
        declaration.annotations = annotations
        declaration.documentation = javadoc

        return declaration

    @parse_debug
    def parse_annotation_method_or_constant_rest(self):
        if self.try_accept('('):
            self.accept(')')

            array_dimension = self.parse_array_dimension()
            default = None

            if self.try_accept('default'):
                default = self.parse_element_value()

            return tree.AnnotationMethod(dimensions=array_dimension,
                                         default=default)
        else:
            return self.parse_constant_declarators_rest()

def parse(tokens, debug=False):
    parser = Parser(tokens)
    parser.set_debug(debug)
    return parser.parse()
