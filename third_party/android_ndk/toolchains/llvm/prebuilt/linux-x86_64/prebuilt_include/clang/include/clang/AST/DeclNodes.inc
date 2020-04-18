/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* List of AST nodes of a particular kind                                     *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef ABSTRACT_DECL
#  define ABSTRACT_DECL(Type) Type
#endif
#ifndef DECL_RANGE
#  define DECL_RANGE(Base, First, Last)
#endif

#ifndef LAST_DECL_RANGE
#  define LAST_DECL_RANGE(Base, First, Last) DECL_RANGE(Base, First, Last)
#endif

#ifndef ACCESSSPEC
#  define ACCESSSPEC(Type, Base) DECL(Type, Base)
#endif
ACCESSSPEC(AccessSpec, Decl)
#undef ACCESSSPEC

#ifndef BLOCK
#  define BLOCK(Type, Base) DECL(Type, Base)
#endif
BLOCK(Block, Decl)
#undef BLOCK

#ifndef CAPTURED
#  define CAPTURED(Type, Base) DECL(Type, Base)
#endif
CAPTURED(Captured, Decl)
#undef CAPTURED

#ifndef CLASSSCOPEFUNCTIONSPECIALIZATION
#  define CLASSSCOPEFUNCTIONSPECIALIZATION(Type, Base) DECL(Type, Base)
#endif
CLASSSCOPEFUNCTIONSPECIALIZATION(ClassScopeFunctionSpecialization, Decl)
#undef CLASSSCOPEFUNCTIONSPECIALIZATION

#ifndef EMPTY
#  define EMPTY(Type, Base) DECL(Type, Base)
#endif
EMPTY(Empty, Decl)
#undef EMPTY

#ifndef EXPORT
#  define EXPORT(Type, Base) DECL(Type, Base)
#endif
EXPORT(Export, Decl)
#undef EXPORT

#ifndef EXTERNCCONTEXT
#  define EXTERNCCONTEXT(Type, Base) DECL(Type, Base)
#endif
EXTERNCCONTEXT(ExternCContext, Decl)
#undef EXTERNCCONTEXT

#ifndef FILESCOPEASM
#  define FILESCOPEASM(Type, Base) DECL(Type, Base)
#endif
FILESCOPEASM(FileScopeAsm, Decl)
#undef FILESCOPEASM

#ifndef FRIEND
#  define FRIEND(Type, Base) DECL(Type, Base)
#endif
FRIEND(Friend, Decl)
#undef FRIEND

#ifndef FRIENDTEMPLATE
#  define FRIENDTEMPLATE(Type, Base) DECL(Type, Base)
#endif
FRIENDTEMPLATE(FriendTemplate, Decl)
#undef FRIENDTEMPLATE

#ifndef IMPORT
#  define IMPORT(Type, Base) DECL(Type, Base)
#endif
IMPORT(Import, Decl)
#undef IMPORT

#ifndef LINKAGESPEC
#  define LINKAGESPEC(Type, Base) DECL(Type, Base)
#endif
LINKAGESPEC(LinkageSpec, Decl)
#undef LINKAGESPEC

#ifndef NAMED
#  define NAMED(Type, Base) DECL(Type, Base)
#endif
ABSTRACT_DECL(NAMED(Named, Decl))
#ifndef LABEL
#  define LABEL(Type, Base) NAMED(Type, Base)
#endif
LABEL(Label, NamedDecl)
#undef LABEL

#ifndef NAMESPACE
#  define NAMESPACE(Type, Base) NAMED(Type, Base)
#endif
NAMESPACE(Namespace, NamedDecl)
#undef NAMESPACE

#ifndef NAMESPACEALIAS
#  define NAMESPACEALIAS(Type, Base) NAMED(Type, Base)
#endif
NAMESPACEALIAS(NamespaceAlias, NamedDecl)
#undef NAMESPACEALIAS

