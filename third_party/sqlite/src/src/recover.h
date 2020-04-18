/* TODO(shess): sqliteicu.h is able to make this include without
** trouble.  It doesn't work when used with Chromium's SQLite.  For
** now the including code must include sqlite3.h first.
*/
/* #include "sqlite3.h" */

#ifdef __cplusplus
extern "C" {
#endif

/*
** Call to initialize the recover virtual-table modules (see recover.c).
**
** This could be loaded by default in main.c, but that would make the
** virtual table available to Web SQL.  Breaking it out allows only
** selected users to enable it (currently sql/recovery.cc).
*/
SQLITE_API
int chrome_sqlite3_recoverVtableInit(sqlite3 *db);

#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif
