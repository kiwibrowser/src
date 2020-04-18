%{
/* Parser for linker scripts.
   Copyright (C) 2001-2011 Red Hat, Inc.
   This file is part of elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2001.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <error.h>
#include <libintl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <system.h>
#include <ld.h>

/* The error handler.  */
static void yyerror (const char *s);

/* Some helper functions we need to construct the data structures
   describing information from the file.  */
static struct expression *new_expr (int tag);
static struct input_section_name *new_input_section_name (const char *name,
							  bool sort_flag);
static struct input_rule *new_input_rule (int tag);
static struct output_rule *new_output_rule (int tag);
static struct assignment *new_assignment (const char *variable,
					  struct expression *expression,
					  bool provide_flag);
static void new_segment (int mode, struct output_rule *output_rule);
static struct filename_list *new_filename_listelem (const char *string);
static void add_inputfiles (struct filename_list *fnames);
static struct id_list *new_id_listelem (const char *str);
 static struct filename_list *mark_as_needed (struct filename_list *listp);
static struct version *new_version (struct id_list *local,
				    struct id_list *global);
static struct version *merge_versions (struct version *one,
				       struct version *two);
static void add_versions (struct version *versions);

extern int yylex (void);
%}

%union {
  uintmax_t num;
  enum expression_tag op;
  char *str;
  struct expression *expr;
  struct input_section_name *sectionname;
  struct filemask_section_name *filemask_section_name;
  struct input_rule *input_rule;
  struct output_rule *output_rule;
  struct assignment *assignment;
  struct filename_list *filename_list;
  struct version *version;
  struct id_list *id_list;
}

%token kADD_OP
%token kALIGN
%token kAS_NEEDED
%token kENTRY
%token kEXCLUDE_FILE
%token <str> kFILENAME
%token kGLOBAL
%token kGROUP
%token <str> kID
%token kINPUT
%token kINTERP
%token kKEEP
%token kLOCAL
%token <num> kMODE
%token kMUL_OP
%token <num> kNUM
%token kOUTPUT_FORMAT
%token kPAGESIZE
%token kPROVIDE
%token kSEARCH_DIR
%token kSEGMENT
%token kSIZEOF_HEADERS
%token kSORT
%token kVERSION
%token kVERSION_SCRIPT

%left '|'
%left '&'
%left ADD_OP
%left MUL_OP '*'

%type <op> kADD_OP
%type <op> kMUL_OP
%type <str> filename_id
%type <str> filename_id_star
%type <str> exclude_opt
%type <expr> expr
%type <sectionname> sort_opt_name
%type <filemask_section_name> sectionname
%type <input_rule> inputsection
%type <input_rule> inputsections
%type <output_rule> outputsection
%type <output_rule> outputsections
%type <assignment> assignment
%type <filename_list> filename_id_list
%type <filename_list> filename_id_listelem
%type <version> versionlist
%type <version> version
%type <version> version_stmt_list
%type <version> version_stmt
%type <id_list> filename_id_star_list

%expect 16

%%

script_or_version:
		  file
		| kVERSION_SCRIPT versionlist
		    { add_versions ($2); }
		;

file:		  file content
		| content
		;