#ifndef OBJCCOMPATIBLEALIAS
#  define OBJCCOMPATIBLEALIAS(Type, Base) NAMED(Type, Base)
#endif
OBJCCOMPATIBLEALIAS(ObjCCompatibleAlias, NamedDecl)
#undef OBJCCOMPATIBLEALIAS

#ifndef OBJCCONTAINER
#  define OBJCCONTAINER(Type, Base) NAMED(Type, Base)
#endif
ABSTRACT_DECL(OBJCCONTAINER(ObjCContainer, NamedDecl))
#ifndef OBJCCATEGORY
#  define OBJCCATEGORY(Type, Base) OBJCCONTAINER(Type, Base)
#endif
OBJCCATEGORY(ObjCCategory, ObjCContainerDecl)
#undef OBJCCATEGORY

#ifndef OBJCIMPL
#  define OBJCIMPL(Type, Base) OBJCCONTAINER(Type, Base)
#endif
ABSTRACT_DECL(OBJCIMPL(ObjCImpl, ObjCContainerDecl))
#ifndef OBJCCATEGORYIMPL
#  define OBJCCATEGORYIMPL(Type, Base) OBJCIMPL(Type, Base)
#endif
OBJCCATEGORYIMPL(ObjCCategoryImpl, ObjCImplDecl)
#undef OBJCCATEGORYIMPL

#ifndef OBJCIMPLEMENTATION
#  define OBJCIMPLEMENTATION(Type, Base) OBJCIMPL(Type, Base)
#endif
OBJCIMPLEMENTATION(ObjCImplementation, ObjCImplDecl)
#undef OBJCIMPLEMENTATION

DECL_RANGE(ObjCImpl, ObjCCategoryImpl, ObjCImplementation)

#undef OBJCIMPL

#ifndef OBJCINTERFACE
#  define OBJCINTERFACE(Type, Base) OBJCCONTAINER(Type, Base)
#endif
OBJCINTERFACE(ObjCInterface, ObjCContainerDecl)
#undef OBJCINTERFACE

#ifndef OBJCPROTOCOL
#  define OBJCPROTOCOL(Type, Base) OBJCCONTAINER(Type, Base)
#endif
OBJCPROTOCOL(ObjCProtocol, ObjCContainerDecl)
#undef OBJCPROTOCOL

DECL_RANGE(ObjCContainer, ObjCCategory, ObjCProtocol)

#undef OBJCCONTAINER

#ifndef OBJCMETHOD
#  define OBJCMETHOD(Type, Base) NAMED(Type, Base)
#endif
OBJCMETHOD(ObjCMethod, NamedDecl)
#undef OBJCMETHOD

#ifndef OBJCPROPERTY
#  define OBJCPROPERTY(Type, Base) NAMED(Type, Base)
#endif
OBJCPROPERTY(ObjCProperty, NamedDecl)
#undef OBJCPROPERTY

#ifndef TEMPLATE
#  define TEMPLATE(Type, Base) NAMED(Type, Base)
#endif
ABSTRACT_DECL(TEMPLATE(Template, NamedDecl))
#ifndef BUILTINTEMPLATE
#  define BUILTINTEMPLATE(Type, Base) TEMPLATE(Type, Base)
#endif
BUILTINTEMPLATE(BuiltinTemplate, TemplateDecl)
#undef BUILTINTEMPLATE

#ifndef REDECLARABLETEMPLATE
#  define REDECLARABLETEMPLATE(Type, Base) TEMPLATE(Type, Base)
#endif
ABSTRACT_DECL(REDECLARABLETEMPLATE(RedeclarableTemplate, TemplateDecl))
#ifndef CLASSTEMPLATE
#  define CLASSTEMPLATE(Type, Base) REDECLARABLETEMPLATE(Type, Base)
#endif
CLASSTEMPLATE(ClassTemplate, RedeclarableTemplateDecl)
#undef CLASSTEMPLATE

