/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <typeinfo>
#include <cstdio>

using namespace std;

class Foo
{
public:
  virtual ~Foo() { }
  virtual void print()
  {
    std::printf("in Foo!\n");
  }
};

class Bar: public Foo
{
  public:
  void print()
  {
    std::printf("in Bar!\n");
  }
};

struct Base {};
struct Derived : Base {};
struct Poly_Base {virtual void Member(){}};
struct Poly_Derived: Poly_Base {};

#define CHECK(cond)  \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "KO: Assertion failure: %s\n", #cond); \
            fail++;\
        }\
    } while (0)

int main()
{
    int  fail = 0;
    Foo* foo = new Bar();
    Bar* bar;

    // built-in types:
    int i;
    int * pi;

    CHECK(typeid(int) == typeid(i));
    CHECK(typeid(int*) == typeid(pi));
    CHECK(typeid(int) == typeid(*pi));

    printf("int is: %s\n", typeid(int).name());
    printf("  i is: %s\n", typeid(i).name());
    printf(" pi is: %s\n", typeid(pi).name());
    printf("*pi is: %s\n", typeid(*pi).name());

    // non-polymorphic types:
    Derived derived;
    Base* pbase = &derived;

    CHECK(typeid(derived) == typeid(Derived));
    CHECK(typeid(pbase) == typeid(Base*));
    CHECK(typeid(&derived) == typeid(Derived*));

    printf("derived is: %s\n", typeid(derived).name());
    printf(" *pbase is: %s\n", typeid(*pbase).name());

    // polymorphic types:
    Poly_Derived polyderived;
    Poly_Base* ppolybase = &polyderived;

    CHECK(typeid(polyderived) == typeid(Poly_Derived));
    CHECK(typeid(ppolybase) == typeid(Poly_Base*));
    CHECK(typeid(polyderived) == typeid(*ppolybase));

    printf("polyderived is: %s\n", typeid(polyderived).name());
    printf(" *ppolybase is: %s\n", typeid(*ppolybase).name());
    
    bar = dynamic_cast<Bar*>(foo);
    if (bar != NULL) {
        printf("OK: 'foo' is pointing to a Bar class instance.\n");
    } else {
        fprintf(stderr, "KO: Could not dynamically cast 'foo' to a 'Bar*'\n");
        fail++;
    }

    delete foo;

    return (fail > 0);
}
