.. include:: /migration/deprecation.inc

===============================
Contents Of PNaCl Bitcode Files
===============================

.. contents::
   :local:
   :backlinks: none
   :depth: 3


Introduction
============

This document is a reference manual for the contents of PNaCl bitcode files. We
define bitcode files via three layers. The first layer is presented using
assembly language *PNaClAsm*, and defines the textual form of the bitcode
file. The textual form is then lowered to a sequence of :ref:`PNaCl
records<link_for_pnacl_records>`. The final layer applies abbreviations that
convert each PNaCl record into a corresponding sequence of bits.

.. image:: /images/PNaClBitcodeFlow.png

PNaClAsm uses a *static single assignment* (SSA) based representation that
requires generated results to have a single (assignment) source.

PNaClAsm focuses on the semantic content of the file, not the bit-encoding of
that content. However, it does provide annotations that allow one to specify how
the :ref:`abbreviations<link_for_abbreviations_section>` are used to convert
PNaCl records into the sequence of bits.

Each construct in PNaClAsm defines a corresponding :ref:`PNaCl
record<link_for_pnacl_records>`.  A PNaCl bitcode file is simply a sequence of
PNaCl records. The goal of PNaClAsm is to make records easier to read, and not
to define a high-level user programming language.

PNaCl records are an abstract encoding of structured data, similar to XML. Like
XML, A PNaCl record has a notion of a tag (i.e. the first element in a record,
called a *code*). PNaCl records can be nested. Nesting is defined by a
corresponding :ref:`enter<link_for_enter_block_record_section>` and
:ref:`exit<link_for_exit_block_record_section>` block record.

These block records must be used like balanced parentheses to define the block
structure that is imposed on top of records. Each exit record must be preceded
by a corresponding enter record. Blocks can be nested by nesting enter/exit
records appropriately.

The *PNaCl bitcode writer* takes the sequence of records, defined by a PNaClAsm
program, and converts each record into a (variable-length) sequence of bits. The
output of each bit sequence is appended together. The resulting generated
sequence of bits is the contents of the PNaCl bitcode file.

For every kind of record, there is a method for converting records into bit
sequences. These methods correspond to a notion of
:ref:`abbreviations<link_for_abbreviations_section>`.  Each abbreviation defines
a specific bit sequence conversion to be applied.

Abbreviations can be user-defined, but there are also predefined defaults.  All
user-specified abbreviations are included in the generated bitcode
file. Predefined defaults are not.

Each abbreviation defines how a record is converted to a bit sequence. The
:ref:`PNaCl translator<link_for_pnacl_translator>` uses these abbreviations
to convert the bit sequence back to the corresponding sequence of PNaCl records.
As a result, all records have an abbreviation (user or default) associated with
them.

Conceptually, abbreviations are used to define how to pack the contents of
records into bit sequences.  The main reason for defining abbreviations is to
save space. The default abbreviations are simplistic and are intended to handle
all possible records. The default abbreviations do not really worry about being
efficient, in terms of the number of bits generated.

By separating the concepts of PNaCl records and abbreviations, the notion of
data compression is cleanly separated from semantic content. This allows
different use cases to decide how much effort should be spent on compressing
records.

For a JIT compiler that produces bitcode, little (if any) compression should be
applied. In fact, the API to the JIT may just be the records themselves.  The
goal of a JIT is to perform the final translation to machine code as quickly as
possible.

On the other hand, when delivering across the web, one may want to compress the
sequence of bits considerably, to reduce costs in delivering web pages. Note
that :ref:`pnacl-compress<pnacl_compress>` is provided as part of the SDK to do
this job.

Data Model
==========

The data model for PNaCl bitcode is fixed at little-endian ILP32: pointers are
32 bits in size. 64-bit integer types are also supported natively via the i64
type (for example, a front-end can generate these from the C/C++ type ``long
long``).

Integers are assumed to be modeled using two's complement.  Floating point
support is fixed at :ref:`IEEE 754<c_cpp_floating_point>` 32-bit and 64-bit
values (float and double, respectively).

PNaCl Blocks
============

Blocks are used to organize records in the bitcode file.  The kinds of blocks
defined in PNaClAsm are:

Module block
  A top-level block defining the program. The :ref:`module
  block<link_for_module_block>` defines global information used by the program,
  followed by function blocks defining the implementation of functions within
  the program. All other blocks (listed below) must appear within a module
  block.

Types block
  The :ref:`types block<link_for_types_block_section>` defines the set of types
  used by the program. All types used in the program must be defined in the
  types block.  These types consist of primitive types as well as high level
  constructs such as vectors and function signatures.

Globals block
  The :ref:`globals block<link_for_globals_block_section>` defines the set of
  addresses of global variables and constants used by the program. It also
  defines how each global (associated with the global address) is initialized.

Valuesymtab block
  The :ref:`valuesymtab block<link_for_valuesymtab_block_section>` defines
  textual names for external function addresses.

Function block
  Each function (implemented) in a program has its own :ref:`function
  block<link_for_function_blocks_section>` that defines the implementation of
  the corresponding function.

Constants block
  Each implemented function that uses constants in its instructions defines a
  :ref:`constants block<link_for_constants_block_section>`. Constants blocks
  appear within the corresponding function block of the implemented function.

Abbreviations block
  Defines global abbreviations that are used to compress PNaCl records. The
  :ref:`abbreviations block<link_for_abbreviations_block_section>` is segmented
  into multiple sections, one section for each kind of block. This block appears
  at the beginning of the module block.

This section is only intended as a high-level discussion of blocks. Later
sections will dive more deeply into the constraints on how blocks must be laid
out. This section only presents the overall concepts of what kinds of data are
stored in each of the blocks.

A PNaCl program consists of a :ref:`header
record<link_for_header_record_section>` and a :ref:`module
block<link_for_module_block>`. The header record defines a sequence of bytes
uniquely identifying the file as a bitcode file. The module block defines the
program to run.

Each block, within a bitcode file, defines values. These values are associated
with IDs. Each type of block defines different kinds of IDs.  The
:ref:`module<link_for_module_block>`,
:ref:`types<link_for_types_block_section>`,
:ref:`globals<link_for_globals_block_section>`, and
:ref:`abbreviations<link_for_abbreviations_block_section>` blocks define global
identifiers, and only a single instance can appear.  The
:ref:`function<link_for_function_blocks_section>` and
:ref:`constant<link_for_constants_block_section>` blocks define local
identifiers, and can have multiple instances (one for each implemented
function).

The only records in the module block that define values, are :ref:`function
address<link_for_function_address_section>` records.  Each function address
record defines a different function address, and the :ref:`type
signature<link_for_function_type>` associated with that function address.

Each :ref:`function block<link_for_function_blocks_section>` defines the
implementation of a single function. Each function block defines the
intermediate representation of the function, consisting of basic blocks and
instructions. If constants are used within instructions, they are defined in a
:ref:`constants block<link_for_constants_block_section>`, nested within the
corresponding function block.

All function blocks are associated with a corresponding function address. This
association is positional rather than explicit. That is, the Nth function block
in a module block corresponds to the Nth
:ref:`defining<link_for_function_address_section>` (rather than declared)
function address record in the module block.

Hence, within a function block, there is no explicit reference to the function
address the block defines. For readability, PNaClAsm uses the corresponding
function signature, associated with the corresponding function address record,
even though that data does not appear in the corresponding records.

.. _link_for_pnacl_records:

PNaCl Records
=============

A PNaCl record is a non-empty sequence of unsigned, 64-bit, integers. A record
is identified by the record *code*, which is the first element in the
sequence. Record codes are unique within a specific kind of block, but are not
necessarily unique across different kinds of blocks. The record code acts as the
variant discriminator (i.e. tag) within a block, to identify what kind of record
it is.

Record codes that are local to a specific kind of block are small values
(starting from zero). In an ideal world, they would be a consecutive sequence of
integers, starting at zero. However, the reality is that PNaCl records evolved
over time (and actually started as `LLVM records
<http://llvm.org/docs/BitCodeFormat.html>`_).  For backward compatibility,
obsolete numbers have not been reused, leaving gaps in the actual record code
values used.

Global record codes are record codes that have the same meaning in multiple
kinds of blocks. To separate global record codes from local record codes, large
values are used.  Currently there are four :ref:`global record
codes<link_for_global_record_codes>`.  To make these cases clear, and to leave
ample room for future growth in PNaClAsm, these special records have record
codes close to the value 2\ :sup:`16`\ . Note: Well-formed PNaCl bitcode files
do not have record codes >= 2\ :sup:`16`\ .
 
A PNaCl record is denoted as follows: ::

  a: <v0, v1, ... , vN>

The value ``v0`` is the record code. The remaining values, ``v1`` through
``vN``, are parameters that fill in additional information needed by the
construct it represents. All records must have a record code. Hence, empty PNaCl
records are not allowed. ``a`` is the index to the abbreviation used to convert
the record to a bit sequence.

While most records (for a given record code) have the same length, it is not
true of all record codes. Some record codes can have arbitrary length. In
particular, function type signatures, call instructions, phi instructions,
switch instructions, and global variable initialization records all have
variable length. The expected length is predefined and part of the PNaClAsm
language. See the corresponding construct (associated with the record) to
determine the expected length.

The *PNaCl bitstream writer*, which converts records to bit sequences, does
this by writing out the abbreviation index used to encode the record, followed
by the contents of the record. The details of this are left to the section on
:ref:`abbreviations<link_for_abbreviations_section>`. However, at the record
level, one important aspect of this appears in :ref:`block
enter<link_for_enter_block_record_section>` records.  These records must define
how many bits are required to hold abbreviation indices associated with records
of that block.

.. _link_for_default_abbreviations:

Default Abbreviations
=====================

There are 4 predefined (default) abbreviation indices, used as the default
abbreviations for PNaCl records. They are:

0
  Abbreviation index for the abbreviation used to bit-encode an exit block
  record.

1
  Abbreviation index for the abbreviation used to bit-encode an enter block
  record.

2
  Abbreviation index for the abbreviation used to bit-encode a user-defined
  abbreviation. Note: User-defined abbreviations are also encoded as records,
  and hence need an abbreviation index to bit-encode them.

3
  Abbreviation index for the default abbreviation to bit-encode all other
  records in the bitcode file.

A block may, in addition, define a list of block specific, user-defined,
abbreviations (of length ``U``). The number of bits ``B`` specified for an enter
record must be sufficiently large such that::

   2**B >= U + 4

In addition, the upper limit for ``B`` is ``16``.

PNaClAsm requires specifying the number of bits needed to read abbreviations as
part of the enter block record. This allows the PNaCl bitcode reader/writer to
use the specified number of bits to encode abbreviation indices.

PNaCl Identifiers
=================

A program is defined by a :ref:`module block<link_for_module_block>`. Blocks can
be nested within other blocks, including the module block. Each block defines a
sequence of records.

Most of the records, within a block, also define unique values.  Each unique
value is given a corresponding unique identifier (i.e. *ID*). In PNaClAsm, each
kind of block defines its own kind of identifiers. The names of these
identifiers are defined by concatenating a prefix character (``'@'`` or
``'%'``), the kind of block (a single character), and a suffix index. The suffix
index is defined by the positional location of the defined value within the
records of the corresponding block. The indices are all zero based, meaning that
the first defined value (within a block) is defined using index 0.

Identifiers are categorized into two types, *local* and *global*.  Local
identifiers are identifiers that are associated with the implementation of a
single function. In that sense, they are local to the block they appear in.

All other identifiers are global, and can appear in multiple blocks. This split
is intentional. Global identifiers are used by multiple functions, and therefore
must be known in all function implementations. Local identifiers only apply to a
single function, and can be reused between functions.  The :ref:`PNaCl
translator<link_for_pnacl_translator>` uses this separation to parallelize the
compilation of functions.

Note that local abbreviation identifiers are unique to the block they appear
in. Global abbreviation identifiers are only unique to the block type they are
defined for. Different block types can reuse global abbreviation identifiers.

Global identifiers use the prefix character ``'@'`` while local identifiers use
the prefix character ``'%'``.

Note that by using positional location to define identifiers (within a block),
the values defined in PNaCl bitcode files need not be explicitly included in the
bitcode file. Rather, they are inferred by the (ordered) position of the record
in the block.  This is also intentional. It is used to reduce the amount of data
that must be (explicitly) passed to the :ref:`PNaCl
translator<link_for_pnacl_translator>`, when downloaded into Chrome.

In general, most of the records within blocks are assumed to be topologically
sorted, putting value definitions before their uses.  This implies that records
do not need to encode data if they can deduce the corresponding information from
their uses.

The most common use of this is that many instructions use the type of their
operands to determine the type of the instruction. Again, this is
intentional. It allows less information to be stored.

However, for function blocks (which define instructions), a topological sort may
not exist. Loop carried value dependencies simply do not allow topologically
sorting. To deal with this, function blocks have a notion of (instruction value)
:ref:`forward type
declarations<link_for_forward_type_declaration_section>`. These declarations
must appear before any of the uses of that value, if the (instruction) value is
defined later in the function than its first use.

The kinds of identifiers used in PNaClAsm are:

@a
  Global abbreviation identifier.

%a
  Local abbreviation identifier.

%b
  Function basic block identifier.

%c
  Function constant identifier.

@f
  Global function address identifier.

@g
  Global variable/constant address identifier.

%p
  Function parameter identifier.

@t
  Global type identifier.

%v
  Value generated by an instruction in a function block.


Conventions For Describing Records
==================================

PNaClAsm is the textual representation of :ref:`PNaCl
records<link_for_pnacl_records>`.  Each PNaCl record is described by a
corresponding PNaClAsm construct. These constructs are described using syntax
rules, and semantics on how they are converted to records. Along with the rules,
is a notion of :ref:`global state<link_for_global_state_section>`. The global
state is updated by syntax rules.  The purpose of the global state is to track
positional dependencies between records.

For each PNaCl construct, we define multiple sections. The **Syntax**
section defines a syntax rule for the construct. The **Record** section
defines the corresponding record associated with the syntax rule. The
**Semantics** section describes the semantics associated with the record, in
terms of data within the global state and the corresponding syntax. It also
includes other high-level semantics, when appropriate.

The **Constraints** section (if present) defines any constraints associated
with the construct, including the global state. The **Updates** section (if
present) defines how the global state is updated when the construct is
processed.  The **Examples** section gives one or more examples of using the
corresponding PNaClAsm construct.

Some semantics sections use functions to compute values. The meaning of
functions can be found in :ref:`support
functions<link_for_support_functions_section>`.

The syntax rule may include the
:ref:`abbreviation<link_for_abbreviations_section>` to use, when converting to a
bit-sequence.  These abbreviations, if allowed, are at the end of the construct,
and enclosed in ``<`` and ``>`` brackets. These abbreviations are optional in
the syntax, and can be omitted. If they are used, the abbreviation brackets are
part of the actual syntax of the construct. If the abbreviation is omitted, the
default abbreviation index is used. To make it clear that abbreviations are
optional, syntax rules separate abbreviations using plenty of whitespace.

Within a syntax rule, lower case characters are literal values. Sequences of
upper case alphanumeric characters are named values.  If we mix lower and upper
case letters within a name appearing in a syntax rule, the lower case letters
are literal while the upper case sequence of alphanumeric characters denote rule
specific values.  The valid values for each of these names will be defined in
the corresponding semantics and constraints subsections.

For example, consider the following syntax rule::

   %vN = add T O1, O2;                            <A>

This rule defines a PNaClAsm add instruction.  This construct defines an
instruction that adds two values (``O1`` and ``O2``) to generate instruction
value ``%vN``. The types of the arguments, and the result, are all of type
``T``. If abbreviation ID ``A`` is present, the record is encoded using that
abbreviation. Otherwise the corresponding :ref:`default abbreviation
index<link_for_default_abbreviations>` is used.

To be concrete, the syntactic rule above defines the structure of the following
PNaClAsm examples::

  %v10 = add i32 %v1, %v2;  <@a5>
  %v11 = add i32 %v10, %v3;

In addition to specifying the syntax, each syntax rule can also also specify the
contents of the corresponding record in the corresponding record subsection. In
simple cases, the elements of the corresponding record are predefined (literal)
constants. Otherwise the record element is an identifier from another subsection
associated with the construct.

Factorial Example
=================

This section provides a simple example of a PNaCl bitcode file. Its contents
describe a bitcode file that only defines a function to compute the factorial
value of a number.

In C, the factorial function can be defined as::

  int fact(int n) {
    if (n == 1) return 1;
    return n * fact(n-1);
  }

Compiling this into a PNaCl bitcode file, and dumping out its contents with
utility :ref:`pnacl-bcdis<pnacl-bcdis>`, the corresponding output is::

    0:0|<65532, 80, 69, 88, 69, 1, 0,|Magic Number: 'PEXE' (80, 69, 88, 69)
       | 8, 0, 17, 0, 4, 0, 2, 0, 0, |PNaCl Version: 2
       | 0>                          |
   16:0|1: <65535, 8, 2>             |module {  // BlockID = 8
   24:0|  3: <1, 1>                  |  version 1;
   26:4|  1: <65535, 0, 2>           |  abbreviations {  // BlockID = 0
   36:0|  0: <65534>                 |  }
   40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
   48:0|    3: <1, 4>                |    count 4;
   50:4|    3: <7, 32>               |    @t0 = i32;
   53:6|    3: <2>                   |    @t1 = void;
   55:4|    3: <21, 0, 0, 0>         |    @t2 = i32 (i32);
   59:4|    3: <7, 1>                |    @t3 = i1;
   62:0|  0: <65534>                 |  }
   64:0|  3: <8, 2, 0, 0, 0>         |  define external i32 @f0(i32);
   68:6|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
   76:0|    3: <5, 0>                |    count 0;
   78:4|  0: <65534>                 |  }
   80:0|  1: <65535, 14, 2>          |  valuesymtab {  // BlockID = 14
   88:0|    3: <1, 0, 102, 97, 99,   |    @f0 : "fact";
       |        116>                 |
   96:4|  0: <65534>                 |  }
  100:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0) {  
       |                             |                   // BlockID = 12
  108:0|    3: <1, 3>                |    blocks 3;
  110:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
  120:0|      3: <1, 0>              |      i32:
  122:4|      3: <4, 2>              |        %c0 = i32 1;
  125:0|    0: <65534>               |      }
       |                             |  %b0:
  128:0|    3: <28, 2, 1, 32>        |    %v0 = icmp eq i32 %p0, %c0;
  132:6|    3: <11, 1, 2, 1>         |    br i1 %v0, label %b1, label %b2;
       |                             |  %b1:
  136:6|    3: <10, 2>               |    ret i32 %c0;
       |                             |  %b2:
  139:2|    3: <2, 3, 2, 1>          |    %v1 = sub i32 %p0, %c0;
  143:2|    3: <34, 0, 5, 1>         |    %v2 = call i32 @f0(i32 %v1);
  148:0|    3: <2, 5, 1, 2>          |    %v3 = mul i32 %p0, %v2;
  152:0|    3: <10, 1>               |    ret i32 %v3;
  154:4|  0: <65534>                 |  }
  156:0|0: <65534>                   |}

Note that there are three columns in this output. The first column contains the
bit positions of the records within the bitcode file. The second column contains
the sequence of records within the bitcode file. The third column contains the
corresponding PNaClAsm program.

Bit positions are defined by a pair ``B:N``. ``B`` is the number of bytes, while
``N`` is the bit offset within the ``B``-th byte. Hence, the bit position (in
bits) is::

  B*8 + N

Hence, the first record is at bit offset ``0`` (``0*8+0``). The second record is
at bit offset ``128`` (``16*8+0``).  The third record is at bit offset ``192``
(``24*8+0``).  The fourth record is at bit offset ``212`` (``26*8+4``).

The :ref:`header record<link_for_header_record_section>` is a sequence of 16
bytes, defining the contents of the first 16 bytes of the bitcode file. These
bytes never change, and are expected for all version 2, PNaCl bitcode files. The
first four bytes define the magic number of the file, i.e. 'PEXE'. All PEXE
bitcode files begin with these four bytes.

All but the header record has an abbreviation index associated with it. Since no
user-defined abbreviations are provided, all records were converted to
bit sequences using default abbreviations.

The types block (starting at bit address ``40:0``), defines 4 types: ``i1``,
``i32``, ``void``, and function signature ``i32 (i32)``.

Bit address ``64:0`` declares the factorial function address ``@f0``, and its
corresponding type signature. Bit address ``88:0`` associates the name ``fact``
with function address ``@f0``.

Bit address ``100:0`` defines the function block that implements function
``fact``. The entry point is ``%b0`` (at bit address ``128:0``). It uses the
32-bit integer constant ``1`` (defined at bit addresses ``122:4``). Bit address
``128:0`` defines an equality comparison of the argument ``%p0`` with ``1``
(constant ``%c0``). Bit address ``132:6`` defines a conditional branch. If the
result of the previous comparison (``%v0``) is true, the program will branch to
block ``%b1``. Otherwise it will branch to block ``%b2``.

Bit address ``136:6`` returns constant ``1`` (``%c0``) when the input parameter
is 1.  Instructions between bit address ``139:2`` and ``154:4`` compute and
return ``n * fact(n-1)``.

Road Map
========

At this point, this document transitions from basic concepts to the details
of how records should be formatted. This section defines the road map to
the remaining sections in this document.

Many records have implicit information associated with them, and must be
maintained across records. :ref:`Global state<link_for_global_state_section>`
describes how this implicit information is modeled. In addition, there are
various :ref:`support functions<link_for_support_functions_section>` that are
used to define the semantics of records, and how they update the global state.

There are just a handful of global records (records that either don't appear in
any block, or can appear in all blocks).  :ref:`Global
records<link_for_global_record_codes>` describes these records. This includes
the block delimiter records :ref:`enter<link_for_enter_block_record_section>`
and :ref:`exit<link_for_exit_block_record_section>` that define block
boundaries.

PNaClAsm is a strongly typed language, and most block values are typed.
:ref:`types<link_for_types_block_section>` describes the set of legal types, and
how to define types.

Global variables and their initializers are presented in the :ref:`globals
block<link_for_globals_block_section>`. :ref:`Function
addresses<link_for_function_address_section>` are part of the :ref:`module
block<link_for_module_block>`, but must be defined before any global variables.

Names to be associated with global variables and function addresses, are defined
in the :ref:`valuesymtab block<link_for_valuesymtab_block_section>`, and must
appear after the :ref:`globals block<link_for_globals_block_section>`, but
before any :ref:`function definition<link_for_function_blocks_section>`.

The :ref:`module block<link_for_module_block>` is the top-most block, and all
other blocks must appear within the module block. The module block defines the
executable in the bitcode file.

Constants used within a :ref:`function
definition<link_for_function_blocks_section>` must be defined using a
:ref:`constants block<link_for_constants_block_section>`.  Each function
definition is defined by a :ref:`function
block<link_for_function_blocks_section>` and constant blocks can only appear
within function blocks. Constants defined within a constant block can only be
used in the enclosing function block.

Function definitions are defined by a sequence of instructions. There are
several types of instructions.

A :ref:`terminator instruction<link_for_terminator_instruction_section>` is the
last instruction in a :ref:`basic block<link_for_function_blocks_section>`, and
is a branch, return, or unreachable instruction.