#ifndef FUNCTIONTEMPLATE
#  define FUNCTIONTEMPLATE(Type, Base) REDECLARABLETEMPLATE(Type, Base)
#endif
FUNCTIONTEMPLATE(FunctionTemplate, RedeclarableTemplateDecl)
#undef FUNCTIONTEMPLATE

#ifndef TYPEALIASTEMPLATE
#  define TYPEALIASTEMPLATE(Type, Base) REDECLARABLETEMPLATE(Type, Base)
#endif
TYPEALIASTEMPLATE(TypeAliasTemplate, RedeclarableTemplateDecl)
#undef TYPEALIASTEMPLATE

#ifndef VARTEMPLATE
#  define VARTEMPLATE(Type, Base) REDECLARABLETEMPLATE(Type, Base)
#endif
VARTEMPLATE(VarTemplate, RedeclarableTemplateDecl)
#undef VARTEMPLATE

DECL_RANGE(RedeclarableTemplate, ClassTemplate, VarTemplate)

#undef REDECLARABLETEMPLATE

#ifndef TEMPLATETEMPLATEPARM
#  define TEMPLATETEMPLATEPARM(Type, Base) TEMPLATE(Type, Base)
#endif
TEMPLATETEMPLATEPARM(TemplateTemplateParm, TemplateDecl)
#undef TEMPLATETEMPLATEPARM

DECL_RANGE(Template, BuiltinTemplate, TemplateTemplateParm)

#undef TEMPLATE

#ifndef TYPE
#  define TYPE(Type, Base) NAMED(Type, Base)
#endif
ABSTRACT_DECL(TYPE(Type, NamedDecl))
#ifndef TAG
#  define TAG(Type, Base) TYPE(Type, Base)
#endif
ABSTRACT_DECL(TAG(Tag, TypeDecl))
#ifndef ENUM
#  define ENUM(Type, Base) TAG(Type, Base)
#endif
ENUM(Enum, TagDecl)
#undef ENUM

#ifndef RECORD
#  define RECORD(Type, Base) TAG(Type, Base)
#endif
RECORD(Record, TagDecl)
#ifndef CXXRECORD
#  define CXXRECORD(Type, Base) RECORD(Type, Base)
#endif
CXXRECORD(CXXRecord, RecordDecl)
#ifndef CLASSTEMPLATESPECIALIZATION
#  define CLASSTEMPLATESPECIALIZATION(Type, Base) CXXRECORD(Type, Base)
#endif
CLASSTEMPLATESPECIALIZATION(ClassTemplateSpecialization, CXXRecordDecl)
#ifndef CLASSTEMPLATEPARTIALSPECIALIZATION
#  define CLASSTEMPLATEPARTIALSPECIALIZATION(Type, Base) CLASSTEMPLATESPECIALIZATION(Type, Base)
#endif
CLASSTEMPLATEPARTIALSPECIALIZATION(ClassTemplatePartialSpecialization, ClassTemplateSpecializationDecl)
#undef CLASSTEMPLATEPARTIALSPECIALIZATION

DECL_RANGE(ClassTemplateSpecialization, ClassTemplateSpecialization, ClassTemplatePartialSpecialization)

#undef CLASSTEMPLATESPECIALIZATION

DECL_RANGE(CXXRecord, CXXRecord, ClassTemplatePartialSpecialization)

#undef CXXRECORD

DECL_RANGE(Record, Record, ClassTemplatePartialSpecialization)

#undef RECORD

DECL_RANGE(Tag, Enum, ClassTemplatePartialSpecialization)

#undef TAG

#ifndef TEMPLATETYPEPARM
#  define TEMPLATETYPEPARM(Type, Base) TYPE(Type, Base)
#endif
TEMPLATETYPEPARM(TemplateTypeParm, TypeDecl)
#undef TEMPLATETYPEPARM

#ifndef TYPEDEFNAME
#  define TYPEDEFNAME(Type, Base) TYPE(Type, Base)
#endif
ABSTRACT_DECL(TYPEDEFNAME(TypedefName, TypeDecl))
#ifndef OBJCTYPEPARAM
#  define OBJCTYPEPARAM(Type, Base) TYPEDEFNAME(Type, Base)
#endif
OBJCTYPEPARAM(ObjCTypeParam, TypedefNameDecl)
#undef OBJCTYPEPARAM

