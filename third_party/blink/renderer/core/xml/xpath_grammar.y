/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

%{

#include "third_party/blink/renderer/core/xml/xpath_functions.h"
#include "third_party/blink/renderer/core/xml/xpath_ns_resolver.h"
#include "third_party/blink/renderer/core/xml/xpath_parser.h"
#include "third_party/blink/renderer/core/xml/xpath_path.h"
#include "third_party/blink/renderer/core/xml/xpath_predicate.h"
#include "third_party/blink/renderer/core/xml/xpath_step.h"
#include "third_party/blink/renderer/core/xml/xpath_variable_reference.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"

void* YyFastMalloc(size_t size)
{
  return WTF::Partitions::FastMalloc(size, nullptr);
}

#define YYMALLOC YyFastMalloc
#define YYFREE WTF::Partitions::FastFree

#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1
#define YYDEBUG 0
#define YYMAXDEPTH 10000

using blink::XPath::Step;
%}

%pure-parser
%parse-param { blink::XPath::Parser* parser }

%union
{
  blink::XPath::Step::Axis axis;
  blink::XPath::Step::NodeTest* node_test;
  blink::XPath::NumericOp::Opcode num_op;
  blink::XPath::EqTestOp::Opcode eq_op;
  String* str;
  blink::XPath::Expression* expr;
  blink::HeapVector<blink::Member<blink::XPath::Predicate>>* pred_list;
  blink::HeapVector<blink::Member<blink::XPath::Expression>>* arg_list;
  blink::XPath::Step* step;
  blink::XPath::LocationPath* location_path;
}

%{

static int xpathyylex(YYSTYPE* yylval) { return blink::XPath::Parser::Current()->Lex(yylval); }
static void xpathyyerror(void*, const char*) { }

%}

%left <num_op> MULOP
%left <eq_op> EQOP RELOP
%left PLUS MINUS
%left OR AND
%token <axis> AXISNAME
%token <str> NODETYPE PI FUNCTIONNAME LITERAL
%token <str> VARIABLEREFERENCE NUMBER
%token DOTDOT SLASHSLASH
%token <str> NAMETEST
%token XPATH_ERROR

%type <location_path> LocationPath
%type <location_path> AbsoluteLocationPath
%type <location_path> RelativeLocationPath
%type <step> Step
%type <axis> AxisSpecifier
%type <step> DescendantOrSelf
%type <node_test> NodeTest
%type <expr> Predicate
%type <pred_list> OptionalPredicateList
%type <pred_list> PredicateList
%type <step> AbbreviatedStep
%type <expr> Expr
%type <expr> PrimaryExpr
%type <expr> FunctionCall
%type <arg_list> ArgumentList
%type <expr> Argument
%type <expr> UnionExpr
%type <expr> PathExpr
%type <expr> FilterExpr
%type <expr> OrExpr
%type <expr> AndExpr
%type <expr> EqualityExpr
%type <expr> RelationalExpr
%type <expr> AdditiveExpr
%type <expr> MultiplicativeExpr
%type <expr> UnaryExpr

%%

Expr:
    OrExpr
    {
      parser->top_expr_ = $1;
    }
    ;

LocationPath:
    RelativeLocationPath
    {
      $$->SetAbsolute(false);
    }
    |
    AbsoluteLocationPath
    {
      $$->SetAbsolute(true);
    }
    ;

AbsoluteLocationPath:
    '/'
    {
      $$ = new blink::XPath::LocationPath;
    }
    |
    '/' RelativeLocationPath
    {
      $$ = $2;
    }
    |
    DescendantOrSelf RelativeLocationPath
    {
      $$ = $2;
      $$->InsertFirstStep($1);
    }
    ;

RelativeLocationPath:
    Step
    {
      $$ = new blink::XPath::LocationPath;
      $$->AppendStep($1);
    }
    |
    RelativeLocationPath '/' Step
    {
      $$->AppendStep($3);
    }
    |
    RelativeLocationPath DescendantOrSelf Step
    {
      $$->AppendStep($2);
      $$->AppendStep($3);
    }
    ;