There are :ref:`integer<link_for_integer_binary_instructions>` and
:ref:`floating point<link_for_floating_point_binary_instructions>` binary
operations.  Integer binary instructions include both arithmetic and logical
operations. Floating point instructions define arithmetic operations.

There are also :ref:`memory
access<link_for_memory_creation_and_access_instructions>` instructions that
allow one to load and store values. That section also includes how to define
local variables using the :ref:`alloca
instruction<link_for_alloca_instruction>`.

One can also convert integer and floating point values using :ref:`conversion
instructions<link_for_conversion_instructions>`.

:ref:`Comparison instructions<link_for_compare_instructions>`
allow you to compare values.

:ref:`Vector instructions<link_for_vector_instructions>` allow you to build and
update vectors. Corresponding :ref:`intrinsic
functions<link_for_intrinsic_functions_section>`, as well as
:ref:`integer<link_for_integer_binary_instructions>` and :ref:`floating
point<link_for_floating_point_binary_instructions>` binary instructions allow
you to apply operations to vectors.

In addition, :ref:`other instructions<link_for_other_pnaclasm_instructions>` are
available. This includes function and procedure calls.

There are also :ref:`memory
alignment<link_for_memory_blocks_and_alignment_section>` issues that should be
considered for global and local variables, as well as load and store
instructions.

Finally, how to pack records is described in the
:ref:`abbreviations<link_for_abbreviations_section>` section.

.. _link_for_global_state_section:

Global State
============

This section describes the global state associated with PNaClAsm. It is used to
define contextual data that is carried between records.

In particular, PNaClAsm is a strongly typed language, and hence, we must track
the type associated with values. Subsection :ref:`link_to_typing_functions`
describes the functions used to maintain typing information associated with
values.

Values are implicitly ordered within a block, and the indices associated with
the values do not appear in records.  Rather, ID counters are used to figure out
what corresponding ID name is associated with a value generating record.
Subsection :ref:`link_to_ID_Counters` defines counters maintained in the global
state.

In several blocks, one of the first records in the block defines how many values
are defined in in the block. The main purpose of these counts is to communicate
to the :ref:`PNaCl translator<link_for_pnacl_translator>` space requirements, or
a limit so that it can detect bad references to values. Subsection
:ref:`link_for_Size_Variables` defines variables that hold size definitions in
the corresponding records.

Finally, the function and constants block contain implicit context between
records in those blocks. Subsection :ref:`link_to_Other_Variables` defines the
variables that contain this implicit context.

.. _link_to_typing_functions:

Typing Functions
----------------

Associated with most identifiers is a type. This type defines what type the
corresponding value has. It is defined by the (initially empty) map::

  TypeOf: ID -> Type

For each type in the :ref:`types block<link_for_types_block_section>`, a
corresponding inverse map::

  TypeID: Type -> ID

is maintained to convert syntactic types to the corresponding type ID.

Note: This document assumes that map ``TypeID`` is automatically maintained
during updates to map ``TypeOf`` (when given a type ``ID``). Hence, *Updates*
subsections will not contain assignments to this map.

Associated with each function identifier is its :ref:`type
signature<link_for_function_type>`. This is different than the type of the
function identifier, since function identifiers represent the function address
which is a pointer (and pointers are always implemented as a 32-bit integer
following the ILP32 data model).

Function type signatures are maintained using::

  TypeOfFcn: ID -> Type

In addition, if a function address has an implementing block, there is a
corresponding implementation associated with the function address.  To indicate
which function addresses have implementations, we use the set::

  DefiningFcnIDs: set(ID)

.. _link_to_ID_Counters:

ID Counters
-----------

Each block defines one or more kinds of values.  Value indices are generated
sequentially, starting at zero. To capture this, the following counters are
defined:

NumTypes
  The number of types defined so far (in the :ref:`types
  block<link_for_types_block_section>`).

NumFuncAddresses
  The number of function addresses defined so far (in the :ref:`module
  block<link_for_module_block>`).

NumGlobalAddresses
  The number of global variable/constant addresses defined so far (in the
  :ref:`globals block<link_for_globals_block_section>`).

NumParams
  The number of parameters defined for a function. Note: Unlike other counters,
  this value is set once, at the beginning of the corresponding :ref:`function
  block<link_for_function_blocks_section>`, based on the type signature
  associated with the function.

NumFcnConsts
  The number of constants defined in a function so far (in the corresponding
  nested :ref:`constants block<link_for_constants_block_section>`).

NumBasicBlocks
  The number of basic blocks defined so far (within a :ref:`function
  block<link_for_function_blocks_section>`).

NumValuedInsts
  The number of instructions, generating values, defined so far (within a
  :ref:`function block<link_for_function_blocks_section>`).

.. _link_for_Size_Variables:

Size Variables
--------------

A number of blocks define expected sizes of constructs. These sizes are recorded
in the following size variables:

ExpectedBasicBlocks
  The expected :ref:`number of basic blocks<link_for_basic_blocks_count>` within
  a function implementation.

ExpectedTypes
  The expected :ref:`number of types<link_for_types_count_record>` defined in
  the types block.

ExpectedGlobals
  The expected :ref:`number of global variable/constant
  addresses<link_for_globals_count_record>` in the globals block.

ExpectedInitializers
  The expected :ref:`number of initializers<link_for_compound_initializer>` for
  a global variable/constant address in the globals block.

It is assumed that the corresponding :ref:`ID counters<link_to_ID_counters>` are
always smaller than the corresponding size variables (except
ExpectedInitializers). That is::

  NumBasicBlocks < ExpectedBasicBlocks
  NumTypes < ExpectedTypes
  NumGlobalAddresses < ExpectedGlobals

.. _link_to_Other_Variables:

Other Variables
---------------

EnclosingFcnID
  The function ID of the function block being processed.

ConstantsSetType
  Holds the type associated with the last :ref:`set type
  record<link_for_constants_set_type_record>` in the constants block. Note: at
  the beginning of each constants block, this variable is set to type void.

.. _link_for_global_record_codes:

Global Records
==============

Global records are records that can appear in any block. These records have
the same meaning in multiple kinds of blocks.

There are four global PNaCl records, each having its own record code. These
global records are:

Header
  The :ref:`header record<link_for_header_record_section>` is the first record
  of a PNaCl bitcode file, and identifies the file's magic number, as well as
  the bitcode version it uses. The record defines the sequence of bytes that
  make up the header and uniquely identifies the file as a PNaCl bitcode file.

Enter
  An :ref:`enter record<link_for_enter_block_record_section>` defines the
  beginning of a block. Since blocks can be nested, one can appear inside other
  blocks, as well as at the top level.

Exit
  An :ref:`exit record<link_for_exit_block_record_section>` defines the end of a
  block. Hence, it must appear in every block, to end the block.

Abbreviation
  An :ref:`abbreviation record<link_for_abbreviation_record>` defines a
  user-defined abbreviation to be applied to records within blocks.
  Abbreviation records appearing in the abbreviations block define global
  abbreviations. All other abbreviations are local to the block they appear in,
  and can only be used in that block.

All global records can't have user-defined abbreviations associated with
them. The :ref:`default abbreviation<link_for_default_abbreviations>` is always
used.

.. _link_for_header_record_section:

Header Record
-------------

The header record must be the first record in the file. It is the only record in
the bitcode file that doesn't have a corresponding construct in PNaClAsm.  In
addition, no abbreviation index is associated with it.

**Syntax**:

There is no syntax for header records in PNaClAsm.

**Record**::

   <65532, 80, 69, 88, 69, 1, 0, 8, 0, 17, 0, 4, 0, 2, 0, 0, 0>

**Semantics**:

The header record defines the initial sequence of bytes that must appear at the
beginning of all (PNaCl bitcode version 2) files. That sequence is the list of
bytes inside the record (excluding the record code). As such, it uniquely
identifies all PNaCl bitcode files.

**Examples**::

    0:0|<65532, 80, 69, 88, 69, 1, 0,|Magic Number: 'PEXE' (80, 69, 88, 69)
       | 8, 0, 17, 0, 4, 0, 2, 0, 0, |PNaCl Version: 2
       | 0>                          |

.. _link_for_enter_block_record_section:

Enter Block Record
------------------

Block records can be top-level, as well as nested in other blocks. Blocks must
begin with an *enter* record, and end with an
:ref:`exit<link_for_exit_block_record_section>` record.

**Syntax**::

  N {                                             <B>

**Record**::

  1: <65535, ID, B>

**Semantics**:

Enter block records define the beginning of a block.  ``B``, if present, is the
number of bits needed to represent all possible abbreviation indices used within
the block. If omitted, ``B=2`` is assumed.

The block ``ID`` value is dependent on the name ``N``. Valid names and
corresponding ``BlockID`` values are defined as follows:

============= ========
N             Block ID
============= ========
abbreviations 0
constants     11
function      12
globals       19
module        8
types         17
valuesymtab   14
============= ========

Note: For readability, PNaClAsm defines a more readable form of a function block
enter record.  See :ref:`function blocks<link_for_function_blocks_section>` for
more details.

**Examples**::

   16:0|1: <65535, 8, 2>             |module {  // BlockID = 8
   24:0|  3: <1, 1>                  |  version 1;
   26:4|  1: <65535, 0, 2>           |  abbreviations {  // BlockID = 0
   36:0|  0: <65534>                 |  }
   40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
   48:0|    3: <1, 2>                |    count 2;
   50:4|    3: <2>                   |    @t0 = void;
   52:2|    3: <21, 0, 0>            |    @t1 = void ();
   55:4|  0: <65534>                 |  }
   56:0|  3: <8, 1, 0, 1, 0>         |  declare external void @f0();
   60:6|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
   68:0|    3: <5, 0>                |    count 0;
   70:4|  0: <65534>                 |  }
   72:0|0: <65534>                   |}

.. _link_for_exit_block_record_section:

Exit Block Record
-----------------

Block records can be top-level, as well as nested, records. Blocks must begin
with an :ref:`enter<link_for_enter_block_record_section>` record, and end with
an *exit* record.

**Syntax**::

  }

**Record**::

  0: <65534>

**Semantics**:

All exit records are identical, no matter what block they are ending. An exit
record defines the end of the block.

**Examples**::

   16:0|1: <65535, 8, 2>             |module {  // BlockID = 8
   24:0|  3: <1, 1>                  |  version 1;
   26:4|  1: <65535, 0, 2>           |  abbreviations {  // BlockID = 0
   36:0|  0: <65534>                 |  }
   40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
   48:0|    3: <1, 2>                |    count 2;
   50:4|    3: <2>                   |    @t0 = void;
   52:2|    3: <21, 0, 0>            |    @t1 = void ();
   55:4|  0: <65534>                 |  }
   56:0|  3: <8, 1, 0, 1, 0>         |  declare external void @f0();
   60:6|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
   68:0|    3: <5, 0>                |    count 0;
   70:4|  0: <65534>                 |  }
   72:0|0: <65534>                   |}

.. _link_for_abbreviation_record:

Abbreviation Record
-------------------

Abbreviation records define abbreviations. See
:ref:`abbreviations<link_for_abbreviations_section>` for details on how
abbreviations should be written. This section only presents the mechanical
details for converting an abbreviation into a PNaCl record.

**Syntax**::

  A = abbrev <E1, ... , EM>;

**Record**::

  2: <65533, M, EE1, ... , EEM>

**Semantics**:

Defines an abbreviation ``A`` as the sequence of encodings ``E1`` through
``EM``.  If the abbreviation appears within the :ref:`abbreviations
block<link_for_abbreviations_block_section>`, ``A`` must be a global
abbreviation. Otherwise, ``A`` must be a local abbreviation.

Abbreviations within a block (or a section within the abbreviations block), must
be enumerated in order, starting at index ``0``.

Valid encodings ``Ei``, and the corresponding sequence of (unsigned) integers
``EEi``, ( for ``1 <= i <= M``) are defined by the following table:

========= ======= ==================================================
Ei        EEi     Form
========= ======= ==================================================
C         1, C    Literal C in corresponding position in record.
fixed(N)  0, 1, N Encode value as a fixed sequence of N bits.
vbr(N)    0, 2, N Encode value using a variable bit rate of N.
char6     0, 4    Encode value as 6-bit char containing
                  characters [a-zA-Z0-9._].
array     0, 3    Allow zero or more of the succeeding abbreviation.
========= ======= ==================================================

Note that 'array' can only appear as the second to last element in the
abbreviation.  Notationally, ``array(EM)`` is used in place of ``array`` and
``EM``, the last two entries in an abbreviation.

**Examples**::

    0:0|<65532, 80, 69, 88, 69, 1, 0,|Magic Number: 'PEXE' (80, 69, 88, 69)
       | 8, 0, 17, 0, 4, 0, 2, 0, 0, |PNaCl Version: 2
       | 0>                          |
   16:0|1: <65535, 8, 2>             |module {  // BlockID = 8
   24:0|  3: <1, 1>                  |  version 1;
   26:4|  1: <65535, 0, 2>           |  abbreviations {  // BlockID = 0
   36:0|    1: <1, 14>               |    valuesymtab:
   38:4|    2: <65533, 4, 0, 1, 3, 0,|      @a0 = abbrev <fixed(3), vbr(8), 
       |        2, 8, 0, 3, 0, 1, 8> |                   array(fixed(8))>;
   43:2|    2: <65533, 4, 1, 1, 0, 2,|      @a1 = abbrev <1, vbr(8), 
       |        8, 0, 3, 0, 1, 7>    |                   array(fixed(7))>;
   48:0|    2: <65533, 4, 1, 1, 0, 2,|      @a2 = abbrev <1, vbr(8), 
       |        8, 0, 3, 0, 4>       |                   array(char6)>;
   52:1|    2: <65533, 4, 1, 2, 0, 2,|      @a3 = abbrev <2, vbr(8), 
       |        8, 0, 3, 0, 4>       |                   array(char6)>;
   56:2|    1: <1, 11>               |    constants:
   58:6|    2: <65533, 2, 1, 1, 0, 1,|      @a0 = abbrev <1, fixed(2)>;
       |        2>                   |
   61:7|    2: <65533, 2, 1, 4, 0, 2,|      @a1 = abbrev <4, vbr(8)>;
       |        8>                   |
   65:0|    2: <65533, 2, 1, 4, 1, 0>|      @a2 = abbrev <4, 0>;
   68:1|    2: <65533, 2, 1, 6, 0, 2,|      @a3 = abbrev <6, vbr(8)>;
       |        8>                   |
   71:2|    1: <1, 12>               |    function:
   73:6|    2: <65533, 4, 1, 20, 0,  |      @a0 = abbrev <20, vbr(6), vbr(4),
       |        2, 6, 0, 2, 4, 0, 2, |                   vbr(4)>;
       |        4>                   |
   79:1|    2: <65533, 4, 1, 2, 0, 2,|      @a1 = abbrev <2, vbr(6), vbr(6), 
       |        6, 0, 2, 6, 0, 1, 4> |                   fixed(4)>;
   84:4|    2: <65533, 4, 1, 3, 0, 2,|      @a2 = abbrev <3, vbr(6), 
       |        6, 0, 1, 2, 0, 1, 4> |                   fixed(2), fixed(4)>;
   89:7|    2: <65533, 1, 1, 10>     |      @a3 = abbrev <10>;
   91:7|    2: <65533, 2, 1, 10, 0,  |      @a4 = abbrev <10, vbr(6)>;
       |        2, 6>                |
   95:0|    2: <65533, 1, 1, 15>     |      @a5 = abbrev <15>;
   97:0|    2: <65533, 3, 1, 43, 0,  |      @a6 = abbrev <43, vbr(6), 
       |        2, 6, 0, 1, 2>       |                   fixed(2)>;
  101:2|    2: <65533, 4, 1, 24, 0,  |      @a7 = abbrev <24, vbr(6), vbr(6),
       |        2, 6, 0, 2, 6, 0, 2, |                   vbr(4)>;
       |        4>                   |
  106:5|    1: <1, 19>               |    globals:
  109:1|    2: <65533, 3, 1, 0, 0, 2,|      @a0 = abbrev <0, vbr(6), 
       |        6, 0, 1, 1>          |                   fixed(1)>;
  113:3|    2: <65533, 2, 1, 1, 0, 2,|      @a1 = abbrev <1, vbr(8)>;
       |        8>                   |
  116:4|    2: <65533, 2, 1, 2, 0, 2,|      @a2 = abbrev <2, vbr(8)>;
       |        8>                   |
  119:5|    2: <65533, 3, 1, 3, 0, 3,|      @a3 = abbrev <3, array(fixed(8))>
       |        0, 1, 8>             |          ;
  123:2|    2: <65533, 2, 1, 4, 0, 2,|      @a4 = abbrev <4, vbr(6)>;
       |        6>                   |
  126:3|    2: <65533, 3, 1, 4, 0, 2,|      @a5 = abbrev <4, vbr(6), vbr(6)>;
       |        6, 0, 2, 6>          |
  130:5|  0: <65534>                 |  }
  132:0|  1: <65535, 17, 3>          |  types {  // BlockID = 17
  140:0|    2: <65533, 4, 1, 21, 0,  |    %a0 = abbrev <21, fixed(1), 
       |        1, 1, 0, 3, 0, 1, 2> |                  array(fixed(2))>;
  144:7|    3: <1, 3>                |    count 3;
  147:4|    3: <7, 32>               |    @t0 = i32;
  150:7|    4: <21, 0, 0, 0, 0>      |    @t1 = i32 (i32, i32); <%a0>
  152:7|    3: <2>                   |    @t2 = void;
  154:6|  0: <65534>                 |  }
  156:0|  3: <8, 1, 0, 0, 0>         |  define external i32 @f0(i32, i32);
  160:6|  1: <65535, 19, 4>          |  globals {  // BlockID = 19
  168:0|    3: <5, 0>                |    count 0;
  170:6|  0: <65534>                 |  }
  172:0|  1: <65535, 14, 3>          |  valuesymtab {  // BlockID = 14
  180:0|    6: <1, 0, 102>           |    @f0 : "f"; <@a2>
  182:7|  0: <65534>                 |  }
  184:0|  1: <65535, 12, 4>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  192:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  194:6|    5: <2, 2, 1, 0>          |    %v0 = add i32 %p0, %p1; <@a1>
  197:2|    5: <2, 3, 1, 0>          |    %v1 = add i32 %p0, %v0; <@a1>
  199:6|    8: <10, 1>               |    ret i32 %v1; <@a4>
  201:0|  0: <65534>                 |  }
  204:0|0: <65534>                   |}

Note that the example above shows the standard abbreviations used by
*pnacl-finalize*.

.. _link_for_types_block_section:

Types Block
===========

The types block defines all types used in a program. It must appear in the
:ref:`module block<link_for_module_block>`, before any :ref:`function
address<link_for_function_address_section>` records, the :ref:`globals
block<link_for_globals_block_section>`, the :ref:`valuesymtab
block<link_for_valuesymtab_block_section>`, and any :ref:`function
blocks<link_for_function_blocks_section>`.

All types used in a program must be defined in the types block. Many PNaClAsm
constructs allow one to use explicit type names, rather than the type
identifiers defined by this block. However, they are internally converted to the
corresponding type identifier in the types block. Hence, the requirement that
the types block must appear early in the module block.

Each record in the types block defines a type used by the program.  Types can be
broken into the following groups:

Primitive value types
  Defines the set of base types for values. This includes various sizes of
  integer and floating point types.

Void type
  A primitive type that doesn't represent any value and has no size.

Function types
  The type signatures of functions.

Vector type
  Defines vectors of primitive types.

In addition, any type that is not defined using another type is a primitive
type.  All other types (i.e. function and vector) are composite types.

Types must be defined in a topological order, causing primitive types to appear
before the composite types that use them. Each type must be unique. There are no
additional restrictions on the order that types can be defined in a types block.

The following subsections introduce each valid PNaClAsm type, and the
corresponding PNaClAsm construct that defines the type. Types not defined in the
types block, can't be used in a PNaCl program.

The first record of a types block must be a :ref:`count
record<link_for_types_count_record>`, defining how many types are defined by the
types block. All remaining records defines a type. The following subsections
defines valid records within a types block. The order of type records is
important. The position of each defining record implicitly defines the type ID
that will be used to denote that type, within other PNaCl records of the bitcode
file.

To make this more concrete, consider the following example types block::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <2>                   |    @t2 = void;
      57:2|    3: <21, 0, 2, 0, 1>      |    @t3 = void (i32, float);
      62:0|  0: <65534>                 |  }

This example defines a types block that defines four type IDs:

@t0
  A 32-bit integer type.
@t1
  A 32-bit floating point type.
@t2
  The void type.
@t3
   A function, taking 32-bit integer and float point arguments that returns
   void.

.. _link_for_types_count_record:

Count Record
------------

The *count record* defines how many types are defined in the types
block. Following the types count record are records that define types used by
the PNaCl program.

**Syntax**::

  count N;                                       <A>

**Record**::

  AA: <1, N>

**Semantics**:

This construct defines the number of types used by the PNaCl program. ``N`` is
the number of types defined in the types block. It is an error to define more
(or fewer) types than value ``N``, within the enclosing types block.

**Constraints**::

  AA == AbbrevIndex(A) &
  0 == NumTypes

**Updates**::

  ExpectedTypes = N;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <2>                   |    @t2 = void;
      57:2|    3: <21, 0, 2, 0, 1>      |    @t3 = void (i32, float);
      62:0|  0: <65534>                 |  }

Void Type
---------

The *void* type record defines the void type, which corresponds to the type that
doesn't define any value, and has no size.

**Syntax**::

  @tN = void;                                     <A>

**Record**::

  AA: <2>

**Semantics**:

The void type record defines the type that has no values and has no size.

**Constraints**::

  AA == AbbrevIndex(A) &
  N == NumTypes

**Updates**::

  ++NumTypes;
  TypeOf(@tN) = void;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <2>                   |    @t2 = void;
      62:0|  0: <65534>                 |  }

Integer Types
-------------

PNaClAsm allows integer types for various bit sizes. Valid bit sizes are 1, 8,
16, 32, and 64. Integers can be signed or unsigned, but the signed component of
an integer is not specified by the type. Rather, individual instructions
determine whether the value is assumed to be signed or unsigned.

It should be noted that in PNaClAsm, all pointers are implemented as 32-bit
(unsigned) integers.  There isn't a separate type for pointers. The only way to
tell that a 32-bit integer is a pointer, is when it is used in an instruction
that requires a pointer (such as load and store instructions).

**Syntax**::

  @tN = iB;                                      <A>

**Record**::

  AA: <7, B>

**Semantics**:

An integer type record defines an integer type. ``B`` defines the number of bits
of the integer type.

**Constraints**::

  AA == AbbrevIndex(A) &
  N == NumTypes &
  B in {1, 8, 16, 32, 64}

**Updates**::

  ++NumTypes;
  TypeOf(@tN) = iB;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 7>                |    count 7;
      50:4|    3: <7, 64>               |    @t0 = i64;
      53:6|    3: <7, 1>                |    @t1 = i1;
      56:2|    3: <7, 8>                |    @t2 = i8;
      58:6|    3: <7, 16>               |    @t3 = i16;
      61:2|    3: <7, 32>               |    @t4 = i32;
      64:4|    3: <21, 0, 0, 1>         |    @t5 = i64 (i1);
      68:4|    3: <2>                   |    @t6 = void;
      70:2|  0: <65534>                 |  }