#ifndef TYPEALIAS
#  define TYPEALIAS(Type, Base) TYPEDEFNAME(Type, Base)
#endif
TYPEALIAS(TypeAlias, TypedefNameDecl)
#undef TYPEALIAS

#ifndef TYPEDEF
#  define TYPEDEF(Type, Base) TYPEDEFNAME(Type, Base)
#endif
TYPEDEF(Typedef, TypedefNameDecl)
#undef TYPEDEF

DECL_RANGE(TypedefName, ObjCTypeParam, Typedef)

#undef TYPEDEFNAME

#ifndef UNRESOLVEDUSINGTYPENAME
#  define UNRESOLVEDUSINGTYPENAME(Type, Base) TYPE(Type, Base)
#endif
UNRESOLVEDUSINGTYPENAME(UnresolvedUsingTypename, TypeDecl)
#undef UNRESOLVEDUSINGTYPENAME

DECL_RANGE(Type, Enum, UnresolvedUsingTypename)

#undef TYPE

#ifndef USING
#  define USING(Type, Base) NAMED(Type, Base)
#endif
USING(Using, NamedDecl)
#undef USING

#ifndef USINGDIRECTIVE
#  define USINGDIRECTIVE(Type, Base) NAMED(Type, Base)
#endif
USINGDIRECTIVE(UsingDirective, NamedDecl)
#undef USINGDIRECTIVE

#ifndef USINGPACK
#  define USINGPACK(Type, Base) NAMED(Type, Base)
#endif
USINGPACK(UsingPack, NamedDecl)
#undef USINGPACK

#ifndef USINGSHADOW
#  define USINGSHADOW(Type, Base) NAMED(Type, Base)
#endif
USINGSHADOW(UsingShadow, NamedDecl)
#ifndef CONSTRUCTORUSINGSHADOW
#  define CONSTRUCTORUSINGSHADOW(Type, Base) USINGSHADOW(Type, Base)
#endif
CONSTRUCTORUSINGSHADOW(ConstructorUsingShadow, UsingShadowDecl)
#undef CONSTRUCTORUSINGSHADOW

DECL_RANGE(UsingShadow, UsingShadow, ConstructorUsingShadow)

#undef USINGSHADOW

#ifndef VALUE
#  define VALUE(Type, Base) NAMED(Type, Base)
#endif
ABSTRACT_DECL(VALUE(Value, NamedDecl))
#ifndef BINDING
#  define BINDING(Type, Base) VALUE(Type, Base)
#endif
BINDING(Binding, ValueDecl)
#undef BINDING

#ifndef DECLARATOR
#  define DECLARATOR(Type, Base) VALUE(Type, Base)
#endif
ABSTRACT_DECL(DECLARATOR(Declarator, ValueDecl))
#ifndef FIELD
#  define FIELD(Type, Base) DECLARATOR(Type, Base)
#endif
FIELD(Field, DeclaratorDecl)
#ifndef OBJCATDEFSFIELD
#  define OBJCATDEFSFIELD(Type, Base) FIELD(Type, Base)
#endif
OBJCATDEFSFIELD(ObjCAtDefsField, FieldDecl)
#undef OBJCATDEFSFIELD

#ifndef OBJCIVAR
#  define OBJCIVAR(Type, Base) FIELD(Type, Base)
#endif
OBJCIVAR(ObjCIvar, FieldDecl)
#undef OBJCIVAR

DECL_RANGE(Field, Field, ObjCIvar)

#undef FIELD

#ifndef FUNCTION
#  define FUNCTION(Type, Base) DECLARATOR(Type, Base)
#endif
FUNCTION(Function, DeclaratorDecl)
#ifndef CXXDEDUCTIONGUIDE
#  define CXXDEDUCTIONGUIDE(Type, Base) FUNCTION(Type, Base)
#endif
CXXDEDUCTIONGUIDE(CXXDeductionGuide, FunctionDecl)
#undef CXXDEDUCTIONGUIDE

