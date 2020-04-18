/*
** 2012 Jan 11
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
*/
/* TODO(shess): THIS MODULE IS STILL EXPERIMENTAL.  DO NOT USE IT. */
/* Implements a virtual table "recover" which can be used to recover
 * data from a corrupt table.  The table is walked manually, with
 * corrupt items skipped.  Additionally, any errors while reading will
 * be skipped.
 *
 * Given a table with this definition:
 *
 * CREATE TABLE Stuff (
 *   name TEXT PRIMARY KEY,
 *   value TEXT NOT NULL
 * );
 *
 * to recover the data from teh table, you could do something like:
 *
 * -- Attach another database, the original is not trustworthy.
 * ATTACH DATABASE '/tmp/db.db' AS rdb;
 * -- Create a new version of the table.
 * CREATE TABLE rdb.Stuff (
 *   name TEXT PRIMARY KEY,
 *   value TEXT NOT NULL
 * );
 * -- This will read the original table's data.
 * CREATE VIRTUAL TABLE temp.recover_Stuff using recover(
 *   main.Stuff,
 *   name TEXT STRICT NOT NULL,  -- only real TEXT data allowed
 *   value TEXT STRICT NOT NULL
 * );
 * -- Corruption means the UNIQUE constraint may no longer hold for
 * -- Stuff, so either OR REPLACE or OR IGNORE must be used.
 * INSERT OR REPLACE INTO rdb.Stuff (rowid, name, value )
 *   SELECT rowid, name, value FROM temp.recover_Stuff;
 * DROP TABLE temp.recover_Stuff;
 * DETACH DATABASE rdb;
 * -- Move db.db to replace original db in filesystem.
 *
 *
 * Usage
 *
 * Given the goal of dealing with corruption, it would not be safe to
 * create a recovery table in the database being recovered.  So
 * recovery tables must be created in the temp database.  They are not
 * appropriate to persist, in any case.  [As a bonus, sqlite_master
 * tables can be recovered.  Perhaps more cute than useful, though.]
 *
 * The parameters are a specifier for the table to read, and a column
 * definition for each bit of data stored in that table.  The named
 * table must be convertable to a root page number by reading the
 * sqlite_master table.  Bare table names are assumed to be in
 * database 0 ("main"), other databases can be specified in db.table
 * fashion.
 *
 * Column definitions are similar to BUT NOT THE SAME AS those
 * provided to CREATE statements:
 *  column-def: column-name [type-name [STRICT] [NOT NULL]]
 *  type-name: (ANY|ROWID|INTEGER|FLOAT|NUMERIC|TEXT|BLOB)
 *
 * Only those exact type names are accepted, there is no type
 * intuition.  The only constraints accepted are STRICT (see below)
 * and NOT NULL.  Anything unexpected will cause the create to fail.
 *
 * ANY is a convenience to indicate that manifest typing is desired.
 * It is equivalent to not specifying a type at all.  The results for
 * such columns will have the type of the data's storage.  The exposed
 * schema will contain no type for that column.
 *
 * ROWID is used for columns representing aliases to the rowid
 * (INTEGER PRIMARY KEY, with or without AUTOINCREMENT), to make the
 * concept explicit.  Such columns are actually stored as NULL, so
 * they cannot be simply ignored.  The exposed schema will be INTEGER
 * for that column.
 *
 * NOT NULL causes rows with a NULL in that column to be skipped.  It
 * also adds NOT NULL to the column in the exposed schema.  If the
 * table has ever had columns added using ALTER TABLE, then those
 * columns implicitly contain NULL for rows which have not been
 * updated.  [Workaround using COALESCE() in your SELECT statement.]
 *
 * The created table is read-only, with no indices.  Any SELECT will
 * be a full-table scan, returning each valid row read from the
 * storage of the backing table.  The rowid will be the rowid of the
 * row from the backing table.  "Valid" means:
 * - The cell metadata for the row is well-formed.  Mainly this means that
 *   the cell header info describes a payload of the size indicated by
 *   the cell's payload size.
 * - The cell does not run off the page.
 * - The cell does not overlap any other cell on the page.
 * - The cell contains doesn't contain too many columns.
 * - The types of the serialized data match the indicated types (see below).
 *
 *
 * Type affinity versus type storage.
 *
 * http://www.sqlite.org/datatype3.html describes SQLite's type
 * affinity system.  The system provides for automated coercion of
 * types in certain cases, transparently enough that many developers
 * do not realize that it is happening.  Importantly, it implies that
 * the raw data stored in the database may not have the obvious type.
 *
 * Differences between the stored data types and the expected data
 * types may be a signal of corruption.  This module makes some
 * allowances for automatic coercion.  It is important to be concious
 * of the difference between the schema exposed by the module, and the
 * data types read from storage.  The following table describes how
 * the module interprets things:
 *
 * type     schema   data                     STRICT
 * ----     ------   ----                     ------
 * ANY      <none>   any                      any
 * ROWID    INTEGER  n/a                      n/a
 * INTEGER  INTEGER  integer                  integer
 * FLOAT    FLOAT    integer or float         float
 * NUMERIC  NUMERIC  integer, float, or text  integer or float
 * TEXT     TEXT     text or blob             text
 * BLOB     BLOB     blob                     blob
 *
 * type is the type provided to the recover module, schema is the
 * schema exposed by the module, data is the acceptable types of data
 * decoded from storage, and STRICT is a modification of that.
 *
 * A very loose recovery system might use ANY for all columns, then
 * use the appropriate sqlite3_column_*() calls to coerce to expected
 * types.  This doesn't provide much protection if a page from a
 * different table with the same column count is linked into an
 * inappropriate btree.
 *
 * A very tight recovery system might use STRICT to enforce typing on
 * all columns, preferring to skip rows which are valid at the storage
 * level but don't contain the right types.  Note that FLOAT STRICT is
 * almost certainly not appropriate, since integral values are
 * transparently stored as integers, when that is more efficient.
 *
 * Another option is to use ANY for all columns and inspect each
 * result manually (using sqlite3_column_*).  This should only be
 * necessary in cases where developers have used manifest typing (test
 * to make sure before you decide that you aren't using manifest
 * typing!).
 *
 *
 * Caveats
 *
 * Leaf pages not referenced by interior nodes will not be found.
 *
 * Leaf pages referenced from interior nodes of other tables will not
 * be resolved.
 *
 * Rows referencing invalid overflow pages will be skipped.
 *
 * SQlite rows have a header which describes how to interpret the rest
 * of the payload.  The header can be valid in cases where the rest of
 * the record is actually corrupt (in the sense that the data is not
 * the intended data).  This can especially happen WRT overflow pages,
 * as lack of atomic updates between pages is the primary form of
 * corruption I have seen in the wild.
 */
/* The implementation is via a series of cursors.  The cursor
 * implementations follow the pattern:
 *
 * // Creates the cursor using various initialization info.
 * int cursorCreate(...);
 *
 * // Returns 1 if there is no more data, 0 otherwise.
 * int cursorEOF(Cursor *pCursor);
 *
 * // Various accessors can be used if not at EOF.
 *
 * // Move to the next item.
 * int cursorNext(Cursor *pCursor);
 *
 * // Destroy the memory associated with the cursor.
 * void cursorDestroy(Cursor *pCursor);
 *
 * References in the following are to sections at
 * http://www.sqlite.org/fileformat2.html .
 *
 * RecoverLeafCursor iterates the records in a leaf table node
 * described in section 1.5 "B-tree Pages".  When the node is
 * exhausted, an interior cursor is used to get the next leaf node,
 * and iteration continues there.
 *
 * RecoverInteriorCursor iterates the child pages in an interior table
 * node described in section 1.5 "B-tree Pages".  When the node is
 * exhausted, a parent interior cursor is used to get the next
 * interior node at the same level, and iteration continues there.
 *
 * Together these record the path from the leaf level to the root of
 * the tree.  Iteration happens from the leaves rather than the root
 * both for efficiency and putting the special case at the front of
 * the list is easier to implement.
 *
 * RecoverCursor uses a RecoverLeafCursor to iterate the rows of a
 * table, returning results via the SQLite virtual table interface.
 */
/* TODO(shess): It might be useful to allow DEFAULT in types to
 * specify what to do for NULL when an ALTER TABLE case comes up.
 * Unfortunately, simply adding it to the exposed schema and using
 * sqlite3_result_null() does not cause the default to be generate.
 * Handling it ourselves seems hard, unfortunately.
 */

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"

/* Some SQLite internals use, cribbed from fts5int.h. */
#ifndef SQLITE_AMALGAMATION
typedef uint8_t u8;
typedef uint32_t u32;
typedef sqlite3_int64 i64;
typedef sqlite3_uint64 u64;

#define ArraySize(x) (sizeof(x) / sizeof(x[0]))
#endif

/* From recover_varint.c. */
u8 recoverGetVarint(const unsigned char *p, u64 *v);

/* For debugging. */
#if 0
#define FNENTRY() fprintf(stderr, "In %s\n", __FUNCTION__)
#else
#define FNENTRY()
#endif

/* Generic constants and helper functions. */

static const unsigned char kTableLeafPage = 0x0D;
static const unsigned char kTableInteriorPage = 0x05;

/* From section 1.2. */
static const unsigned kiHeaderPageSizeOffset = 16;
static const unsigned kiHeaderReservedSizeOffset = 20;
static const unsigned kiHeaderEncodingOffset = 56;
/* TODO(shess) |static const unsigned| fails creating the header in GetPager()
** because |knHeaderSize| isn't |constexpr|.  But this isn't C++, either.
*/
enum { knHeaderSize = 100};

/* From section 1.5. */
static const unsigned kiPageTypeOffset = 0;
/* static const unsigned kiPageFreeBlockOffset = 1; */
static const unsigned kiPageCellCountOffset = 3;
/* static const unsigned kiPageCellContentOffset = 5; */
/* static const unsigned kiPageFragmentedBytesOffset = 7; */
static const unsigned knPageLeafHeaderBytes = 8;
/* Interior pages contain an additional field. */
static const unsigned kiPageRightChildOffset = 8;
static const unsigned kiPageInteriorHeaderBytes = 12;

/* Accepted types are specified by a mask. */
#define MASK_ROWID (1<<0)
#define MASK_INTEGER (1<<1)
#define MASK_FLOAT (1<<2)
#define MASK_TEXT (1<<3)
#define MASK_BLOB (1<<4)
#define MASK_NULL (1<<5)

