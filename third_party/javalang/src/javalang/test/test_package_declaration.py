import unittest

from pkg_resources import resource_string
from .. import parse


# From my reading of the spec (http://docs.oracle.com/javase/specs/jls/se7/html/jls-7.html) the
# allowed order is javadoc, optional annotation, package declaration
class PackageInfo(unittest.TestCase):
    def testPackageDeclarationOnly(self):
        source_file = "source/package-info/NoAnnotationNoJavadoc.java"
        ast = self.get_ast(source_file)

        self.failUnless(ast.package.name == "org.javalang.test")
        self.failIf(ast.package.annotations)
        self.failIf(ast.package.documentation)

    def testAnnotationOnly(self):
        source_file = "source/package-info/AnnotationOnly.java"
        ast = self.get_ast(source_file)

        self.failUnless(ast.package.name == "org.javalang.test")
        self.failUnless(ast.package.annotations)
        self.failIf(ast.package.documentation)

    def testJavadocOnly(self):
        source_file = "source/package-info/JavadocOnly.java"
        ast = self.get_ast(source_file)

        self.failUnless(ast.package.name == "org.javalang.test")
        self.failIf(ast.package.annotations)
        self.failUnless(ast.package.documentation)

    def testAnnotationThenJavadoc(self):
        source_file = "source/package-info/AnnotationJavadoc.java"
        ast = self.get_ast(source_file)

        self.failUnless(ast.package.name == "org.javalang.test")
        self.failUnless(ast.package.annotations)
        self.failIf(ast.package.documentation)

    def testJavadocThenAnnotation(self):
        source_file = "source/package-info/JavadocAnnotation.java"
        ast = self.get_ast(source_file)

        self.failUnless(ast.package.name == "org.javalang.test")
        self.failUnless(ast.package.annotations)
        self.failUnless(ast.package.documentation)

    def get_ast(self, filename):
        source = resource_string(__name__, filename)
        ast = parse.parse(source)

        return ast


def main():
    unittest.main()

if __name__ == '__main__':
    main()
