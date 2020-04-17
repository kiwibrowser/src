import unittest

from pkg_resources import resource_string
from .. import parse, parser, tree


def setup_java_class(content_to_add):
    """ returns an example java class with the
        given content_to_add contained within a method.
    """
    template = """
public class Lambda {

    public static void main(String args[]) {
        %s
    }
}
        """
    return template % content_to_add


def filter_type_in_method(clazz, the_type, method_name):
    """ yields the result of filtering the given class for the given
        type inside the given method identified by its name.
    """
    for path, node in clazz.filter(the_type):
        for p in reversed(path):
            if isinstance(p, tree.MethodDeclaration):
                if p.name == method_name:
                    yield path, node


class LambdaSupportTest(unittest.TestCase):

    """ Contains tests for java 8 lambda syntax. """

    def assert_contains_lambda_expression_in_m(
            self, clazz, method_name='main'):
        """ asserts that the given tree contains a method with the supplied
            method name containing a lambda expression.
        """
        matches = list(filter_type_in_method(
            clazz, tree.LambdaExpression, method_name))
        if not matches:
            self.fail('No matching lambda expression found.')
        return matches

    def test_lambda_support_no_parameters_no_body(self):
        """ tests support for lambda with no parameters and no body. """
        self.assert_contains_lambda_expression_in_m(
            parse.parse(setup_java_class("() -> {};")))

    def test_lambda_support_no_parameters_expression_body(self):
        """ tests support for lambda with no parameters and an
            expression body.
        """
        test_classes = [
            setup_java_class("() -> 3;"),
            setup_java_class("() -> null;"),
            setup_java_class("() -> { return 21; };"),
            setup_java_class("() -> { System.exit(1); };"),
        ]
        for test_class in test_classes:
            clazz = parse.parse(test_class)
            self.assert_contains_lambda_expression_in_m(clazz)

    def test_lambda_support_no_parameters_complex_expression(self):
        """ tests support for lambda with no parameters and a
            complex expression body.
        """
        code = """
                () -> {
            if (true) return 21;
            else
            {
                int result = 21;
                return result / 2;
            }
        };"""
        self.assert_contains_lambda_expression_in_m(
            parse.parse(setup_java_class(code)))

    def test_parameter_no_type_expression_body(self):
        """ tests support for lambda with parameters with inferred types. """
        test_classes = [
            setup_java_class("(bar) -> bar + 1;"),
            setup_java_class("bar -> bar + 1;"),
            setup_java_class("x -> x.length();"),
            setup_java_class("y -> { y.boom(); };"),
        ]
        for test_class in test_classes:
            clazz = parse.parse(test_class)
            self.assert_contains_lambda_expression_in_m(clazz)

    def test_parameter_with_type_expression_body(self):
        """ tests support for lambda with parameters with formal types. """
        test_classes = [
            setup_java_class("(int foo) -> { return foo + 2; };"),
            setup_java_class("(String s) -> s.length();"),
            setup_java_class("(int foo) -> foo + 1;"),
            setup_java_class("(Thread th) -> { th.start(); };"),
            setup_java_class("(String foo, String bar) -> "
                             "foo + bar;"),
        ]
        for test_class in test_classes:
            clazz = parse.parse(test_class)
            self.assert_contains_lambda_expression_in_m(clazz)

    def test_parameters_with_no_type_expression_body(self):
        """ tests support for multiple lambda parameters
            that are specified without their types.
        """
        self.assert_contains_lambda_expression_in_m(
            parse.parse(setup_java_class("(x, y) -> x + y;")))

    def test_parameters_with_mixed_inferred_and_declared_types(self):
        """ this tests that lambda type specification mixing is considered
            invalid as per the specifications.
        """
        with self.assertRaises(parser.JavaSyntaxError):
            parse.parse(setup_java_class("(x, int y) -> x+y;"))

    def test_parameters_inferred_types_with_modifiers(self):
        """ this tests that lambda inferred type parameters with modifiers are
            considered invalid as per the specifications.
        """
        with self.assertRaises(parser.JavaSyntaxError):
            parse.parse(setup_java_class("(x, final y) -> x+y;"))

    def test_invalid_parameters_are_invalid(self):
        """ this tests that invalid lambda parameters are are
            considered invalid as per the specifications.
        """
        with self.assertRaises(parser.JavaSyntaxError):
            parse.parse(setup_java_class("(a b c) -> {};"))

    def test_cast_works(self):
        """ this tests that a cast expression works as expected. """
        parse.parse(setup_java_class("String x = (String) A.x() ;"))


class MethodReferenceSyntaxTest(unittest.TestCase):

    """ Contains tests for java 8 method reference syntax. """

    def assert_contains_method_reference_expression_in_m(
            self, clazz, method_name='main'):
        """ asserts that the given class contains a method with the supplied
            method name containing a method reference.
        """
        matches = list(filter_type_in_method(
            clazz, tree.MethodReference, method_name))
        if not matches:
            self.fail('No matching method reference found.')
        return matches

    def test_method_reference(self):
        """ tests that method references are supported. """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("String::length;")))

    def test_method_reference_to_the_new_method(self):
        """ test support for method references to 'new'. """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("String::new;")))

    def test_method_reference_to_the_new_method_with_explict_type(self):
        """ test support for method references to 'new' with an
            explicit type.
        """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("String::<String> new;")))

    def test_method_reference_from_super(self):
        """ test support for method references from 'super'. """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("super::toString;")))

    def test_method_reference_from_super_with_identifier(self):
        """ test support for method references from Identifier.super. """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("String.super::toString;")))

    @unittest.expectedFailure
    def test_method_reference_explicit_type_arguments_for_generic_type(self):
        """ currently there is no support for method references
            for an explicit type.
        """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("List<String>::size;")))

    def test_method_reference_explicit_type_arguments(self):
        """ test support for method references with an explicit type.
        """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("Arrays::<String> sort;")))

    @unittest.expectedFailure
    def test_method_reference_from_array_type(self):
        """ currently there is no support for method references
            from a primary type.
        """
        self.assert_contains_method_reference_expression_in_m(
            parse.parse(setup_java_class("int[]::new;")))


class InterfaceSupportTest(unittest.TestCase):

    """ Contains tests for java 8 interface extensions. """

    def test_interface_support_static_methods(self):
        parse.parse("""
interface Foo {
    void foo();

    static Foo create() {
        return new Foo() {
            @Override
            void foo() {
                System.out.println("foo");
            }
        };
    }
}
        """)

    def test_interface_support_default_methods(self):
        parse.parse("""
interface Foo {
    default void foo() {
        System.out.println("foo");
    }
}
        """)


def main():
    unittest.main()

if __name__ == '__main__':
    main()