/* Helpers to decode fixed-size fields. */
static u32 decodeUnsigned16(const unsigned char *pData){
  return (pData[0]<<8) + pData[1];
}
static u32 decodeUnsigned32(const unsigned char *pData){
  return (decodeUnsigned16(pData)<<16) + decodeUnsigned16(pData+2);
}
static i64 decodeSigned(const unsigned char *pData, unsigned nBytes){
  i64 r = (char)(*pData);
  while( --nBytes ){
    r <<= 8;
    r += *(++pData);
  }
  return r;
}
/* Derived from vdbeaux.c, sqlite3VdbeSerialGet(), case 7. */
/* TODO(shess): Determine if swapMixedEndianFloat() applies. */
static double decodeFloat64(const unsigned char *pData){
#if !defined(NDEBUG)
  static const u64 t1 = ((u64)0x3ff00000)<<32;
  static const double r1 = 1.0;
  u64 t2 = t1;
  assert( sizeof(r1)==sizeof(t2) && memcmp(&r1, &t2, sizeof(r1))==0 );
#endif
  i64 x = decodeSigned(pData, 8);
  double d;
  memcpy(&d, &x, sizeof(x));
  return d;
}

/* Return true if a varint can safely be read from pData/nData. */
/* TODO(shess): DbPage points into the middle of a buffer which
 * contains the page data before DbPage.  So code should always be
 * able to read a small number of varints safely.  Consider whether to
 * trust that or not.
 */
static int checkVarint(const unsigned char *pData, unsigned nData){
  unsigned i;

  /* In the worst case the decoder takes all 8 bits of the 9th byte. */
  if( nData>=9 ){
    return 1;
  }

  /* Look for a high-bit-clear byte in what's left. */
  for( i=0; i<nData; ++i ){
    if( !(pData[i]&0x80) ){
      return 1;
    }
  }

  /* Cannot decode in the space given. */
  return 0;
}

/* Return 1 if n varints can be read from pData/nData. */
static int checkVarints(const unsigned char *pData, unsigned nData,
                        unsigned n){
  unsigned nCur = 0;   /* Byte offset within current varint. */
  unsigned nFound = 0; /* Number of varints found. */
  unsigned i;

  /* In the worst case the decoder takes all 8 bits of the 9th byte. */
  if( nData>=9*n ){
    return 1;
  }

  for( i=0; nFound<n && i<nData; ++i ){
    nCur++;
    if( nCur==9 || !(pData[i]&0x80) ){
      nFound++;
      nCur = 0;
    }
  }

  return nFound==n;
}

/* ctype and str[n]casecmp() can be affected by locale (eg, tr_TR).
 * These versions consider only the ASCII space.
 */
/* TODO(shess): It may be reasonable to just remove the need for these
 * entirely.  The module could require "TEXT STRICT NOT NULL", not
 * "Text Strict Not Null" or whatever the developer felt like typing
 * that day.  Handling corrupt data is a PERFECT place to be pedantic.
 */
static int ascii_isspace(char c){
  /* From fts3_expr.c */
  return c==' ' || c=='\t' || c=='\n' || c=='\r' || c=='\v' || c=='\f';
}
static int ascii_isalnum(int x){
  /* From fts3_tokenizer1.c */
  return (x>='0' && x<='9') || (x>='A' && x<='Z') || (x>='a' && x<='z');
}
static int ascii_tolower(int x){
  /* From fts3_tokenizer1.c */
  return (x>='A' && x<='Z') ? x-'A'+'a' : x;
}
/* TODO(shess): Consider sqlite3_strnicmp() */
static int ascii_strncasecmp(const char *s1, const char *s2, size_t n){
  const unsigned char *us1 = (const unsigned char *)s1;
  const unsigned char *us2 = (const unsigned char *)s2;
  while( *us1 && *us2 && n && ascii_tolower(*us1)==ascii_tolower(*us2) ){
    us1++, us2++, n--;
  }
  return n ? ascii_tolower(*us1)-ascii_tolower(*us2) : 0;
}
static int ascii_strcasecmp(const char *s1, const char *s2){
  /* If s2 is equal through strlen(s1), will exit while() due to s1's
   * trailing NUL, and return NUL-s2[strlen(s1)].
   */
  return ascii_strncasecmp(s1, s2, strlen(s1)+1);
}

/* Provide access to the pages of a SQLite database in a way similar to SQLite's
** Pager.
*/
typedef struct RecoverPager RecoverPager;
struct RecoverPager {
  sqlite3_file *pSqliteFile;      /* Reference to database's file handle */
  u32 nPageSize;                  /* Size of pages in pSqliteFile */
};

static void pagerDestroy(RecoverPager *pPager){
  pPager->pSqliteFile->pMethods->xUnlock(pPager->pSqliteFile, SQLITE_LOCK_NONE);
  memset(pPager, 0xA5, sizeof(*pPager));
  sqlite3_free(pPager);
}

/* pSqliteFile should already have a SHARED lock. */
static int pagerCreate(sqlite3_file *pSqliteFile, u32 nPageSize,
                       RecoverPager **ppPager){
  RecoverPager *pPager = sqlite3_malloc(sizeof(RecoverPager));
  if( !pPager ){
    return SQLITE_NOMEM;
  }

  memset(pPager, 0, sizeof(*pPager));
  pPager->pSqliteFile = pSqliteFile;
  pPager->nPageSize = nPageSize;
  *ppPager = pPager;
  return SQLITE_OK;
}

/* Matches DbPage (aka PgHdr) from SQLite internals. */
/* TODO(shess): SQLite by default allocates page metadata in a single allocation
** such that the page's data and metadata are contiguous, see pcache1AllocPage
** in pcache1.c.  I believe this was intended to reduce malloc churn.  It means
** that Chromium's automated tooling would be unlikely to see page-buffer
** overruns.  I believe that this code is safe, but for now replicate SQLite's
** approach with kExcessSpace.
*/
const int kExcessSpace = 128;
typedef struct RecoverPage RecoverPage;
struct RecoverPage {
  u32 pgno;                      /* Page number for this page */
  void *pData;                   /* Page data for pgno */
  RecoverPager *pPager;          /* The pager this page is part of */
};

static void pageDestroy(RecoverPage *pPage){
  sqlite3_free(pPage->pData);
  memset(pPage, 0xA5, sizeof(*pPage));
  sqlite3_free(pPage);
}

static int pageCreate(RecoverPager *pPager, u32 pgno, RecoverPage **ppPage){
  RecoverPage *pPage = sqlite3_malloc(sizeof(RecoverPage));
  if( !pPage ){
    return SQLITE_NOMEM;
  }

  memset(pPage, 0, sizeof(*pPage));
  pPage->pPager = pPager;
  pPage->pgno = pgno;
  pPage->pData = sqlite3_malloc(pPager->nPageSize + kExcessSpace);
  if( pPage->pData==NULL ){
    pageDestroy(pPage);
    return SQLITE_NOMEM;
  }
  memset((u8 *)pPage->pData + pPager->nPageSize, 0, kExcessSpace);

  *ppPage = pPage;
  return SQLITE_OK;
}

static int pagerGetPage(RecoverPager *pPager, u32 iPage, RecoverPage **ppPage) {
  sqlite3_int64 iOfst;
  sqlite3_file *pFile = pPager->pSqliteFile;
  RecoverPage *pPage;
  int rc = pageCreate(pPager, iPage, &pPage);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  /* xRead() can return SQLITE_IOERR_SHORT_READ, which should be treated as
  ** SQLITE_OK plus an EOF indicator.  The excess space is zero-filled.
  */
  iOfst = ((sqlite3_int64)iPage - 1) * pPager->nPageSize;
  rc = pFile->pMethods->xRead(pFile, pPage->pData, pPager->nPageSize, iOfst);
  if( rc!=SQLITE_OK && rc!=SQLITE_IOERR_SHORT_READ ){
    pageDestroy(pPage);
    return rc;
  }

  *ppPage = pPage;
  return SQLITE_OK;
}

/* For some reason I kept making mistakes with offset calculations. */
static const unsigned char *PageData(RecoverPage *pPage, unsigned iOffset){
  assert( iOffset<=pPage->pPager->nPageSize );
  return (unsigned char *)pPage->pData + iOffset;
}

/* The first page in the file contains a file header in the first 100
 * bytes.  The page's header information comes after that.  Note that
 * the offsets in the page's header information are relative to the
 * beginning of the page, NOT the end of the page header.
 */
static const unsigned char *PageHeader(RecoverPage *pPage){
  if( pPage->pgno==1 ){
    return PageData(pPage, knHeaderSize);
  }else{
    return PageData(pPage, 0);
  }
}

/* Helper to fetch the pager and page size for the named database. */
static int GetPager(sqlite3 *db, const char *zName,
                    RecoverPager **ppPager, unsigned *pnPageSize,
                    int *piEncoding){
  int rc, iEncoding;
  unsigned nPageSize, nReservedSize;
  unsigned char header[knHeaderSize];
  sqlite3_file *pFile = NULL;
  RecoverPager *pPager;

  rc = sqlite3_file_control(db, zName, SQLITE_FCNTL_FILE_POINTER, &pFile);
  if( rc!=SQLITE_OK ) {
    return rc;
  } else if( pFile==NULL ){
    /* The documentation for sqlite3PagerFile() indicates it can return NULL if
    ** the file has not yet been opened.  That should not be possible here...
    */
    return SQLITE_MISUSE;
  }

  /* Get a shared lock to make sure the on-disk version of the file is truth. */
  rc = pFile->pMethods->xLock(pFile, SQLITE_LOCK_SHARED);
  if( rc != SQLITE_OK ){
    return rc;
  }

  /* Read the Initial header information.  In case of SQLITE_IOERR_SHORT_READ,
  ** the header is incomplete, which means no data could be recovered anyhow.
  */
  rc = pFile->pMethods->xRead(pFile, header, sizeof(header), 0);
  if( rc != SQLITE_OK ){
    pFile->pMethods->xUnlock(pFile, SQLITE_LOCK_NONE);
    if( rc==SQLITE_IOERR_SHORT_READ ){
      return SQLITE_CORRUPT;
    }
    return rc;
  }

  /* Page size must be a power of two between 512 and 32768 inclusive. */
  nPageSize = decodeUnsigned16(header + kiHeaderPageSizeOffset);
  if( (nPageSize&(nPageSize-1)) || nPageSize>32768 || nPageSize<512 ){
    pFile->pMethods->xUnlock(pFile, SQLITE_LOCK_NONE);
    return rc;
  }

  /* Space reserved a the end of the page for extensions.  Usually 0. */
  nReservedSize = header[kiHeaderReservedSizeOffset];

  /* 1 for UTF-8, 2 for UTF-16le, 3 for UTF-16be. */
  iEncoding = decodeUnsigned32(header + kiHeaderEncodingOffset);
  if( iEncoding==3 ){
    *piEncoding = SQLITE_UTF16BE;
  } else if( iEncoding==2 ){
    *piEncoding = SQLITE_UTF16LE;
  } else if( iEncoding==1 ){
    *piEncoding = SQLITE_UTF8;
  } else {
    /* This case should not be possible. */
    *piEncoding = SQLITE_UTF8;
  }

  rc = pagerCreate(pFile, nPageSize, &pPager);
  if( rc!=SQLITE_OK ){
    pFile->pMethods->xUnlock(pFile, SQLITE_LOCK_NONE);
    return rc;
  }

  *ppPager = pPager;
  *pnPageSize = nPageSize - nReservedSize;
  *piEncoding = iEncoding;
  return SQLITE_OK;
}

/* iSerialType is a type read from a record header.  See "2.1 Record Format".
 */

/* Storage size of iSerialType in bytes.  My interpretation of SQLite
 * documentation is that text and blob fields can have 32-bit length.
 * Values past 2^31-12 will need more than 32 bits to encode, which is
 * why iSerialType is u64.
 */
static u32 SerialTypeLength(u64 iSerialType){
  switch( iSerialType ){
    case 0 : return 0;  /* NULL */
    case 1 : return 1;  /* Various integers. */
    case 2 : return 2;
    case 3 : return 3;
    case 4 : return 4;
    case 5 : return 6;
    case 6 : return 8;
    case 7 : return 8;  /* 64-bit float. */
    case 8 : return 0;  /* Constant 0. */
    case 9 : return 0;  /* Constant 1. */
    case 10 : case 11 : assert( "RESERVED TYPE"==NULL ); return 0;
  }
  return (u32)((iSerialType>>1) - 6);
}

/* True if iSerialType refers to a blob. */
static int SerialTypeIsBlob(u64 iSerialType){
  assert( iSerialType>=12 );
  return (iSerialType%2)==0;
}

/* Returns true if the serialized type represented by iSerialType is
 * compatible with the given type mask.
 */
static int SerialTypeIsCompatible(u64 iSerialType, unsigned char mask){
  switch( iSerialType ){
    case 0  : return (mask&MASK_NULL)!=0;
    case 1  : return (mask&MASK_INTEGER)!=0;
    case 2  : return (mask&MASK_INTEGER)!=0;
    case 3  : return (mask&MASK_INTEGER)!=0;
    case 4  : return (mask&MASK_INTEGER)!=0;
    case 5  : return (mask&MASK_INTEGER)!=0;
    case 6  : return (mask&MASK_INTEGER)!=0;
    case 7  : return (mask&MASK_FLOAT)!=0;
    case 8  : return (mask&MASK_INTEGER)!=0;
    case 9  : return (mask&MASK_INTEGER)!=0;
    case 10 : assert( "RESERVED TYPE"==NULL ); return 0;
    case 11 : assert( "RESERVED TYPE"==NULL ); return 0;
  }
  return (mask&(SerialTypeIsBlob(iSerialType) ? MASK_BLOB : MASK_TEXT));
}

/* Versions of strdup() with return values appropriate for
 * sqlite3_free().  malloc.c has sqlite3DbStrDup()/NDup(), but those
 * need sqlite3DbFree(), which seems intrusive.
 */
static char *sqlite3_strndup(const char *z, unsigned n){
  char *zNew;

  if( z==NULL ){
    return NULL;
  }

  zNew = sqlite3_malloc(n+1);
  if( zNew!=NULL ){
    memcpy(zNew, z, n);
    zNew[n] = '\0';
  }
  return zNew;
}
static char *sqlite3_strdup(const char *z){
  if( z==NULL ){
    return NULL;
  }
  return sqlite3_strndup(z, strlen(z));
}

/* Fetch the page number of zTable in zDb from sqlite_master in zDb,
 * and put it in *piRootPage.
 */
static int getRootPage(sqlite3 *db, const char *zDb, const char *zTable,
                       u32 *piRootPage){
  char *zSql;  /* SQL selecting root page of named element. */
  sqlite3_stmt *pStmt;
  int rc;

  if( strcmp(zTable, "sqlite_master")==0 ){
    *piRootPage = 1;
    return SQLITE_OK;
  }

  zSql = sqlite3_mprintf("SELECT rootpage FROM %s.sqlite_master "
                         "WHERE type = 'table' AND tbl_name = %Q",
                         zDb, zTable);
  if( !zSql ){
    return SQLITE_NOMEM;
  }

  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  /* Require a result. */
  rc = sqlite3_step(pStmt);
  if( rc==SQLITE_DONE ){
    rc = SQLITE_CORRUPT;
  }else if( rc==SQLITE_ROW ){
    *piRootPage = sqlite3_column_int(pStmt, 0);

    /* Require only one result. */
    rc = sqlite3_step(pStmt);
    if( rc==SQLITE_DONE ){
      rc = SQLITE_OK;
    }else if( rc==SQLITE_ROW ){
      rc = SQLITE_CORRUPT;
    }
  }
  sqlite3_finalize(pStmt);
  return rc;
}

/* Cursor for iterating interior nodes.  Interior page cells contain a
 * child page number and a rowid.  The child page contains items left
 * of the rowid (less than).  The rightmost page of the subtree is
 * stored in the page header.
 *
 * interiorCursorDestroy - release all resources associated with the
 *                         cursor and any parent cursors.
 * interiorCursorCreate - create a cursor with the given parent and page.
 * interiorCursorNextPage - fetch the next child page from the cursor.
 *
 * Logically, interiorCursorNextPage() returns the next child page
 * number from the page the cursor is currently reading, calling the
 * parent cursor as necessary to get new pages to read, until done.
 * SQLITE_ROW if a page is returned, SQLITE_DONE if out of pages,
 * error otherwise.  Unfortunately, if the table is corrupted
 * unexpected pages can be returned.  If any unexpected page is found,
 * leaf or otherwise, it is returned to the caller for processing,
 * with the interior cursor left empty.  The next call to
 * interiorCursorNextPage() will recurse to the parent cursor until an
 * interior page to iterate is returned.
 *
 * Note that while interiorCursorNextPage() will refuse to follow
 * loops, it does not keep track of pages returned for purposes of
 * preventing duplication.
 */
typedef struct RecoverInteriorCursor RecoverInteriorCursor;
struct RecoverInteriorCursor {
  RecoverInteriorCursor *pParent; /* Parent node to this node. */
  RecoverPage *pPage;             /* Reference to leaf page. */
  unsigned nPageSize;             /* Size of page. */
  unsigned nChildren;             /* Number of children on the page. */
  unsigned iChild;                /* Index of next child to return. */
};

static void interiorCursorDestroy(RecoverInteriorCursor *pCursor){
  /* Destroy all the cursors to the root. */
  while( pCursor ){
    RecoverInteriorCursor *p = pCursor;
    pCursor = pCursor->pParent;

    if( p->pPage ){
      pageDestroy(p->pPage);
      p->pPage = NULL;
    }

    memset(p, 0xA5, sizeof(*p));
    sqlite3_free(p);
  }
}

/* Internal helper.  Reset storage in preparation for iterating pPage. */
static void interiorCursorSetPage(RecoverInteriorCursor *pCursor,
                                  RecoverPage *pPage){
  const unsigned knMinCellLength = 2 + 4 + 1;
  unsigned nMaxChildren;
  assert( PageHeader(pPage)[kiPageTypeOffset]==kTableInteriorPage );

  if( pCursor->pPage ){
    pageDestroy(pCursor->pPage);
    pCursor->pPage = NULL;
  }
  pCursor->pPage = pPage;
  pCursor->iChild = 0;

  /* A child for each cell, plus one in the header. */
  pCursor->nChildren = decodeUnsigned16(PageHeader(pPage) +
                                        kiPageCellCountOffset) + 1;

  /* Each child requires a 16-bit offset from an array after the header,
   * and each child contains a 32-bit page number and at least a varint
   * (min size of one byte).  The final child page is in the header.  So
   * the maximum value for nChildren is:
   *   (nPageSize - kiPageInteriorHeaderBytes) /
   *      (sizeof(uint16) + sizeof(uint32) + 1) + 1
   */
  /* TODO(shess): This count is very unlikely to be corrupted in
   * isolation, so seeing this could signal to skip the page.  OTOH, I
   * can't offhand think of how to get here unless this or the page-type
   * byte is corrupted.  Could be an overflow page, but it would require
   * a very large database.
   */
  nMaxChildren =
      (pCursor->nPageSize - kiPageInteriorHeaderBytes) / knMinCellLength + 1;
  if (pCursor->nChildren > nMaxChildren) {
    pCursor->nChildren = nMaxChildren;
  }
}

static int interiorCursorCreate(RecoverInteriorCursor *pParent,
                                RecoverPage *pPage, int nPageSize,
                                RecoverInteriorCursor **ppCursor){
  RecoverInteriorCursor *pCursor =
    sqlite3_malloc(sizeof(RecoverInteriorCursor));
  if( !pCursor ){
    return SQLITE_NOMEM;
  }

  memset(pCursor, 0, sizeof(*pCursor));
  pCursor->pParent = pParent;
  pCursor->nPageSize = nPageSize;
  interiorCursorSetPage(pCursor, pPage);
  *ppCursor = pCursor;
  return SQLITE_OK;
}

/* Internal helper.  Return the child page number at iChild. */
static unsigned interiorCursorChildPage(RecoverInteriorCursor *pCursor){
  const unsigned char *pPageHeader;  /* Header of the current page. */
  const unsigned char *pCellOffsets; /* Offset to page's cell offsets. */
  unsigned iCellOffset;              /* Offset of target cell. */

  assert( pCursor->iChild<pCursor->nChildren );

  /* Rightmost child is in the header. */
  pPageHeader = PageHeader(pCursor->pPage);
  if( pCursor->iChild==pCursor->nChildren-1 ){
    return decodeUnsigned32(pPageHeader + kiPageRightChildOffset);
  }

  /* Each cell is a 4-byte integer page number and a varint rowid
   * which is greater than the rowid of items in that sub-tree (this
   * module ignores ordering). The offset is from the beginning of the
   * page, not from the page header.
   */
  pCellOffsets = pPageHeader + kiPageInteriorHeaderBytes;
  iCellOffset = decodeUnsigned16(pCellOffsets + pCursor->iChild*2);
  if( iCellOffset<=pCursor->nPageSize-4 ){
    return decodeUnsigned32(PageData(pCursor->pPage, iCellOffset));
  }

  /* TODO(shess): Check for cell overlaps?  Cells require 4 bytes plus
   * a varint.  Check could be identical to leaf check (or even a
   * shared helper testing for "Cells starting in this range"?).
   */

  /* If the offset is broken, return an invalid page number. */
  return 0;
}

/* Internal helper.  Used to detect if iPage would cause a loop. */
static int interiorCursorPageInUse(RecoverInteriorCursor *pCursor,
                                   unsigned iPage){
  /* Find any parent using the indicated page. */
  while( pCursor && pCursor->pPage->pgno!=iPage ){
    pCursor = pCursor->pParent;
  }
  return pCursor!=NULL;
}