content:	  kENTRY '(' kID ')' ';'
		    {
		      if (likely (ld_state.entry == NULL))
			ld_state.entry = $3;
		    }
		| kSEARCH_DIR '(' filename_id ')' ';'
		    {
		      ld_new_searchdir ($3);
		    }
		| kPAGESIZE '(' kNUM ')' ';'
		    {
		      if (likely (ld_state.pagesize == 0))
			ld_state.pagesize = $3;
		    }
		| kINTERP '(' filename_id ')' ';'
		    {
		      if (likely (ld_state.interp == NULL)
			  && ld_state.file_type != dso_file_type)
			ld_state.interp = $3;
		    }
		| kSEGMENT kMODE '{' outputsections '}'
		    {
		      new_segment ($2, $4);
		    }
		| kSEGMENT error '{' outputsections '}'
		    {
		      fputs_unlocked (gettext ("mode for segment invalid\n"),
				      stderr);
		      new_segment (0, $4);
		    }
		| kGROUP '(' filename_id_list ')'
		    {
		      /* First little optimization.  If there is only one
			 file in the group don't do anything.  */
		      if ($3 != $3->next)
			{
			  $3->next->group_start = 1;
			  $3->group_end = 1;
			}
		      add_inputfiles ($3);
		    }
		| kINPUT '(' filename_id_list ')'
		    { add_inputfiles ($3); }
		| kAS_NEEDED '(' filename_id_list ')'
		    { add_inputfiles (mark_as_needed ($3)); }
		| kVERSION '{' versionlist '}'
		    { add_versions ($3); }
		| kOUTPUT_FORMAT '(' filename_id ')'
		    { /* XXX TODO */ }
		;

outputsections:	  outputsections outputsection
		    {
		      $2->next = $1->next;
		      $$ = $1->next = $2;
		    }
		| outputsection
		    { $$ = $1; }
		;

outputsection:	  assignment ';'
		    {
		      $$ = new_output_rule (output_assignment);
		      $$->val.assignment = $1;
		    }
		| kID '{' inputsections '}'
		    {
		      $$ = new_output_rule (output_section);
		      $$->val.section.name = $1;
		      $$->val.section.input = $3->next;
		      if (ld_state.strip == strip_debug
			  && ebl_debugscn_p (ld_state.ebl, $1))
			$$->val.section.ignored = true;
		      else
			$$->val.section.ignored = false;
		      $3->next = NULL;
		    }
		| kID ';'
		    {
		      /* This is a short cut for "ID { *(ID) }".  */
		      $$ = new_output_rule (output_section);
		      $$->val.section.name = $1;
		      $$->val.section.input = new_input_rule (input_section);
		      $$->val.section.input->next = NULL;
		      $$->val.section.input->val.section =
			(struct filemask_section_name *)
			  obstack_alloc (&ld_state.smem,
					 sizeof (struct filemask_section_name));
		      $$->val.section.input->val.section->filemask = NULL;
		      $$->val.section.input->val.section->excludemask = NULL;
		      $$->val.section.input->val.section->section_name =
			new_input_section_name ($1, false);
		      $$->val.section.input->val.section->keep_flag = false;
		      if (ld_state.strip == strip_debug
			  && ebl_debugscn_p (ld_state.ebl, $1))
			$$->val.section.ignored = true;
		      else
			$$->val.section.ignored = false;
		    }
		;

assignment:	  kID '=' expr
		    { $$ = new_assignment ($1, $3, false); }
		| kPROVIDE '(' kID '=' expr ')'
		    { $$ = new_assignment ($3, $5, true); }
		;

inputsections:	  inputsections inputsection
		    {
		      $2->next = $1->next;
		      $$ = $1->next = $2;
		    }
		| inputsection
		    { $$ = $1; }
		;

inputsection:	  sectionname
		    {
		      $$ = new_input_rule (input_section);
		      $$->val.section = $1;
		    }
		| kKEEP '(' sectionname ')'
		    {
		      $3->keep_flag = true;

		      $$ = new_input_rule (input_section);
		      $$->val.section = $3;
		    }
		| assignment ';'
		    {
		      $$ = new_input_rule (input_assignment);
		      $$->val.assignment = $1;
		    }
		;

sectionname:	  filename_id_star '(' exclude_opt sort_opt_name ')'
		    {
		      $$ = (struct filemask_section_name *)
			obstack_alloc (&ld_state.smem, sizeof (*$$));
		      $$->filemask = $1;
		      $$->excludemask = $3;
		      $$->section_name = $4;
		      $$->keep_flag = false;
		    }
		;

sort_opt_name:	  kID
		    { $$ = new_input_section_name ($1, false); }
		| kSORT '(' kID ')'
		    { $$ = new_input_section_name ($3, true); }
		;