Step:
    NodeTest OptionalPredicateList
    {
      if ($2)
        $$ = new Step(Step::kChildAxis, *$1, *$2);
      else
        $$ = new Step(Step::kChildAxis, *$1);
    }
    |
    NAMETEST OptionalPredicateList
    {
      AtomicString local_name;
      AtomicString namespace_uri;
      if (!parser->ExpandQName(*$1, local_name, namespace_uri)) {
        parser->got_namespace_error_ = true;
        YYABORT;
      }

      if ($2)
        $$ = new Step(Step::kChildAxis, Step::NodeTest(Step::NodeTest::kNameTest, local_name, namespace_uri), *$2);
      else
        $$ = new Step(Step::kChildAxis, Step::NodeTest(Step::NodeTest::kNameTest, local_name, namespace_uri));
       parser->DeleteString($1);
    }
    |
    AxisSpecifier NodeTest OptionalPredicateList
    {
      if ($3)
        $$ = new Step($1, *$2, *$3);
      else
        $$ = new Step($1, *$2);
    }
    |
    AxisSpecifier NAMETEST OptionalPredicateList
    {
      AtomicString local_name;
      AtomicString namespace_uri;
      if (!parser->ExpandQName(*$2, local_name, namespace_uri)) {
        parser->got_namespace_error_ = true;
        YYABORT;
      }

      if ($3)
        $$ = new Step($1, Step::NodeTest(Step::NodeTest::kNameTest, local_name, namespace_uri), *$3);
      else
        $$ = new Step($1, Step::NodeTest(Step::NodeTest::kNameTest, local_name, namespace_uri));
      parser->DeleteString($2);
    }
    |
    AbbreviatedStep
    ;

AxisSpecifier:
    AXISNAME
    |
    '@'
    {
      $$ = Step::kAttributeAxis;
    }
    ;

NodeTest:
    NODETYPE '(' ')'
    {
      if (*$1 == "node")
        $$ = new Step::NodeTest(Step::NodeTest::kAnyNodeTest);
      else if (*$1 == "text")
        $$ = new Step::NodeTest(Step::NodeTest::kTextNodeTest);
      else if (*$1 == "comment")
        $$ = new Step::NodeTest(Step::NodeTest::kCommentNodeTest);

      parser->DeleteString($1);
    }
    |
    PI '(' ')'
    {
      $$ = new Step::NodeTest(Step::NodeTest::kProcessingInstructionNodeTest);
      parser->DeleteString($1);
    }
    |
    PI '(' LITERAL ')'
    {
      $$ = new Step::NodeTest(Step::NodeTest::kProcessingInstructionNodeTest, $3->StripWhiteSpace());
      parser->DeleteString($1);
      parser->DeleteString($3);
    }
    ;

OptionalPredicateList:
    /* empty */
    {
      $$ = 0;
    }
    |
    PredicateList
    ;

PredicateList:
    Predicate
    {
      $$ = new blink::HeapVector<blink::Member<blink::XPath::Predicate>>;
      $$->push_back(new blink::XPath::Predicate($1));
    }
    |
    PredicateList Predicate
    {
      $$->push_back(new blink::XPath::Predicate($2));
    }
    ;

Predicate:
    '[' Expr ']'
    {
      $$ = $2;
    }
    ;

DescendantOrSelf:
    SLASHSLASH
    {
      $$ = new Step(Step::kDescendantOrSelfAxis, Step::NodeTest(Step::NodeTest::kAnyNodeTest));
    }
    ;

AbbreviatedStep:
    '.'
    {
      $$ = new Step(Step::kSelfAxis, Step::NodeTest(Step::NodeTest::kAnyNodeTest));
    }
    |
    DOTDOT
    {
      $$ = new Step(Step::kParentAxis, Step::NodeTest(Step::NodeTest::kAnyNodeTest));
    }
    ;