/* Get the next page from the interior cursor at *ppCursor.  Returns
 * SQLITE_ROW with the page in *ppPage, or SQLITE_DONE if out of
 * pages, or the error SQLite returned.
 *
 * If the tree is uneven, then when the cursor attempts to get a new
 * interior page from the parent cursor, it may get a non-interior
 * page.  In that case, the new page is returned, and *ppCursor is
 * updated to point to the parent cursor (this cursor is freed).
 */
/* TODO(shess): I've tried to avoid recursion in most of this code,
 * but this case is more challenging because the recursive call is in
 * the middle of operation.  One option for converting it without
 * adding memory management would be to retain the head pointer and
 * use a helper to "back up" as needed.  Another option would be to
 * reverse the list during traversal.
 */
static int interiorCursorNextPage(RecoverInteriorCursor **ppCursor,
                                  RecoverPage **ppPage){
  RecoverInteriorCursor *pCursor = *ppCursor;
  while( 1 ){
    int rc;
    const unsigned char *pPageHeader;  /* Header of found page. */

    /* Find a valid child page which isn't on the stack. */
    while( pCursor->iChild<pCursor->nChildren ){
      const unsigned iPage = interiorCursorChildPage(pCursor);
      pCursor->iChild++;
      if( interiorCursorPageInUse(pCursor, iPage) ){
        fprintf(stderr, "Loop detected at %d\n", iPage);
      }else{
        int rc = pagerGetPage(pCursor->pPage->pPager, iPage, ppPage);
        if( rc==SQLITE_OK ){
          return SQLITE_ROW;
        }
      }
    }

    /* This page has no more children.  Get next page from parent. */
    if( !pCursor->pParent ){
      return SQLITE_DONE;
    }
    rc = interiorCursorNextPage(&pCursor->pParent, ppPage);
    if( rc!=SQLITE_ROW ){
      return rc;
    }

    /* If a non-interior page is received, that either means that the
     * tree is uneven, or that a child was re-used (say as an overflow
     * page).  Remove this cursor and let the caller handle the page.
     */
    pPageHeader = PageHeader(*ppPage);
    if( pPageHeader[kiPageTypeOffset]!=kTableInteriorPage ){
      *ppCursor = pCursor->pParent;
      pCursor->pParent = NULL;
      interiorCursorDestroy(pCursor);
      return SQLITE_ROW;
    }

    /* Iterate the new page. */
    interiorCursorSetPage(pCursor, *ppPage);
    *ppPage = NULL;
  }

  assert(NULL);  /* NOTREACHED() */
  return SQLITE_CORRUPT;
}

/* Large rows are spilled to overflow pages.  The row's main page
 * stores the overflow page number after the local payload, with a
 * linked list forward from there as necessary.  overflowMaybeCreate()
 * and overflowGetSegment() provide an abstraction for accessing such
 * data while centralizing the code.
 *
 * overflowDestroy - releases all resources associated with the structure.
 * overflowMaybeCreate - create the overflow structure if it is needed
 *                       to represent the given record.  See function comment.
 * overflowGetSegment - fetch a segment from the record, accounting
 *                      for overflow pages.  Segments which are not
 *                      entirely contained with a page are constructed
 *                      into a buffer which is returned.  See function comment.
 */
typedef struct RecoverOverflow RecoverOverflow;
struct RecoverOverflow {
  RecoverOverflow *pNextOverflow;
  RecoverPage *pPage;
  unsigned nPageSize;
};

static void overflowDestroy(RecoverOverflow *pOverflow){
  while( pOverflow ){
    RecoverOverflow *p = pOverflow;
    pOverflow = p->pNextOverflow;

    if( p->pPage ){
      pageDestroy(p->pPage);
      p->pPage = NULL;
    }

    memset(p, 0xA5, sizeof(*p));
    sqlite3_free(p);
  }
}

/* Internal helper.  Used to detect if iPage would cause a loop. */
static int overflowPageInUse(RecoverOverflow *pOverflow, unsigned iPage){
  while( pOverflow && pOverflow->pPage->pgno!=iPage ){
    pOverflow = pOverflow->pNextOverflow;
  }
  return pOverflow!=NULL;
}

/* Setup to access an nRecordBytes record beginning at iRecordOffset
 * in pPage.  If nRecordBytes can be satisfied entirely from pPage,
 * then no overflow pages are needed an *pnLocalRecordBytes is set to
 * nRecordBytes.  Otherwise, *ppOverflow is set to the head of a list
 * of overflow pages, and *pnLocalRecordBytes is set to the number of
 * bytes local to pPage.
 *
 * overflowGetSegment() will do the right thing regardless of whether
 * those values are set to be in-page or not.
 */
static int overflowMaybeCreate(RecoverPage *pPage, unsigned nPageSize,
                               unsigned iRecordOffset, unsigned nRecordBytes,
                               unsigned *pnLocalRecordBytes,
                               RecoverOverflow **ppOverflow){
  unsigned nLocalRecordBytes;  /* Record bytes in the leaf page. */
  unsigned iNextPage;          /* Next page number for record data. */
  unsigned nBytes;             /* Maximum record bytes as of current page. */
  int rc;
  RecoverOverflow *pFirstOverflow;  /* First in linked list of pages. */
  RecoverOverflow *pLastOverflow;   /* End of linked list. */

  /* Calculations from the "Table B-Tree Leaf Cell" part of section
   * 1.5 of http://www.sqlite.org/fileformat2.html .  maxLocal and
   * minLocal to match naming in btree.c.
   */
  const unsigned maxLocal = nPageSize - 35;
  const unsigned minLocal = ((nPageSize-12)*32/255)-23;  /* m */

  /* Always fit anything smaller than maxLocal. */
  if( nRecordBytes<=maxLocal ){
    *pnLocalRecordBytes = nRecordBytes;
    *ppOverflow = NULL;
    return SQLITE_OK;
  }

  /* Calculate the remainder after accounting for minLocal on the leaf
   * page and what packs evenly into overflow pages.  If the remainder
   * does not fit into maxLocal, then a partially-full overflow page
   * will be required in any case, so store as little as possible locally.
   */
  nLocalRecordBytes = minLocal+((nRecordBytes-minLocal)%(nPageSize-4));
  if( maxLocal<nLocalRecordBytes ){
    nLocalRecordBytes = minLocal;
  }

  /* Don't read off the end of the page. */
  if( iRecordOffset+nLocalRecordBytes+4>nPageSize ){
    return SQLITE_CORRUPT;
  }

  /* First overflow page number is after the local bytes. */
  iNextPage =
      decodeUnsigned32(PageData(pPage, iRecordOffset + nLocalRecordBytes));
  nBytes = nLocalRecordBytes;

  /* While there are more pages to read, and more bytes are needed,
   * get another page.
   */
  pFirstOverflow = pLastOverflow = NULL;
  rc = SQLITE_OK;
  while( iNextPage && nBytes<nRecordBytes ){
    RecoverOverflow *pOverflow;  /* New overflow page for the list. */

    rc = pagerGetPage(pPage->pPager, iNextPage, &pPage);
    if( rc!=SQLITE_OK ){
      break;
    }

    pOverflow = sqlite3_malloc(sizeof(RecoverOverflow));
    if( !pOverflow ){
      pageDestroy(pPage);
      rc = SQLITE_NOMEM;
      break;
    }
    memset(pOverflow, 0, sizeof(*pOverflow));
    pOverflow->pPage = pPage;
    pOverflow->nPageSize = nPageSize;

    if( !pFirstOverflow ){
      pFirstOverflow = pOverflow;
    }else{
      pLastOverflow->pNextOverflow = pOverflow;
    }
    pLastOverflow = pOverflow;

    iNextPage = decodeUnsigned32(pPage->pData);
    nBytes += nPageSize-4;

    /* Avoid loops. */
    if( overflowPageInUse(pFirstOverflow, iNextPage) ){
      fprintf(stderr, "Overflow loop detected at %d\n", iNextPage);
      rc = SQLITE_CORRUPT;
      break;
    }
  }

  /* If there were not enough pages, or too many, things are corrupt.
   * Not having enough pages is an obvious problem, all the data
   * cannot be read.  Too many pages means that the contents of the
   * row between the main page and the overflow page(s) is
   * inconsistent (most likely one or more of the overflow pages does
   * not really belong to this row).
   */
  if( rc==SQLITE_OK && (nBytes<nRecordBytes || iNextPage) ){
    rc = SQLITE_CORRUPT;
  }

  if( rc==SQLITE_OK ){
    *ppOverflow = pFirstOverflow;
    *pnLocalRecordBytes = nLocalRecordBytes;
  }else if( pFirstOverflow ){
    overflowDestroy(pFirstOverflow);
  }
  return rc;
}

/* Use in concert with overflowMaybeCreate() to efficiently read parts
 * of a potentially-overflowing record.  pPage and iRecordOffset are
 * the values passed into overflowMaybeCreate(), nLocalRecordBytes and
 * pOverflow are the values returned by that call.
 *
 * On SQLITE_OK, *ppBase points to nRequestBytes of data at
 * iRequestOffset within the record.  If the data exists contiguously
 * in a page, a direct pointer is returned, otherwise a buffer from
 * sqlite3_malloc() is returned with the data.  *pbFree is set true if
 * sqlite3_free() should be called on *ppBase.
 */
/* Operation of this function is subtle.  At any time, pPage is the
 * current page, with iRecordOffset and nLocalRecordBytes being record
 * data within pPage, and pOverflow being the overflow page after
 * pPage.  This allows the code to handle both the initial leaf page
 * and overflow pages consistently by adjusting the values
 * appropriately.
 */