32-Bit Floating Point Type
--------------------------

PNaClAsm allows computation on 32-bit floating point values. A floating point
type record defines the 32-bit floating point type.

**Syntax**::

  @tN = float;                                    <A>

**Record**::

  AA: <3>

**Semantics**:

A floating point type record defines the 32-bit floating point type.

**Constraints**::

  AA == AbbrevIndex(A) &
  N == NumTypes

**Updates**::

  ++NumTypes;
  TypeOf(@tN) = float;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <4>                   |    @t0 = double;
      52:2|    3: <3>                   |    @t1 = float;
      54:0|    3: <21, 0, 0, 1>         |    @t2 = double (float);
      58:0|    3: <2>                   |    @t3 = void;
      59:6|  0: <65534>                 |  }

64-bit Floating Point Type
--------------------------

PNaClAsm allows computation on 64-bit floating point values. A 64-bit floating
type record defines the 64-bit floating point type.

**Syntax**::

  @tN = double;                                   <A>

**Record**::

  AA: <4>

**Semantics**:

A double type record defines the 64-bit floating point type.

**Constraints**::

  AA == AbbrevIndex(A) &
  N == NumTypes

**Updates**::

  ++NumTypes;
  TypeOf(@tN) = double;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <4>                   |    @t0 = double;
      52:2|    3: <3>                   |    @t1 = float;
      54:0|    3: <21, 0, 0, 1>         |    @t2 = double (float);
      58:0|    3: <2>                   |    @t3 = void;
      59:6|  0: <65534>                 |  }

Vector Types
------------

A vector type is a derived type that represents a vector of elements.  Vector
types are used when multiple primitive data values are operated in parallel
using a single (SIMD) :ref:`vector instruction<link_for_vector_instructions>`. A
vector type requires a size (number of elements) and an underlying primitive
data type.

**Syntax**::

  @tN = < E x T >                                 <A>

**Record**::

  AA: <12, E, TT>

**Semantics**:

The vector type defines a vector of elements. ``T`` is the type of each
element. ``E`` is the number of elements in the vector.

Vector types can only be defined on ``i1``, ``i8``, ``i16``, ``i32``, and
``float``.  All vector types, except those on ``i1``, must contain exactly 128
bits.  The valid element sizes are restricted as follows:

====== ===================
Type   Valid element sizes
====== ===================
i1     4, 8, 16
i8     16
i16    8
i32    4
float  4
====== ===================

**Constraints**::

  AA == AbbrevIndex(A) &
  TT == AbsoluteIndex(TypeID(T)) &
  N == NumTypes

**Updates**::

  ++NumTypes
  TypeOf(@tN) = <E x T>

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 14>               |    count 14;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <7, 1>                |    @t1 = i1;
      56:2|    3: <2>                   |    @t2 = void;
      58:0|    3: <12, 4, 1>            |    @t3 = <4 x i1>;
      61:2|    3: <12, 8, 1>            |    @t4 = <8 x i1>;
      64:4|    3: <12, 16, 1>           |    @t5 = <16 x i1>;
      67:6|    3: <7, 8>                |    @t6 = i8;
      70:2|    3: <12, 16, 6>           |    @t7 = <16 x i8>;
      73:4|    3: <7, 16>               |    @t8 = i16;
      76:0|    3: <12, 8, 8>            |    @t9 = <8 x i16>;
      79:2|    3: <12, 4, 0>            |    @t10 = <4 x i32>;
      82:4|    3: <3>                   |    @t11 = float;
      84:2|    3: <12, 4, 11>           |    @t12 = <4 x float>;
      87:4|    3: <21, 0, 2>            |    @t13 = void ();
      90:6|  0: <65534>                 |  }

.. _link_for_function_type:

Function Type
-------------

The *function* type can be thought of as a function signature. It consists of a
return type, and a (possibly empty) list of formal parameter types.

**Syntax**::

  %tN = RT (T1, ... , TM)                         <A>

**Record**::

  AA: <21, 0, IRT, IT1, ... , ITM>

**Semantics**:

The function type defines the signature of a function. ``RT`` is the return type
of the function, while types ``T1`` through ``TM`` are the types of the
arguments. Indices to the corresponding type identifiers are stored in the
corresponding record.

The return value must either be a primitive type, type ``void``, or a vector
type.  Parameter types can be a primitive or vector type.

For ordinary functions, the only valid integer types that can be used for a
return or parameter type are ``i32`` and ``i64``.  All other integer types are
not allowed.

For :ref:`intrinsic functions<link_for_intrinsic_functions_section>`, all
integer types are allowed for both return and parameter types.

**Constraints**::

  AA == AbbrevIndex(A) &
  M >= 0 &
  IRT == AbsoluteIndex(TypeID(RT)) &
  IT1 == AbsoluteIndex(TypeID(T1)) &
  ...
  ITM == AbsoluteIndex(TypeID(TM)) &
  N == NumTypes

**Updates**::

  ++NumTypes
  TypeOf(@tN) = RT (T1, ... , TM)

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 7>                |    count 7;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <4>                   |    @t2 = double;
      57:2|    3: <21, 0, 2, 1>         |    @t3 = double (float);
      61:2|    3: <2>                   |    @t4 = void;
      63:0|    3: <21, 0, 4>            |    @t5 = void ();
      66:2|    3: <21, 0, 0, 0, 1, 0, 2>|    @t6 = 
          |                             |        i32 (i32, float, i32, double);
      72:4|  0: <65534>                 |  }

.. _link_for_globals_block_section:

Globals Block
=============

The globals block defines global addresses of variables and constants, used by
the PNaCl program. It also defines the memory associated with the global
addresses, and how to initialize each global variable/constant. It must appear
in the :ref:`module block<link_for_module_block>`. It must appear after the
:ref:`types block<link_for_types_block_section>`, as well as after all
:ref:`function address<link_for_function_address_section>` records. But, it must
also appear before the :ref:`valuesymtab
block<link_for_valuesymtab_block_section>`, and any
:ref:`function blocks<link_for_function_blocks_section>`.

The globals block begins with a :ref:`count
record<link_for_globals_count_record>`, defining how many global addresses are
defined by the PNaCl program. It is then followed by a sequence of records that
defines each global address, and how each global address is initialized.

The standard sequence, for defining global addresses, begins with a global
address record.  It is then followed by a sequence of records defining how the
global address is initialized.  If the initializer is simple, a single record is
used. Otherwise, the initializer is preceded with a :ref:`compound
record<link_for_compound_initializer>`, specifying a number *N*, followed by
sequence of *N* simple initializer records.

The size of the memory referenced by each global address is defined by its
initializer records. All simple initializer records define a sequence of
bytes. A compound initializer defines the sequence of bytes by concatenating the
corresponding sequence of bytes for each of its simple initializer records.

For notational convenience, PNaClAsm begins a compound record with a "{", and
inserts a "}" after the last initializer record associated with the compound
record. This latter "}" does not correspond to any record. It is implicitly
assumed by the size specified in the compound record, and is added only to
improve readability.

Explicit alignment is specified for global addresses, and must be a power of
2. See :ref:`memory blocks and
alignment<link_for_memory_blocks_and_alignment_section>` for a more detailed
discussion on how to define alignment.

For example, consider the following pnacl-bcdis output snippet::

      52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
      60:0|    3: <5, 2>                |    count 2;
      62:4|    3: <0, 1, 1>             |    const @g0, align 1,
      65:6|    3: <2, 8>                |      zerofill 8;
      68:2|    3: <0, 1, 0>             |    var @g1, align 1,
      71:4|    3: <1, 2>                |      initializers 2 {
      74:0|    3: <3, 1, 2, 3, 4>       |        {  1,   2,   3,   4}
      78:6|    3: <2, 2>                |        zerofill 2;
          |                             |      }
      81:2|  0: <65534>                 |  }

This snippet defines the global constant ``@g0``, and the global variable
``@g1``. ``@g0`` is 8 bytes long, and initialized to zero. ``@g1`` is
initialized with 6 bytes: ``1 2 3 4 0 0``.

.. _link_for_globals_count_record:

Count Record
------------

The count record defines the number of global addresses used by the PNaCl
program.

**Syntax**::

  count N;                                       <A>

**Record**::

   AA: <5, N>

**Semantics**:

This record must appear first in the globals block.  The count record defines
the number of global addresses used by the program.

**Constraints**::

  AA == AbbrevIndex(A)

**Updates**::

  ExpectedGlobals = N;
  ExpectedInitializers = 0;

**Examples**::

  52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
  60:0|    3: <5, 2>                |    count 2;
  62:4|    3: <0, 1, 1>             |    const @g0, align 1,
  65:6|    3: <2, 8>                |      zerofill 8;
  68:2|    3: <0, 1, 0>             |    var @g1, align 1,
  71:4|    3: <1, 2>                |      initializers 2 {
  74:0|    3: <3, 1, 2, 3, 4>       |        {  1,   2,   3,   4}
  78:6|    3: <2, 2>                |        zerofill 2;
      |                             |      }
  81:2|  0: <65534>                 |  }

.. _link_for_global_variable_address:

Global Variable Addresses
-------------------------

A global variable address record defines a global address to global data.  The
global variable address record must be immediately followed by initializer
record(s) that define how the corresponding global variable is initialized.

**Syntax**::

  var @gN, align V,                               <A>

**Record**::

  AA: <0, VV, 0>

**Semantics**:

A global variable address record defines a global address for a global variable.
``V`` is the :ref:`memory
alignment<link_for_memory_blocks_and_alignment_section>` for the global variable
address, and is a power of 2.

It is assumed that the memory, referenced by the global variable address, can be
both read and written to.

**Constraints**::

  AA == AbbrevIndex(A) &
  N == NumGlobalAddresses &
  ExpectedInitializers == 0 &
  VV == Log2(V+1)

**Updates**::

  ++NumGlobalAddresses;
  ExpectedInitializers = 1;
  TypeOf(@gN) = i32;

**Examples**::

      52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
      60:0|    3: <5, 2>                |    count 2;
      62:4|    3: <0, 3, 0>             |    var @g0, align 4,
      65:6|    3: <2, 8>                |      zerofill 8;
      68:2|    3: <0, 1, 0>             |    var @g1, align 1,
      71:4|    3: <3, 1, 2, 3, 4>       |      {  1,   2,   3,   4}
      76:2|  0: <65534>                 |  }
      80:0|0: <65534>                   |}

.. _link_for_global_constant_address:

Global Constant Addresses
-------------------------

A global constant address record defines an address corresponding to a global
constant that can't be modified by the program. The global constant address
record must be immediately followed by initializer record(s) that define how
the corresponding global constant is initialized.

**Syntax**::

  const @gN, align V,                             <A>

**Record**::

  AA: <0, VV, 1>

**Semantics**:

A global constant address record defines a global address for a global constant.
``V`` is the :ref:`memory
alignment<link_for_memory_blocks_and_alignment_section>` for the global constant
address, and is a power of 2.

It is assumed that the memory, referenced by the global constant address, is
only read, and can't be written to.

Note that the only difference between a global variable address and a global
constant address record is the third element of the record. If the value is
zero, it defines a global variable address. If the value is one, it defines a
global constant address.

**Constraints**::

  AA == AbbrevIndex(A) &
  N == NumGlobalAddresses &
  ExpectedInitializers == 0 &
  VV == Log2(V+1)

**Updates**::

  ++NumGlobalAddresses;
  ExpectedInitializers = 1;
  TypeOf(@gN) = i32;

**Examples**::

      52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
      60:0|    3: <5, 2>                |    count 2;
      62:4|    3: <0, 3, 1>             |    const @g0, align 4,
      65:6|    3: <2, 8>                |      zerofill 8;
      68:2|    3: <0, 1, 1>             |    const @g1, align 1,
      71:4|    3: <3, 1, 2, 3, 4>       |      {  1,   2,   3,   4}
      76:2|  0: <65534>                 |  }

Zerofill Initializer
--------------------

The zerofill initializer record initializes a sequence of bytes, associated with
a global address, with zeros.

**Syntax**::

  zerofill N;                                     <A>

**Record**::

  AA: <2, N>

**Semantics**:

A zerofill initializer record initializes a sequence of bytes, associated with a
global address, with zeros. The number of bytes initialized to zero is ``N``.

**Constraints**::

  AA == AbbrevIndex(A) &
  ExpectedInitializers > 0

**Updates**::

  --ExpectedInitializers;

**Examples**::

      52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
      60:0|    3: <5, 2>                |    count 2;
      62:4|    3: <0, 3, 1>             |    const @g0, align 4,
      65:6|    3: <2, 8>                |      zerofill 8;
      68:2|    3: <0, 1, 0>             |    var @g1, align 1,
      71:4|    3: <2, 4>                |      zerofill 4;
      74:0|  0: <65534>                 |  }

Data Initializer
----------------

Data records define a sequence of bytes. These bytes define the initial value of
the contents of the corresponding memory.

**Syntax**::

  { B1 , .... , BN }                              <A>

**Record**::

  AA: <3, B1, ..., BN>

**Semantics**:

A data record defines a sequence of (unsigned) bytes ``B1`` through ``BN``, that
initialize ``N`` bytes of memory.

**Constraints**::

  AA == AbbrevIndex(A) &
  ExpectedInitializers > 0

**Updates**::

  --ExpectedInitializers;

**Examples**::

   56:0|  3: <8, 1, 0, 1, 0>         |  declare external void @f0();
   60:6|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
   68:0|    3: <5, 2>                |    count 2;
   70:4|    3: <0, 1, 1>             |    const @g0, align 1,
   73:6|    3: <3, 1, 2, 97, 36, 44, |      {  1,   2,  97,  36,  44,  88, 
       |        88, 44, 50>          |        44,  50}
   86:0|    3: <0, 1, 1>             |    const @g1, align 1,
   89:2|    3: <1, 3>                |      initializers 3 {
   91:6|    3: <3, 1, 2, 3, 4>       |        {  1,   2,   3,   4}
   96:4|    3: <4, 0>                |        reloc @f0;
   99:0|    3: <3, 99, 66, 22, 12>   |        { 99,  66,  22,  12}
       |                             |      }
  105:2|  0: <65534>                 |  }

Relocation Initializer
----------------------

A relocation initializer record allows one to define the initial value of a
global address with the value of another global address (i.e. either
:ref:`function<link_for_function_address_section>`,
:ref:`variable<link_for_global_variable_address>`, or
:ref:`constant<link_for_global_constant_address>`). Since addresses are
pointers, a relocation initializer record defines 4 bytes of memory.

**Syntax**::

  reloc V;                                        <A>

**Record**::

  AA: <4, VV>

**Semantics**:

A relocation initializer record defines a 4-byte value containing the specified
global address ``V``.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV == AbsoluteIndex(V) &
  VV >= NumFuncAddresses &
  VV < NumFuncAddresses + ExpectedGlobals &
  ExpectedInitializers > 0

**Updates**::

  --ExpectedInitializers;

**Examples**::

  40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
  48:0|    3: <1, 2>                |    count 2;
  50:4|    3: <2>                   |    @t0 = void;
  52:2|    3: <21, 0, 0>            |    @t1 = void ();
  55:4|  0: <65534>                 |  }
  56:0|  3: <8, 1, 0, 1, 0>         |  declare external void @f0();
  60:6|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
  68:0|    3: <5, 2>                |    count 2;
  70:4|    3: <0, 1, 0>             |    var @g0, align 1,
  73:6|    3: <1, 3>                |      initializers 3 {
  76:2|    3: <4, 0>                |        reloc @f0;
  78:6|    3: <4, 1>                |        reloc @g0;
  81:2|    3: <4, 2>                |        reloc @g1;
      |                             |      }
  83:6|    3: <0, 3, 0>             |    var @g1, align 4,
  87:0|    3: <2, 4>                |      zerofill 4;
  89:4|  0: <65534>                 |  }

This example defines global address ``@g0`` and ``@g1``. ``@g0`` defines 12
bytes of memory, and is initialized with three addresses ``@f1``, ``@g0``, and
``@g1``. Note that all global addresses can be used in a relocation
initialization record, even if it isn't defined yet.

Subfield Relocation Initializer
-------------------------------

A subfield relocation initializer record allows one to define the initial value
of a global address with the value of another (non-function) global address
(i.e. either :ref:`variable<link_for_global_variable_address>` or
:ref:`constant<link_for_global_constant_address>` address), plus a
constant. Since addresses are pointers, a relocation initializer record defines
4 bytes of memory.

**Syntax**::

  reloc V + X;                                    <A>
  reloc V - X;                                    <A>

**Record**::

  AA: <4, VV, XXX>

**Semantics**:

A subfield relocation initializer record defines a 4-byte value containing the
specified global (non-function) address ``V``, modified by the unsigned offset
``X``. ``XX`` is the corresponding signed offset. In the first form, ``XX ==
X``. In the second form, ``XX == -X``.

**Constraints**::

  AA == AbbrevIndex(A)
  VV == AbsoluteIndex(V)
  VV >= NumFuncAddresses
  VV < NumFuncAddresses + ExpectedGlobals
  ExpectedInitializers > 0
  XXX == SignRotate(XX)

**Updates**::

  --ExpectedInitializers;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 0>                |    count 0;
      50:4|  0: <65534>                 |  }
      52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
      60:0|    3: <5, 3>                |    count 3;
      62:4|    3: <0, 1, 0>             |    var @g0, align 1,
      65:6|    3: <1, 3>                |      initializers 3 {
      68:2|    3: <4, 0, 1>             |        reloc @g0 + 1;
      71:4|    3: <4, 1, 4294967295>    |        reloc @g1 - 1;
      79:2|    3: <4, 2, 4>             |        reloc @g2 + 4;
          |                             |      }
      82:4|    3: <0, 3, 0>             |    var @g1, align 4,
      85:6|    3: <2, 4>                |      zerofill 4;
      88:2|    3: <0, 3, 0>             |    var @g2, align 4,
      91:4|    3: <2, 8>                |      zerofill 8;
      94:0|  0: <65534>                 |  }

.. _link_for_compound_initializer:

Compound Initializer
--------------------

The compound initializer record must immediately follow a global
:ref:`variable<link_for_global_variable_address>` or
:ref:`constant<link_for_global_constant_address>` address record. It defines how
many simple initializer records are used to define the initializer. The size of
the corresponding memory is the sum of the bytes needed for each of the
succeeding initializers.

Note that a compound initializer can't be used as a simple initializer of
another compound initializer (i.e. nested compound initializers are not
allowed).

**Syntax**::

  initializers N {                                <A>
   ...
  }

**Record**::

  AA: <1, N>

**Semantics**:

Defines that the next `N` initializers should be associated with the global
address of the previous record.

**Constraints**::

  AA == AbbrevIndex(A) &
  ExpectedInitializers == 1

**Updates**::

  ExpectedInitializers = N;

**Examples**::

  40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
  48:0|    3: <1, 0>                |    count 0;
  50:4|  0: <65534>                 |  }
  52:0|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
  60:0|    3: <5, 2>                |    count 2;
  62:4|    3: <0, 0, 1>             |    const @g0, align 0,
  65:6|    3: <1, 2>                |      initializers 2 {
  68:2|    3: <2, 8>                |        zerofill 8;
  70:6|    3: <3, 3, 2, 1, 0>       |        {  3,   2,   1,   0}
      |                             |      }
  75:4|    3: <0, 0, 0>             |    var @g1, align 0,
  78:6|    3: <1, 2>                |      initializers 2 {
  81:2|    3: <3, 1, 2, 3, 4>       |        {  1,   2,   3,   4}
  86:0|    3: <2, 2>                |        zerofill 2;
      |                             |      }
  88:4|  0: <65534>                 |  }

.. _link_for_valuesymtab_block_section:

Valuesymtab Block
=================

The valuesymtab block does not define any values.  Its only goal is to associate
text names with external :ref:`function
addresses<link_for_function_address_section>`.  Each association is defined by a
record in the valuesymtab block.  Currently, only
:ref:`intrinsic<link_for_intrinsic_functions_section>` function addresses and
the (external) start function (``_start``) can be named.  All named function
addresses must be external.  Each record in the valuesymtab block is a *entry*
record, defining a single name association.

Entry Record
------------

The *entry* record defines a name for a function address.

**Syntax**::

  V : "NAME";                                     <A>

**Record**::

  AA: <1, B1, ... , BN>

**Semantics**:

The *entry* record defines a name ``NAME`` for function address ``V``. ``NAME``
is a sequence of ASCII characters ``B1`` through ``BN``.

**Examples**::

      72:0|  3: <8, 4, 0, 1, 0>         |  declare external 
          |                             |      void @f0(i32, i32, i32, i32, i1);
      76:6|  3: <8, 4, 0, 1, 0>         |  declare external 
          |                             |      void @f1(i32, i32, i32, i32, i1);
      81:4|  3: <8, 5, 0, 0, 0>         |  define external void @f2(i32);
      86:2|  1: <65535, 19, 2>          |  globals {  // BlockID = 19
      92:0|    3: <5, 0>                |    count 0;
      94:4|  0: <65534>                 |  }
      96:0|  1: <65535, 14, 2>          |  valuesymtab {  // BlockID = 14
     104:0|    3: <1, 1, 108, 108, 118, |    @f1 : "llvm.memmove.p0i8.p0i8.i32";
          |        109, 46, 109, 101,   |
          |        109, 109, 111, 118,  |
          |        101, 46, 112, 48,    |
          |        105, 56, 46, 112, 48,|
          |        105, 56, 46, 105, 51,|
          |        50>                  |
     145:4|    3: <1, 2, 95, 115, 116,  |    @f2 : "_start";
          |        97, 114, 116>        |
     157:0|    3: <1, 0, 108, 108, 118, |    @f0 : "llvm.memcpy.p0i8.p0i8.i32";
          |        109, 46, 109, 101,   |
          |        109, 99, 112, 121,   |
          |        46, 112, 48, 105, 56,|
          |        46, 112, 48, 105, 56,|
          |        46, 105, 51, 50>     |
     197:0|  0: <65534>                 |  }

.. _link_for_module_block:

Module Block
============

The module block, like all blocks, is enclosed in a pair of
:ref:`enter<link_for_enter_block_record_section>` /
:ref:`exit<link_for_exit_block_record_section>` records, using block ID 8. A
well-formed module block consists of the following records (in order):

A version record
   The :ref:`version record<link_for_version_record>` communicates which version
   of the PNaCl bitcode reader/writer should be used. Note that this is
   different than the PNaCl bitcode (ABI) version. The PNaCl bitcode (ABI)
   version defines what is expected in records, and is defined in the header
   record of the bitcode file. The version record defines the version of the
   PNaCl bitcode reader/writer to use to convert records into bit sequences.

Optional local abbreviations
   Defines a list of local :ref:`abbreviations<link_for_abbreviations_section>`
   to use for records within the module block.

An abbreviations block
   The :ref:`abbreviations block<link_for_abbreviations_block_section>` defines
   user-defined, global abbreviations that are used to convert PNaCl records to
   bit sequences in blocks following the abbreviations block.

