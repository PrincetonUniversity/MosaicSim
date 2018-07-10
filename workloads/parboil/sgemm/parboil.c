/*
 * (c) 2007 The Board of Trustees of the University of Illinois.
 */

#include "parboil.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Free an array of owned strings. */
static void
free_string_array(char **string_array)
{
  char **p;

  if (!string_array) return;
  for (p = string_array; *p; p++) free(*p);
  free(string_array);
}

/* Parse a comma-delimited list of strings into an
 * array of strings. */
static char ** 
read_string_array(char *in)
{
  char **ret;
  int i;
  int count;			/* Number of items in the input */
  char *substring;		/* Current substring within 'in' */

  /* Count the number of items in the string */
  count = 1;
  for (i = 0; in[i]; i++) if (in[i] == ',') count++;

  /* Allocate storage */
  ret = (char **)malloc((count + 1) * sizeof(char *));

  /* Create copies of the strings from the list */
  substring = in;
  for (i = 0; i < count; i++) {
    char *substring_end;
    int substring_length;

    /* Find length of substring */
    for (substring_end = substring;
	 (*substring_end != ',') && (*substring_end != 0);
	 substring_end++);

    substring_length = substring_end - substring;

    /* Allocate memory and copy the substring */
    ret[i] = (char *)malloc(substring_length + 1);
    memcpy(ret[i], substring, substring_length);
    ret[i][substring_length] = 0;
    printf("%s input  \n", ret[i]);

    /* go to next substring */
    substring = substring_end + 1;
  }
  ret[i] = NULL;		/* Write the sentinel value */

  return ret;
}

struct argparse {
  int argc;			/* Number of arguments.  Mutable. */
  char **argv;			/* Argument values.  Immutable. */

  int argn;			/* Current argument number. */
  char **argv_get;		/* Argument value being read. */
  char **argv_put;		/* Argument value being written.
				 * argv_put <= argv_get. */
};

static void
initialize_argparse(struct argparse *ap, int argc, char **argv)
{
  ap->argc = argc;
  ap->argn = 0;
  ap->argv_get = ap->argv_put = ap->argv = argv;
}

static void
finalize_argparse(struct argparse *ap)
{
  /* Move the remaining arguments */
  for(; ap->argn < ap->argc; ap->argn++)
    *ap->argv_put++ = *ap->argv_get++;
}

/* Delete the current argument. */
static void
delete_argument(struct argparse *ap)
{
  if (ap->argn >= ap->argc) {
    fprintf(stderr, "delete_argument\n");
  }
  ap->argc--;
  ap->argv_get++;
}

/* Go to the next argument.  Also, move the current argument to its
 * final location in argv. */
static void
next_argument(struct argparse *ap)
{
  if (ap->argn >= ap->argc) {
    fprintf(stderr, "next_argument\n");
  }
  /* Move argument to its new location. */
  *ap->argv_put++ = *ap->argv_get++;
  ap->argn++;
}

static int
is_end_of_arguments(struct argparse *ap)
{
  return ap->argn == ap->argc;
}

static char *
get_argument(struct argparse *ap)
{
  return *ap->argv_get;
}

static char *
consume_argument(struct argparse *ap)
{
  char *ret = get_argument(ap);
  delete_argument(ap);
  return ret;
}

struct pb_Parameters *
pb_ReadParameters(int *_argc, char **argv)
{
  char *err_message;
  struct argparse ap;
  struct pb_Parameters *ret =
    (struct pb_Parameters *)malloc(sizeof(struct pb_Parameters));

  /* Initialize the parameters structure */
  ret->outFile = NULL;
  ret->inpFiles = (char **)malloc(sizeof(char *));
  ret->inpFiles[0] = NULL;

  /* Each argument */
  initialize_argparse(&ap, *_argc, argv);
  while(!is_end_of_arguments(&ap)) {
    char *arg = get_argument(&ap);
    /* Single-character flag */
    if ((arg[0] == '-') && (arg[1] != 0) && (arg[2] == 0)) {
      delete_argument(&ap);	/* This argument is consumed here */

      switch(arg[1]) {
      case 'o':			/* Output file name */
	if (is_end_of_arguments(&ap))
	  {
	    err_message = "Expecting file name after '-o'\n";
	    goto error;
	  }
	free(ret->outFile);
	ret->outFile = strdup(consume_argument(&ap));
	break;
      case 'i':			/* Input file name */
	if (is_end_of_arguments(&ap))
	  {
	    err_message = "Expecting file name after '-i'\n";
	    goto error;
	  }
	ret->inpFiles = read_string_array(consume_argument(&ap));
	break;
      case '-':			/* End of options */
	goto end_of_options;
      default:
	err_message = "Unexpected command-line parameter\n";
	goto error;
      }
    }
    else {
      /* Other parameters are ignored */
      next_argument(&ap);
    }
  } /* end for each argument */

 end_of_options:
  *_argc = ap.argc;		/* Save the modified argc value */
  finalize_argparse(&ap);

  return ret;

 error:
  fputs(err_message, stderr);
  pb_FreeParameters(ret);
  return NULL;
}

void
pb_FreeParameters(struct pb_Parameters *p)
{
  char **cpp;

  free(p->outFile);
  free_string_array(p->inpFiles);
  free(p);
}

int
pb_Parameters_CountInputs(struct pb_Parameters *p)
{
  int n;

  for (n = 0; p->inpFiles[n]; n++);
  return n;
}