static int overflowGetSegment(RecoverPage *pPage, unsigned iRecordOffset,
                              unsigned nLocalRecordBytes,
                              RecoverOverflow *pOverflow,
                              unsigned iRequestOffset, unsigned nRequestBytes,
                              unsigned char **ppBase, int *pbFree){
  unsigned nBase;         /* Amount of data currently collected. */
  unsigned char *pBase;   /* Buffer to collect record data into. */

  /* Skip to the page containing the start of the data. */
  while( iRequestOffset>=nLocalRecordBytes && pOverflow ){
    /* Factor out current page's contribution. */
    iRequestOffset -= nLocalRecordBytes;

    /* Move forward to the next page in the list. */
    pPage = pOverflow->pPage;
    iRecordOffset = 4;
    nLocalRecordBytes = pOverflow->nPageSize - iRecordOffset;
    pOverflow = pOverflow->pNextOverflow;
  }

  /* If the requested data is entirely within this page, return a
   * pointer into the page.
   */
  if( iRequestOffset+nRequestBytes<=nLocalRecordBytes ){
    /* TODO(shess): "assignment discards qualifiers from pointer target type"
     * Having ppBase be const makes sense, but sqlite3_free() takes non-const.
     */
    *ppBase = (unsigned char *)PageData(pPage, iRecordOffset + iRequestOffset);
    *pbFree = 0;
    return SQLITE_OK;
  }

  /* The data range would require additional pages. */
  if( !pOverflow ){
    /* Should never happen, the range is outside the nRecordBytes
     * passed to overflowMaybeCreate().
     */
    assert(NULL);  /* NOTREACHED */
    return SQLITE_ERROR;
  }

  /* Get a buffer to construct into. */
  nBase = 0;
  pBase = sqlite3_malloc(nRequestBytes);
  if( !pBase ){
    return SQLITE_NOMEM;
  }
  while( nBase<nRequestBytes ){
    /* Copy over data present on this page. */
    unsigned nCopyBytes = nRequestBytes - nBase;
    if( nLocalRecordBytes-iRequestOffset<nCopyBytes ){
      nCopyBytes = nLocalRecordBytes - iRequestOffset;
    }
    memcpy(pBase + nBase, PageData(pPage, iRecordOffset + iRequestOffset),
           nCopyBytes);
    nBase += nCopyBytes;

    if( pOverflow ){
      /* Copy from start of record data in future pages. */
      iRequestOffset = 0;

      /* Move forward to the next page in the list.  Should match
       * first while() loop.
       */
      pPage = pOverflow->pPage;
      iRecordOffset = 4;
      nLocalRecordBytes = pOverflow->nPageSize - iRecordOffset;
      pOverflow = pOverflow->pNextOverflow;
    }else if( nBase<nRequestBytes ){
      /* Ran out of overflow pages with data left to deliver.  Not
       * possible if the requested range fits within nRecordBytes
       * passed to overflowMaybeCreate() when creating pOverflow.
       */
      assert(NULL);  /* NOTREACHED */
      sqlite3_free(pBase);
      return SQLITE_ERROR;
    }
  }
  assert( nBase==nRequestBytes );
  *ppBase = pBase;
  *pbFree = 1;
  return SQLITE_OK;
}

/* Primary structure for iterating the contents of a table.
 *
 * leafCursorDestroy - release all resources associated with the cursor.
 * leafCursorCreate - create a cursor to iterate items from tree at
 *                    the provided root page.
 * leafCursorNextValidCell - get the cursor ready to access data from
 *                           the next valid cell in the table.
 * leafCursorCellRowid - get the current cell's rowid.
 * leafCursorCellColumns - get current cell's column count.
 * leafCursorCellColInfo - get type and data for a column in current cell.
 *
 * leafCursorNextValidCell skips cells which fail simple integrity
 * checks, such as overlapping other cells, or being located at
 * impossible offsets, or where header data doesn't correctly describe
 * payload data.  Returns SQLITE_ROW if a valid cell is found,
 * SQLITE_DONE if all pages in the tree were exhausted.
 *
 * leafCursorCellColInfo() accounts for overflow pages in the style of
 * overflowGetSegment().
 */
typedef struct RecoverLeafCursor RecoverLeafCursor;
struct RecoverLeafCursor {
  RecoverInteriorCursor *pParent;  /* Parent node to this node. */
  RecoverPager *pPager;            /* Page provider. */
  RecoverPage *pPage;              /* Current leaf page. */
  unsigned nPageSize;              /* Size of pPage. */
  unsigned nCells;                 /* Number of cells in pPage. */
  unsigned iCell;                  /* Current cell. */

  /* Info parsed from data in iCell. */
  i64 iRowid;                      /* rowid parsed. */
  unsigned nRecordCols;            /* how many items in the record. */
  u64 iRecordOffset;               /* offset to record data. */
  /* TODO(shess): nRecordBytes and nRecordHeaderBytes are used in
   * leafCursorCellColInfo() to prevent buffer overruns.
   * leafCursorCellDecode() already verified that the cell is valid, so
   * those checks should be redundant.
   */
  u64 nRecordBytes;                /* Size of record data. */
  unsigned nLocalRecordBytes;      /* Amount of record data in-page. */
  unsigned nRecordHeaderBytes;     /* Size of record header data. */
  unsigned char *pRecordHeader;    /* Pointer to record header data. */
  int bFreeRecordHeader;           /* True if record header requires free. */
  RecoverOverflow *pOverflow;      /* Cell overflow info, if needed. */
};

/* Internal helper shared between next-page and create-cursor.  If
 * pPage is a leaf page, it will be stored in the cursor and state
 * initialized for reading cells.
 *
 * If pPage is an interior page, a new parent cursor is created and
 * injected on the stack.  This is necessary to handle trees with
 * uneven depth, but also is used during initial setup.
 *
 * If pPage is not a table page at all, it is discarded.
 *
 * If SQLITE_OK is returned, the caller no longer owns pPage,
 * otherwise the caller is responsible for discarding it.
 */
static int leafCursorLoadPage(RecoverLeafCursor *pCursor, RecoverPage *pPage){
  const unsigned char *pPageHeader;  /* Header of *pPage */
  unsigned nCells;                   /* Number of cells in the page */

  /* Release the current page. */
  if( pCursor->pPage ){
    pageDestroy(pCursor->pPage);
    pCursor->pPage = NULL;
    pCursor->iCell = pCursor->nCells = 0;
  }

  /* If the page is an unexpected interior node, inject a new stack
   * layer and try again from there.
   */
  pPageHeader = PageHeader(pPage);
  if( pPageHeader[kiPageTypeOffset]==kTableInteriorPage ){
    RecoverInteriorCursor *pParent;
    int rc = interiorCursorCreate(pCursor->pParent, pPage, pCursor->nPageSize,
                                  &pParent);
    if( rc!=SQLITE_OK ){
      return rc;
    }
    pCursor->pParent = pParent;
    return SQLITE_OK;
  }

  /* Not a leaf page, skip it. */
  if( pPageHeader[kiPageTypeOffset]!=kTableLeafPage ){
    pageDestroy(pPage);
    return SQLITE_OK;
  }

  /* Leaf contains no data, skip it.  Empty tables, for instance. */
  nCells = decodeUnsigned16(pPageHeader + kiPageCellCountOffset);;
  if( nCells<1 ){
    pageDestroy(pPage);
    return SQLITE_OK;
  }

  /* Take ownership of the page and start decoding. */
  pCursor->pPage = pPage;
  pCursor->iCell = 0;
  pCursor->nCells = nCells;
  return SQLITE_OK;
}

/* Get the next leaf-level page in the tree.  Returns SQLITE_ROW when
 * a leaf page is found, SQLITE_DONE when no more leaves exist, or any
 * error which occurred.
 */
static int leafCursorNextPage(RecoverLeafCursor *pCursor){
  if( !pCursor->pParent ){
    return SQLITE_DONE;
  }

  /* Repeatedly load the parent's next child page until a leaf is found. */
  do {
    RecoverPage *pNextPage;
    int rc = interiorCursorNextPage(&pCursor->pParent, &pNextPage);
    if( rc!=SQLITE_ROW ){
      assert( rc==SQLITE_DONE );
      return rc;
    }

    rc = leafCursorLoadPage(pCursor, pNextPage);
    if( rc!=SQLITE_OK ){
      pageDestroy(pNextPage);
      return rc;
    }
  } while( !pCursor->pPage );

  return SQLITE_ROW;
}

static void leafCursorDestroyCellData(RecoverLeafCursor *pCursor){
  if( pCursor->bFreeRecordHeader ){
    sqlite3_free(pCursor->pRecordHeader);
  }
  pCursor->bFreeRecordHeader = 0;
  pCursor->pRecordHeader = NULL;

  if( pCursor->pOverflow ){
    overflowDestroy(pCursor->pOverflow);
    pCursor->pOverflow = NULL;
  }
}

static void leafCursorDestroy(RecoverLeafCursor *pCursor){
  leafCursorDestroyCellData(pCursor);

  if( pCursor->pParent ){
    interiorCursorDestroy(pCursor->pParent);
    pCursor->pParent = NULL;
  }

  if( pCursor->pPage ){
    pageDestroy(pCursor->pPage);
    pCursor->pPage = NULL;
  }

  if( pCursor->pPager ){
    pagerDestroy(pCursor->pPager);
    pCursor->pPager = NULL;
  }

  memset(pCursor, 0xA5, sizeof(*pCursor));
  sqlite3_free(pCursor);
}

/* Create a cursor to iterate the rows from the leaf pages of a table
 * rooted at iRootPage.
 */
/* TODO(shess): recoverOpen() calls this to setup the cursor, and I
 * think that recoverFilter() may make a hard assumption that the
 * cursor returned will turn up at least one valid cell.
 *
 * The cases I can think of which break this assumption are:
 * - pPage is a valid leaf page with no valid cells.
 * - pPage is a valid interior page with no valid leaves.
 * - pPage is a valid interior page who's leaves contain no valid cells.
 * - pPage is not a valid leaf or interior page.
 */
static int leafCursorCreate(RecoverPager *pPager, unsigned nPageSize,
                            u32 iRootPage, RecoverLeafCursor **ppCursor){
  RecoverPage *pPage;          /* Reference to page at iRootPage. */
  RecoverLeafCursor *pCursor;  /* Leaf cursor being constructed. */
  int rc;

  /* Start out with the root page. */
  rc = pagerGetPage(pPager, iRootPage, &pPage);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  pCursor = sqlite3_malloc(sizeof(RecoverLeafCursor));
  if( !pCursor ){
    pageDestroy(pPage);
    return SQLITE_NOMEM;
  }
  memset(pCursor, 0, sizeof(*pCursor));

  pCursor->nPageSize = nPageSize;
  pCursor->pPager = pPager;

  rc = leafCursorLoadPage(pCursor, pPage);
  if( rc!=SQLITE_OK ){
    pageDestroy(pPage);
    leafCursorDestroy(pCursor);
    return rc;
  }

  /* pPage wasn't a leaf page, find the next leaf page. */
  if( !pCursor->pPage ){
    rc = leafCursorNextPage(pCursor);
    if( rc!=SQLITE_DONE && rc!=SQLITE_ROW ){
      leafCursorDestroy(pCursor);
      return rc;
    }
  }

  *ppCursor = pCursor;
  return SQLITE_OK;
}

/* Useful for setting breakpoints. */
static int ValidateError(){
  return SQLITE_ERROR;
}

/* Setup the cursor for reading the information from cell iCell. */
static int leafCursorCellDecode(RecoverLeafCursor *pCursor){
  const unsigned char *pPageHeader;  /* Header of current page. */
  const unsigned char *pPageEnd;     /* Byte after end of current page. */
  const unsigned char *pCellOffsets; /* Pointer to page's cell offsets. */
  unsigned iCellOffset;              /* Offset of current cell (iCell). */
  const unsigned char *pCell;        /* Pointer to data at iCellOffset. */
  unsigned nCellMaxBytes;            /* Maximum local size of iCell. */
  unsigned iEndOffset;               /* End of iCell's in-page data. */
  u64 nRecordBytes;                  /* Expected size of cell, w/overflow. */
  u64 iRowid;                        /* iCell's rowid (in table). */
  unsigned nRead;                    /* Amount of cell read. */
  unsigned nRecordHeaderRead;        /* Header data read. */
  u64 nRecordHeaderBytes;            /* Header size expected. */
  unsigned nRecordCols;              /* Columns read from header. */
  u64 nRecordColBytes;               /* Bytes in payload for those columns. */
  unsigned i;
  int rc;

  assert( pCursor->iCell<pCursor->nCells );

  leafCursorDestroyCellData(pCursor);

  /* Find the offset to the row. */
  pPageHeader = PageHeader(pCursor->pPage);
  pCellOffsets = pPageHeader + knPageLeafHeaderBytes;
  pPageEnd = PageData(pCursor->pPage, pCursor->nPageSize);
  if( pCellOffsets + pCursor->iCell*2 + 2 > pPageEnd ){
    return ValidateError();
  }
  iCellOffset = decodeUnsigned16(pCellOffsets + pCursor->iCell*2);
  if( iCellOffset>=pCursor->nPageSize ){
    return ValidateError();
  }

  pCell = PageData(pCursor->pPage, iCellOffset);
  nCellMaxBytes = pCursor->nPageSize - iCellOffset;

  /* B-tree leaf cells lead with varint record size, varint rowid and
   * varint header size.
   */
  /* TODO(shess): The smallest page size is 512 bytes, which has an m
   * of 39.  Three varints need at most 27 bytes to encode.  I think.
   */
  if( !checkVarints(pCell, nCellMaxBytes, 3) ){
    return ValidateError();
  }

  nRead = recoverGetVarint(pCell, &nRecordBytes);
  assert( iCellOffset+nRead<=pCursor->nPageSize );
  pCursor->nRecordBytes = nRecordBytes;

  nRead += recoverGetVarint(pCell + nRead, &iRowid);
  assert( iCellOffset+nRead<=pCursor->nPageSize );
  pCursor->iRowid = (i64)iRowid;

  pCursor->iRecordOffset = iCellOffset + nRead;

  /* Start overflow setup here because nLocalRecordBytes is needed to
   * check cell overlap.
   */
  rc = overflowMaybeCreate(pCursor->pPage, pCursor->nPageSize,
                           pCursor->iRecordOffset, pCursor->nRecordBytes,
                           &pCursor->nLocalRecordBytes,
                           &pCursor->pOverflow);
  if( rc!=SQLITE_OK ){
    return ValidateError();
  }

  /* Check that no other cell starts within this cell. */
  iEndOffset = pCursor->iRecordOffset + pCursor->nLocalRecordBytes;
  for( i=0; i<pCursor->nCells && pCellOffsets + i*2 + 2 <= pPageEnd; ++i ){
    const unsigned iOtherOffset = decodeUnsigned16(pCellOffsets + i*2);
    if( iOtherOffset>iCellOffset && iOtherOffset<iEndOffset ){
      return ValidateError();
    }
  }

  nRecordHeaderRead = recoverGetVarint(pCell + nRead, &nRecordHeaderBytes);
  assert( nRecordHeaderBytes<=nRecordBytes );
  pCursor->nRecordHeaderBytes = nRecordHeaderBytes;

  /* Large headers could overflow if pages are small. */
  rc = overflowGetSegment(pCursor->pPage,
                          pCursor->iRecordOffset, pCursor->nLocalRecordBytes,
                          pCursor->pOverflow, 0, nRecordHeaderBytes,
                          &pCursor->pRecordHeader, &pCursor->bFreeRecordHeader);
  if( rc!=SQLITE_OK ){
    return ValidateError();
  }

  /* Tally up the column count and size of data. */
  nRecordCols = 0;
  nRecordColBytes = 0;
  while( nRecordHeaderRead<nRecordHeaderBytes ){
    u64 iSerialType;  /* Type descriptor for current column. */
    if( !checkVarint(pCursor->pRecordHeader + nRecordHeaderRead,
                     nRecordHeaderBytes - nRecordHeaderRead) ){
      return ValidateError();
    }
    nRecordHeaderRead += recoverGetVarint(
        pCursor->pRecordHeader + nRecordHeaderRead, &iSerialType);
    if( iSerialType==10 || iSerialType==11 ){
      return ValidateError();
    }
    nRecordColBytes += SerialTypeLength(iSerialType);
    nRecordCols++;
  }
  pCursor->nRecordCols = nRecordCols;

  /* Parsing the header used as many bytes as expected. */
  if( nRecordHeaderRead!=nRecordHeaderBytes ){
    return ValidateError();
  }

  /* Calculated record is size of expected record. */
  if( nRecordHeaderBytes+nRecordColBytes!=nRecordBytes ){
    return ValidateError();
  }

  return SQLITE_OK;
}

static i64 leafCursorCellRowid(RecoverLeafCursor *pCursor){
  return pCursor->iRowid;
}

static unsigned leafCursorCellColumns(RecoverLeafCursor *pCursor){
  return pCursor->nRecordCols;
}

/* Get the column info for the cell.  Pass NULL for ppBase to prevent
 * retrieving the data segment.  If *pbFree is true, *ppBase must be
 * freed by the caller using sqlite3_free().
 */
static int leafCursorCellColInfo(RecoverLeafCursor *pCursor,
                                 unsigned iCol, u64 *piColType,
                                 unsigned char **ppBase, int *pbFree){
  const unsigned char *pRecordHeader;  /* Current cell's header. */
  u64 nRecordHeaderBytes;              /* Bytes in pRecordHeader. */
  unsigned nRead;                      /* Bytes read from header. */
  u64 iColEndOffset;                   /* Offset to end of column in cell. */
  unsigned nColsSkipped;               /* Count columns as procesed. */
  u64 iSerialType;                     /* Type descriptor for current column. */

  /* Implicit NULL for columns past the end.  This case happens when
   * rows have not been updated since an ALTER TABLE added columns.
   * It is more convenient to address here than in callers.
   */
  if( iCol>=pCursor->nRecordCols ){
    *piColType = 0;
    if( ppBase ){
      *ppBase = 0;
      *pbFree = 0;
    }
    return SQLITE_OK;
  }

  /* Must be able to decode header size. */
  pRecordHeader = pCursor->pRecordHeader;
  if( !checkVarint(pRecordHeader, pCursor->nRecordHeaderBytes) ){
    return SQLITE_CORRUPT;
  }

  /* Rather than caching the header size and how many bytes it took,
   * decode it every time.
   */
  nRead = recoverGetVarint(pRecordHeader, &nRecordHeaderBytes);
  assert( nRecordHeaderBytes==pCursor->nRecordHeaderBytes );

  /* Scan forward to the indicated column.  Scans to _after_ column
   * for later range checking.
   */
  /* TODO(shess): This could get expensive for very wide tables.  An
   * array of iSerialType could be built in leafCursorCellDecode(), but
   * the number of columns is dynamic per row, so it would add memory
   * management complexity.  Enough info to efficiently forward
   * iterate could be kept, if all clients forward iterate
   * (recoverColumn() may not).
   */
  iColEndOffset = 0;
  nColsSkipped = 0;
  while( nColsSkipped<=iCol && nRead<nRecordHeaderBytes ){
    if( !checkVarint(pRecordHeader + nRead, nRecordHeaderBytes - nRead) ){
      return SQLITE_CORRUPT;
    }
    nRead += recoverGetVarint(pRecordHeader + nRead, &iSerialType);
    iColEndOffset += SerialTypeLength(iSerialType);
    nColsSkipped++;
  }

  /* Column's data extends past record's end. */
  if( nRecordHeaderBytes+iColEndOffset>pCursor->nRecordBytes ){
    return SQLITE_CORRUPT;
  }

  *piColType = iSerialType;
  if( ppBase ){
    const u32 nColBytes = SerialTypeLength(iSerialType);

    /* Offset from start of record to beginning of column. */
    const unsigned iColOffset = nRecordHeaderBytes+iColEndOffset-nColBytes;

    return overflowGetSegment(pCursor->pPage, pCursor->iRecordOffset,
                              pCursor->nLocalRecordBytes, pCursor->pOverflow,
                              iColOffset, nColBytes, ppBase, pbFree);
  }
  return SQLITE_OK;
}

static int leafCursorNextValidCell(RecoverLeafCursor *pCursor){
  while( 1 ){
    int rc;

    /* Move to the next cell. */
    pCursor->iCell++;

    /* No more cells, get the next leaf. */
    if( pCursor->iCell>=pCursor->nCells ){
      rc = leafCursorNextPage(pCursor);
      if( rc!=SQLITE_ROW ){
        return rc;
      }
      assert( pCursor->iCell==0 );
    }

    /* If the cell is valid, indicate that a row is available. */
    rc = leafCursorCellDecode(pCursor);
    if( rc==SQLITE_OK ){
      return SQLITE_ROW;
    }

    /* Iterate until done or a valid row is found. */
    /* TODO(shess): Remove debugging output. */
    fprintf(stderr, "Skipping invalid cell\n");
  }
  return SQLITE_ERROR;
}

typedef struct Recover Recover;
struct Recover {
  sqlite3_vtab base;
  sqlite3 *db;                /* Host database connection */
  char *zDb;                  /* Database containing target table */
  char *zTable;               /* Target table */
  unsigned nCols;             /* Number of columns in target table */
  unsigned char *pTypes;      /* Types of columns in target table */
};

/* Internal helper for deleting the module. */
static void recoverRelease(Recover *pRecover){
  sqlite3_free(pRecover->zDb);
  sqlite3_free(pRecover->zTable);
  sqlite3_free(pRecover->pTypes);
  memset(pRecover, 0xA5, sizeof(*pRecover));
  sqlite3_free(pRecover);
}

/* Helper function for initializing the module.  Forward-declared so
 * recoverCreate() and recoverConnect() can see it.
 */
static int recoverInit(
  sqlite3 *, void *, int, const char *const*, sqlite3_vtab **, char **
);

static int recoverCreate(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  FNENTRY();
  return recoverInit(db, pAux, argc, argv, ppVtab, pzErr);
}

/* This should never be called. */
static int recoverConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  FNENTRY();
  return recoverInit(db, pAux, argc, argv, ppVtab, pzErr);
}

/* No indices supported. */
static int recoverBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo){
  FNENTRY();
  return SQLITE_OK;
}

