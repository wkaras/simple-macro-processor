/*
Copyright (c) 2016 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
  main program for Simple Macro Processor.
*/

#include <stdio.h>
#include <string.h>
#include "stralloc.h"
#include "trfile.h"
#include "macro.h"

/* results buffer */
#define SIZE_RES_BUF 16*1024
static char res_buf[SIZE_RES_BUF];

/* external function which defines most of the builtins */
const char *def_builtins(void);

/* maximum level of include file nesting */
#define MAX_INCLUDE_NEST 10

/* stack of input file structures */
static TR_DESC input_desc[MAX_INCLUDE_NEST + 1];
/* index of current input file structure */
static int input_desc_idx;

/*
  function to open a (traced) file
*/
static const char *open_input
  (
    const char *fname
  )
  {
    if (input_desc_idx == MAX_INCLUDE_NEST)
      return("too many nested include files");

    input_desc_idx++;

    if (strcmp(fname,"-") == 0)
      {
        if (tr_open((input_desc + input_desc_idx),(const char *) 0) !=
            S_TR_GOOD)
          {
            input_desc_idx--;
            return("error accessing standard input");
          }
      }
    else
      {
        if (tr_open((input_desc + input_desc_idx),fname) != S_TR_GOOD)
          {
            input_desc_idx--;
            return("error opening file");
          }
      }

    return((const char *) 0);
  }


/*
  include macro
*/
static const char *bi_include
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg != 2)
      return("include macro requires exactly 1 argument");

    return(open_input(arg[1]));
  }


/* pointer to output file structure */
static FILE *out_p;

/*
  opens file for output
*/
const char *open_output
  (
    /* name of file */
    const char *filename,
    /* mode to pass to fopen */
    const char *mode
  )
  {
    if ((out_p != stdout) && (out_p != (FILE *) 0))
      /* close current output file */
      if (fclose(out_p) < 0)
        return("error closing current output file");

    if (filename == (const char *) 0)
      out_p = (FILE *) 0;
    else if (strcmp(filename,"-") == 0)
      out_p = stdout;
    else
      {
        out_p = fopen(filename,mode);
        if (out_p == (FILE *) 0)
          return("error opening new output file");
      }

    return((const char *) 0);
  }


/*
  output macro
*/
const char *bi_output
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg > 2)
      return ("output macro requires 0 or 1 arguments");

    if (n_arg == 1)
      return(open_output(((const char *) 0),((const char *) 0)));
    else
      return(open_output(arg[1],"w"));
  }


/*
  append macro
*/
const char *bi_append
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg > 2)
      return ("append macro requires 0 or 1 arguments");

    if (n_arg == 1)
      return(open_output(((const char *) 0),((const char *) 0)));
    else
      return(open_output(arg[1],"a"));
  }


/*
  get a character, handling include levels.  returns value from
  TR fucntions.
*/
int get_next_char
  (
    /* pointer to character variable to put character into */
    char *c
  )
  {
    /* return value */
    int rv;


    for ( ; ; )
      {
        rv = tr_getc((input_desc + input_desc_idx),c);
        if (rv == S_TR_READ)
          {
            tr_print_error((input_desc + input_desc_idx),
              "error reading input");
            return(S_TR_READ);
          }
        else if (rv == S_TR_EOF)
	  {
            if (input_desc_idx == 0)
              return(S_TR_EOF);
            else
              {
	        rv = tr_close(input_desc + input_desc_idx);
	        input_desc_idx--;
                if (rv != S_TR_GOOD)
                  {
                    tr_print_error((input_desc + input_desc_idx),
                      "error closing include file");
                    return(rv);
                  }
              }
	  }
	else
	  /* no problems */
	  return(S_TR_GOOD);
      }
  }


int main
  (
    int argc,
    const char **argv
  )
  {
    const char *msg;
    char c;
    int rv;


    init_str_alloc();

    /* initialize macro processor */
    msg = mcr_init();
    if (msg != (const char *) 0)
      {
        fprintf(stderr,"%s\n",msg);
        return(-1);
      }

    /* define the builtin macros */
    msg = def_builtins();
    if (msg != (const char *) 0)
      {
        fprintf(stderr,"%s\n",msg);
        return(-1);
      }

    /* define include builtin */
    msg = mcr_def("include",(void *) bi_include,0);
    if (msg != (const char *) 0)
      {
        fprintf(stderr,"%s\n",msg);
        return(-1);
      }

    /* define output builtin */
    msg = mcr_def("output",(void *) bi_output,0);
    if (msg != (const char *) 0)
      {
        fprintf(stderr,"%s\n",msg);
        return(-1);
      }

    /* define append builtin */
    msg = mcr_def("append",(void *) bi_append,0);
    if (msg != (const char *) 0)
      {
        fprintf(stderr,"%s\n",msg);
        return(-1);
      }

    /* output goes to standard output by default */
    out_p = stdout;

    /* process input file */
    input_desc_idx = -1;
    if (argc > 1)
      msg = open_input(argv[1]);
    else
      msg = open_input("-");
    if (msg != (const char *) 0)
      {
        fprintf(stderr,"%s\n",msg);
        return(-1);
      }

    mcr_start_expand(argc,argv);
    mcr_result = res_buf;
    mcr_n_result = SIZE_RES_BUF;
    for ( ; ; )
      {
        rv = get_next_char(&c);
        if (rv == S_TR_EOF)
          break;
        if (rv != S_TR_GOOD)
          return(-1);

        msg = mcr_next_char(c);
        if (msg != (const char *) 0)
          {
            tr_print_error((input_desc + input_desc_idx),msg);
            return(-1);
          }

        if (mcr_result > res_buf)
          /* print result */
          {
            if (out_p != (FILE *) 0)
              {
                /* terminate result string */
                *mcr_result = (char) '\0';
                if (fputs(res_buf,out_p) == EOF)
                  {
                    fprintf(stderr,"error writing to output\n");
                    return(-1);
                  }
              }

            /* reset result buffer */
            mcr_result = res_buf;
            mcr_n_result = SIZE_RES_BUF;
          }
      }

    if (mcr_expanding())
      {
        tr_print_error((input_desc + 0),
        "input ended in middle of macro expansion");
        return(-1);
      }

    if (tr_close(input_desc + 0) != S_TR_GOOD)
      {
        fprintf(stderr,"cannot close the original input file\n");
        return(-1);
      }

    msg = open_output(((const char *) 0),((const char *) 0));
    if (msg != (const char *) 0)
      {
	fprintf(stderr,"%s\n",msg);
        return(-1);
      }


    return(0);
  }
