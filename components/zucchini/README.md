
## Basic Definitions for Patching

**Binary**: Executable image and data. Binaries may persist in an archive
(e.g., chrome.7z), and need to be periodically updated. Formats for binaries
include {PE files EXE / DLL, ELF, DEX}. Architectures binaries include
{x86, x64, ARM, AArch64, Dalvik}. A binary is also referred to as an executable
or an image file.

**Patching**: Sending a "new" file to clients who have an "old" file by
computing and transmitting a "patch" that can be used to transform "old" into
"new". Patches are compressed for transmission. A key performance metric is
patch size, which refers to the size of compressed patch file. For our
experiments we use 7z.

**Patch generation**: Computation of a "patch" from "old" and "new". This can be
expensive (e.g., ~15-20 min for Chrome, using 1 GB of RAM), but since patch
generation is a run-once step on the server-side when releasing "new" binaries,
the expense is not too critical.

**Patch application**: Transformation from "old" binaries to "new", using a
(downloaded) "patch". This is executed on client side on updates, so resource
constraints (e.g., time, RAM, disk space) is more stringent. Also, fault-
tolerance is important. This is usually achieved by an update system by having
a fallback method of directly downloading "new" in case of patching failure.

**Offset**: Position relative to the start of a file.

**Local offset**: An offset relative to the start of a region of a file.

**Element**: A region in a file with associated executable type, represented by
the tuple (exe_type, offset, length). Every Element in new file is associated
with an Element in old file and patched independently.

**Reference**: A directed connection between two offsets in a binary. For
example, consider jump instructions in x86:

    00401000: E9 3D 00 00 00     jmp         00401042

Here, the 4 bytes `[3D 00 00 00]` starting at address `00401001` point to
address `00401042` in memory. This forms a reference from `offset(00401001)`
(length 4) to `offset(00401042)`, where `offset(addr)` indicates the disk
offset corresponding to `addr`. A reference has a location, length (implicitly
determined by reference type), body, and target.

**Location**: The starting offset of bytes that store a reference. In the
preceding example, `offset(00401001)` is a location. Each location is the
beginning of a reference body.

**Body**: The span of bytes that encodes reference data, i.e.,
[location, location + length) =
[location, location + 1, ..., location + length - 1].
In the preceding example, `length = 4`, so the reference body is
`[00401001, 00401001 + 4) = [00401001, 00401002, 00401003, 00401004]`.
All reference bodies in an image must not overlap, and often regions boundaries
are required to not straddle a reference body.

**Target**: The offset that's the destination of a reference. In the preceding
example, `offset(00401042)` is the target. Different references can share common
targets. For example, in

    00401000: E9 3D 00 00 00     jmp         00401042
    00401005: EB 3B              jmp         00401042

we have two references with different locations and bodies, but same target
of `00401042`.

Because the bytes that encode a reference depend on its target, and potentially
on its location, they are more likely to get modified from an old version of a
binary to a newer version. This is why "naive" patching does not do well on
binaries.

**Disassembler**: Architecture specific data and operations, used to extract and
correct references in a binary.

**Type of reference**: The type of a reference determines the binary
representation used to encode its target. This affects how references are parsed
and written by a disassembler. There can be many types of references in the same
binary.

A reference is represented by the tuple (disassembler, location, target, type).
This tuple contains sufficient information to write the reference in a binary.

**Pool of targets**: Collection of targets that is assumed to have some semantic
relationship. Each reference type belong to exactly one reference pool. Targets
for references in the same pool are shared.

For example, the following describes two pools defined for Dalvik Executable
format (DEX). Both pools spawn multiple types of references.

1. Index in string table.
  - From bytecode to string index using 16 bits.
  - From bytecode to string index using 32 bits.
  - From field item to string index using 32 bits.
2. Address in code.
  - Relative 16 bits pointer.
  - Relative 32 bits pointer.

Boundaries between different pools can be ambiguous. Having all targets belong
to the same pool can reduce redundancy, but will use more memory and might
cause larger corrections to happen, so this is a trade-off that can be resolved
with benchmarks.

**Abs32 references**: References whose targets are adjusted by the OS during
program load. In an image, a **relocation table** typically provides locations
of abs32 references. At each abs32 location, the stored bytes then encode
semantic information about the target (e.g., as RVA).

**Rel32 references**: References embedded within machine code, in which targets
are encoded as some delta relative to the reference's location. Typical examples
of rel32 references are branching instructions and instruction pointer-relative
memory access.

**Equivalence**: A (src_offset, dst_offset, length) tuple describing a region of
"old" binary, at an offset of |src_offset|, that is similar to a region of "new"
binary, at an offset of |dst_offset|.

**Raw delta unit**: Describes a raw modification to apply on the new image, as a
pair (copy_offset, diff), where copy_offset describes the position in new file
as an offset in the data that was copied from the old file, and diff is the
bytewise difference to apply.