/* Logically, this should never be called. */
static int recoverDisconnect(sqlite3_vtab *pVtab){
  FNENTRY();
  recoverRelease((Recover*)pVtab);
  return SQLITE_OK;
}

static int recoverDestroy(sqlite3_vtab *pVtab){
  FNENTRY();
  recoverRelease((Recover*)pVtab);
  return SQLITE_OK;
}

typedef struct RecoverCursor RecoverCursor;
struct RecoverCursor {
  sqlite3_vtab_cursor base;
  RecoverLeafCursor *pLeafCursor;
  int iEncoding;
  int bEOF;
};

static int recoverOpen(sqlite3_vtab *pVTab, sqlite3_vtab_cursor **ppCursor){
  Recover *pRecover = (Recover*)pVTab;
  u32 iRootPage;                   /* Root page of the backing table. */
  int iEncoding;                   /* UTF encoding for backing database. */
  unsigned nPageSize;              /* Size of pages in backing database. */
  RecoverPager *pPager;            /* Backing database pager. */
  RecoverLeafCursor *pLeafCursor;  /* Cursor to read table's leaf pages. */
  RecoverCursor *pCursor;          /* Cursor to read rows from leaves. */
  int rc;

  FNENTRY();

  iRootPage = 0;
  rc = getRootPage(pRecover->db, pRecover->zDb, pRecover->zTable,
                   &iRootPage);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  rc = GetPager(pRecover->db, pRecover->zDb, &pPager, &nPageSize, &iEncoding);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  rc = leafCursorCreate(pPager, nPageSize, iRootPage, &pLeafCursor);
  if( rc!=SQLITE_OK ){
    pagerDestroy(pPager);
    return rc;
  }

  pCursor = sqlite3_malloc(sizeof(RecoverCursor));
  if( !pCursor ){
    leafCursorDestroy(pLeafCursor);
    return SQLITE_NOMEM;
  }
  memset(pCursor, 0, sizeof(*pCursor));
  pCursor->base.pVtab = pVTab;
  pCursor->pLeafCursor = pLeafCursor;
  pCursor->iEncoding = iEncoding;

  /* If no leaf pages were found, empty result set. */
  /* TODO(shess): leafCursorNextValidCell() would return SQLITE_ROW or
   * SQLITE_DONE to indicate whether there is further data to consider.
   */
  pCursor->bEOF = (pLeafCursor->pPage==NULL);

  *ppCursor = (sqlite3_vtab_cursor*)pCursor;
  return SQLITE_OK;
}

static int recoverClose(sqlite3_vtab_cursor *cur){
  RecoverCursor *pCursor = (RecoverCursor*)cur;
  FNENTRY();
  if( pCursor->pLeafCursor ){
    leafCursorDestroy(pCursor->pLeafCursor);
    pCursor->pLeafCursor = NULL;
  }
  memset(pCursor, 0xA5, sizeof(*pCursor));
  sqlite3_free(cur);
  return SQLITE_OK;
}

/* Helpful place to set a breakpoint. */
static int RecoverInvalidCell(){
  return SQLITE_ERROR;
}

/* Returns SQLITE_OK if the cell has an appropriate number of columns
 * with the appropriate types of data.
 */
static int recoverValidateLeafCell(Recover *pRecover, RecoverCursor *pCursor){
  unsigned i;

  /* If the row's storage has too many columns, skip it. */
  if( leafCursorCellColumns(pCursor->pLeafCursor)>pRecover->nCols ){
    return RecoverInvalidCell();
  }

  /* Skip rows with unexpected types. */
  for( i=0; i<pRecover->nCols; ++i ){
    u64 iType;  /* Storage type of column i. */
    int rc;

    /* ROWID alias. */
    if( (pRecover->pTypes[i]&MASK_ROWID) ){
      continue;
    }

    rc = leafCursorCellColInfo(pCursor->pLeafCursor, i, &iType, NULL, NULL);
    assert( rc==SQLITE_OK );
    if( rc!=SQLITE_OK || !SerialTypeIsCompatible(iType, pRecover->pTypes[i]) ){
      return RecoverInvalidCell();
    }
  }

  return SQLITE_OK;
}

static int recoverNext(sqlite3_vtab_cursor *pVtabCursor){
  RecoverCursor *pCursor = (RecoverCursor*)pVtabCursor;
  Recover *pRecover = (Recover*)pCursor->base.pVtab;
  int rc;

  FNENTRY();

  /* Scan forward to the next cell with valid storage, then check that
   * the stored data matches the schema.
   */
  while( (rc = leafCursorNextValidCell(pCursor->pLeafCursor))==SQLITE_ROW ){
    if( recoverValidateLeafCell(pRecover, pCursor)==SQLITE_OK ){
      return SQLITE_OK;
    }
  }

  if( rc==SQLITE_DONE ){
    pCursor->bEOF = 1;
    return SQLITE_OK;
  }

  assert( rc!=SQLITE_OK );
  return rc;
}

static int recoverFilter(
  sqlite3_vtab_cursor *pVtabCursor,
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  RecoverCursor *pCursor = (RecoverCursor*)pVtabCursor;
  Recover *pRecover = (Recover*)pCursor->base.pVtab;
  int rc;

  FNENTRY();

  /* There were no valid leaf pages in the table. */
  if( pCursor->bEOF ){
    return SQLITE_OK;
  }

  /* Load the first cell, and iterate forward if it's not valid.  If no cells at
   * all are valid, recoverNext() sets bEOF and returns appropriately.
   */
  rc = leafCursorCellDecode(pCursor->pLeafCursor);
  if( rc!=SQLITE_OK || recoverValidateLeafCell(pRecover, pCursor)!=SQLITE_OK ){
    return recoverNext(pVtabCursor);
  }

  return SQLITE_OK;
}

static int recoverEof(sqlite3_vtab_cursor *pVtabCursor){
  RecoverCursor *pCursor = (RecoverCursor*)pVtabCursor;
  FNENTRY();
  return pCursor->bEOF;
}

static int recoverColumn(sqlite3_vtab_cursor *cur, sqlite3_context *ctx, int i){
  RecoverCursor *pCursor = (RecoverCursor*)cur;
  Recover *pRecover = (Recover*)pCursor->base.pVtab;
  u64 iColType;             /* Storage type of column i. */
  unsigned char *pColData;  /* Column i's data. */
  int shouldFree;           /* Non-zero if pColData should be freed. */
  int rc;

  FNENTRY();

  if( (unsigned)i>=pRecover->nCols ){
    return SQLITE_ERROR;
  }

  /* ROWID alias. */
  if( (pRecover->pTypes[i]&MASK_ROWID) ){
    sqlite3_result_int64(ctx, leafCursorCellRowid(pCursor->pLeafCursor));
    return SQLITE_OK;
  }

  pColData = NULL;
  shouldFree = 0;
  rc = leafCursorCellColInfo(pCursor->pLeafCursor, i, &iColType,
                             &pColData, &shouldFree);
  if( rc!=SQLITE_OK ){
    return rc;
  }
  /* recoverValidateLeafCell() should guarantee that this will never
   * occur.
   */
  if( !SerialTypeIsCompatible(iColType, pRecover->pTypes[i]) ){
    if( shouldFree ){
      sqlite3_free(pColData);
    }
    return SQLITE_ERROR;
  }

  switch( iColType ){
    case 0 : sqlite3_result_null(ctx); break;
    case 1 : sqlite3_result_int64(ctx, decodeSigned(pColData, 1)); break;
    case 2 : sqlite3_result_int64(ctx, decodeSigned(pColData, 2)); break;
    case 3 : sqlite3_result_int64(ctx, decodeSigned(pColData, 3)); break;
    case 4 : sqlite3_result_int64(ctx, decodeSigned(pColData, 4)); break;
    case 5 : sqlite3_result_int64(ctx, decodeSigned(pColData, 6)); break;
    case 6 : sqlite3_result_int64(ctx, decodeSigned(pColData, 8)); break;
    case 7 : sqlite3_result_double(ctx, decodeFloat64(pColData)); break;
    case 8 : sqlite3_result_int(ctx, 0); break;
    case 9 : sqlite3_result_int(ctx, 1); break;
    case 10 : assert( iColType!=10 ); break;
    case 11 : assert( iColType!=11 ); break;

    default : {
      u32 l = SerialTypeLength(iColType);

      /* If pColData was already allocated, arrange to pass ownership. */
      sqlite3_destructor_type pFn = SQLITE_TRANSIENT;
      if( shouldFree ){
        pFn = sqlite3_free;
        shouldFree = 0;
      }

      if( SerialTypeIsBlob(iColType) ){
        sqlite3_result_blob(ctx, pColData, l, pFn);
      }else{
        if( pCursor->iEncoding==SQLITE_UTF16LE ){
          sqlite3_result_text16le(ctx, (const void*)pColData, l, pFn);
        }else if( pCursor->iEncoding==SQLITE_UTF16BE ){
          sqlite3_result_text16be(ctx, (const void*)pColData, l, pFn);
        }else{
          sqlite3_result_text(ctx, (const char*)pColData, l, pFn);
        }
      }
    } break;
  }
  if( shouldFree ){
    sqlite3_free(pColData);
  }
  return SQLITE_OK;
}

static int recoverRowid(sqlite3_vtab_cursor *pVtabCursor, sqlite_int64 *pRowid){
  RecoverCursor *pCursor = (RecoverCursor*)pVtabCursor;
  FNENTRY();
  *pRowid = leafCursorCellRowid(pCursor->pLeafCursor);
  return SQLITE_OK;
}

static sqlite3_module recoverModule = {
  0,                         /* iVersion */
  recoverCreate,             /* xCreate - create a table */
  recoverConnect,            /* xConnect - connect to an existing table */
  recoverBestIndex,          /* xBestIndex - Determine search strategy */
  recoverDisconnect,         /* xDisconnect - Disconnect from a table */
  recoverDestroy,            /* xDestroy - Drop a table */
  recoverOpen,               /* xOpen - open a cursor */
  recoverClose,              /* xClose - close a cursor */
  recoverFilter,             /* xFilter - configure scan constraints */
  recoverNext,               /* xNext - advance a cursor */
  recoverEof,                /* xEof */
  recoverColumn,             /* xColumn - read data */
  recoverRowid,              /* xRowid - read data */
  0,                         /* xUpdate - write data */
  0,                         /* xBegin - begin transaction */
  0,                         /* xSync - sync transaction */
  0,                         /* xCommit - commit transaction */
  0,                         /* xRollback - rollback transaction */
  0,                         /* xFindFunction - function overloading */
  0,                         /* xRename - rename the table */
};

SQLITE_API
int chrome_sqlite3_recoverVtableInit(sqlite3 *db){
  return sqlite3_create_module_v2(db, "recover", &recoverModule, NULL, 0);
}