exclude_opt:	  kEXCLUDE_FILE '(' filename_id ')'
		    { $$ = $3; }
		|
		    { $$ = NULL; }
		;

expr:		  kALIGN '(' expr ')'
		    {
		      $$ = new_expr (exp_align);
		      $$->val.child = $3;
		    }
		| '(' expr ')'
		    { $$ = $2; }
		| expr '*' expr
		    {
		      $$ = new_expr (exp_mult);
		      $$->val.binary.left = $1;
		      $$->val.binary.right = $3;
		    }
		| expr kMUL_OP expr
		    {
		      $$ = new_expr ($2);
		      $$->val.binary.left = $1;
		      $$->val.binary.right = $3;
		    }
		| expr kADD_OP expr
		    {
		      $$ = new_expr ($2);
		      $$->val.binary.left = $1;
		      $$->val.binary.right = $3;
		    }
		| expr '&' expr
		    {
		      $$ = new_expr (exp_and);
		      $$->val.binary.left = $1;
		      $$->val.binary.right = $3;
		    }
		| expr '|' expr
		    {
		      $$ = new_expr (exp_or);
		      $$->val.binary.left = $1;
		      $$->val.binary.right = $3;
		    }
		| kNUM
		    {
		      $$ = new_expr (exp_num);
		      $$->val.num = $1;
		    }
		| kID
		    {
		      $$ = new_expr (exp_id);
		      $$->val.str = $1;
		    }
		| kSIZEOF_HEADERS
		    { $$ = new_expr (exp_sizeof_headers); }
		| kPAGESIZE
		    { $$ = new_expr (exp_pagesize); }
		;

filename_id_list: filename_id_list comma_opt filename_id_listelem
		    {
		      $3->next = $1->next;
		      $$ = $1->next = $3;
		    }
		| filename_id_listelem
		    { $$ = $1; }
		;

comma_opt:	  ','
		|
		;

filename_id_listelem: kGROUP '(' filename_id_list ')'
		    {
		      /* First little optimization.  If there is only one
			 file in the group don't do anything.  */
		      if ($3 != $3->next)
			{
			  $3->next->group_start = 1;
			  $3->group_end = 1;
			}
		      $$ = $3;
		    }
		| kAS_NEEDED '(' filename_id_list ')'
		    { $$ = mark_as_needed ($3); }
		| filename_id
		    { $$ = new_filename_listelem ($1); }
		;


versionlist:	  versionlist version
		    {
		      $2->next = $1->next;
		      $$ = $1->next = $2;
		    }
		| version
		    { $$ = $1; }
		;

version:	  '{' version_stmt_list '}' ';'
		    {
		      $2->versionname = "";
		      $2->parentname = NULL;
		      $$ = $2;
		    }
		| filename_id '{' version_stmt_list '}' ';'
		    {
		      $3->versionname = $1;
		      $3->parentname = NULL;
		      $$ = $3;
		    }
		| filename_id '{' version_stmt_list '}' filename_id ';'
		    {
		      $3->versionname = $1;
		      $3->parentname = $5;
		      $$ = $3;
		    }
		;

version_stmt_list:
		  version_stmt_list version_stmt
		    { $$ = merge_versions ($1, $2); }
		| version_stmt
		    { $$ = $1; }
		;

version_stmt:	  kGLOBAL filename_id_star_list
		    { $$ = new_version (NULL, $2); }
		| kLOCAL filename_id_star_list
		    { $$ = new_version ($2, NULL); }
		;

filename_id_star_list:
		  filename_id_star_list filename_id_star ';'
		    {
		      struct id_list *newp = new_id_listelem ($2);
		      newp->next = $1->next;
		      $$ = $1->next = newp;
		    }
		| filename_id_star ';'
		    { $$ = new_id_listelem ($1); }
		;

filename_id:	  kFILENAME
		    { $$ = $1; }
		| kID
		    { $$ = $1; }
		;

filename_id_star: filename_id
		    { $$ = $1; }
		| '*'
		    { $$ = NULL; }
		;

%%