A types block
   The :ref:`types block<link_for_types_block_section>` defines the set of all
   types used in the program.

A non-empty sequence of function address records
   Each record defines a :ref:`function
   address<link_for_function_address_section>` used by the program. Function
   addresses must either be external, or defined internally by the program. If
   they are defined by the program, there must be a :ref:`function
   block<link_for_function_blocks_section>` (appearing later in the module) that
   defines the sequence of instructions for each defined function.

A globals block defining the global variables.
   This :ref:`block<link_for_globals_block_section>` defines the set of
   global :ref:`variable<link_for_global_variable_address>` and
   :ref:`constant<link_for_global_constant_address>` addresses used by the
   program. In addition to the addresses, each global variable also defines how
   the corresponding global variable is initialized.

An optional value symbol table block.
   This :ref:`block<link_for_valuesymtab_block_section>`, if defined, provides
   textual names for :ref:`function
   addresses<link_for_function_address_section>` (previously defined in the
   module). Note that only names for intrinsic functions and the start function
   are specified.

A sequence of function blocks.
   Each :ref:`function block<link_for_Function_blocks_section>` defines the
   corresponding intermediate representation for each defined function. The
   order of function blocks is used to associate them with :ref:`function
   addresses<link_for_function_address_section>`.  The order of the defined
   function blocks must follow the same order as the corresponding function
   addresses defined in the module block.

Descriptions of the :ref:`abbreviations<link_for_abbreviations_section>`,
:ref:`types<link_for_types_block_section>`,
:ref:`globals<link_for_globals_block_section>`, :ref:`value symbol
table<link_for_valuesymtab_block_section>`, and
:ref:`function<link_for_function_blocks_section>` blocks are not provided
here. See the appropriate reference for more details. The following subsections
describe each of the records that can appear in a module block.

.. _link_for_version_record:

Version Record
--------------

The version record defines the implementation of the PNaCl bitstream
reader/writer to use. That is, the implementation that converts PNaCl records to
bit sequences, and converts them back to PNaCl records. Note that this is
different than the PNaCl version of the bitcode file (encoded in the header
record of the bitcode file).  The PNaCl version defines the valid forms of PNaCl
records. The version record is specific to the PNaCl version, and may have
different values for different PNaCl versions.

Note that currently, only PNaCl bitcode version 2, and version record value 1 is
defined.

**Syntax**::

  version N;                                  <A>

**Record**::

  AA: <1, N>

**Semantics**:

The version record defines which PNaCl reader/writer rules should be
followed. ``N`` is the version number. Currently ``N`` must be 1. Future
versions of PNaCl may define additional legal values.

**Constraints**::

  AA == AbbrevIndex(A)

*Examples*::

      16:0|1: <65535, 8, 2>             |module {  // BlockID = 8
      24:0|  3: <1, 1>                  |  version 1;
      26:4|  1: <65535, 0, 2>           |  abbreviations {  // BlockID = 0
      36:0|  0: <65534>                 |  }

.. _link_for_function_address_section:

Function Address
----------------

A function address record describes a function address. *Defined* function
addresses define :ref:`implementations<link_for_function_blocks_section>` while
*declared* function addresses do not.

Since a PNaCl program is assumed to be a complete (statically linked)
executable, All functions should be *defined* and *internal*.  The exception to
this are :ref:`intrinsic functions<link_for_intrinsic_functions_section>`, which
should only be *declared* and *external*, since intrinsic functions will be
automatically converted to appropriate code by the :ref:`PNaCl
translator<link_for_pnacl_translator>`.

The implementation of a *defined* function address is provided by a
corresponding function block, appearing later in the module block. The
association of a *defined* function address with the corresponding function
block is based on position.  The *Nth* defined function address record, in the
module block, has its implementation in the *Nth* function block of that module
block.

**Syntax**::

  PN LN T0 @fN ( T1 , ... , TM );                 <A>

**Record**::

  AA: <8, T, C, P, L>

**Semantics**:

Describes the function address ``@fN``. ``PN`` is the name that specifies the
prototype value ``P`` associated with the function. A function address is
*defined* only if ``P == 0``. Otherwise, it is only *declared*.  The type of the
function is :ref:`function type<link_for_function_type>` ``@tT``. ``L`` is the
linkage specification corresponding to name ``LN``. ``C`` is the calling
convention used by the function.

Note that function signature must be defined by a function type in the types
block. Hence, the return value must either be a primitive type, type ``void``,
or a vector type.

For ordinary functions, integer parameter and types can only be ``i32`` and
``i64``.  All other integer types are not allowed. For intrinsic functions, all
integer types are allowed.

Valid prototype names ``PN``, and corresponding ``P`` values, are:

= =======
P PN
= =======
1 declare
0 define
= =======

Valid linkage names ``LN``, and corresponding ``L`` values, are:

= ========
L LN
= ========
3 internal
0 external
= ========

Currently, only one calling convention ``C`` is supported:

= ====================
C Calling Convention
= ====================
0 C calling convention
= ====================

**Constraints**::

  AA = AbbrevIndex(A) &
  T = TypeID(TypeOf(T0 ( T1 , ... , TN ))) &
  N = NumFuncAddresses

**Updates**::

  ++NumFuncAddresses;
  TypeOf(@fN) = TypeOf(TypeID(i32));
  TypeOfFcn(@fN) = TypeOf(@tT);

  if PN == 0:
    DefiningFcnIDs += @FN;
    ++NumDefinedFunctionAddresses;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 7>                |    count 7;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <4>                   |    @t2 = double;
      57:2|    3: <2>                   |    @t3 = void;
      59:0|    3: <21, 0, 2, 1>         |    @t4 = double (float);
      63:0|    3: <21, 0, 0, 0, 1, 0, 2>|    @t5 = 
          |                             |        i32 (i32, float, i32, double);
      69:2|    3: <21, 0, 3>            |    @t6 = void ();
      72:4|  0: <65534>                 |  }
      76:0|  3: <8, 4, 0, 1, 0>         |  declare external double @f0(float);
      80:6|  3: <8, 5, 0, 1, 0>         |  declare external 
          |                             |      i32 @f1(i32, float, i32, double);
      85:4|  3: <8, 6, 0, 0, 0>         |  define external void @f2();

.. _link_for_constants_block_section:

Constants Blocks
================

Constants blocks define literal constants used within each function. Its intent
is to define them once, before instructions. A constants block can only appear
in a :ref:`function block<link_for_function_blocks_section>`, and must appear
before any instructions in the function block.

Currently, only integer literals, floating point literals, and undefined vector
constants can be defined.

To minimize type information put in a constants block, the type information is
separated from the constants. This allows a sequence of constants to be given
the same type. This is done by defining a :ref:`set type
record<link_for_constants_set_type_record>`, followed by a sequence of literal
constants. These literal constants all get converted to the type of the
preceding set type record.

Note that constants that are used for switch case selectors should not be added
to the constants block, since the switch instruction contains the constants used
for case selectors. All other constants in the function block must be put into a
constants block, so that instructions can use them.

To make this more concrete, consider the following example constants block::

     106:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     116:0|      3: <1, 0>              |      i32:
     118:4|      3: <4, 2>              |        %c0 = i32 1;
     121:0|      3: <4, 4>              |        %c1 = i32 2;
     123:4|      3: <1, 2>              |      i8:
     126:0|      3: <4, 8>              |        %c2 = i8 4;
     128:4|      3: <4, 6>              |        %c3 = i8 3;
     131:0|      3: <1, 1>              |      float:
     133:4|      3: <6, 1065353216>     |        %c4 = float 1;
     139:6|    0: <65534>               |      }

.. _link_for_constants_set_type_record:

Set Type Record
---------------

The *set type* record defines the type to use for the (immediately) succeeding
literals.

**Syntax**::

  T:                                              <A>

**Record**::

  AA: <1, TT>

**Semantics**:

The *set type* record defines type ``T`` to be used to type the (immediately)
succeeding literals. ``T`` must be a non-void primitive value type or a vector
type.

**Constraints**::

  TT == TypeID(T)

**Updates**::

  ConstantsSetType = T;

**Examples**::

     106:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     116:0|      3: <1, 0>              |      i32:
     118:4|      3: <4, 2>              |        %c0 = i32 1;
     121:0|      3: <4, 4>              |        %c1 = i32 2;
     123:4|      3: <1, 2>              |      i8:
     126:0|      3: <4, 8>              |        %c2 = i8 4;
     128:4|      3: <4, 6>              |        %c3 = i8 3;
     131:0|      3: <1, 1>              |      float:
     133:4|      3: <6, 1065353216>     |        %c4 = float 1;
     139:6|    0: <65534>               |      }

.. _link_for_undefined_literal:

Undefined Literal
-----------------

The *undefined* literal record creates an undefined literal for the type *T*
defined by the preceding *set type* record.

Note: See :ref:`insert element
instruction<link_for_insert_element_instruction_section>` for an example of how
you would use the undefined literal with vector types.

**Syntax**::

  %cN = T undef;                              <50>

**Record**::

  AA: <3>

**Semantics**:

The *undefined* literal record creates an undefined literal constant ``%cN`` for
type ``T``.  ``T`` must be the type defined by the preceding *set type* record,
and be a primitive value type or a vector type.

**Constraints**::

   N == NumFcnConsts &
   T == ConstantsSetType &
   IsPrimitive(T) or IsVector(T)

**Updates**::

   ++NumFcnConsts;
   TypeOf(%cN) = T;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 5>                |    count 5;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <2>                   |    @t2 = void;
      57:2|    3: <12, 4, 0>            |    @t3 = <4 x i32>;
      60:4|    3: <21, 0, 2>            |    @t4 = void ();
      63:6|  0: <65534>                 |  }
      ...
     106:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     116:0|      3: <1, 0>              |      i32:
     118:4|      3: <3>                 |        %c0 = i32 undef;
     120:2|      3: <4, 2>              |        %c1 = i32 1;
     122:6|      3: <1, 3>              |      <4 x i32>:
     125:2|      3: <3>                 |        %c2 = <4 x i32> undef;
     127:0|      3: <1, 1>              |      float:
     129:4|      3: <3>                 |        %c3 = float undef;
     131:2|    0: <65534>               |      }

.. _link_for_integer_literal:

Integer Literal
---------------

The *integer literal* record creates an integer literal for the integer type *T*
defined by the preceding *set type* record.

**Syntax**::

  %cN = T V;                                      <A>

**Record**::

  AA: <4, VV>

**Semantics**:

The *integer literal* record creates an integer literal constant ``%cN`` for
type ``T``.  ``T`` must be the type defined by the preceding *set type* record,
and an integer type. The literal ``V`` can be signed, but must be definable by
type ``T``.

**Constraints**::

  N == NumFcnConsts &
  T == ConstantsSetType &
  VV == SignRotate(V) &
  IsInteger(T)

**Updates**::

  TypeOf(%cN) = T;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 7>                |    count 7;
      50:4|    3: <7, 8>                |    @t0 = i8;
      53:0|    3: <7, 16>               |    @t1 = i16;
      55:4|    3: <7, 32>               |    @t2 = i32;
      58:6|    3: <7, 64>               |    @t3 = i64;
      62:0|    3: <7, 1>                |    @t4 = i1;
      64:4|    3: <2>                   |    @t5 = void;
      66:2|    3: <21, 0, 5>            |    @t6 = void ();
      69:4|  0: <65534>                 |  }
      ...
     114:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     124:0|      3: <1, 0>              |      i8:
     126:4|      3: <4, 2>              |        %c0 = i8 1;
     129:0|      3: <4, 4>              |        %c1 = i8 2;
     131:4|      3: <1, 1>              |      i16:
     134:0|      3: <4, 6>              |        %c2 = i16 3;
     136:4|      3: <4, 8>              |        %c3 = i16 4;
     139:0|      3: <1, 2>              |      i32:
     141:4|      3: <4, 10>             |        %c4 = i32 5;
     144:0|      3: <4, 12>             |        %c5 = i32 6;
     146:4|      3: <1, 3>              |      i64:
     149:0|      3: <4, 3>              |        %c6 = i64 -1;
     151:4|      3: <4, 5>              |        %c7 = i64 -2;
     154:0|      3: <1, 4>              |      i1:
     156:4|      3: <4, 3>              |        %c8 = i1 1;
     159:0|      3: <4, 0>              |        %c9 = i1 0;
     161:4|    0: <65534>               |      }

Floating Point Literal
----------------------

The *floating point literal* record creates a floating point literal for the
floating point type *T* defined by the preceding *set type* record.

**Syntax**::

  %cN = T V;                                      <A>

**Record**::

  AA: <6, VV>

**Semantics**:

The *floating point literal* record creates a floating point literal constant
``%cN`` for type ``T``. ``T`` must the type type defined by the preceding *set
type* record, and be a floating point type. The literal ``V`` is the floating
value to be defined. The value ``VV`` if the corresponding IEEE unsigned integer
that defines value ``V``. That is, the literal ``VV`` must be a valid IEEE 754
32-bit (unsigned integer) value if ``T`` is ``float``, and a valid IEEE 754
64-bit (unsigned integer) value if ``T`` is ``double``.

**Constraints**::

  N == NumFcnConsts
  T == ConstantsSetType
  IsFloat(T)

**Updates**::

  TypeOf(%cN) = T;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <3>                   |    @t0 = float;
      52:2|    3: <4>                   |    @t1 = double;
      54:0|    3: <2>                   |    @t2 = void;
      55:6|    3: <21, 0, 2>            |    @t3 = void ();
      59:0|  0: <65534>                 |  }
      ...
     102:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     112:0|      3: <1, 0>              |      float:
     114:4|      3: <6, 0>              |        %c0 = float 0;
     117:0|      3: <6, 1065353216>     |        %c1 = float 1;
     123:2|      3: <6, 1088421888>     |        %c2 = float 7;
     130:2|      3: <6, 1090519040>     |        %c3 = float 8;
     137:2|      3: <3>                 |        %c4 = float undef;
     139:0|      3: <6, 2143289344>     |        %c5 = float nan;
     146:0|      3: <6, 2139095040>     |        %c6 = float inf;
     153:0|      3: <6, 4286578688>     |        %c7 = float -inf;
     160:0|      3: <1, 1>              |      double:
     162:4|      3: <6,                 |        %c8 = double 1;
          |        4607182418800017408> |
     174:0|      3: <6, 0>              |        %c9 = double 0;
     176:4|      3: <6,                 |        %c10 = double 5;
          |        4617315517961601024> |
     188:0|      3: <6,                 |        %c11 = double 6;
          |        4618441417868443648> |
     199:4|      3: <6,                 |        %c12 = double nan;
          |        9221120237041090560> |
     211:0|      3: <6,                 |        %c13 = double inf;
          |        9218868437227405312> |
     222:4|      3: <6,                 |        %c14 = double -inf;
          |        18442240474082181120>|
     234:0|    0: <65534>               |      }

.. _link_for_function_blocks_section:

Function Blocks
===============

A function block defines the implementation of a defined :ref:`function
address<link_for_function_address_section>`. The function address it defines is
based on the position of the corresponding defined function address. The Nth
defined function address always corresponds to the Nth function block in the
module block.

A function implementation contains a list of basic blocks, forming the control
flow graph. Each *basic block* contains a list of instructions, and ends with a
:ref:`terminator instruction<link_for_terminator_instruction_section>`
(e.g. branch).

Basic blocks are not represented by records. Rather, context is implicit. The
first basic block begins with the first instruction record in the function
block.  Block boundaries are determined by terminator instructions. The
instruction that follows a terminator instruction begins a new basic block.

The first basic block in a function is special in two ways: it is immediately
executed on entrance to the function, and it is not allowed to have predecessor
basic blocks (i.e. there can't be any branches to the entry block of a
function). Because the entry block has no predecessors, it also can't have any
:ref:`phi<link_for_phi_instruction_section>` instructions.

The parameters are implied by the type of the corresponding function
address. One parameter is defined for each argument of the function :ref:`type
signature<link_for_function_type>` of the corresponding :ref:`function
address<link_for_function_address_section>`.

The number of basic blocks is defined by the :ref:`count
record<link_for_basic_blocks_count>`. Each :ref:`terminator
instruction<link_for_terminator_instruction_section>` ends the current basic
block, and the next instruction begins a new basic block.  Basic blocks are
numbered by the order they appear (starting with index 0). Basic block IDs have
the form ``%bN``, where ``N`` corresponds to the position of the basic block
within the function block.

Each instruction, within a function block, corresponds to a corresponding PNaCl
record.  The layout of a function block is the (basic block) count record,
followed by a sequence of instruction records.

For readability, PNaClAsm introduces basic block IDs. These basic block IDs do
not correspond to PNaCl records, since basic block boundaries are defined
implicitly, after terminator instructions. They appear only for readability.

Operands of instructions are defined using an :ref:`absolute
index<link_for_absolute_index_section>`. This absolute index implicitly encodes
function addresses, global addresses, parameters, constants, and instructions
that generate values. The encoding takes advantage of the implied ordering of
these values in the bitcode file, defining a contiguous sequence of indices for
each kind of identifier.  That is, indices are ordered by putting function
address identifiers first, followed by global address identifiers, followed by
parameter identifiers, followed by constant identifiers, and lastly instruction
value identifiers.

To save space in the encoded bitcode file, most operands are encoded using a
:ref:`relative index<link_for_relative_index>` value, rather than
:ref:`absolute<link_for_absolute_index_section>`. This
is done because most instruction operands refer to values defined earlier in the
(same) basic block.  As a result, the relative distance (back) from the next
value defining instruction is frequently a small number. Small numbers tend to
require fewer bits when they are converted to bit sequences.

Note that instructions that can appear in a function block are defined in
sections :ref:`link_for_terminator_instruction_section`,
:ref:`link_for_integer_binary_instructions`,
:ref:`link_for_floating_point_binary_instructions`,
:ref:`link_for_memory_creation_and_access_instructions`,
:ref:`link_for_conversion_instructions`, :ref:`link_for_compare_instructions`,
:ref:`link_for_vector_instructions`, and
:ref:`link_for_other_pnaclasm_instructions`.

The following subsections define the remaining records that can appear in a
function block.

Function Enter
--------------

PNaClAsm defines a function enter block construct. The corresponding record is
simply an :ref:`enter block<link_for_enter_block_record_section>` record, with
BlockID value ``12``. All context about the defining address is implicit by the
position of the function block, and the corresponding defining :ref:`function
address<link_for_function_address_section>`. To improve readability, PNaClAsm
includes the function signature into the syntax rule.

**Syntax**::

  function TR @fN ( T0 %p0, ... , TM %pM ) {             <B>

**Record**::

  1: <65535, 12, B>

**Semantics**:

``B`` is the number of bits reserved for abbreviations in the block. If it is
omitted, 2 is assumed. See :ref:`enter<link_for_enter_block_record_section>`
block records for more details.

The value of ``N`` corresponds to the positional index of the corresponding
defining function address this block is associated with. ``M`` is the number of
defined parameters (minus one) in the function heading.

**Constraints**::

  N == NumFcnImpls &
  @fN in DefiningFcnIDs &
  TypeOfFcn(@fN) == TypeOf(TypeID(TR (T0, ... , TM)))

**Updates**::

  ++NumFcnImpls;
  EnclosingFcnID = @fN;
  NumBasicBlocks = 0;
  ExpectedBlocks = 0;
  NumParams = M;
  for I in [0..M]:
    TypeOf(%pI) = TypeOf(TypeID(TI));

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <2>                   |    @t1 = void;
      55:4|    3: <21, 0, 1>            |    @t2 = void ();
      58:6|    3: <21, 0, 0, 0>         |    @t3 = i32 (i32);
      62:6|  0: <65534>                 |  }
      ...
     104:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     112:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     114:4|    3: <10>                  |    ret void;
     116:2|  0: <65534>                 |  }
     120:0|  1: <65535, 12, 2>          |  function i32 @f1(i32 %p0) {  
          |                             |                   // BlockID = 12
     128:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     130:4|    3: <10, 1>               |    ret i32 %p0;
     133:0|  0: <65534>                 |  }

.. _link_for_basic_blocks_count:

Count Record
------------

The count record, within a function block, defines the number of basic blocks
used to define the function implementation. It must be the first record in the
function block.

**Syntax**::

    blocks: N;                                    <A>
  %b0:

**Record**::

  AA: <1, N>

**Semantics**:

The count record defines the number ``N`` of basic blocks in the implemented
function.

**Constraints**::

  AA == AbbrevIndex(A) &
  ExpectedBasicBlocks == N &
  NumBasicBlocks == 0

**Updates**::

     104:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     112:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     114:4|    3: <10>                  |    ret void;
     116:2|  0: <65534>                 |  }
     120:0|  1: <65535, 12, 2>          |  function i32 @f1(i32 %p0) {  
          |                             |                   // BlockID = 12
     128:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     130:4|    3: <10, 1>               |    ret i32 %p0;
     133:0|  0: <65534>                 |  }

.. _link_for_terminator_instruction_section:

Terminator Instructions
=======================

Terminator instructions are instructions that appear in a :ref:`function
block<link_for_function_blocks_section>`, and define the end of the current
basic block. A terminator instruction indicates which block should be executed
after the current block is finished. The function block is well formed only if
the number of terminator instructions, in the function block, corresponds to the
value defined by the corresponding function basic block :ref:`count
record<link_for_basic_blocks_count>`.

Note that any branch instruction to label ``%bN``, where ``N >=
ExpectedBasicBlocks``, is illegal. For ease of readability, this constraint
hasn't been put on branch instructions. Rather it is only implied.

In addition, it must be the case that ``NumBasicBlocks < ExpectedBasicBlocks``,
and will not be listed as a constraint. Further, if ``B = NumBasicBlocks + 1``
is the number associated with the next basic block.  Label `%bB:` only appears
if::

  B < ExpectedBasicBlocks

That is, the label is omitted only if this terminator instruction is the last
instruction in the function block.

Return Void Instruction
-----------------------

The return void instruction is used to return control from a function back to
the caller, without returning any value.

**Syntax**::

    ret void;                                     <A>
  %bB:

**Record**::

   AA: <10>

**Semantics**:

The return void instruction returns control to the calling function.

**Constraints**::

  AA == AbbrevIndex(A) &
  B == NumBasicBlocks + 1 &
  ReturnType(TypeOf(EnclosingFcnID)) == void

**Updates**::

  ++NumBasicBlocks;

**Examples**::

     104:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     112:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     114:4|    3: <10>                  |    ret void;
     116:2|  0: <65534>                 |  }

Return Value Instruction
------------------------

The return value instruction is used to return control from a function back to
the caller, including a value. The value must correspond to the return type of
the enclosing function.

**Syntax**::

    ret T V;                                      <A>
  %bB:

**Record**::

   AA: <10, VV>

**Semantics**:

The return value instruction returns control to the calling function, returning
the provided value.

``V`` is the value to return. Type ``T`` must be of the type returned by the
function.  It must also be the type associated with value ``V``.

The return type ``T`` must either be a (non-void) primitive type, or a vector
type. If the function block is implementing an ordinary function, and the return
type is an integer type, it must be either ``i32`` or ``i64``.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV == RelativeIndex(V) &
  B == NumBasicBlocks + 1 &
  T == TypeOf(V) == ReturnType(TypeOf(EnclosingFcnID))