PrimaryExpr:
    VARIABLEREFERENCE
    {
      $$ = new blink::XPath::VariableReference(*$1);
      parser->DeleteString($1);
    }
    |
    '(' Expr ')'
    {
      $$ = $2;
    }
    |
    LITERAL
    {
      $$ = new blink::XPath::StringExpression(*$1);
      parser->DeleteString($1);
    }
    |
    NUMBER
    {
      $$ = new blink::XPath::Number($1->ToDouble());
      parser->DeleteString($1);
    }
    |
    FunctionCall
    ;

FunctionCall:
    FUNCTIONNAME '(' ')'
    {
      $$ = blink::XPath::CreateFunction(*$1);
      if (!$$)
        YYABORT;
      parser->DeleteString($1);
    }
    |
    FUNCTIONNAME '(' ArgumentList ')'
    {
      $$ = blink::XPath::CreateFunction(*$1, *$3);
      if (!$$)
        YYABORT;
      parser->DeleteString($1);
    }
    ;

ArgumentList:
    Argument
    {
      $$ = new blink::HeapVector<blink::Member<blink::XPath::Expression>>;
      $$->push_back($1);
    }
    |
    ArgumentList ',' Argument
    {
      $$->push_back($3);
    }
    ;

Argument:
    Expr
    ;

UnionExpr:
    PathExpr
    |
    UnionExpr '|' PathExpr
    {
      $$ = new blink::XPath::Union;
      $$->AddSubExpression($1);
      $$->AddSubExpression($3);
    }
    ;

PathExpr:
    LocationPath
    {
      $$ = $1;
    }
    |
    FilterExpr
    |
    FilterExpr '/' RelativeLocationPath
    {
      $3->SetAbsolute(true);
      $$ = new blink::XPath::Path($1, $3);
    }
    |
    FilterExpr DescendantOrSelf RelativeLocationPath
    {
      $3->InsertFirstStep($2);
      $3->SetAbsolute(true);
      $$ = new blink::XPath::Path($1, $3);
    }
    ;

FilterExpr:
    PrimaryExpr
    |
    PrimaryExpr PredicateList
    {
      $$ = new blink::XPath::Filter($1, *$2);
    }
    ;

OrExpr:
    AndExpr
    |
    OrExpr OR AndExpr
    {
      $$ = new blink::XPath::LogicalOp(blink::XPath::LogicalOp::kOP_Or, $1, $3);
    }
    ;

AndExpr:
    EqualityExpr
    |
    AndExpr AND EqualityExpr
    {
      $$ = new blink::XPath::LogicalOp(blink::XPath::LogicalOp::kOP_And, $1, $3);
    }
    ;

EqualityExpr:
    RelationalExpr
    |
    EqualityExpr EQOP RelationalExpr
    {
      $$ = new blink::XPath::EqTestOp($2, $1, $3);
    }
    ;

RelationalExpr:
    AdditiveExpr
    |
    RelationalExpr RELOP AdditiveExpr
    {
      $$ = new blink::XPath::EqTestOp($2, $1, $3);
    }
    ;

AdditiveExpr:
    MultiplicativeExpr
    |
    AdditiveExpr PLUS MultiplicativeExpr
    {
      $$ = new blink::XPath::NumericOp(blink::XPath::NumericOp::kOP_Add, $1, $3);
    }
    |
    AdditiveExpr MINUS MultiplicativeExpr
    {
      $$ = new blink::XPath::NumericOp(blink::XPath::NumericOp::kOP_Sub, $1, $3);
    }
    ;

MultiplicativeExpr:
    UnaryExpr
    |
    MultiplicativeExpr MULOP UnaryExpr
    {
      $$ = new blink::XPath::NumericOp($2, $1, $3);
    }
    ;

UnaryExpr:
    UnionExpr
    |
    MINUS UnaryExpr
    {
      $$ = new blink::XPath::Negative;
      $$->AddSubExpression($2);
    }
    ;

%%