#ifndef CXXMETHOD
#  define CXXMETHOD(Type, Base) FUNCTION(Type, Base)
#endif
CXXMETHOD(CXXMethod, FunctionDecl)
#ifndef CXXCONSTRUCTOR
#  define CXXCONSTRUCTOR(Type, Base) CXXMETHOD(Type, Base)
#endif
CXXCONSTRUCTOR(CXXConstructor, CXXMethodDecl)
#undef CXXCONSTRUCTOR

#ifndef CXXCONVERSION
#  define CXXCONVERSION(Type, Base) CXXMETHOD(Type, Base)
#endif
CXXCONVERSION(CXXConversion, CXXMethodDecl)
#undef CXXCONVERSION

#ifndef CXXDESTRUCTOR
#  define CXXDESTRUCTOR(Type, Base) CXXMETHOD(Type, Base)
#endif
CXXDESTRUCTOR(CXXDestructor, CXXMethodDecl)
#undef CXXDESTRUCTOR

DECL_RANGE(CXXMethod, CXXMethod, CXXDestructor)

#undef CXXMETHOD

DECL_RANGE(Function, Function, CXXDestructor)

#undef FUNCTION

#ifndef MSPROPERTY
#  define MSPROPERTY(Type, Base) DECLARATOR(Type, Base)
#endif
MSPROPERTY(MSProperty, DeclaratorDecl)
#undef MSPROPERTY

#ifndef NONTYPETEMPLATEPARM
#  define NONTYPETEMPLATEPARM(Type, Base) DECLARATOR(Type, Base)
#endif
NONTYPETEMPLATEPARM(NonTypeTemplateParm, DeclaratorDecl)
#undef NONTYPETEMPLATEPARM

#ifndef VAR
#  define VAR(Type, Base) DECLARATOR(Type, Base)
#endif
VAR(Var, DeclaratorDecl)
#ifndef DECOMPOSITION
#  define DECOMPOSITION(Type, Base) VAR(Type, Base)
#endif
DECOMPOSITION(Decomposition, VarDecl)
#undef DECOMPOSITION

#ifndef IMPLICITPARAM
#  define IMPLICITPARAM(Type, Base) VAR(Type, Base)
#endif
IMPLICITPARAM(ImplicitParam, VarDecl)
#undef IMPLICITPARAM

#ifndef OMPCAPTUREDEXPR
#  define OMPCAPTUREDEXPR(Type, Base) VAR(Type, Base)
#endif
OMPCAPTUREDEXPR(OMPCapturedExpr, VarDecl)
#undef OMPCAPTUREDEXPR

#ifndef PARMVAR
#  define PARMVAR(Type, Base) VAR(Type, Base)
#endif
PARMVAR(ParmVar, VarDecl)
#undef PARMVAR

#ifndef VARTEMPLATESPECIALIZATION
#  define VARTEMPLATESPECIALIZATION(Type, Base) VAR(Type, Base)
#endif
VARTEMPLATESPECIALIZATION(VarTemplateSpecialization, VarDecl)
#ifndef VARTEMPLATEPARTIALSPECIALIZATION
#  define VARTEMPLATEPARTIALSPECIALIZATION(Type, Base) VARTEMPLATESPECIALIZATION(Type, Base)
#endif
VARTEMPLATEPARTIALSPECIALIZATION(VarTemplatePartialSpecialization, VarTemplateSpecializationDecl)
#undef VARTEMPLATEPARTIALSPECIALIZATION

DECL_RANGE(VarTemplateSpecialization, VarTemplateSpecialization, VarTemplatePartialSpecialization)

#undef VARTEMPLATESPECIALIZATION

DECL_RANGE(Var, Var, VarTemplatePartialSpecialization)

#undef VAR