static void
yyerror (const char *s)
{
  error (0, 0, (ld_scan_version_script
		? gettext ("while reading version script '%s': %s at line %d")
		: gettext ("while reading linker script '%s': %s at line %d")),
	 ldin_fname, gettext (s), ldlineno);
}


static struct expression *
new_expr (int tag)
{
  struct expression *newp = (struct expression *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->tag = tag;
  return newp;
}


static struct input_section_name *
new_input_section_name (const char *name, bool sort_flag)
{
  struct input_section_name *newp = (struct input_section_name *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->name = name;
  newp->sort_flag = sort_flag;
  return newp;
}


static struct input_rule *
new_input_rule (int tag)
{
  struct input_rule *newp = (struct input_rule *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->tag = tag;
  newp->next = newp;
  return newp;
}


static struct output_rule *
new_output_rule (int tag)
{
  struct output_rule *newp = (struct output_rule *)
    memset (obstack_alloc (&ld_state.smem, sizeof (*newp)),
	    '\0', sizeof (*newp));

  newp->tag = tag;
  newp->next = newp;
  return newp;
}


static struct assignment *
new_assignment (const char *variable, struct expression *expression,
		bool provide_flag)
{
  struct assignment *newp = (struct assignment *)
    obstack_alloc (&ld_state.smem, sizeof (*newp));

  newp->variable = variable;
  newp->expression = expression;
  newp->sym = NULL;
  newp->provide_flag = provide_flag;

  /* Insert the symbol into a hash table.  We will later have to matc*/
  return newp;
}


static void
new_segment (int mode, struct output_rule *output_rule)
{
  struct output_segment *newp;

  newp
    = (struct output_segment *) obstack_alloc (&ld_state.smem, sizeof (*newp));
  newp->mode = mode;
  newp->next = newp;

  newp->output_rules = output_rule->next;
  output_rule->next = NULL;

  /* Enqueue the output segment description.  */
  if (ld_state.output_segments == NULL)
    ld_state.output_segments = newp;
  else
    {
      newp->next = ld_state.output_segments->next;
      ld_state.output_segments = ld_state.output_segments->next = newp;
    }

  /* If the output file should be stripped of all symbol set the flag
     in the structures of all output sections.  */
  if (mode == 0 && ld_state.strip == strip_all)
    {
      struct output_rule *runp;

      for (runp = newp->output_rules; runp != NULL; runp = runp->next)
	if (runp->tag == output_section)
	  runp->val.section.ignored = true;
    }
}


static struct filename_list *
new_filename_listelem (const char *string)
{
  struct filename_list *newp;

  /* We use calloc and not the obstack since this object can be freed soon.  */
  newp = (struct filename_list *) xcalloc (1, sizeof (*newp));
  newp->name = string;
  newp->next = newp;
  return newp;
}


static struct filename_list *
mark_as_needed (struct filename_list *listp)
{
  struct filename_list *runp = listp;
  do
    {
      runp->as_needed = true;
      runp = runp->next;
    }
  while (runp != listp);

  return listp;
}


static void
add_inputfiles (struct filename_list *fnames)
{
  assert (fnames != NULL);

  if (ld_state.srcfiles == NULL)
    ld_state.srcfiles = fnames;
  else
    {
      struct filename_list *first = ld_state.srcfiles->next;

      ld_state.srcfiles->next = fnames->next;
      fnames->next = first;
      ld_state.srcfiles->next = fnames;
    }
}


static _Bool
special_char_p (const char *str)
{
  while (*str != '\0')
    {
      if (__builtin_expect (*str == '*', 0)
	  || __builtin_expect (*str == '?', 0)
	  || __builtin_expect (*str == '[', 0))
	return true;

      ++str;
    }

  return false;
}


static struct id_list *
new_id_listelem (const char *str)
{
  struct id_list *newp;

  newp = (struct id_list *) obstack_alloc (&ld_state.smem, sizeof (*newp));
  if (str == NULL)
    newp->u.id_type = id_all;
  else if (__builtin_expect (special_char_p (str), false))
    newp->u.id_type = id_wild;
  else
    newp->u.id_type = id_str;
  newp->id = str;
  newp->next = newp;

  return newp;
}


static struct version *
new_version (struct id_list *local, struct id_list *global)
{
  struct version *newp;

  newp = (struct version *) obstack_alloc (&ld_state.smem, sizeof (*newp));
  newp->next = newp;
  newp->local_names = local;
  newp->global_names = global;
  newp->versionname = NULL;
  newp->parentname = NULL;

  return newp;
}


static struct version *
merge_versions (struct version *one, struct version *two)
{
  assert (two->local_names == NULL || two->global_names == NULL);

  if (two->local_names != NULL)
    {
      if (one->local_names == NULL)
	one->local_names = two->local_names;
      else
	{
	  two->local_names->next = one->local_names->next;
	  one->local_names = one->local_names->next = two->local_names;
	}
    }
  else
    {
      if (one->global_names == NULL)
	one->global_names = two->global_names;
      else
	{
	  two->global_names->next = one->global_names->next;
	  one->global_names = one->global_names->next = two->global_names;
	}
    }

  return one;
}


static void
add_id_list (const char *versionname, struct id_list *runp, _Bool local)
{
  struct id_list *lastp = runp;

  if (runp == NULL)
    /* Nothing to do.  */
    return;

  /* Convert into a simple single-linked list.  */
  runp = runp->next;
  assert (runp != NULL);
  lastp->next = NULL;

  do
    if (runp->u.id_type == id_str)
      {
	struct id_list *curp;
	struct id_list *defp;
	unsigned long int hval = elf_hash (runp->id);

	curp = runp;
	runp = runp->next;

	defp = ld_version_str_tab_find (&ld_state.version_str_tab, hval, curp);
	if (defp != NULL)
	  {
	    /* There is already a version definition for this symbol.  */
	    while (strcmp (defp->u.s.versionname, versionname) != 0)
	      {
		if (defp->next == NULL)
		  {
		    /* No version like this so far.  */
		    defp->next = curp;
		    curp->u.s.local = local;
		    curp->u.s.versionname = versionname;
		    curp->next = NULL;
		    defp = NULL;
		    break;
		  }

		defp = defp->next;
	      }

	    if (defp != NULL && defp->u.s.local != local)
	      error (EXIT_FAILURE, 0, versionname[0] == '\0'
		     ? gettext ("\
symbol '%s' is declared both local and global for unnamed version")
		     : gettext ("\
symbol '%s' is declared both local and global for version '%s'"),
		     runp->id, versionname);
	  }
	else
	  {
	    /* This is the first version definition for this symbol.  */
	    ld_version_str_tab_insert (&ld_state.version_str_tab, hval, curp);

	    curp->u.s.local = local;
	    curp->u.s.versionname = versionname;
	    curp->next = NULL;
	  }
      }
    else if (runp->u.id_type == id_all)
      {
	if (local)
	  {
	    if (ld_state.default_bind_global)
	      error (EXIT_FAILURE, 0,
		     gettext ("default visibility set as local and global"));
	    ld_state.default_bind_local = true;
	  }
	else
	  {
	    if (ld_state.default_bind_local)
	      error (EXIT_FAILURE, 0,
		     gettext ("default visibility set as local and global"));
	    ld_state.default_bind_global = true;
	  }

	runp = runp->next;
      }
    else
      {
	assert (runp->u.id_type == id_wild);
	/* XXX TBI */
	abort ();
      }
  while (runp != NULL);
}


static void
add_versions (struct version *versions)
{
  struct version *lastp = versions;

  if (versions == NULL)
    return;

  /* Convert into a simple single-linked list.  */
  versions = versions->next;
  assert (versions != NULL);
  lastp->next = NULL;

  do
    {
      add_id_list (versions->versionname, versions->local_names, true);
      add_id_list (versions->versionname, versions->global_names, false);

      versions = versions->next;
    }
  while (versions != NULL);
}