**Updates**::

  ++NumBasicBlocks;

**Examples**::

     120:0|  1: <65535, 12, 2>          |  function i32 @f1(i32 %p0) {  
          |                             |                   // BlockID = 12
     128:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     130:4|    3: <10, 1>               |    ret i32 %p0;

Unconditional Branch Instruction
--------------------------------

The unconditional branch instruction is used to cause control flow to transfer
to a different basic block of the function.

**Syntax**::

    br %bN;                                       <A>
  %bB:

**Record**::

  AA: <11, N>

**Semantics**:

The unconditional branch instruction causes control flow to transfer to basic
block ``N``.

**Constraints**::

  AA == AbbrevIndex(A) &
  B == NumBasicBlocks + 1 &
  0 < N &
  N < ExpectedBasicBlocks

**Updates**::

  ++NumBasicBlocks;

**Examples**::

      88:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
      96:0|    3: <1, 5>                |    blocks 5;
          |                             |  %b0:
      98:4|    3: <11, 3>               |    br label %b3;
          |                             |  %b1:
     101:0|    3: <11, 4>               |    br label %b4;
          |                             |  %b2:
     103:4|    3: <11, 1>               |    br label %b1;
          |                             |  %b3:
     106:0|    3: <11, 2>               |    br label %b2;
          |                             |  %b4:
     108:4|    3: <10>                  |    ret void;
     110:2|  0: <65534>                 |  }

Conditional Branch Instruction
------------------------------

The conditional branch instruction is used to cause control flow to transfer to
a different basic block of the function, based on a boolean test condition.

**Syntax**::

    br i1 C, %bT, %bBF;                          <A>
  %bB:

**Record**::

  AA: <11, T, F, CC>

**Semantics**:

Upon execution of a conditional branch instruction, the *i1* (boolean) argument
``C`` is evaluated. If the value is ``true``, control flows to basic block
``%bT``. Otherwise control flows to basic block ``%bF``.

**Constraints**::

  AA == AbbrevIndex(A) &
  CC == RelativeIndex(C) &
  B == NumBasicBlocks + 1 &
  0 < T &
  B1 < ExpectedBasicBlocks &
  0 < F &
  B2 < ExpectedBasicBlocks &
  TypeOf(C) == i1

**Updates**::

  ++NumBasicBlocks;

**Examples**::

      92:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     100:0|    3: <1, 5>                |    blocks 5;
     102:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     112:0|      3: <1, 1>              |      i1:
     114:4|      3: <4, 3>              |        %c0 = i1 1;
     117:0|      3: <4, 0>              |        %c1 = i1 0;
     119:4|    0: <65534>               |      }
          |                             |  %b0:
     120:0|    3: <11, 3>               |    br label %b3;
          |                             |  %b1:
     122:4|    3: <11, 2, 4, 2>         |    br i1 %c0, label %b2, label %b4;
          |                             |  %b2:
     126:4|    3: <11, 3>               |    br label %b3;
          |                             |  %b3:
     129:0|    3: <10>                  |    ret void;
          |                             |  %b4:
     130:6|    3: <11, 2, 3, 1>         |    br i1 %c1, label %b2, label %b3;
     134:6|  0: <65534>                 |  }

Unreachable
-----------

The unreachable instruction has no defined semantics. The instruction is used to
inform the :ref:`PNaCl translator<link_for_pnacl_translator>` that control
can't reach this instruction.

**Syntax**::

    unreachable;                                  <A>
  %bB:

**Record**::

  AA: <15>

**Semantics**:

Directive to the :ref:`PNaCl translator<link_for_pnacl_translator>` that
this instruction is unreachable.

**Constraints**::

  AA == AbbrevIndex(A)
  B == NumBasicBlocks + 1 &

**Updates**::

  ++NumBasicBlocks;

**Examples**::

     108:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0) {  
          |                             |                   // BlockID = 12
     116:0|    3: <1, 5>                |    blocks 5;
     118:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     128:0|      3: <1, 2>              |      i1:
     130:4|      3: <4, 3>              |        %c0 = i1 1;
     133:0|      3: <4, 0>              |        %c1 = i1 0;
     135:4|    0: <65534>               |      }
          |                             |  %b0:
     136:0|    3: <11, 1, 2, 2>         |    br i1 %c0, label %b1, label %b2;
          |                             |  %b1:
     140:0|    3: <11, 3, 4, 1>         |    br i1 %c1, label %b3, label %b4;
          |                             |  %b2:
     144:0|    3: <15>                  |    unreachable;
          |                             |  %b3:
     145:6|    3: <15>                  |    unreachable;
          |                             |  %b4:
     147:4|    3: <10>                  |    ret void;
     149:2|  0: <65534>                 |  }

Switch Instruction
------------------

The *switch* instruction transfers control flow to one of several different
places, based on a selector value. It is a generalization of the conditional
branch instruction.

**Syntax**::

    switch T V0 {
      default: br label %bB0;
      T V1:    br label %bB1;
      ...
      T VN:    br label %bBN;
    }                                             <A>
  %bB:

**Record**::

  AA: <12, TT, B0, N, (1, 1, VVI, BI | 1 <= i <= N)>

**Semantics**:

The switch instruction transfers control to a basic block in ``B0`` through
``BN``.  Value ``V`` is used to conditionally select which block to branch
to. ``T`` is the type of ``V`` and ``V1`` through ``VN``, and must be an integer
type. Value ``V1`` through ``VN`` are integers to compare against ``V``. If
selector ``V`` matches ``VI`` (for some ``I``, ``1 <= I <= N``), then the
instruction branches to block ``BI``. If ``V`` is not in ``V1`` through ``VN``,
the instruction branches to block ``B0``.

**Constraints**::

  AA == AbbrevIndex(A) &
  B == NumBasicBlocks + 1 &
  TT == TypeID(T) &
  VI == SignRotate(VI) for all I, 1 <= I <= N &

**Updates**::

  ++NumBasicBlocks;

**Examples**::

     116:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0) {  
          |                             |                   // BlockID = 12
     124:0|    3: <1, 6>                |    blocks 6;
          |                             |  %b0:
     126:4|    3: <12, 1, 1, 2, 4, 1, 1,|    switch i32 %p0 {
          |        2, 3, 1, 1, 4, 3, 1, |      default: br label %b2;
          |        1, 8, 4, 1, 1, 10, 4>|      i32 1: br label %b3;
          |                             |      i32 2: br label %b3;
          |                             |      i32 4: br label %b4;
          |                             |      i32 5: br label %b4;
          |                             |    }
          |                             |  %b1:
     143:2|    3: <11, 5>               |    br label %b5;
          |                             |  %b2:
     145:6|    3: <11, 5>               |    br label %b5;
          |                             |  %b3:
     148:2|    3: <11, 5>               |    br label %b5;
          |                             |  %b4:
     150:6|    3: <11, 5>               |    br label %b5;
          |                             |  %b5:
     153:2|    3: <10>                  |    ret void;
     155:0|  0: <65534>                 |  }
     156:0|  1: <65535, 12, 2>          |  function void @f1(i64 %p0) {  
          |                             |                   // BlockID = 12
     164:0|    3: <1, 6>                |    blocks 6;
          |                             |  %b0:
     166:4|    3: <12, 2, 1, 2, 4, 1, 1,|    switch i64 %p0 {
          |        2, 3, 1, 1, 4, 3, 1, |      default: br label %b2;
          |        1, 8, 4, 1, 1,       |      i64 1: br label %b3;
          |        39777555332, 4>      |      i64 2: br label %b3;
          |                             |      i64 4: br label %b4;
          |                             |      i64 19888777666: br label %b4;
          |                             |    }
          |                             |  %b1:
     188:4|    3: <11, 5>               |    br label %b5;
          |                             |  %b2:
     191:0|    3: <11, 5>               |    br label %b5;
          |                             |  %b3:
     193:4|    3: <11, 5>               |    br label %b5;
          |                             |  %b4:
     196:0|    3: <11, 5>               |    br label %b5;
          |                             |  %b5:
     198:4|    3: <10>                  |    ret void;
     200:2|  0: <65534>                 |  }

.. _link_for_integer_binary_instructions:

Integer Binary Instructions
===========================

Binary instructions are used to do most of the computation in a program. This
section focuses on binary instructions that operator on integer values, or
vectors of integer values.

All binary operations require two operands of the same type, execute an
operation on them, and produce a value. The value may represent multiple values
if the type is a vector type.  The result value always has the same type as its
operands.

Some integer binary operations can be applied to both signed and unsigned
integers. Others, the sign is significant. In general, if the sign plays a role
in the instruction, the sign information is encoded into the name of the
instruction.

For most binary operations (except some of the logical operations), integer
type i1 is disallowed.

Integer Add
-----------

The integer add instruction returns the sum of its two arguments. Both arguments
and the result must be of the same type. That type must be integer, or an
integer vector type.

**Syntax**::

  %vN = add T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 0>

**Semantics**:

The integer add instruction returns the sum of its two arguments. Arguments
``V1`` and ``V2``, and the result ``%vN``, must be of type ``T``. ``T`` must be
an integer type, or an integer vector type.  ``N`` is defined by the record
position, defining the corresponding value generated by the instruction.

The result returned is the mathematical result modulo 2\ :sup:`n`\ , where ``n``
is the bit width of the integer result.

Because integers are assumed to use a two's complement representation,
this instruction is appropriate for both signed and unsigned integers.

In the add instruction, integer type ``i1`` (and a vector of integer type
``i1``) is disallowed.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 0>          |    %v0 = add i32 %p0, %p1;
  110:4|    3: <2, 3, 1, 0>          |    %v1 = add i32 %p0, %v0;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Integer Subtract
----------------

The integer subtract instruction returns the difference of its two arguments.
Both arguments and the result must be of the same type. That type must be
integer, or an integer vector type.

Note: Since there isn't a negate instruction, subtraction from constant zero
should be used to negate values.

**Syntax**::

  %vN = sub T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 1>

**Semantics**:

The integer subtract returns the difference of its two arguments. Arguments
``V1`` and ``V2``, and the result ``%vN`` must be of type ``T``. ``T`` must be
an integer type, or an integer vector type. ``N`` is defined by the record
position, defining the corresponding value generated by the instruction.

The result returned is the mathematical result modulo 2\ :sup:`n`\ , where ``n``
is the bit width of the integer result.

Because integers are assumed to use a two's complement representation,
this instruction is appropriate for both signed and unsigned integers.

In the subtract instruction, integer type ``i1`` (and a vector of integer type
``i1``) is disallowed.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 1>          |    %v0 = sub i32 %p0, %p1;
  110:4|    3: <2, 3, 1, 1>          |    %v1 = sub i32 %p0, %v0;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Integer Multiply
----------------

The integer multiply instruction returns the product of its two arguments.  Both
arguments and the result must be of the same type. That type must be integer,
or an integer based vector type.

**Syntax**::

  &vN = mul T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 2>

**Semantics**:

The integer multiply instruction returns the product of its two
arguments. Arguments ``V1`` and ``V2``, and the result ``%vN``, must be of type
``T``.  ``T`` must be an integer type, or an integer vector type. ``N`` is
defined by the record position, defining the corresponding value generated by
the instruction.

The result returned is the mathematical result modulo 2\ :sup:`n`\ , where ``n``
is the bit width of the integer result.

Because integers are assumed to use a two's complement representation,
this instruction is appropriate for both signed and unsigned integers.

In the subtract instruction, integer type ``i1`` (or a vector on integer type
``i1``) is disallowed.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 2>          |    %v0 = mul i32 %p0, %p1;
  110:4|    3: <2, 1, 3, 2>          |    %v1 = mul i32 %v0, %p0;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Signed Integer Divide
---------------------

The signed integer divide instruction returns the quotient of its two arguments.
Both arguments and the result must be of the same type. That type must be
integer, or an integer vector type.

**Syntax**::

  %vN = sdiv T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 4>

**Semantics**:

The signed integer divide instruction returns the quotient of its two
arguments. Arguments ``V1`` and ``V2``, and the result ``%vN``, must be of type
``T``. ``T`` must be a integer type, or an integer vector type. ``N`` is defined
by the record position, defining the corresponding value generated by the
instruction.

Signed values are assumed.  Note that signed and unsigned integer division are
distinct operations. For unsigned integer division use the unsigned integer
divide instruction (udiv).

In the signed integer divide instruction, integer type ``i1`` (and a vector of
integer type ``i1``) is disallowed. Integer division by zero is guaranteed to
trap.

Note that overflow can happen with this instruction when dividing the maximum
negative integer by ``-1``. The behavior for this case is currently undefined.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**:: 

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 4>          |    %v0 = sdiv i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 4>          |    %v1 = sdiv i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Unsigned Integer Divide
-----------------------

The unsigned integer divide instruction returns the quotient of its two
arguments. Both the arguments and the result must be of the same type. That type
must be integer, or an integer vector type.

**Syntax**::

  %vN = udiv T V1, V2;                            <a>

**Record**::

  AA: <2, A1, A2, 3>

**Semantics**:

The unsigned integer divide instruction returns the quotient of its two
arguments. Arguments ``V1`` and ``V2``, and the result ``%vN``, must be of type
``T``. ``T`` must be an integer type, or an integer vector type.  ``N`` is
defined by the record position, defining the corresponding value generated by
the instruction.

Unsigned integer values are assumed.  Note that signed and unsigned integer
division are distinct operations. For signed integer division use the signed
integer divide instruction (sdiv).

In the unsigned integer divide instruction, integer type ``i1`` (and a vector of
integer type ``i1``) is disallowed. Division by zero is guaranteed to trap.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 3>          |    %v0 = udiv i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 3>          |    %v1 = udiv i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Signed Integer Remainder
------------------------

The signed integer remainder instruction returns the remainder of the quotient
of its two arguments. Both arguments and the result must be of the same
type. That type must be integer, or an integer based vector type.

**Syntax**::

  %vN = srem T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 6>

**Semantics**:

The signed integer remainder instruction returns the remainder of the quotient
of its two arguments. Arguments ``V1`` and ``V2``, and the result ``%vN``, must
be of type ``T``. ``T`` must be a integer type, or an integer vector type. ``N``
is defined by the record position, defining the corresponding value generated by
the instruction.

Signed values are assumed.  Note that signed and unsigned integer division are
distinct operations. For unsigned integer division use the unsigned integer
remainder instruction (urem).

In the signed integer remainder instruction, integer type ``i1`` (and a vector
of integer type ``i1``) is disallowed.  Division by zero is guaranteed to trap.

Note that overflow can happen with this instruction when dividing the maximum
negative integer by ``-1``. The behavior for this case is currently undefined.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 6>          |    %v0 = srem i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 6>          |    %v1 = srem i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Unsigned Integer Remainder Instruction
--------------------------------------

The unsigned integer remainder instruction returns the remainder of the quotient
of its two arguments. Both the arguments and the result must be of the same
type. The type must be integer, or an integer vector type.

**Syntax**::

  %vN = urem T V1, V2;                            <A>

**Record**::

  AA: <2, A1, A2, 5>

**Semantics**:

The unsigned integer remainder instruction returns the remainder of the quotient
of its two arguments. Arguments ``V1`` and ``V2``, and the result ``%vN``, must
be of type ``T``. ``T`` must be an integer type, or an integer vector type.
``N`` is defined by the record position, defining the corresponding value
generated by the instruction.

Unsigned values are assumed. Note that signed and unsigned integer division are
distinct operations. For signed integer division use the remainder instruction
(srem).

In the unsigned integer remainder instruction, integer type ``i1`` (and a vector
of integer type ``i1``) is disallowed.  Division by zero is guaranteed to trap.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

      96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
          |                             |                   // BlockID = 12
     104:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     106:4|    3: <2, 2, 1, 5>          |    %v0 = urem i32 %p0, %p1;
     110:4|    3: <2, 1, 2, 5>          |    %v1 = urem i32 %v0, %p1;
     114:4|    3: <10, 1>               |    ret i32 %v1;
     117:0|  0: <65534>                 |  }

Shift Left
----------

The (integer) shift left instruction returns the first operand, shifted to the
left a specified number of bits with zero fill. The shifted value must be
integer, or an integer vector type.

**Syntax**::

  %vN = shl T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 7>

**Semantics**:

This instruction performs a shift left operation. Arguments ``V1`` and ``V2``
and the result ``%vN`` must be of type ``T``. ``T`` must be an integer, or a
vector of integers. ``N`` is defined by the record position, defining the
corresponding value generated by the instruction.

``V2`` is assumed to be unsigned.  The least significant bits of the result will
be filled with zero bits after the shift. If ``V2`` is (statically or
dynamically) negative or equal to or larger than the number of bits in
``V1``, the result is undefined. If the arguments are vectors, each vector
element of ``V1`` is shifted by the corresponding shift amount in ``V2``.

In the shift left instruction, integer type ``i1`` (and a vector of integer type
``i1``) is disallowed.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 7>          |    %v0 = shl i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 7>          |    %v1 = shl i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Logical Shift Right
-------------------
 
The logical shift right instruction returns the first operand, shifted to the
right a specified number of bits with zero fill.

**Syntax**::

  %vN = lshr T V1, V2;                            <A>

**Record**::

  AA: <2, VV1, VV2, 8>

**Semantics**:

This instruction performs a logical shift right operation. Arguments ``V1`` and
``V2`` and the result ``%vN`` must be of type ``T``. ``T`` must be an integer,
or a vector of integers. ``N`` is defined by the record position, defining the
corresponding value generated by the instruction.

``V2`` is assumed to be unsigned. The most significant bits of the result will
be filled with zero bits after the shift. If ``V2`` is (statically or
dynamically) negative or equal to or larger than the number of bits in ``V1``,
the result is undefined. If the arguments are vectors, each vector element of
``V1`` is shifted by the corresponding shift amount in ``V2``.

In the logical shift right instruction, integer type ``i1`` (and a vector of
integer type ``i1``) is disallowed.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 8>          |    %v0 = lshr i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 8>          |    %v1 = lshr i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Arithmetic Shift Right
----------------------
 
The arithmetic shift right instruction returns the first operand, shifted to the
right a specified number of bits with sign extension.

**Syntax**::

  %vN = ashr T V1, V2;                            <A>

**Record**::

  AA: <2, VV1, VVA2, 9>

**Semantics**:

This instruction performs an arithmetic shift right operation. Arguments ``V1``
and ``V2`` and and the result ``%vN`` must be of type ``T``. ``T`` must be an
integer, or a vector of integers. ``N`` is defined by the record position,
defining the corresponding value generated by the instruction.

``V2`` is assumed to be unsigned. The most significant bits of the result will
be filled with the sign bit of ``V1``. If ``V2`` is (statically or dynamically)
negative or equal to or larger than the number of bits in ``V1``, the result is
undefined. If the arguments are vectors, each vector element of ``V1`` is
shifted by the corresponding shift amount in ``V2``.

In the arithmetic shift right instruction, integer type ``i1`` (and a vector of
integral type ``i1``) is disallowed.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T)) &
  UnderlyingType(T) != i1 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 9>          |    %v0 = ashr i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 9>          |    %v1 = ashr i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Logical And
-----------

The *and* instruction returns the bitwise logical and of its two operands.

**Syntax**::
 
  %vN = and T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 10>

**Semantics**:

This instruction performs a bitwise logical and of its arguments.  Arguments
``V1`` and ``V2``, and the result ``%vN`` must be of type ``T``. ``T`` must be
an integer, or a vector of integers. ``N`` is defined by the record position,
defining the corresponding value generated by the instruction.  ``A`` is the
(optional) abbreviation associated with the corresponding record.

The truth table used for the *and* instruction is:

===== ===== ======
Arg 1 Arg 2 Result
===== ===== ======
0     0     0
0     1     0
1     0     0
1     1     1
===== ===== ======

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T))) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
       |                             |                   // BlockID = 12
  104:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  106:4|    3: <2, 2, 1, 10>         |    %v0 = and i32 %p0, %p1;
  110:4|    3: <2, 1, 2, 10>         |    %v1 = and i32 %v0, %p1;
  114:4|    3: <10, 1>               |    ret i32 %v1;
  117:0|  0: <65534>                 |  }

Logical Or
----------
 
The *or* instruction returns the bitwise logical inclusive or of its
two operands.

**Syntax**::

  %vN = or T V1, V2;                              <A>

**Record**::

  AA: <2, VV1, VV2, 11>

**Semantics**:

This instruction performs a bitwise logical inclusive or of its arguments.
Arguments ``V1`` and ``V2``, and the result ``%vN`` must be of type ``T``. ``T``
must be an integer, or a vector of integers. ``N`` is defined by the record
position, defining the corresponding value generated by the instruction.

The truth table used for the *or* instruction is:

===== ===== ======
Arg 1 Arg 2 Result
===== ===== ======
0     0     0
0     1     1
1     0     1
1     1     1
===== ===== ======

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T))) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

      96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
          |                             |                   // BlockID = 12
     104:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     106:4|    3: <2, 2, 1, 11>         |    %v0 = or i32 %p0, %p1;
     110:4|    3: <2, 1, 2, 11>         |    %v1 = or i32 %v0, %p1;
     114:4|    3: <10, 1>               |    ret i32 %v1;
     117:0|  0: <65534>                 |  }

Logical Xor
-----------
 
The *xor* instruction returns the bitwise logical exclusive or of its
two operands.

**Syntax**::

  %vN = xor T V1, V2;                             <A>

**Record**::

  AA: <2, VV1, VV2, 12>

**Semantics**:

This instruction performs a bitwise logical exclusive or of its arguments.
Arguments ``V1`` and ``V2``, and the result ``%vN`` must be of type ``T``. ``T``
must be an integer, or a vector of integers. ``N`` is defined by the record
position, defining the corresponding value generated by the instruction.

The truth table used for the *xor* instruction is:

===== ===== ======
Arg 1 Arg 2 Result
===== ===== ======
0     0     0
0     1     1
1     0     1
1     1     0
===== ===== ======

**Constraints**::

  AA == AbbrevIndex(A) &
  A1 == RelativeIndex(V1) &
  A2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsInteger(UnderlyingType(T))) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

    96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
        |                             |                   // BlockID = 12
   104:0|    3: <1, 1>                |    blocks 1;
        |                             |  %b0:
   106:4|    3: <2, 2, 1, 12>         |    %v0 = xor i32 %p0, %p1;
   110:4|    3: <2, 1, 2, 12>         |    %v1 = xor i32 %v0, %p1;
   114:4|    3: <10, 1>               |    ret i32 %v1;
   117:0|  0: <65534>                 |  }

.. _link_for_floating_point_binary_instructions:

Floating Point Binary Instructions
==================================

Floating point binary instructions require two operands of the same type,
execute an operation on them, and produce a value. The value may represent
multiple values if the type is a vector type.  The result value always has the
same type as its operands.

Floating Point Add
------------------

The floating point add instruction returns the sum of its two arguments. Both
arguments and the result must be of the same type. That type must be a floating
point type, or a vector of a floating point type.

**Syntax**::

  %vN = fadd T V1, V2;                            <A>

**Record**::

  AA: <2, VV1, VV2, 0>

**Semantics**:

