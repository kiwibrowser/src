#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "liblouis.h"
#include "resolve_table.h"

#define ASSERT(test)  \
  do {                \
    if (!(test))      \
      result = 1;     \
  } while(0)          \
    
int
main(int argc, char **argv)
{
    
  /* ====================================================================== *
   * `-- resolve_table                                                      *
   *     |-- dir_1                                                          *
   *     |   |-- dir_1.1                                                    *
   *     |   |   `-- table_1.1.1                                            *
   *     |   |-- table_1.1                                                  *
   *     |   |-- table_1.2      ------------------------------              *
   *     |   `-- table_1.3 --> | include dir_1.1/table_1.1.1 |              *
   *     |-- dir_2             `-----------------------------               *
   *     |   |-- table_2.1      ------------------                          *
   *     |   `-- table_2.2 --> | include table_1 |                          *
   *     |-- table_1           `-----------------                           *
   *     |-- table_2      ------------------                                *
   *     |-- table_3 --> | include table_2 |                                *
   *     |               `-----------------                                 *
   *     |                --------------------------                        *
   *     |-- table_4 --> | include dir_1/table_1.3 |                        *
   *     |               `-------------------------                         *
   *     |                --------------------------                        *
   *     |-- table_5 --> | include dir_2/table_2.2 |                        *
   *     |               `-------------------------                         *
   *     |                ----------------------                            *
   *     `-- table_6 --> | dir_1.1/table_1.1.1 |                            *
   *                     `---------------------                             *
   * ====================================================================== */
  
  int result = 0;

  // this test relies on being in the test dir, so that it can test
  // finding tables by relative path
  if (chdir(TEST_SRC_DIR)) return 1;

  // Full path
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_1"));
  
  // File name not on LOUIS_TABLEPATH
  ASSERT (!lou_getTable ("table_1"));
  
  // File name on LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table", 1);
  ASSERT (lou_getTable ("table_1"));
  
  // First is full path, second is in same directory
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_1,"
			"table_2"));
  
  // First is full path, second is not in same directory
  ASSERT (!lou_getTable ("tables/resolve_table/table_1,"
			 "table_1.1.1"));
  
  // Two full paths
  ASSERT (lou_getTable ("tables/resolve_table/dir_1/table_1.1,"
			"tables/resolve_table/dir_2/table_2.1"));
  
  // First is full path, second is on LOUIS_TABLEPATH, third is in same
  // directory as first
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table/dir_2", 1);
  ASSERT (lou_getTable ("tables/resolve_table/dir_1/table_1.1,"
			"table_2.1,"
			"table_1.2"));
  
  // First is full path, second is in subdirectory
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_1,"
			"dir_1/table_1.1"));
  
  // Two file names in different directories, but both on LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table/dir_1,"
			     "tables/resolve_table/dir_2", 1);
  ASSERT (lou_getTable ("table_1.2,"
			"table_2.1"));
  
  // First is file name on LOUIS_TABLEPATH, second is full path, third is in
  // same directory as second
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table", 1);
  ASSERT (!lou_getTable ("table_1,"
			 "tables/resolve_table/dir_1/table_1.1,"
			 "table_1.2"));
  
  // Full path, include table in same directory
  setenv ("LOUIS_TABLEPATH", "", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_3"));
  
  // Full path, include table in subdirectory
  ASSERT (lou_getTable ("tables/resolve_table/dir_1/table_1.3"));
  
  // Full path, include table in subdirectory, from there include table in
  // sub-subdirectory
  ASSERT (lou_getTable ("tables/resolve_table/table_4"));
  
  // Full path, include table in subdirectory, from there include table in
  // first directory
  ASSERT (!lou_getTable ("tables/resolve_table/table_5"));
  
  // Full path, include table in subdirectory, from there include table on
  // LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_5"));
  
  // Full path, include table in subdirectory of LOUIS_TABLEPATH
  setenv ("LOUIS_TABLEPATH", "tables/resolve_table/dir_1", 1);
  ASSERT (lou_getTable ("tables/resolve_table/table_6"));
  
  return result;
  
}