**Associated Targets**: A target in "old" binary is associated with a target in
"new" binary if both targets:
1. are part of similar regions from the same equivalence, and
2. have the same local offset (relative to respective start regions), and
3. are not part of any larger region from a different equivalence.
Not all targets are necessarily associated with another target.

**Label**: An (offset, index) pair, where |offset| is a target, and |index| is
an integer used to uniquely identify |offset| in its corresponding pool of
targets. Labels are created for each Reference in "old" and "new" binary as part
of generating a patch, and used to alias targets when searching for similar
regions that will form equivalences. Labels are created such that associated
targets in old and new binaries share the same |index|, and such that indices in
a pool are tightly packed. For example, suppose "old" Labels are:
  - (0x1111, 0), (0x3333, 4), (0x5555, 1), (0x7777, 3)
and given the following association of targets between "old" and "new":
  - 0x1111 <=> 0x6666,  0x3333 <=> 0x2222.
then we could assign indices for "new" Labels as:
  - (0x2222, 4}, (0x4444, 8), (0x6666, 0), (0x8888, 2)

**Encoded Image**: The result of projecting the content of an image to scalar
values that describe content on a higher level of abstraction, masking away
undesirable noise in raw content. Notably, the projection encodes references
based on their associated label.

## Zucchini Ensemble Patch Format

### Types

**int8**: 8-bit unsigned int.

**uint32**: 32-bit unsigned int, little-endian.

**int32**:  32-bit signed int, little-endian.

**Varints**: This is a generic variable-length encoding for integer quantities
that strips away leading (most-significant) null bytes.
The Varints format is borrowed from protocol-buffers, see
[documentation](https://developers.google.com/protocol-buffers/docs/encoding#varints)
for more info.

**varuint32**: A uint32 encoded using Varints format.

**varint32**: A int32 encoded using Varints format.

### File Layout

Name | Format | Description
--- | --- | ---
header | PatchHeader | The header.
elements_count | uint32 | Number of patch units.
elements | PatchElement[elements_count] | List of all patch elements.

Position of elements in new file is ascending.

### Structures

**PatchHeader**

Name | Format | Description
--- | --- | ---
magic | uint32 = kMagic | Magic value.
old_size | uint32 | Size of old file in bytes.
old_crc | uint32 | CRC32 of old file.
new_size | uint32 | Size of new file in bytes.
new_crc | uint32 | CRC32 of new file.

**kMagic** == `'Z' | ('u' << 8) | ('c' << 16)`

**PatchElement**
Contains all the information required to produce a single element in new file.

Name | Format | Description
--- | --- | ---
header | PatchElementHeader | The header.
equivalences | EquivalenceList | List of equivalences.
raw_deltas | RawDeltaList | List of raw deltas.
reference_deltas | ReferenceDeltaList | List of reference deltas.
pool_count | uint32 | Number of pools.
extra_targets | ExtraTargetList[pool_count] | Lists of extra targets.

**PatchElementHeader**
Describes a correspondence between an element in old and in new files. Some
redundancy arise from storing |new_offset|, but it is necessary to make
PatchElement self contained.

Name | Format | Description
--- | --- | ---
old_offset | uint32 | Starting offset of the element in old file.
old_length | uint32 | Length of the element in old file.
new_offset | uint32 | Starting offset of the element in new file.
new_length | uint32 | Length of the element in new file.
exe_type | uint32 | Executable type for this unit, see `enum ExecutableType`.

**EquivalenceList**
Encodes a list of equivalences, where dst offsets (in new image) are ascending.

Name | Format | Description
--- | --- | ---
src_skip | Buffer<varint32> | Src offset for each equivalence, delta encoded.
dst_skip | Buffer<varuint32> | Dst offset for each equivalence, delta encoded.
copy_count | Buffer<varuint32> | Length for each equivalence.

**RawDeltaList**
Encodes a list of raw delta units, with ascending copy offsets.

Name | Format | Description
--- | --- | ---
raw_delta_skip | Buffer<varuint32> | Copy offset for each delta unit, delta encoded and biased by -1.
raw_delta_diff | Buffer<int8> | Bytewise difference for each delta unit.

**ReferenceDeltaList**
Encodes a list of reference deltas, in the order they appear in the new
image file. A reference delta is a signed integer representing a jump through a
list of targets.

Name | Format | Description
--- | --- | ---
reference_delta | Buffer<varuint32> | Vector of reference deltas.

**ExtraTargetList**
Encodes a list of additional targets in the new image file, in ascending
order.

Name | Format | Description
--- | --- | ---
pool_tag | uint8_t | Unique identifier for this pool of targets.
extra_targets | Buffer<varuint32> | Additional targets, delta encoded and biased by -1.

**Buffer<T>**
A generic vector of data.

Name | Format | Description
--- | --- | ---
size |uint32 | Size of content in bytes.
content |T[] | List of integers.