The floating point add instruction returns the sum of its two arguments.
Arguments ``V1`` and ``V2`` and the result ``%vN`` must be of type ``T``. ``T``
must be a floating point type, or a vector of a floating point type. ``N`` is
defined by the record position, defining the corresponding value generated by
the instruction.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsFloat(UnderlyingType(T)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   92:0|  1: <65535, 12, 2>          |  function 
       |                             |      float @f0(float %p0, float %p1) {
       |                             |                    // BlockID = 12
  100:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  102:4|    3: <2, 2, 1, 0>          |    %v0 = fadd float %p0, %p1;
  106:4|    3: <2, 3, 1, 0>          |    %v1 = fadd float %p0, %v0;
  110:4|    3: <10, 1>               |    ret float %v1;
  113:0|  0: <65534>                 |  }

Floating Point Subtract
-----------------------

The floating point subtract instruction returns the difference of its two
arguments. Both arguments and the result must be of the same type. That type
must be a floating point type, or a vector of a floating point type.

**Syntax**::

  %vN = fsub T V1, V2;                            <a>

**Record**::

  AA: <2, VV1, VV2, 1>

**Semantics**:

The floating point subtract instruction returns the difference of its two
arguments. Arguments ``V1`` and ``V2``, and the result ``%vN`` must be of type
``T``. ``T`` must be a floating point type, or a vector of a floating point
type. ``N`` is defined by the record position, defining the corresponding value
generated by the instruction.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsFloat(UnderlyingType(T)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   92:0|  1: <65535, 12, 2>          |  function 
       |                             |      float @f0(float %p0, float %p1) {
       |                             |                    // BlockID = 12
  100:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  102:4|    3: <2, 2, 1, 1>          |    %v0 = fsub float %p0, %p1;
  106:4|    3: <2, 3, 1, 1>          |    %v1 = fsub float %p0, %v0;
  110:4|    3: <10, 1>               |    ret float %v1;
  113:0|  0: <65534>                 |  }

Floating Point Multiply
-----------------------

The floating point multiply instruction returns the product of its two
arguments. Both arguments and the result must be of the same type. That type
must be a floating point type, or a vector of a floating point type.

**Syntax**::

  &vN = fmul T V1, V2;                            <A>

**Record**::

  AA: <2, VV1, VV2, 2>

**Semantics**:

The floating point multiply instruction returns the product of its two
arguments. Arguments ``V1`` and ``V2``, and the result ``%vN`` must be of type
``T``.  ``T`` must be a floating point type, or a vector of a floating point
type. ``N`` is defined by the record position, defining the corresponding value
generated by the instruction.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsFloat(UnderlyingType(T)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

      92:0|  1: <65535, 12, 2>          |  function 
          |                             |      float @f0(float %p0, float %p1) {
          |                             |                    // BlockID = 12
     100:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     102:4|    3: <2, 2, 1, 2>          |    %v0 = fmul float %p0, %p1;
     106:4|    3: <2, 3, 1, 2>          |    %v1 = fmul float %p0, %v0;
     110:4|    3: <10, 1>               |    ret float %v1;
     113:0|  0: <65534>                 |  }

Floating Point Divide
---------------------

The floating point divide instruction returns the quotient of its two
arguments. Both arguments and the result must be of the same type. That type
must be a floating point type, or a vector of a floating point type.

**Syntax**::

  %vN = fdiv T V1, V2;                            <A>

**Record**::

  AA: <2, V1, V2, 4>

**Semantics**:

The floating point divide instruction returns the quotient of its two
arguments. Arguments ``V1`` and ``V2``, and the result ``%vN`` must be of type
``T``. ``T`` must be a floating point type, or a vector of a floating point
type. ``N`` is defined by the record position, defining the corresponding value
generated by the instruction.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV22 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  IsFloat(UnderlyingType(T)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T;

**Examples**::

   92:0|  1: <65535, 12, 2>          |  function 
       |                             |      double 
       |                             |      @f0(double %p0, double %p1) {  
       |                             |                   // BlockID = 12
  100:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  102:4|    3: <2, 2, 1, 4>          |    %v0 = fdiv double %p0, %p1;
  106:4|    3: <2, 3, 1, 4>          |    %v1 = fdiv double %p0, %v0;
  110:4|    3: <10, 1>               |    ret double %v1;
  113:0|  0: <65534>                 |  }

Floating Point Remainder
------------------------

The floating point remainder instruction returns the remainder of the quotient
of its two arguments. Both arguments and the result must be of the same
type. That type must be a floating point type, or a vector of a floating point
type.

**Syntax**::

  %vN = frem T V1, V2;                            <A>

**Record**::

  AA: <2, VV1, VV2, 6>

**Semantics**:

The floating point remainder instruction returns the remainder of the quotient
of its two arguments. Arguments ``V1`` and ``V2``, and the result ``%vN`` must
be of type ``T``. ``T`` must be a floating point type, or a vector of a floating
point type. ``N`` is defined by the record position, defining the corresponding
value generated by the instruction.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2)  &
  IsFloat(UnderlyingType(T)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T

**Examples**::

   92:0|  1: <65535, 12, 2>          |  function 
       |                             |      double 
       |                             |      @f0(double %p0, double %p1) {  
       |                             |                   // BlockID = 12
  100:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  102:4|    3: <2, 2, 1, 6>          |    %v0 = frem double %p0, %p1;
  106:4|    3: <2, 3, 1, 6>          |    %v1 = frem double %p0, %v0;
  110:4|    3: <10, 1>               |    ret double %v1;
  113:0|  0: <65534>                 |  }

.. _link_for_memory_creation_and_access_instructions:

Memory Creation and Access Instructions
=======================================

A key design point of SSA-based representation is how it represents
memory. In PNaCl bitcode files, no memory locations are in SSA
form. This makes things very simple.

.. _link_for_alloca_instruction:

Alloca Instruction
------------------

The *alloca* instruction allocates memory on the stack frame of the
currently executing function. This memory is automatically released
when the function returns to its caller.

**Syntax**::

  %vN = alloca i8, i32 S, align V;                <A>

**Record**::

  AA: <19, SS, VV>

**Semantics**:

The *alloca* instruction allocates memory on the stack frame of the currently
executing function.  The resulting value is a pointer to the allocated memory
(i.e. of type i32). ``S`` is the number of bytes that are allocated on the
stack. ``S`` must be of integer type i32. ``V`` is the alignment of the
generated stack address.

Alignment must be a power of 2. See :ref:`memory blocks and
alignment<link_for_memory_blocks_and_alignment_section>` for a more detailed
discussion on how to define alignment.

**Constraints**::

  AA == AbbrevIndex(A) &
  VV == Log2(V+1) &
  SS == RelativeIndex(S) &
  i32 == TypeOf(S) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = i32;

**Examples**::

     112:0|  1: <65535, 12, 2>          |  function void @f1() {  
          |                             |                   // BlockID = 12
     120:0|    3: <1, 1>                |    blocks 1;
     122:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     132:0|      3: <1, 0>              |      i32:
     134:4|      3: <4, 4>              |        %c0 = i32 2;
     137:0|      3: <4, 8>              |        %c1 = i32 4;
     139:4|      3: <4, 16>             |        %c2 = i32 8;
     142:0|    0: <65534>               |      }
          |                             |  %b0:
     144:0|    3: <19, 3, 1>            |    %v0 = alloca i8, i32 %c0, align 1;
     147:2|    3: <19, 3, 3>            |    %v1 = alloca i8, i32 %c1, align 4;
     150:4|    3: <19, 3, 4>            |    %v2 = alloca i8, i32 %c2, align 8;
     153:6|    3: <10>                  |    ret void;
     155:4|  0: <65534>                 |  }

Load Instruction
----------------

The *load* instruction is used to read from memory.

**Syntax**::

  %vN = load T* P, align V;                       <A>

**Record**::

  AA: <20, PP, VV, TT>

**Semantics**:

The load instruction is used to read from memory. ``P`` is the identifier of the
memory address to read. The type of ``P`` must be an ``i32``.  ``T`` is the type
of value to read. ``V`` is the alignment of the memory address.

Type ``T`` must be a vector, integer, or floating point type. Both ``float`` and
``double`` types are allowed for floating point types. All integer types except
i1 are allowed.

Alignment must be a power of 2. See :ref:`memory blocks and
alignment<link_for_memory_blocks_and_alignment_section>` for a more detailed
discussion on how to define alignment.

**Constraints**::

  AA == AbbrevIndex(A) &
  i32 == TypeOf(P) &
  PP == RelativeIndex(P) &
  VV == Log2(V+1) &
  %tTT == TypeID(T) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <2>                   |    @t1 = void;
      55:4|    3: <4>                   |    @t2 = double;
      57:2|    3: <21, 0, 1, 0>         |    @t3 = void (i32);
      61:2|  0: <65534>                 |  }
      ...
      96:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0) {  
          |                             |                   // BlockID = 12
     104:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     106:4|    3: <20, 1, 1, 0>         |    %v0 = load i32* %p0, align 1;
     110:4|    3: <20, 1, 4, 2>         |    %v1 = load double* %v0, align 8;
     114:4|    3: <10>                  |    ret void;
     116:2|  0: <65534>                 |  }

Store Instruction
-----------------

The *store* instruction is used to write to memory.

**Syntax**::

  store T S, T* P, align V;                     <A>

**Record**::

  AA: <24, PP, SS, VV>

**Semantics**:

The store instruction is used to write to memory. ``P`` is the identifier of the
memory address to write to.  The type of ``P`` must be an i32 integer.  ``T`` is
the type of value to store. ``S`` is the value to store, and must be of type
``T``.  ``V`` is the alignment of the memory address.  ``A`` is the (optional)
abbreviation index associated with the record.

Type ``T`` must be an integer or floating point type. Both ``float`` and
``double`` types are allowed for floating point types. All integer types except
i1 are allowed.

Alignment must be a power of 2. See :ref:`memory blocks and
alignment<link_for_memory_Blocks_and_alignment_section>` for a more detailed
discussion on how to define alignment.

**Constraints**::

  AA == AbbrevIndex(A) &
  i32 == TypeOf(P) &
  PP == RelativeIndex(P) &
  VV == Log2(V+1)

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <2>                   |    @t1 = void;
      55:4|    3: <4>                   |    @t2 = double;
      57:2|    3: <21, 0, 1, 0, 0, 0, 2>|    @t3 = void (i32, i32, i32, double);
      63:4|  0: <65534>                 |  }
      ...
      96:0|  1: <65535, 12, 2>          |  function 
          |                             |      void 
          |                             |      @f0(i32 %p0, i32 %p1, i32 %p2, 
          |                             |          double %p3) {  
          |                             |                   // BlockID = 12
     104:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     106:4|    3: <24, 4, 3, 1>         |    store i32 %p1, i32* %p0, align 1;
     110:4|    3: <24, 2, 1, 4>         |    store double %p3, double* %p2, 
          |                             |        align 8;
     114:4|    3: <10>                  |    ret void;
     116:2|  0: <65534>                 |  }

.. _link_for_conversion_instructions:

Conversion Instructions
=======================

Conversion instructions all take a single operand and a type. The value is
converted to the corresponding type.

Integer Truncating Instruction
------------------------------

The integer truncating instruction takes a value to truncate, and a type
defining the truncated type. Both types must be integer types, or integer
vectors with the same number of elements. The bit size of the value must be
larger than the bit size of the destination type. Equal sized types are not
allowed.

**Syntax**::

  %vN = trunc T1 V to T2;                         <A>

**Record**::

  AA: <3, VV, TT2, 0>

**Semantics**:

The integer truncating instruction takes a value ``V``, and truncates to type
``T2``. Both ``T1`` and ``T2`` must be integer types, or integer vectors with
the same number of elements. ``T1`` has to be wider than ``T2``.  If the value
doesn't fit in in ``T2``, then the higher order bits are dropped.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 == TypeID(T2) &
  BitSizeOf(UnderlyingType(T1)) > BitSizeOf(UnderlyingType(T2)) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsInteger(UnderlyingType(T1)) &
  IsInteger(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 5>                |    count 5;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <2>                   |    @t1 = void;
      55:4|    3: <7, 16>               |    @t2 = i16;
      58:0|    3: <21, 0, 1, 0>         |    @t3 = void (i32);
      62:0|    3: <7, 8>                |    @t4 = i8;
      64:4|  0: <65534>                 |  }
      ...
     100:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0) {  
          |                             |                   // BlockID = 12
     108:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     110:4|    3: <3, 1, 2, 0>          |    %v0 = trunc i32 %p0 to i16;
     114:4|    3: <3, 1, 4, 0>          |    %v1 = trunc i16 %v0 to i8;
     118:4|    3: <10>                  |    ret void;
     120:2|  0: <65534>                 |  }

Floating Point Truncating Instruction
--------------------------------------

The floating point truncating instruction takes a value to truncate, and a type
defining the truncated type. Both types must be floating point types, or
floating point vectors with the same number of elements. The source must be
``double`` while the destination is ``float``.  If the source is a vector, the
destination must also be vector with the same size as the source.

**Syntax**::

  %vN = fptrunc T1 V to T2;                       <A>

**Record**::

  AA: <3, VV, TT2, 7>

**Semantics**

The floating point truncating instruction takes a value ``V``, and truncates to
type ``T2``. Both ``T1`` and ``T2`` must be floating point types, or floating
point vectors with the same number of elements. ``T1`` must be defined on
``double`` while ``T2`` is defined on ``float``.  If the value can't fit within
the destination type ``T2``, the results are undefined.

**Constraints**::

  TypeOf(V) == T1 &
  double == UnderlyingType(T1) &
  float == UnderlyingType(T2) &
  VV == RelativeIndex(V) &
  %tTT2 == TypeID(T2) &
  BitSizeOf(UnderlyingType(T1)) > BitSizeOf(UnderlyingType(T2)) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsFloat(UnderlyingType(T1)) &
  IsFloat(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

   40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
   48:0|    3: <1, 4>                |    count 4;
   50:4|    3: <3>                   |    @t0 = float;
   52:2|    3: <4>                   |    @t1 = double;
   54:0|    3: <21, 0, 0, 1>         |    @t2 = float (double);
   58:0|    3: <2>                   |    @t3 = void;
   59:6|  0: <65534>                 |  }
  ...
   92:0|  1: <65535, 12, 2>          |  function float @f0(double %p0) {  
       |                             |                   // BlockID = 12
  100:0|    3: <1, 1>                |    blocks 1;
       |                             |  %b0:
  102:4|    3: <3, 1, 0, 7>          |    %v0 = fptrunc double %p0 to float;
  106:4|    3: <10, 1>               |    ret float %v0;
  109:0|  0: <65534>                 |  }


Zero Extending Instruction
--------------------------

The zero extending instruction takes a value to extend, and a type to extend it
to. Both types must be integer types, or integer vectors with the same number
of elements. The bit size of the source type must be smaller than the bit size
of the destination type. Equal sized types are not allowed.

**Syntax**::

  %vN = zext T1 V to T2;                          <A>

**Record**::

  AA: <3, VV, TT2, 1>


**Semantics**:

The zero extending instruction takes a value ``V``, and expands it to type
``T2``. Both ``T1`` and ``T2`` must be integer types, or integer vectors with
the same number of elements. ``T2`` must be wider than ``T1``.

The instruction fills the high order bits of the value with zero bits until it
reaches the size of the destination type. When zero extending from i1, the
result will always be either 0 or 1.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 == TypeID(T2) &
  BitSizeOf(UnderlyingType(T1)) < BitSizeOf(UnderlyingType(T2)) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsInteger(UnderlyingType(T1)) &
  IsInteger(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 5>                |    count 5;
      50:4|    3: <7, 64>               |    @t0 = i64;
      53:6|    3: <7, 32>               |    @t1 = i32;
      57:0|    3: <21, 0, 0>            |    @t2 = i64 ();
      60:2|    3: <7, 8>                |    @t3 = i8;
      62:6|    3: <2>                   |    @t4 = void;
      64:4|  0: <65534>                 |  }
      ...
     100:0|  1: <65535, 12, 2>          |  function i64 @f0() {  // BlockID = 12
     108:0|    3: <1, 1>                |    blocks 1;
     110:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     120:0|      3: <1, 3>              |      i8:
     122:4|      3: <4, 2>              |        %c0 = i8 1;
     125:0|    0: <65534>               |      }
          |                             |  %b0:
     128:0|    3: <3, 1, 1, 1>          |    %v0 = zext i8 %c0 to i32;
     132:0|    3: <3, 1, 0, 1>          |    %v1 = zext i32 %v0 to i64;
     136:0|    3: <10, 1>               |    ret i64 %v1;
     138:4|  0: <65534>                 |  }

Sign Extending Instruction
--------------------------

The sign extending instruction takes a value to cast, and a type to extend it
to. Both types must be integer types, or integral vectors with the same number
of elements. The bit size of the source type must be smaller than the bit size
of the destination type. Equal sized types are not allowed.

**Syntax**::

  %vN = sext T1 V to T2;                          <A>

**Record**::

  AA: <3, VV, TT2, 2>

**Semantics**:

The sign extending instruction takes a value ``V``, and expands it to type
``T2``. Both ``T1`` and ``T2`` must be integer types, or integer vectors with
the same number of integers.  ``T2`` has to be wider than ``T1``.

When sign extending, the instruction fills the high order bits of the value with
the (current) high order bit of the value. When sign extending from i1, the
extension always results in -1 or 0.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 == TypeID(T2) &
  BitSizeOf(UnderlyingType(T1)) < BitSizeOf(UnderlyingType(T2)) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsInteger(UnderlyingType(T1)) &
  IsInteger(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 5>                |    count 5;
      50:4|    3: <7, 64>               |    @t0 = i64;
      53:6|    3: <7, 32>               |    @t1 = i32;
      57:0|    3: <21, 0, 0>            |    @t2 = i64 ();
      60:2|    3: <7, 8>                |    @t3 = i8;
      62:6|    3: <2>                   |    @t4 = void;
      64:4|  0: <65534>                 |  }
      ...
     100:0|  1: <65535, 12, 2>          |  function i64 @f0() {  // BlockID = 12
     108:0|    3: <1, 1>                |    blocks 1;
     110:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     120:0|      3: <1, 3>              |      i8:
     122:4|      3: <4, 3>              |        %c0 = i8 -1;
     125:0|    0: <65534>               |      }
          |                             |  %b0:
     128:0|    3: <3, 1, 1, 2>          |    %v0 = sext i8 %c0 to i32;
     132:0|    3: <3, 1, 0, 2>          |    %v1 = sext i32 %v0 to i64;
     136:0|    3: <10, 1>               |    ret i64 %v1;
     138:4|  0: <65534>                 |  }

Floating Point Extending Instruction
------------------------------------

The floating point extending instruction takes a value to extend, and a type to
extend it to. Both types must either be floating point types, or vectors of
floating point types with the same number of elements. The source value must be
``float`` while the destination is ``double``.  If the source is a vector, the
destination must also be vector with the same size as the source.

**Syntax**::

  %vN = fpext T1 V to T2;                           <A>

**Record**::

  AA: <3, VV, TT2, 8>

**Semantics**:

The floating point extending instruction converts floating point values.
``V`` is the value to extend, and ``T2`` is the type to extend it
to. Both ``T1`` and ``T2`` must be floating point types, or floating point
vector types with the same number of floating point values. ``T1`` contains
``float`` while ``T2`` contains ``double``.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 == TypeID(T2) &
  BitSizeOf(UnderlyingType(T1)) < BitSizeOf(UnderlyingType(T2)) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsFloat(UnderlyingType(T1)) &
  IsFloat(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <4>                   |    @t0 = double;
      52:2|    3: <3>                   |    @t1 = float;
      54:0|    3: <21, 0, 0, 1>         |    @t2 = double (float);
      58:0|    3: <2>                   |    @t3 = void;
      59:6|  0: <65534>                 |  }
      ...
      92:0|  1: <65535, 12, 2>          |  function double @f0(float %p0) {  
          |                             |                   // BlockID = 12
     100:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     102:4|    3: <3, 1, 0, 8>          |    %v0 = fpext float %p0 to double;
     106:4|    3: <10, 1>               |    ret double %v0;
     109:0|  0: <65534>                 |  }

Floating Point to Unsigned Integer Instruction
----------------------------------------------

The floating point to unsigned integer instruction converts floating point
values to unsigned integers.

**Syntax**::

  %vN = fptoui T1 V to T2;                        <A>

**Record**::

  AA: <3, VV, TT2, 3>

**Semantics**:

The floating point to unsigned integer instruction converts floating point
value(s) in ``V`` to its unsigned integer equivalent of type ``T2``. ``T1`` must
be a floating point type, or a floating point vector type. ``T2`` must be an
integer type, or an integer vector type. If either type is a vector type, they
both must have the same number of elements.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 == TypeID(T2) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsFloat(UnderlyingType(T1)) &
  IsInteger(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 6>                |    count 6;
      50:4|    3: <3>                   |    @t0 = float;
      52:2|    3: <4>                   |    @t1 = double;
      54:0|    3: <2>                   |    @t2 = void;
      55:6|    3: <21, 0, 2, 0, 1>      |    @t3 = void (float, double);
      60:4|    3: <7, 32>               |    @t4 = i32;
      63:6|    3: <7, 16>               |    @t5 = i16;
      66:2|  0: <65534>                 |  }
      ...
     100:0|  1: <65535, 12, 2>          |  function 
          |                             |      void @f0(float %p0, double %p1) {
          |                             |                    // BlockID = 12
     108:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     110:4|    3: <3, 2, 4, 3>          |    %v0 = fptoui float %p0 to i32;
     114:4|    3: <3, 2, 5, 3>          |    %v1 = fptoui double %p1 to i16;
     118:4|    3: <10>                  |    ret void;
     120:2|  0: <65534>                 |  }

Floating Point to Signed Integer Instruction
--------------------------------------------

The floating point to signed integer instruction converts floating point
values to signed integers.

**Syntax**::

  %vN = fptosi T1 V to T2;                        <A>

**Record**::

  AA: <3, VV, TT2, 4>

**Semantics**:

The floating point to signed integer instruction converts floating point
value(s) in ``V`` to its signed integer equivalent of type ``T2``. ``T1`` must
be a floating point type, or a floating point vector type. ``T2`` must be an
integer type, or an integer vector type. If either type is a vector type, they
both must have the same number of elements.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 = TypeID(T2) &
  UnderlyingCount(T1) = UnderlyingCount(T2) &
  IsFloat(UnderlyingType(T1)) &
  IsInteger(UnderlyingType(T2)) &
  N = NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 6>                |    count 6;
      50:4|    3: <3>                   |    @t0 = float;
      52:2|    3: <4>                   |    @t1 = double;
      54:0|    3: <2>                   |    @t2 = void;
      55:6|    3: <21, 0, 2, 0, 1>      |    @t3 = void (float, double);
      60:4|    3: <7, 8>                |    @t4 = i8;
      63:0|    3: <7, 16>               |    @t5 = i16;
      65:4|  0: <65534>                 |  }
      ...
     100:0|  1: <65535, 12, 2>          |  function 
          |                             |      void @f0(float %p0, double %p1) {
          |                             |                    // BlockID = 12
     108:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     110:4|    3: <3, 2, 4, 4>          |    %v0 = fptosi float %p0 to i8;
     114:4|    3: <3, 2, 5, 4>          |    %v1 = fptosi double %p1 to i16;
     118:4|    3: <10>                  |    ret void;
     120:2|  0: <65534>                 |  }

Unsigned Integer to Floating Point Instruction
----------------------------------------------

The unsigned integer to floating point instruction converts unsigned integers to
floating point values.

**Syntax**::

  %vN = uitofp T1 V to T2;                        <A>

**Record**::

  AA: <3, VV, TT2, 5>

**Semantics**:

The unsigned integer to floating point instruction converts unsigned integer(s)
to its floating point equivalent of type ``T2``. ``T1`` must be an integer type,
or a integer vector type. ``T2`` must be a floating point type, or a floating
point vector type. If either type is a vector type, they both must have the same
number of elements.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 = TypeID(T2) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsInteger(UnderlyingType(T1)) &
  IsFloat(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) == T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 7>                |    count 7;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <7, 64>               |    @t1 = i64;
      57:0|    3: <2>                   |    @t2 = void;
      58:6|    3: <3>                   |    @t3 = float;
      60:4|    3: <21, 0, 2, 0, 1>      |    @t4 = void (i32, i64);
      65:2|    3: <7, 1>                |    @t5 = i1;
      67:6|    3: <4>                   |    @t6 = double;
      69:4|  0: <65534>                 |  }
     ...
     104:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0, i64 %p1) {
          |                             |                    // BlockID = 12
     112:0|    3: <1, 1>                |    blocks 1;
     114:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     124:0|      3: <1, 5>              |      i1:
     126:4|      3: <4, 3>              |        %c0 = i1 1;
     129:0|    0: <65534>               |      }
          |                             |  %b0:
     132:0|    3: <3, 1, 6, 5>          |    %v0 = uitofp i1 %c0 to double;
     136:0|    3: <3, 4, 3, 5>          |    %v1 = uitofp i32 %p0 to float;
     140:0|    3: <3, 4, 3, 5>          |    %v2 = uitofp i64 %p1 to float;
     144:0|    3: <10>                  |    ret void;
     145:6|  0: <65534>                 |  }

Signed Integer to Floating Point Instruction
--------------------------------------------

The signed integer to floating point instruction converts signed integers to
floating point values.

**Syntax**::

  %vN = sitofp T1 V to T2;                        <A>

**Record**::

  AA: <3, VV, TT2, 6>

**Semantics**:

The signed integer to floating point instruction converts signed integer(s) to
its floating point equivalent of type ``T2``. ``T1`` must be an integer type, or
a integer vector type. ``T2`` must be a floating point type, or a floating point
vector type. If either type is a vector type, they both must have the same
number of elements.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV == RelativeIndex(V) &
  %tTT2 = TypeID(T2) &
  UnderlyingCount(T1) == UnderlyingCount(T2) &
  IsInteger(UnderlyingType(T1)) &
  IsFloat(UnderlyingType(T2)) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 7>                |    count 7;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <7, 64>               |    @t1 = i64;
      57:0|    3: <2>                   |    @t2 = void;
      58:6|    3: <3>                   |    @t3 = float;
      60:4|    3: <21, 0, 2, 0, 1>      |    @t4 = void (i32, i64);
      65:2|    3: <7, 8>                |    @t5 = i8;
      67:6|    3: <4>                   |    @t6 = double;
      69:4|  0: <65534>                 |  }
      ...
     104:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0, i64 %p1) {
          |                             |                    // BlockID = 12
     112:0|    3: <1, 1>                |    blocks 1;
     114:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     124:0|      3: <1, 5>              |      i8:
     126:4|      3: <4, 3>              |        %c0 = i8 -1;
     129:0|    0: <65534>               |      }
          |                             |  %b0:
     132:0|    3: <3, 1, 6, 6>          |    %v0 = sitofp i8 %c0 to double;
     136:0|    3: <3, 4, 3, 6>          |    %v1 = sitofp i32 %p0 to float;
     140:0|    3: <3, 4, 3, 6>          |    %v2 = sitofp i64 %p1 to float;
     144:0|    3: <10>                  |    ret void;
     145:6|  0: <65534>                 |  }

Bitcast Instruction
-------------------

The bitcast instruction converts the type of the value without changing the bit
contents of the value. The bit size of the type of the value must be the same as
the bit size of the cast type.

**Syntax**::

  %vN = bitcast T1 V to T2;                       <A>

**Record**::

  AA: <3, VV, TT2, 11>

**Semantics**:

The bitcast instruction converts the type of value ``V`` to type ``T2``. ``T1``
and ``T2`` must be primitive types or vectors, and define the same number of
bits.

**Constraints**::

  AA == AbbrevIndex(A) &
  TypeOf(V) == T1 &
  VV = RelativeIndex(V) &
  %tTT2 = TypeID(T2) &
  BitSizeOf(T1) == BitSizeOf(T2) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T2;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 6>                |    count 6;
      50:4|    3: <3>                   |    @t0 = float;
      52:2|    3: <7, 64>               |    @t1 = i64;
      55:4|    3: <2>                   |    @t2 = void;
      57:2|    3: <21, 0, 2, 0, 1>      |    @t3 = void (float, i64);
      62:0|    3: <7, 32>               |    @t4 = i32;
      65:2|    3: <4>                   |    @t5 = double;
      67:0|  0: <65534>                 |  }
      ...
     100:0|  1: <65535, 12, 2>          |  function void @f0(float %p0, i64 %p1)
          |                             |      {  // BlockID = 12
     108:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     110:4|    3: <3, 2, 4, 11>         |    %v0 = bitcast float %p0 to i32;
     114:4|    3: <3, 2, 5, 11>         |    %v1 = bitcast i64 %p1 to double;
     118:4|    3: <10>                  |    ret void;
     120:2|  0: <65534>                 |  }

.. _link_for_compare_instructions:

Comparison Instructions
=======================

The comparison instructions compare values and generates a boolean (i1) result
for each pair of compared values. There are different comparison operations for
integer and floating point values.


Integer Comparison Instructions
-------------------------------

The integer comparison instruction compares integer values and returns a
boolean (i1) result for each pair of compared values.

**Syntax**::

  %vN = icmp C T V1, V2;                          <A>

**Record**::

  AA: <9, VV1, VV2, CC>

**Semantics**:

The integer comparison instruction compares integer values and returns a boolean
(i1) result for each pair of compared values in ``V1`` and ``V2``. ``V1`` and
``V2`` must be of type ``T``. ``T`` must be an integer type, or an integer
vector type. Condition code ``C`` is the condition applied to all elements in
``V1`` and ``V2``.  Each comparison always yields an i1. If ``T`` is a primitive
type, the resulting type is i1. If ``T`` is a vector, then the resulting type is
a vector of i1 with the same size as ``T``.

Legal test conditions are:

=== == ==============================
C   CC Operator
=== == ==============================
eq  32 equal
ne  33 not equal
ugt 34 unsigned greater than
uge 35 unsigned greater than or equal
ult 36 unsigned less than
ule 37 unsigned less than or equal
sgt 38 signed greater than
sge 39 signed greater than or equal
slt 40 signed less than
sle 41 signed less than or equal
=== == ==============================

**Constraints**::

  AA == AbbrevIndex(A) &
  IsInteger(UnderlyingType(T) &
  T == TypeOf(V1) == TypeOf(V2) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  if IsVector(T) then
    TypeOf(%vN) = <UnderlyingCount(T), i1>
  else
    TypeOf(%vN) = i1
  endif

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <7, 1>                |    @t1 = i1;
      56:2|    3: <2>                   |    @t2 = void;
      58:0|    3: <21, 0, 2>            |    @t3 = void ();
      61:2|  0: <65534>                 |  }
      ...
     108:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     116:0|    3: <1, 1>                |    blocks 1;
     118:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     128:0|      3: <1, 0>              |      i32:
     130:4|      3: <4, 0>              |        %c0 = i32 0;
     133:0|      3: <4, 2>              |        %c1 = i32 1;
     135:4|    0: <65534>               |      }
          |                             |  %b0:
     136:0|    3: <28, 2, 1, 32>        |    %v0 = icmp eq i32 %c0, %c1;
     140:6|    3: <28, 3, 2, 33>        |    %v1 = icmp ne i32 %c0, %c1;
     145:4|    3: <28, 4, 3, 34>        |    %v2 = icmp ugt i32 %c0, %c1;
     150:2|    3: <28, 5, 4, 36>        |    %v3 = icmp ult i32 %c0, %c1;
     155:0|    3: <28, 6, 5, 37>        |    %v4 = icmp ule i32 %c0, %c1;
     159:6|    3: <28, 7, 6, 38>        |    %v5 = icmp sgt i32 %c0, %c1;
     164:4|    3: <28, 8, 7, 38>        |    %v6 = icmp sgt i32 %c0, %c1;
     169:2|    3: <28, 9, 8, 39>        |    %v7 = icmp sge i32 %c0, %c1;
     174:0|    3: <28, 10, 9, 40>       |    %v8 = icmp slt i32 %c0, %c1;
     178:6|    3: <28, 11, 10, 41>      |    %v9 = icmp sle i32 %c0, %c1;
     183:4|    3: <10>                  |    ret void;
     185:2|  0: <65534>                 |  }


Floating Point Comparison Instructions
--------------------------------------

The floating point comparison instruction compares floating point values and
returns a boolean (i1) result for each pair of compared values.

**Syntax**::

  %vN = fcmp C T V1, V2;                          <A>

**Record**::

  AA: <9, VV1, VV2, CC>

**Semantics**:

The floating point comparison instruction compares floating point values and
returns a boolean (i1) result for each pair of compared values in ``V1`` and
``V2``. ``V1`` and ``V2`` must be of type ``T``. ``T`` must be a floating point
type, or a floating point vector type. Condition code ``C`` is the condition
applied to all elements in ``V1`` and ``V2``. Each comparison always yields an
i1. If ``T`` is a primitive type, the resulting type is i1. If ``T`` is a
vector, then the resulting type is a vector of i1 with the same size as ``T``.

Legal test conditions are:

===== == ==================================
C     CC Operator
===== == ==================================
false  0 Always false
oeq    1 Ordered and equal
ogt    2 Ordered and greater than
oge    3 Ordered and greater than or equal
olt    4 Ordered and less than
ole    5 Ordered and less than or equal
one    6 Ordered and not equal
ord    7 Ordered (no NaNs)
uno    8 Unordered (either NaNs)
ueq    9 Unordered or equal
ugt   10 Unordered or greater than
uge   11 Unordered or greater than or equal
ult   12 Unordered or less than
ule   13 Unordered or less than or equal
une   14 Unordered or not equal
true  15 Always true
===== == ==================================

**Constraints**::

  AA == AbbrevIndex(A) &
  IsFloat(UnderlyingType(T) &
  T == TypeOf(V1) == TypeOf(V2) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  if IsVector(T) then
    TypeOf(%vN) = <UnderlyingCount(T), i1>
  else
    TypeOf(%vN) = i1
  endif

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <3>                   |    @t0 = float;
      52:2|    3: <7, 1>                |    @t1 = i1;
      54:6|    3: <2>                   |    @t2 = void;
      56:4|    3: <21, 0, 2>            |    @t3 = void ();
      59:6|  0: <65534>                 |  }
      ...
     108:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     116:0|    3: <1, 1>                |    blocks 1;
     118:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     128:0|      3: <1, 0>              |      float:
     130:4|      3: <6, 0>              |        %c0 = float 0;
     133:0|      3: <6, 1065353216>     |        %c1 = float 1;
     139:2|    0: <65534>               |      }
          |                             |  %b0:
     140:0|    3: <28, 2, 1, 0>         |    %v0 = fcmp false float %c0, %c1;
     144:0|    3: <28, 3, 2, 1>         |    %v1 = fcmp oeq float %c0, %c1;
     148:0|    3: <28, 4, 3, 2>         |    %v2 = fcmp ogt float %c0, %c1;
     152:0|    3: <28, 5, 4, 3>         |    %v3 = fcmp oge float %c0, %c1;
     156:0|    3: <28, 6, 5, 4>         |    %v4 = fcmp olt float %c0, %c1;
     160:0|    3: <28, 7, 6, 5>         |    %v5 = fcmp ole float %c0, %c1;
     164:0|    3: <28, 8, 7, 6>         |    %v6 = fcmp one float %c0, %c1;
     168:0|    3: <28, 9, 8, 7>         |    %v7 = fcmp ord float %c0, %c1;
     172:0|    3: <28, 10, 9, 9>        |    %v8 = fcmp ueq float %c0, %c1;
     176:0|    3: <28, 11, 10, 10>      |    %v9 = fcmp ugt float %c0, %c1;
     180:0|    3: <28, 12, 11, 11>      |    %v10 = fcmp uge float %c0, %c1;
     184:0|    3: <28, 13, 12, 12>      |    %v11 = fcmp ult float %c0, %c1;
     188:0|    3: <28, 14, 13, 13>      |    %v12 = fcmp ule float %c0, %c1;
     192:0|    3: <28, 15, 14, 14>      |    %v13 = fcmp une float %c0, %c1;
     196:0|    3: <28, 16, 15, 8>       |    %v14 = fcmp uno float %c0, %c1;
     200:0|    3: <28, 17, 16, 15>      |    %v15 = fcmp true float %c0, %c1;
     204:0|    3: <10>                  |    ret void;
     205:6|  0: <65534>                 |  }
     208:0|0: <65534>                   |}

.. _link_for_vector_instructions:

Vector Instructions
===================

PNaClAsm supports several instructions that process vectors. This includes the
:ref:`integer<link_for_integer_binary_instructions>` and :ref:`floating
point<link_for_floating_point_binary_instructions>` binary instructions as well
as :ref:`compare<link_for_compare_instructions>` instructions. These
instructions work with vectors and generate resulting (new) vectors. This
section introduces the instructions to construct vectors and extract results.

.. _link_for_insert_element_instruction_section:

Insert Element Instruction
--------------------------

The *insert element* instruction inserts a scalar value into a vector at a
specified index. The *insert element* instruction takes an existing vector and
puts a scalar value in one of the elements of the vector.

The *insert element* instruction can be used to construct a vector, one element
at a time.  At first glance, it may appear that one can't construct a vector,
since the *insert element* instruction needs a vector to insert elements into.

The key to understanding vector construction is understand that one can create
an :ref:`undefined<link_for_undefined_literal>` vector literal. Using that
constant as a starting point, one can built up the wanted vector by a sequence
of *insert element* instructions.

**Syntax**::

  %vN = insertelement TV V, TE E, i32 I;          <A>

**Record**::

  AA: <7, VV, EE, II>

**Semantics**:

The *insert element* instruction inserts scalar value ``E`` into index ``I`` of
vector ``V``. ``%vN`` holds the updated vector. Type ``TV`` is the type of
vector.  It is also the type of updated vector ``%vN``.  Type ``TE`` is the type
of scalar value ``E`` and must be the element type of vector ``V``. ``I`` must
be an :ref:`i32 literal<link_for_integer_literal>`.

If ``I`` exceeds the length of ``V``, the result is undefined.

**Constraints**::

  AA == AbbrevIndex(A) &
  IsVector(TV) &
  TypeOf(V) == TV &
  UnderlyingType(TV) == TE &
  TypeOf(I) == i32 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = TV;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 5>                |    count 5;
      50:4|    3: <7, 1>                |    @t0 = i1;
      53:0|    3: <12, 4, 0>            |    @t1 = <4 x i1>;
      56:2|    3: <7, 32>               |    @t2 = i32;
      59:4|    3: <2>                   |    @t3 = void;
      61:2|    3: <21, 0, 3>            |    @t4 = void ();
      64:4|  0: <65534>                 |  }
      ...
     116:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     124:0|    3: <1, 1>                |    blocks 1;
     126:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     136:0|      3: <1, 0>              |      i1:
     138:4|      3: <4, 0>              |        %c0 = i1 0;
     141:0|      3: <4, 3>              |        %c1 = i1 1;
     143:4|      3: <1, 1>              |      <4 x i1>:
     146:0|      3: <3>                 |        %c2 = <4 x i1> undef;
     147:6|      3: <1, 2>              |      i32:
     150:2|      3: <4, 0>              |        %c3 = i32 0;
     152:6|      3: <4, 2>              |        %c4 = i32 1;
     155:2|      3: <4, 4>              |        %c5 = i32 2;
     157:6|      3: <4, 6>              |        %c6 = i32 3;
     160:2|    0: <65534>               |      }
          |                             |  %b0:
     164:0|    3: <7, 5, 7, 4>          |    %v0  =  insertelement <4 x i1> %c2,
          |                             |        i1 %c0, i32 %c3;
     168:0|    3: <7, 1, 7, 4>          |    %v1  =  insertelement <4 x i1> %v0,
          |                             |        i1 %c1, i32 %c4;
     172:0|    3: <7, 1, 9, 4>          |    %v2  =  insertelement <4 x i1> %v1,
          |                             |        i1 %c0, i32 %c5;
     176:0|    3: <7, 1, 9, 4>          |    %v3  =  insertelement <4 x i1> %v2,
          |                             |        i1 %c1, i32 %c6;
     180:0|    3: <10>                  |    ret void;
     181:6|  0: <65534>                 |  }

Extract Element Instruction
---------------------------

The *extract element* instruction extracts a single scalar value from a vector
at a specified index.

**Syntax**::

  %vN = extractelement TV V, i32 I;                <A>

**Record**::

  AA: <6, VV, II>

**Semantics**:

The *extract element* instruction extracts the scalar value at index ``I`` from
vector ``V``. The extracted value is assigned to ``%vN``. Type ``TV`` is the
type of vector ``V``. ``I`` must be an :ref:`i32
literal<link_for_integer_literal>`. The type of ``vN`` must be the element type
of vector ``V``.

If ``I`` exceeds the length of ``V``, the result is undefined.

**Constraints**::

  AA == AbbrevIndex(A) &
  IsVector(TV) &
  TypeOf(V) == TV &
  TypeOf(I) == i32 &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = UnderlyingType(TV);

**Examples**::

      96:0|  1: <65535, 12, 2>          |  function void @f0(<4 x i32> %p0) {  
          |                             |                   // BlockID = 12
     104:0|    3: <1, 1>                |    blocks 1;
     106:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     116:0|      3: <1, 0>              |      i32:
     118:4|      3: <4, 0>              |        %c0 = i32 0;
     121:0|    0: <65534>               |      }
          |                             |  %b0:
     124:0|    3: <6, 2, 1>             |    %v0  =  
          |                             |        extractelement <4 x i32> %p0, 
          |                             |        i32 %c0;
     127:2|    3: <10>                  |    ret void;
     129:0|  0: <65534>                 |  }

.. _link_for_other_pnaclasm_instructions:

Other Instructions
==================

This section defines miscellaneous instructions which defy better
classification.

.. _link_for_forward_type_declaration_section:

Forward Type Declaration
------------------------

The forward type declaration exists to deal with the fact that all instruction
values must have a type associated with them before they are used. For some
simple functions one can easily topologically sort instructions so that
instruction values are defined before they are used. However, if the
implementation contains loops, the loop induced values can't be defined before
they are used.

The solution is to forward declare the type of an instruction value. One could
forward declare the types of all instructions at the beginning of the function
block.  However, this would make the corresponding file size considerably
larger. Rather, one should only generate these forward type declarations
sparingly and only when needed.

**Syntax**::

  declare T %vN;                                  <A>

**Record**::

  AA: <43, N, TT>

**Semantics**:

The forward declare type declaration defines the type to be associated with a
(not yet defined) instruction value ``%vN``. ``T`` is the type of the value
generated by the ``Nth`` value generating instruction in the function block.

Note: It is an error to define the type of ``%vN`` with a different type than
will be generated by the ``Nth`` value generating instruction in the function
block.

Also note that this construct is a declaration and not considered an
instruction, even though it appears in the list of instruction records. Hence,
they may appear before and between :ref:`phi<link_for_phi_instruction_section>`
instructions in a basic block.

**Constraints**::

  AA = AbbrevIndex(A) &
  TT = TypeID(T)

**Updates**::

  TypeOf(%vN) = T;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <2>                   |    @t1 = void;
      55:4|    3: <7, 1>                |    @t2 = i1;
      58:0|    3: <21, 0, 1, 0>         |    @t3 = void (i32);
      62:0|  0: <65534>                 |  }
      ...
     108:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0) {  
          |                             |                   // BlockID = 12
     116:0|    3: <1, 7>                |    blocks 7;
     118:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     128:0|      3: <1, 2>              |      i1:
     130:4|      3: <4, 3>              |        %c0 = i1 1;
     133:0|    0: <65534>               |      }
          |                             |  %b0:
     136:0|    3: <11, 4>               |    br label %b4;
          |                             |  %b1:
     138:4|    3: <43, 6, 0>            |    declare i32 %v3;
     142:4|    3: <2, 2, 4294967293, 0> |    %v0 = add i32 %p0, %v3;
     151:0|    3: <11, 6>               |    br label %b6;
          |                             |  %b2:
     153:4|    3: <43, 7, 0>            |    declare i32 %v4;
     157:4|    3: <2, 3, 4294967293, 0> |    %v1 = add i32 %p0, %v4;
     166:0|    3: <11, 6>               |    br label %b6;
          |                             |  %b3:
     168:4|    3: <2, 4, 4294967295, 0> |    %v2 = add i32 %p0, %v3;
     177:0|    3: <11, 6>               |    br label %b6;
          |                             |  %b4:
     179:4|    3: <2, 5, 5, 0>          |    %v3 = add i32 %p0, %p0;
     183:4|    3: <11, 1, 5, 5>         |    br i1 %c0, label %b1, label %b5;
          |                             |  %b5:
     187:4|    3: <2, 1, 6, 0>          |    %v4 = add i32 %v3, %p0;
     191:4|    3: <11, 2, 3, 6>         |    br i1 %c0, label %b2, label %b3;
          |                             |  %b6:
     195:4|    3: <10>                  |    ret void;
     197:2|  0: <65534>                 |  }

.. _link_for_phi_instruction_section:

Phi Instruction
---------------

The *phi* instruction is used to implement phi nodes in the SSA graph
representing the function. Phi instructions can only appear at the beginning of
a basic block.  There must be no non-phi instructions (other than forward type
declarations) between the start of the basic block and the *phi* instruction.

To clarify the origin of each incoming value, the incoming value is associated
with the incoming edge from the corresponding predecessor block that the
incoming value comes from.

**Syntax**::

  %vN = phi T [V1, %bB1], ... , [VM, %bBM];        <A>

**Record**::

  AA: <16, TT, VV1, B1, ..., VVM, BM>

**Semantics**:

The phi instruction is used to implement phi nodes in the SSA graph representing
the function. ``%vN`` is the resulting value of the corresponding phi
node. ``T`` is the type of the phi node. Values ``V1`` through ``VM`` are the
reaching definitions for the phi node while ``%bB1`` through ``%bBM`` are the
corresponding predecessor blocks.  Each ``VI`` reaches via the incoming
predecessor edge from block ``%bBI`` (for 1 <= I <= M). Type ``T`` must be the
type associated with each ``VI``.