DECL_RANGE(Declarator, Field, VarTemplatePartialSpecialization)

#undef DECLARATOR

#ifndef ENUMCONSTANT
#  define ENUMCONSTANT(Type, Base) VALUE(Type, Base)
#endif
ENUMCONSTANT(EnumConstant, ValueDecl)
#undef ENUMCONSTANT

#ifndef INDIRECTFIELD
#  define INDIRECTFIELD(Type, Base) VALUE(Type, Base)
#endif
INDIRECTFIELD(IndirectField, ValueDecl)
#undef INDIRECTFIELD

#ifndef OMPDECLAREREDUCTION
#  define OMPDECLAREREDUCTION(Type, Base) VALUE(Type, Base)
#endif
OMPDECLAREREDUCTION(OMPDeclareReduction, ValueDecl)
#undef OMPDECLAREREDUCTION

#ifndef UNRESOLVEDUSINGVALUE
#  define UNRESOLVEDUSINGVALUE(Type, Base) VALUE(Type, Base)
#endif
UNRESOLVEDUSINGVALUE(UnresolvedUsingValue, ValueDecl)
#undef UNRESOLVEDUSINGVALUE

DECL_RANGE(Value, Binding, UnresolvedUsingValue)

#undef VALUE

DECL_RANGE(Named, Label, UnresolvedUsingValue)

#undef NAMED

#ifndef OMPTHREADPRIVATE
#  define OMPTHREADPRIVATE(Type, Base) DECL(Type, Base)
#endif
OMPTHREADPRIVATE(OMPThreadPrivate, Decl)
#undef OMPTHREADPRIVATE

#ifndef OBJCPROPERTYIMPL
#  define OBJCPROPERTYIMPL(Type, Base) DECL(Type, Base)
#endif
OBJCPROPERTYIMPL(ObjCPropertyImpl, Decl)
#undef OBJCPROPERTYIMPL

#ifndef PRAGMACOMMENT
#  define PRAGMACOMMENT(Type, Base) DECL(Type, Base)
#endif
PRAGMACOMMENT(PragmaComment, Decl)
#undef PRAGMACOMMENT

#ifndef PRAGMADETECTMISMATCH
#  define PRAGMADETECTMISMATCH(Type, Base) DECL(Type, Base)
#endif
PRAGMADETECTMISMATCH(PragmaDetectMismatch, Decl)
#undef PRAGMADETECTMISMATCH

#ifndef STATICASSERT
#  define STATICASSERT(Type, Base) DECL(Type, Base)
#endif
STATICASSERT(StaticAssert, Decl)
#undef STATICASSERT

#ifndef TRANSLATIONUNIT
#  define TRANSLATIONUNIT(Type, Base) DECL(Type, Base)
#endif
TRANSLATIONUNIT(TranslationUnit, Decl)
#undef TRANSLATIONUNIT

LAST_DECL_RANGE(Decl, AccessSpec, TranslationUnit)

#undef DECL
#undef DECL_RANGE
#undef LAST_DECL_RANGE
#undef ABSTRACT_DECL
/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* List of AST Decl nodes                                                     *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef DECL_CONTEXT
#  define DECL_CONTEXT(DECL)
#endif
#ifndef DECL_CONTEXT_BASE
#  define DECL_CONTEXT_BASE(DECL) DECL_CONTEXT(DECL)
#endif
DECL_CONTEXT_BASE(Function)
DECL_CONTEXT_BASE(Tag)
DECL_CONTEXT_BASE(ObjCContainer)
DECL_CONTEXT(Block)
DECL_CONTEXT(Captured)
DECL_CONTEXT(Export)
DECL_CONTEXT(ExternCContext)
DECL_CONTEXT(LinkageSpec)
DECL_CONTEXT(Namespace)
DECL_CONTEXT(OMPDeclareReduction)
DECL_CONTEXT(ObjCMethod)
DECL_CONTEXT(TranslationUnit)
#undef DECL_CONTEXT
#undef DECL_CONTEXT_BASE