/* This section of code is for parsing the create input and
 * initializing the module.
 */

/* Find the next word in zText and place the endpoints in pzWord*.
 * Returns true if the word is non-empty.  "Word" is defined as
 * ASCII alphanumeric plus '_' at this time.
 */
static int findWord(const char *zText,
                    const char **pzWordStart, const char **pzWordEnd){
  int r;
  while( ascii_isspace(*zText) ){
    zText++;
  }
  *pzWordStart = zText;
  while( ascii_isalnum(*zText) || *zText=='_' ){
    zText++;
  }
  r = zText>*pzWordStart;  /* In case pzWordStart==pzWordEnd */
  *pzWordEnd = zText;
  return r;
}

/* Return true if the next word in zText is zWord, also setting
 * *pzContinue to the character after the word.
 */
static int expectWord(const char *zText, const char *zWord,
                      const char **pzContinue){
  const char *zWordStart, *zWordEnd;
  if( findWord(zText, &zWordStart, &zWordEnd) &&
      ascii_strncasecmp(zWord, zWordStart, zWordEnd - zWordStart)==0 ){
    *pzContinue = zWordEnd;
    return 1;
  }
  return 0;
}

/* Parse the name and type information out of parameter.  In case of
 * success, *pzNameStart/End contain the name of the column,
 * *pzTypeStart/End contain the top-level type, and *pTypeMask has the
 * type mask to use for the column.
 */
static int findNameAndType(const char *parameter,
                           const char **pzNameStart, const char **pzNameEnd,
                           const char **pzTypeStart, const char **pzTypeEnd,
                           unsigned char *pTypeMask){
  unsigned nNameLen;   /* Length of found name. */
  const char *zEnd;    /* Current end of parsed column information. */
  int bNotNull;        /* Non-zero if NULL is not allowed for name. */
  int bStrict;         /* Non-zero if column requires exact type match. */
  const char *zDummy;  /* Dummy parameter, result unused. */
  unsigned i;

  /* strictMask is used for STRICT, strictMask|otherMask if STRICT is
   * not supplied.  zReplace provides an alternate type to expose to
   * the caller.
   */
  static struct {
    const char *zName;
    unsigned char strictMask;
    unsigned char otherMask;
    const char *zReplace;
  } kTypeInfo[] = {
    { "ANY",
      MASK_INTEGER | MASK_FLOAT | MASK_BLOB | MASK_TEXT | MASK_NULL,
      0, "",
    },
    { "ROWID",   MASK_INTEGER | MASK_ROWID,             0, "INTEGER", },
    { "INTEGER", MASK_INTEGER | MASK_NULL,              0, NULL, },
    { "FLOAT",   MASK_FLOAT | MASK_NULL,                MASK_INTEGER, NULL, },
    { "NUMERIC", MASK_INTEGER | MASK_FLOAT | MASK_NULL, MASK_TEXT, NULL, },
    { "TEXT",    MASK_TEXT | MASK_NULL,                 MASK_BLOB, NULL, },
    { "BLOB",    MASK_BLOB | MASK_NULL,                 0, NULL, },
  };

  if( !findWord(parameter, pzNameStart, pzNameEnd) ){
    return SQLITE_MISUSE;
  }

  /* Manifest typing, accept any storage type. */
  if( !findWord(*pzNameEnd, pzTypeStart, pzTypeEnd) ){
    *pzTypeEnd = *pzTypeStart = "";
    *pTypeMask = MASK_INTEGER | MASK_FLOAT | MASK_BLOB | MASK_TEXT | MASK_NULL;
    return SQLITE_OK;
  }

  nNameLen = *pzTypeEnd - *pzTypeStart;
  for( i=0; i<ArraySize(kTypeInfo); ++i ){
    if( ascii_strncasecmp(kTypeInfo[i].zName, *pzTypeStart, nNameLen)==0 ){
      break;
    }
  }
  if( i==ArraySize(kTypeInfo) ){
    return SQLITE_MISUSE;
  }

  zEnd = *pzTypeEnd;
  bStrict = 0;
  if( expectWord(zEnd, "STRICT", &zEnd) ){
    /* TODO(shess): Ick.  But I don't want another single-purpose
     * flag, either.
     */
    if( kTypeInfo[i].zReplace && !kTypeInfo[i].zReplace[0] ){
      return SQLITE_MISUSE;
    }
    bStrict = 1;
  }

  bNotNull = 0;
  if( expectWord(zEnd, "NOT", &zEnd) ){
    if( expectWord(zEnd, "NULL", &zEnd) ){
      bNotNull = 1;
    }else{
      /* Anything other than NULL after NOT is an error. */
      return SQLITE_MISUSE;
    }
  }

  /* Anything else is an error. */
  if( findWord(zEnd, &zDummy, &zDummy) ){
    return SQLITE_MISUSE;
  }

  *pTypeMask = kTypeInfo[i].strictMask;
  if( !bStrict ){
    *pTypeMask |= kTypeInfo[i].otherMask;
  }
  if( bNotNull ){
    *pTypeMask &= ~MASK_NULL;
  }
  if( kTypeInfo[i].zReplace ){
    *pzTypeStart = kTypeInfo[i].zReplace;
    *pzTypeEnd = *pzTypeStart + strlen(*pzTypeStart);
  }
  return SQLITE_OK;
}

/* Parse the arguments, placing type masks in *pTypes and the exposed
 * schema in *pzCreateSql (for sqlite3_declare_vtab).
 */
static int ParseColumnsAndGenerateCreate(unsigned nCols,
                                         const char *const *pCols,
                                         char **pzCreateSql,
                                         unsigned char *pTypes,
                                         char **pzErr){
  unsigned i;
  char *zCreateSql = sqlite3_mprintf("CREATE TABLE x(");
  if( !zCreateSql ){
    return SQLITE_NOMEM;
  }

  for( i=0; i<nCols; i++ ){
    const char *zSep = (i < nCols - 1 ? ", " : ")");
    const char *zNotNull = "";
    const char *zNameStart, *zNameEnd;
    const char *zTypeStart, *zTypeEnd;
    int rc = findNameAndType(pCols[i],
                             &zNameStart, &zNameEnd,
                             &zTypeStart, &zTypeEnd,
                             &pTypes[i]);
    if( rc!=SQLITE_OK ){
      *pzErr = sqlite3_mprintf("unable to parse column %d", i);
      sqlite3_free(zCreateSql);
      return rc;
    }

    if( !(pTypes[i]&MASK_NULL) ){
      zNotNull = " NOT NULL";
    }

    /* Add name and type to the create statement. */
    zCreateSql = sqlite3_mprintf("%z%.*s %.*s%s%s",
                                 zCreateSql,
                                 zNameEnd - zNameStart, zNameStart,
                                 zTypeEnd - zTypeStart, zTypeStart,
                                 zNotNull, zSep);
    if( !zCreateSql ){
      return SQLITE_NOMEM;
    }
  }

  *pzCreateSql = zCreateSql;
  return SQLITE_OK;
}

/* Helper function for initializing the module. */
/* argv[0] module name
 * argv[1] db name for virtual table
 * argv[2] virtual table name
 * argv[3] backing table name
 * argv[4] columns
 */
/* TODO(shess): Since connect isn't supported, could inline into
 * recoverCreate().
 */
/* TODO(shess): Explore cases where it would make sense to set *pzErr. */
static int recoverInit(
  sqlite3 *db,                        /* Database connection */
  void *pAux,                         /* unused */
  int argc, const char *const*argv,   /* Parameters to CREATE TABLE statement */
  sqlite3_vtab **ppVtab,              /* OUT: New virtual table */
  char **pzErr                        /* OUT: Error message, if any */
){
  const int kTypeCol = 4;  /* First argument with column type info. */
  Recover *pRecover;       /* Virtual table structure being created. */
  char *zDot;              /* Any dot found in "db.table" backing. */
  u32 iRootPage;           /* Root page of backing table. */
  char *zCreateSql;        /* Schema of created virtual table. */
  int rc;

  /* Require to be in the temp database. */
  if( ascii_strcasecmp(argv[1], "temp")!=0 ){
    *pzErr = sqlite3_mprintf("recover table must be in temp database");
    return SQLITE_MISUSE;
  }

  /* Need the backing table and at least one column. */
  if( argc<=kTypeCol ){
    *pzErr = sqlite3_mprintf("no columns specified");
    return SQLITE_MISUSE;
  }

  pRecover = sqlite3_malloc(sizeof(Recover));
  if( !pRecover ){
    return SQLITE_NOMEM;
  }
  memset(pRecover, 0, sizeof(*pRecover));
  pRecover->base.pModule = &recoverModule;
  pRecover->db = db;

  /* Parse out db.table, assuming main if no dot. */
  zDot = strchr(argv[3], '.');
  if( !zDot ){
    pRecover->zDb = sqlite3_strdup("main");
    pRecover->zTable = sqlite3_strdup(argv[3]);
  }else if( zDot>argv[3] && zDot[1]!='\0' ){
    pRecover->zDb = sqlite3_strndup(argv[3], zDot - argv[3]);
    pRecover->zTable = sqlite3_strdup(zDot + 1);
  }else{
    /* ".table" or "db." not allowed. */
    *pzErr = sqlite3_mprintf("ill-formed table specifier");
    recoverRelease(pRecover);
    return SQLITE_ERROR;
  }

  pRecover->nCols = argc - kTypeCol;
  pRecover->pTypes = sqlite3_malloc(pRecover->nCols);
  if( !pRecover->zDb || !pRecover->zTable || !pRecover->pTypes ){
    recoverRelease(pRecover);
    return SQLITE_NOMEM;
  }

  /* Require the backing table to exist. */
  /* TODO(shess): Be more pedantic about the form of the descriptor
   * string.  This already fails for poorly-formed strings, simply
   * because there won't be a root page, but it would make more sense
   * to be explicit.
   */
  rc = getRootPage(pRecover->db, pRecover->zDb, pRecover->zTable, &iRootPage);
  if( rc!=SQLITE_OK ){
    *pzErr = sqlite3_mprintf("unable to find backing table");
    recoverRelease(pRecover);
    return rc;
  }

  /* Parse the column definitions. */
  rc = ParseColumnsAndGenerateCreate(pRecover->nCols, argv + kTypeCol,
                                     &zCreateSql, pRecover->pTypes, pzErr);
  if( rc!=SQLITE_OK ){
    recoverRelease(pRecover);
    return rc;
  }

  rc = sqlite3_declare_vtab(db, zCreateSql);
  sqlite3_free(zCreateSql);
  if( rc!=SQLITE_OK ){
    recoverRelease(pRecover);
    return rc;
  }

  *ppVtab = (sqlite3_vtab *)pRecover;
  return SQLITE_OK;
}