**Constraints**::

  AA == AbbrevIndex(A) &
  M > 1 &
  TT == TypeID(T) &
  T = TypeOf(VI) for all I, 1 <= I <= M &
  BI < ExpectedBasicBlocks for all I, 1 <= I <= M &
  VVI = SignRotate(RelativeIndex(VI)) for all I, 1 <= I <= M &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 4>                |    count 4;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <2>                   |    @t1 = void;
      55:4|    3: <21, 0, 1>            |    @t2 = void ();
      58:6|    3: <7, 1>                |    @t3 = i1;
      61:2|  0: <65534>                 |  }
      ...
     112:0|  1: <65535, 12, 2>          |  function void @f0() {  
          |                             |                   // BlockID = 12
     120:0|    3: <1, 4>                |    blocks 4;
     122:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     132:0|      3: <1, 0>              |      i32:
     134:4|      3: <4, 2>              |        %c0 = i32 1;
     137:0|      3: <1, 3>              |      i1:
     139:4|      3: <4, 0>              |        %c1 = i1 0;
     142:0|    0: <65534>               |      }
          |                             |  %b0:
     144:0|    3: <11, 1, 2, 1>         |    br i1 %c1, label %b1, label %b2;
          |                             |  %b1:
     148:0|    3: <2, 2, 2, 0>          |    %v0 = add i32 %c0, %c0;
     152:0|    3: <2, 3, 3, 1>          |    %v1 = sub i32 %c0, %c0;
     156:0|    3: <11, 3>               |    br label %b3;
          |                             |  %b2:
     158:4|    3: <2, 4, 4, 2>          |    %v2 = mul i32 %c0, %c0;
     162:4|    3: <2, 5, 5, 3>          |    %v3 = udiv i32 %c0, %c0;
     166:4|    3: <11, 3>               |    br label %b3;
          |                             |  %b3:
     169:0|    3: <16, 0, 8, 1, 4, 2>   |    %v4 = phi i32 [%v0, %b1], 
          |                             |        [%v2, %b2];
     174:4|    3: <16, 0, 8, 1, 4, 2>   |    %v5 = phi i32 [%v1, %b1], 
          |                             |        [%v3, %b2];
     180:0|    3: <10>                  |    ret void;
     181:6|  0: <65534>                 |  }

Select Instruction
------------------

The *select* instruction is used to choose between pairs of values, based on a
condition, without PNaClAsm-level branching.

**Syntax**::

  %vN = select CT C, T V1, T V2;                <A>

**Record**::

  AA: <29, VV1, VV2, CC>

**Semantics**:

The *select* instruction chooses pairs of values ``V1`` and ``V2``, based on
condition value ``C``.  The type ``CT`` of value ``C`` must either be an i1, or
a vector of type i1. The type of values ``V1`` and ``V2`` must be of type
``T``. Type ``T`` must either be a primitive type, or a vector of a primitive
type.

Both ``CT`` and ``T`` must be primitive types, or both must be vector types of
the same size. When the contents of ``C`` is 1, the corresponding value from
``V1`` will be chosen. Otherwise the corresponding value from ``V2`` will be
chosen.

**Constraints**::

  AA == AbbrevIndex(A) &
  CC == RelativeIndex(C) &
  VV1 == RelativeIndex(V1) &
  VV2 == RelativeIndex(V2) &
  T == TypeOf(V1) == TypeOf(V2) &
  UnderlyingType(CT) == i1 &
  IsInteger(UnderlyingType(T)) or IsFloat(UnderlyingType(T)) &
  UnderlyingCount(C) == UnderlyingCount(T) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = T;

**Examples**::

      96:0|  1: <65535, 12, 2>          |  function i32 @f0(i32 %p0, i32 %p1) { 
          |                             |                   // BlockID = 12
     104:0|    3: <1, 1>                |    blocks 1;
     106:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     116:0|      3: <1, 2>              |      i1:
     118:4|      3: <4, 3>              |        %c0 = i1 1;
     121:0|    0: <65534>               |      }
          |                             |  %b0:
     124:0|    3: <29, 3, 2, 1>         |    %v0 = select i1 %c0, i32 %p0, 
          |                             |        i32 %p1;
     128:0|    3: <10, 1>               |    ret i32 %v0;
     130:4|  0: <65534>                 |  }


Call Instructions
-----------------

The *call* instruction does a function call. The call instruction is used to
cause control flow to transfer to a specified routine, with its incoming
arguments bound to the specified values. When a return instruction in the called
function is reached, control flow continues with the instruction after the
function call. If the call is to a function, the returned value is the value
generated by the call instruction. Otherwise no result is defined by the call.

If the *tail* flag is associated with the call instruction, then the :ref:`PNaCl
translator<link_for_pnacl_translator>` is free to perform tail call
optimization. That is, the *tail* flag is a hint that may be ignored by the
PNaCl translator.

There are two kinds of calls: *direct* and *indirect*. A *direct* call calls a
defined :ref:`function address<link_for_function_address_section>` (i.e. a
reference to a bitcode ID of the form ``%fF``). All other calls are *indirect*.

Direct Procedure Call
^^^^^^^^^^^^^^^^^^^^^

The direct procedure call calls a defined :ref:`function
address<link_for_function_address_section>` whose :ref:`type
signature<link_for_function_type>` returns type void.

**Syntax**::

  TAIL call void @fF (T1 A1, ... , TN AN);        <A>

**Record**::

  AA: <34, CC, F, AA1, ... , AAN>

**Semantics**:

The direct procedure call calls a define function address ``%fF`` whose type
signature return type is void. The arguments ``A1`` through ``AN`` are passed in
the order specified. The type of argument ``AI`` must be type ``TI`` (for all I,
1 <=I <= N).  Flag ``TAIL`` is optional. If it is included, it must be the
literal ``tail``.

The types of the arguments must match the corresponding types of the function
signature associated with ``%fF``. The return type of ``%f`` must be void.

TAIL is encoded into calling convention value ``CC`` as follows:

====== ==
TAIL   CC
====== ==
""     0
"tail" 1
====== ==

**Constraints**::

  AA == AbbrevIndex(A) &
  N >= 0 &
  TypeOfFcn(%fF) == void (T1, ... , TN) &
  TypeOf(AI) == TI for all I, 1 <= I <= N

**Updates**::

  ++NumValuedInsts;

**Examples**::

      72:0|  3: <8, 3, 0, 1, 0>         |  declare external 
          |                             |      void @f0(i32, i64, i32);
      ...
     116:0|  1: <65535, 12, 2>          |  function void @f1(i32 %p0) {  
          |                             |                   // BlockID = 12
     124:0|    3: <1, 1>                |    blocks 1;
     126:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     136:0|      3: <1, 2>              |      i64:
     138:4|      3: <4, 2>              |        %c0 = i64 1;
     141:0|    0: <65534>               |      }
          |                             |  %b0:
     144:0|    3: <34, 0, 4, 2, 1, 2>   |    call void 
          |                             |        @f0(i32 %p0, i64 %c0, i32 %p0);
     150:2|    3: <10>                  |    ret void;
     152:0|  0: <65534>                 |  }

Direct Function Call
^^^^^^^^^^^^^^^^^^^^

The direct function call calls a defined function address whose type signature
returns a value.

**Syntax**::

  %vN = TAIL call RT %fF (T1 A1, ... , TM AM);  <A>


**Record**::

  AA: <34, CC, F, AA1, ... , AAM>

**Semantics**:

The direct function call calls a defined function address ``%fF`` whose type
signature returned is not type void. The arguments ``A1`` through ``AM`` are
passed in the order specified. The type of argument ``AI`` must be type ``TI``
(for all I, 1 <= I <= N).  Flag ``TAIL`` is optional. If it is included, it must
be the literal ``tail``.

The types of the arguments must match the corresponding types of the function
signature associated with ``%fF``. The return type must match ``RT``.

Each parameter type ``TI``, and return type ``RT``, must either be a primitive
type, or a vector type.  If the parameter type is an integer type, it must
either be i32 or i64.

TAIL is encoded into calling convention value ``CC`` as follows:

====== ==
TAIL   CC
====== ==
""     0
"tail" 1
====== ==

**Constraints**::

  AA == AbbrevIndex(A) &
  N >= 0 &
  TypeOfFcn(%fF) == RT (T1, ... , TN) &
  TypeOf(AI) == TI for all I, 1 <= I <= M &
  IsFcnArgType(TI) for all I, 1 <= I <= M &
  IsFcnArgType(RT) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = RT;

**Examples**::

      72:0|  3: <8, 2, 0, 1, 0>         |  declare external 
          |                             |      i32 @f0(i32, i64, i32);
      ...
     116:0|  1: <65535, 12, 2>          |  function i32 @f1(i32 %p0) {  
          |                             |                   // BlockID = 12
     124:0|    3: <1, 1>                |    blocks 1;
     126:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     136:0|      3: <1, 1>              |      i64:
     138:4|      3: <4, 2>              |        %c0 = i64 1;
     141:0|    0: <65534>               |      }
          |                             |  %b0:
     144:0|    3: <34, 0, 4, 2, 1, 2>   |    %v0 = call i32 
          |                             |        @f0(i32 %p0, i64 %c0, i32 %p0);
     150:2|    3: <34, 1, 4, 1>         |    %v1 = tail call i32 @f1(i32 %v0);
     155:0|    3: <10, 2>               |    ret i32 %v0;
     157:4|  0: <65534>                 |  }

Indirect Procedure Call
^^^^^^^^^^^^^^^^^^^^^^^

The indirect procedure call calls a function using an indirect function address,
and whose type signature is assumed to return type void. It is different from
the direct procedure call because we can't use the type signature of the
corresponding direct function address to type check the construct.

**Syntax**::

  TAIL call void V (T1 A1, ... , TN AN);        <A>

**Record**::

  AA: <44, CC, TV, VV, AA1, ... , AAN>

**Semantics**:

The indirect call procedure calls a function using value ``V`` that is an
indirect function address, and whose type signature is assumed to return type
void.  The arguments ``A1`` through ``AN`` are passed in the order
specified. The type of argument ``AI`` must be type ``TI`` (for all I, 1 <= I <=
N).  Flag ``TAIL`` is optional. If it is included, it must be the literal
``tail``.

Each parameter type ``TI`` (1 <= I <= N) must either be a primitive type, or a
vector type.  If the parameter type is an integer type, it must either be i32
or i64.

TAIL is encoded into calling convention value ``CC`` as follows:

====== ==
TAIL   CC
====== ==
""     0
"tail" 1
====== ==

The type signature of the called procedure is assumed to be::

  void (T1, ... , TN)

It isn't necessary to define this type in the :ref:`types
block<link_for_types_block_section>`, since the type is inferred rather than
used.

**Constraints**::

  AA == AbbrevIndex(A) &
  N >= 0 &
  TV = TypeID(void) &
  AbsoluteIndex(V) >= NumFuncAddresses &
  TypeOf(AI) == TI for all I, 1 <= I <= N &
  IsFcnArgType(TI) for all I, 1 <= I <= N

**Updates**::

  ++NumValuedInsts;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 3>                |    count 3;
      50:4|    3: <2>                   |    @t0 = void;
      52:2|    3: <7, 32>               |    @t1 = i32;
      55:4|    3: <21, 0, 0, 1>         |    @t2 = void (i32);
      59:4|  0: <65534>                 |  }
      ...
      92:0|  1: <65535, 12, 2>          |  function void @f0(i32 %p0) {  
          |                             |                   // BlockID = 12
     100:0|    3: <1, 1>                |    blocks 1;
     102:4|    1: <65535, 11, 2>        |    constants {  // BlockID = 11
     112:0|      3: <1, 1>              |      i32:
     114:4|      3: <4, 2>              |        %c0 = i32 1;
     117:0|    0: <65534>               |      }
          |                             |  %b0:
     120:0|    3: <44, 0, 2, 0, 1>      |    call void %p0(i32 %c0);
     125:4|    3: <10>                  |    ret void;
     127:2|  0: <65534>                 |  }

Indirect Function Call
^^^^^^^^^^^^^^^^^^^^^^

The indirect function call calls a function using a value that is an indirect
function address. It is different from the direct function call because we can't
use the type signature of the corresponding literal function address to type
check the construct.

**Syntax**::

  %vN = TAIL call RT V (T1 A1, ... , TM AM);  <A>

**Record**::

  AA: <34, CC, RRT, VV, AA1, ... , AAM>

**Semantics**:

The indirect function call calls a function using a value ``V`` that is an
indirect function address, and is assumed to return type ``RT``.  The arguments
``A1`` through ``AM`` are passed in the order specified. The type of argument
``AI`` must be type ``TI`` (for all I, 1 <= I <= N).  Flag ``TAIL`` is
optional. If it is included, it must be the literal ``tail``.

Each parameter type ``TI`` (1 <= I <= M), and return type ``RT``, must either be
a primitive type, or a vector type.  If the parameter type is an integer type,
it must either be i32 or i64.

TAIL is encoded into calling convention value ``CC`` as follows:

====== ==
TAIL   CC
====== ==
''     0
'tail' 1
====== ==

The type signature of the called function is assumed to be::

   RT (T1, ... , TN)

It isn't necessary to define this type in the :ref:`types
block<link_for_types_block_section>`, since the type is inferred rather than
used.

**Constraints**::

  AA == AbbrevIndex(A) &
  RRT = TypeID(RT) &
  VV = RelativeIndex(V) &
  M >= 0 &
  AbsoluteIndex(V) >= NumFcnAddresses &
  TypeOf(AI) == TI for all I, 1 <= I <= M &
  IsFcnArgType(TI) for all I, 1 <= I <= M &
  IsFcnArgType(RT) &
  N == NumValuedInsts

**Updates**::

  ++NumValuedInsts;
  TypeOf(%vN) = RT;

**Examples**::

      40:0|  1: <65535, 17, 2>          |  types {  // BlockID = 17
      48:0|    3: <1, 6>                |    count 6;
      50:4|    3: <7, 32>               |    @t0 = i32;
      53:6|    3: <3>                   |    @t1 = float;
      55:4|    3: <4>                   |    @t2 = double;
      57:2|    3: <21, 0, 0, 0, 1, 2>   |    @t3 = i32 (i32, float, double);
      62:6|    3: <21, 0, 0, 1, 2>      |    @t4 = i32 (float, double);
      67:4|    3: <2>                   |    @t5 = void;
      69:2|  0: <65534>                 |  }
      ...
     104:0|  1: <65535, 12, 2>          |  function 
          |                             |      i32 
          |                             |      @f0(i32 %p0, float %p1, 
          |                             |          double %p2) {  
          |                             |                   // BlockID = 12
     112:0|    3: <1, 1>                |    blocks 1;
          |                             |  %b0:
     114:4|    3: <44, 0, 3, 0, 2, 1>   |    %v0 = call i32 
          |                             |        %p0(float %p1, double %p2);
     120:6|    3: <10, 1>               |    ret i32 %v0;
     123:2|  0: <65534>                 |  }

.. _link_for_memory_blocks_and_alignment_section:

Memory Blocks and Alignment
===========================

In general, variable and heap allocated data are represented as byte addressable
memory blocks. Alignment is always a power of 2, and defines an expectation on
the memory address. That is, an alignment is met if the memory address is
(evenly) divisible by the alignment. Note that alignment of 0 is never allowed.

 Alignment plays a role at two points:

* When you create a local/global variable

* When you load/store data using a pointer.

PNaClAsm allows most types to be placed at any address, and therefore can
have alignment of 1. However, many architectures can load more efficiently
if the data has an alignment that is larger than 1. As such, choosing a larger
alignment can make load/stores more efficient.

On loads and stores, the alignment in the instruction is used to communicate
what assumptions the :ref:`PNaCl translator<link_for_pnacl_translator>` can
make when choosing the appropriate machine instructions. If the alignment is 1,
it can't assume anything about the memory address used by the instruction. When
the alignment is greater than one, it can use that information to potentially
chose a more efficient sequence of instructions to do the load/store.

When laying out data within a variable, one also considers alignment. The reason
for this is that if you want an address to be aligned, within the bytes defining
the variable, you must choose an alignment for the variable that guarantees that
alignment.

In PNaClAsm, the valid load/store alignments are:

=========== ==============
Type        Alignment
=========== ==============
i1          1
i8          1
i16         1
i32         1
i64         1
Float       1, 4
Double      1, 8
<4 x i1>    not applicable
<8 x i1>    not applicable
<16 x i1>   not applicable
<16 x i8>   1
<8 x i16>   2
<4 x i32>   4
<4 x float> 4
=========== ==============

Note that only vectors do not have an alignment value of 1. Hence, they can't be
placed at an arbitrary memory address. Also, since vectors on ``i1`` can't be
loaded/stored, the alignment is not applicable for these types.

.. _link_for_intrinsic_functions_section:

Intrinsic Functions
===================

Intrinsic functions are special in PNaClAsm. They are implemented as specially
named (external) function calls. The purpose of these intrinsic functions is to
extend the PNaClAsm instruction set with additional functionality that is
architecture specific. Hence, they either can't be implemented within PNaClAsm,
or a non-architecture specific implementation may be too slow on some
architectures. In such cases, the :ref:`PNaCl
translator<link_for_pnacl_translator>` must fill in the corresponding
implementation, since only it knows the architecture it is compiling down to.

Examples of intrinsic function calls are for concurrent operations, atomic
operations, bulk memory moves, thread pointer operations, and long jumps.

It should be noted that calls to intrinsic functions do not have the same
calling type constraints as ordinary functions. That is, an intrinsic can use
any integer type for arguments/results, unlike ordinary functions (which
restrict integer types to ``i32`` and ``i64``).

See the :doc:`PNaCl bitcode reference manual<pnacl-bitcode-abi>` for the full
set of intrinsic functions allowed. Note that in PNaClAsm, all pointer types to
an (LLVM) intrinsic function is converted to type i32.

.. _link_for_support_functions_section:

Support Functions
=================

Defines functions used to convert syntactic representation to values in the
corresponding record.

SignRotate
----------

The SignRotate function encodes a signed integer in an easily compressible
form. This is done by rotating the sign bit to the rightmost bit, rather than
the leftmost bit. By doing this rotation, both small positive and negative
integers are small (unsigned) integers. Therefore, all small integers can be
encoded as a small (unsigned) integers.

The definition of SignRotate(N) is:

======== ============= =========
Argument Value         Condition
======== ============= =========
N        abs(N)<<1     N >= 0
N        abs(N)<<1 + 1 N < 0
======== ============= =========

.. _link_for_absolute_index_section:

AbsoluteIndex
-------------

Bitcode IDs of the forms ``@fN``, ``@gN``, ``%pN``, ``%cN``, and ``%vN``, are
combined into a single index space. This can be done because of the ordering
imposed by PNaClAsm. All function address bitcode IDs must be defined before any
of the other forms of bitcode IDs. All global address bitcode IDs must be
defined before any local bitcode IDs. Within a function block, the parameter
bitcode IDs must be defined before constant IDs, and constant IDs must be
defined before instruction value IDs.

Hence, within a function block, it is safe to refer to all of these
bitcode IDs using a single *absolute* index. The absolute index for
each kind of bitcode ID is computed as follows:

========== ===================================================================
Bitcode ID AbsoluteIndex
========== ===================================================================
@tN        N
@fN        N
@gN        N + NumFcnAddresses
@pN        N + NumFcnAddresses + NumGlobalAddresses
@cN        N + NumFcnAddresses + NumGlobalAddresses + NumParams
@vN        N + NumFcnAddresses + NumGlobalAddresses + NumParams + NumFcnConsts
========== ===================================================================

.. _link_for_relative_index:

RelativeIndex
-------------

Relative indices are used to refer to values within instructions of a function.
The relative index of an ID is always defined in terms of the index associated
with the next value generating instruction. It is defined as follows::

   RelativeIndex(J) = AbsoluteIndex(%vN) - AbsoluteIndex(J)

where::

   N = NumValuedInsts

AbbrevIndex
-----------

This function converts user-defined abbreviation indices to the corresponding
internal abbreviation index saved in the bitcode file. It adds 4 to its
argument, since there are 4 predefined internal abbreviation indices (0, 1, 2,
and 3).

========= ==============
N         AbbrevIndex(N)
========= ==============
undefined 3
%aA       A + 4
@aA       A + 4
========= ==============

Log2
----

This is the 32-bit log2 value of its argument.

BitSizeOf
---------

Returns the number of bits needed to represent its argument (a type).

======= ================
T       BitSizeOf
======= ================
i1       1
i8       8
i16     16
i32     32
i64     64
float   32
double  64
<N X T> N * BitSizeOf(T)
======= ================

UnderlyingType
--------------

Returns the primitive type of the type construct. For primitive types, the
*UnderlyingType* is itself. For vector types, the base type of the vector is the
underlying type.

UnderlyingCount
---------------

Returns the size of the vector if given a vector, and 0 for primitive types.
Note that this function is used to check if two vectors are of the same size.
It is also used to test if two types are either primitive (i.e. UnderlyingCount
returns 0 for both types) or are vectors of the same size (i.e. UnderlyingCount
returns the same non-zero value).

IsInteger
---------

Returns true if the argument is in {i1, i8, i16, i32, i64}.

IsFloat
-------

Returns true if the argument is in {``float``, ``double``}.

IsVector
--------

Returns true if the argument is a vector type.

IsPrimitive
-----------

Returns true if the argument is a primitive type. That is::

  IsPrimitive(T) == IsInteger(T) or IsFloat(T)

IsFcnArgType
------------

Returns true if the argument is a primitive type or a vector type. Further,
if it is an integer type, it must be i32 or i64. That is::

  IsFcnArgType(T) = (IsInteger(T) and (i32 = BitSizeOf(T)
                                       or i64 == BitSizeOf(T)))
                    or IsFloat(T) or IsVector(T)

.. _link_for_abbreviations_section:

Abbreviations
=============

Abbreviations are used to convert PNaCl records to a sequence of bits. PNaCl
uses the same strategy as `LLVM's bitcode file format
<http://llvm.org/docs/BitCodeFormat.html>`_.  See that document for more
details.

It should be noted that we replace LLVM's header (called the *Bitcode Wrapper
Format*) with the bytes of the :ref:`PNaCl record
header<link_for_header_record_section>`.  In addition, PNaCl bitcode files do
not allow *blob* abbreviation.

.. _link_for_abbreviations_block_section:

Abbreviations Block
-------------------

The abbreviations block is the first block in the module build. The
block is divided into sections.  Each section is a sequence of records. Each
record in the sequence defines a user-defined abbreviation.  Each section
defines abbreviations that can be applied to all (succeeding) blocks of a
particular kind. These abbreviations are denoted by the (global) ID of the form
*@aN*.

In terms of `LLVM's bitcode file format
<http://llvm.org/docs/BitCodeFormat.html>`_, the abbreviations block is called a
*BLOCKINFO* block. Records *SETBID* and *DEFINE_ABBREV* are the only records
allowed in PNaCl's abbreviation block (i.e. it doesn't allow *BLOCKNAME* and
*SETRECORDNAME* records).

TODO
----

Extend this document to describe PNaCl's bitcode bit sequencer
without requiring the reader to refer to `LLVM's bitcode file
format <http://llvm.org/docs/BitCodeFormat.html>`_.
