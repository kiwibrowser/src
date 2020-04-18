Introduction
===============================================================

This document will as the C++ port matures serve as a log to how
different parts of the library work. As of today, there is some general
info but mostly CMap specific details.

------------------------------------------------------------------------

Font Data Tables
===========================================================================

One of the important goals in `sfntly` is thread safety which is why
tables can only be created with their nested `Builder` class and are
immutable after creation.

`CMapTable`
--------------------------------------------------------

*CMap* = character map; it converts *code points* in a *code page* to
*glyph IDs*.

The CMapTable is a table of CMaps (CMaps are also tables; one for every
encoding supported by the font). Representing an encoding-dependent
character map is in one of 14 formats, out of which formats 0 and 4 are
the most used; sfntly/C++ will initially only support formats 0, 2, 4
and 12.

### `CMapFormat0` Byte encoding table

Format 0 is a basic table where a character’s glyph ID is looked up in a
glyphIdArray256. As it only supports 256 characters it can only encode
ASCII and ISO 8859-x (alphabet-based languages).

### `CMapFormat2` High-byte mapping through table

Chinese, Japanese and Korean (CJK) need special 2 byte encodings for
each code point like Shift-JIS.

### `CMapFormat4` Segment mapping to delta values

This is the preferred format for Unicode Basic Multilingual Plane (BMP)
encodings according to the Microsoft spec. Format 4 defines segments
(contiguous ranges of characters; variable length). Finding a
character’s glyph id first means finding the segment it is part of using
a binary search (the segments are sorted). A segment has a
**`startCode`**, an **`endCode`** (the minimum and maximum code points
in the segment), an **`idDelta`** (delta for all code points in the
segment) and an **`idRangeOffset`** (offset into glyphIdArray or 0).

`idDelta` and `idRangeOffset` seem to be the same thing, offsets. In
fact, `idRangeOffset` uses the glyph array to get the index by relying
on the fact that the array is immediately after the `idRangeOffset`
table in the font file. So, the segment’s offset is `idRangeOffset[i]`
but since the `idRangeOffset` table contains words and not bytes, the
value is divided by 2.

``` {.prettyprint}
glyphIndex = *(&idRangeOffset[i] + idRangeOffset[i] / 2 + (c - startCode[i]))
```

`idDelta[i]` is another kind of segment offset used when
`idRangeOffset[i] = 0`, in which case it is added directly to the
character code.

``` {.prettyprint}
glyphIndex = idDelta[i] + c
```

### Class Hierarchy

`CMapTable` is the main class and the container for all other CMap
related classes.

#### Utility classes

-   `CMapTable::CMapId` describes a pair of IDs, platform ID and
    encoding ID that form the CMaps ID. The ID a CMap has is usually a
    good indicator as to what kind of format the CMap uses (Unicode
    CMaps are usually either format 4 or format 12).
-   `CMapTable::CMapIdComparator`
-   `CMapTable::CMapIterator` iteration through the CMapTable is
    supported through a Java-style iterator.
-   `CMapTable::CMapFilter` Java-style filter; CMapIterator supports
    filtering CMaps. By default, it accepts everything CMap.
-   `CMapTable::CMapIdFilter` extends CMapFilter; only accepts one type
    of CMap. Used in conjunction with CMapIterator, this is how the CMap
    getters are implemented.
-   **`CMapTable::Builder`** is the only way to create a CMapTable.

#### CMaps

-   **`CMapTable::CMap`** is the abstract base class that all
    `CMapFormat*` derive. It defines basic functions and the abstract
    `CMapTable::CMap::CharacterIterator` class to iterate through the
    characters in the map. The basic implementation just loops through
    every character between a start and an end. This is overridden so
    that format specific iteration is performed.
-   `CMapFormat0` (mostly done?)
-   `CMapFormat2` (needs builders)
-   ... coming soon

`[todo: will add images soon; need to upload to svn]`

------------------------------------------------------------------------

# Table Building Pipeline

Building a data table in sfntly is done by the
`FontDataTable::Builder::build` method which defines the general
pipeline and leaves the details to each implementing subclass
(`CMapTable::Builder` for example). Note: **`sub*`** methods are table
specific

**`ReadableFontDataPtr data = internalReadData()`**
> There are 2 private fields in the `FontDataTable::Builder` class:
> `rData` and `wData` for `ReadableFontData` and `WritableFontData`.
> This function returns `rData` if there is any or `wData` (it is cast
> to readable font data) if `rData` is null. *They hold the same data!*

**`if (model_changed_)`**
> A font is essentially a binary blob when loaded inside a `FontData`
> object. A *model* is the Java/C++ collection of objects that represent
> the same data in a manipulable format. If you ask for the model (even
> if you dont write to it), it will count as changed and the underlying
> raw data will get updated.

**`if (!subReadyToSerialize())`**
**`return NULL`**
`else`
1.  **`size = subDataToSerialize()`**
2.  **`WritableDataPtr new_data = container_->getNewData(size)`**
3.  **`subSerialize(new_data)`**
4.  **`data = new_data`**

**`FontDataTablePtr table = subBuildTable(data)`**
> The table is actually built, where `subBuildTable` is overridden by
> every class of table but a table header is always added.

Subtable Builders
------------------------------------------------------------------------------

Subtables are lazily built

When creating the object view of the font and dealing with lots of
tables, it would be wasteful to create builders for every subtable there
is since most users only do fairly high level manipulation of the font.
Instead, **only the tables at font level are fully built**.

All other subtables have builders that contain valid FontData but the
object view is not created by default. For the `CMapTable`, this means
that if you don’t go through the `getCMapBuilders()` method, the CMap
builders are not initialized. So, the builder map would seem to be empty
when calling its `size()` method but there are CMaps in the font when
calling `numCMaps(internalReadFont())`.

------------------------------------------------------------------------

Character encoders
---------------------------------------------------------------------------------

Sfntly/Java uses a native ICU-based API for encoding characters.
Sfntly/C++ uses ICU directly. In unit tests we assume text is encoded in
UTF16. Public APIs will use ICU classes like `UnicodeString`.
