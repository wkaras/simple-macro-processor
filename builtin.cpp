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
  functions for built-in macros for smac
*/

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "macro.h"
#include "calc.h"


/*
  local function to copy long int as string into output without
  evalation
*/
static const char *outnum
  (
    long int num
  )
  {
    /* buffer to contain numeric result in string form */
    char num_str[((sizeof(long int) * CHAR_BIT) / 3) + 5];

    const char *p,*q;


    (void) sprintf(num_str,"%ld",num);
    q = num_str;
    while (*q != (char) '\0')
      {
        p = mcr_noeval_char(*(q++));
        if (p != (const char *) 0)
          return(p);
      }

    return((const char *) 0);
  }


/*
  set associates one or more macro names with a body
*/
static const char *bi_set
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p;
    int i;


    if (n_arg < 3)
      return("set macro requires at least 2 arguments");

    /* make n_arg = index of last argument */
    n_arg--;

    for (i = 1; i < n_arg; i++)
      {
        /* define the macro */
        p = mcr_def(arg[i],(void *) arg[n_arg],1);
        if (p != (const char *) 0)
	  return(p);
      }

    return((const char *) 0);
  }


/*
  let associates one or more macro names with the
  evaluated result of a numeric expression
*/
static const char *bi_let
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p;
    int i;
    long int value;
    /* buffer to contain numeric result in string form */
    char num_str[((sizeof(long int) * CHAR_BIT) / 3) + 5];


    if (n_arg < 3)
      return("let macro requires at least 2 arguments");

    /* make n_arg = index of last argument */
    n_arg--;

    p = calc(arg[n_arg],&value);
    if (p != (const char *) 0)
      return(p);

    (void) sprintf(num_str,"%ld",value);

    for (i = 1; i < n_arg; i++)
      {
        /* define the macro */
        p = mcr_def(arg[i],(void *) num_str,1);
        if (p != (const char *) 0)
	  return(p);
      }

    return((const char *) 0);
  }


/*
  evaluate numeric expression
*/
static const char *bi_calc
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p;
    long int r;


    if (n_arg != 2)
      return("calc macro requires exactly 1 argument");

    p = calc(arg[1],&r);
    if (p != (const char *) 0)
      return(p);

    return(outnum(r));
  }


/*
  expand macro
*/
static const char *bi_expand
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p,*q;


    if (n_arg != 2)
      return("expand macro requires exactly 1 argument");

    p = arg[1];
    while (*p != (char) '\0')
      {
        q = mcr_next_char(*(p++));
        if (q != (const char *) 0)
          return(q);
      }

    return((const char *) 0);
  }


/*
  if macro
*/
static const char *bi_if
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p;
    long int r;


    if ((n_arg < 3) || (n_arg > 4))
      return("if macro requires 2 or 3 arguments");

    p = calc(arg[1],&r);
    if (p != (const char *) 0)
      return(p);

    /* when calling bi_expand, first argument is not used, only
       second */

    if (r)
      return(bi_expand(2,arg + 1));
    else if (n_arg == 4)
      return(bi_expand(2,arg + 2));

    return((const char *) 0);
  }


/*
  repeat macro
*/
static const char *bi_repeat
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p,*q;
    long int r;


    if (n_arg != 3)
      return("repeat macro requires exactly 2 arguments");

    /* get number of times to repeat */
    p = calc(arg[2],&r);
    if (p != (const char *) 0)
      return(p);

    while (r--)
      {
	q = arg[1];
	while (*q != (char) '\0')
	  {
	    p = mcr_noeval_char(*(q++));
	    if (p != (const char *) 0)
	      return(p);
	  }
      }

    return((const char *) 0);
  }


/*
  check if argument is null
*/
static const char *bi_null
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg != 2)
      return("null macro requires exactly 1 argument");

    if (arg[1][0] == (char) '\0')
      return(mcr_noeval_char((char) '1'));
    else
      return(mcr_noeval_char((char) '0'));
  }


/*
  try to find first string within second.  returns 1 offset location
  of where first string starts in second.  if not found, returns 0.
*/
static const char *bi_index
  (
    int n_arg,
    const char **arg
  )
  {
    const char *start_match,*p,*q;


    if (n_arg != 3)
      return("index macro requires exactly 2 arguments");

    start_match = arg[2];
    q = arg[1];
    while (*start_match != (char) '\0')
      {
        if (*start_match == *q)
          {
            p = start_match;
            do
              {
                if (*(++q) == (char) '\0')
                  /* match has been found */
                  return(outnum(
                    ((long int) (start_match - arg[2])) + 1L));
              }
            while (*(++p) == *q);
	    q = arg[1];
          }
        start_match++;
      }

    return(mcr_noeval_char((char) '0'));
  }


/*
  returns length of its argument
*/
static const char *bi_length
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg != 2)
      return("length macro requires exactly 1 argument");

    return(outnum((long int) strlen(arg[1])));
  }


/*
  returns a substring.  first argument is original string.  second
  is base 1 offset of start of substring.  third is length of
  substring.
*/
static const char *bi_substring
  (
    int n_arg,
    const char **arg
  )
  {
    /* length of original string */
    long int len;

    long int start,count;
    const char *p,*q;


    if (n_arg != 4)
    if ((n_arg < 3) || (n_arg > 4))
      return("substring macro requires 2 or 3 arguments");

    len = (long int) strlen(arg[1]);
    p = calc(arg[2],&start);
    if (p != (const char *) 0)
      return(p);
    if (n_arg == 4)
      {
        p = calc(arg[3],&count);
        if (p != (const char *) 0)
          return(p);
      }
    else
      count = len - start + 1L;

    if ((start < 1L) || (count < 1L) || ((start - 1L + count) > len))
      return("illegal substring");

    q = arg[1] + start - 1L;
    while (count--)
      {
        p = mcr_noeval_char(*(q++));
        if (p != (const char *) 0)
          return(p);
      }

    return((const char *) 0);
  }


/* flag which indicates break macro invoked within loop macro */
static int break_flag;

/*
  break macro
*/
static const char *bi_break
  (
    int n_arg,
    const char **
  )
  {
    if (n_arg != 1)
      return("break macro should have no arguments");

    break_flag = 1;
    return((const char *) 0);
  }


/*
  loop
*/
static const char *bi_loop
  (
    int n_arg,
    const char **arg
  )
  {
    int i;
    const char *p;


    if (n_arg < 2)
      return("loop macro must have at least one argument");

    break_flag = 0;
    for ( ; ; )
      for (i = 1; i < n_arg; i++)
        {
          /* bi_expand() requires 2 args but ignores first one */
          p = bi_expand(2,arg - 1 + i);
          if (p != (const char *) 0)
            return(p);
          if (break_flag)
	    {
              /* reset so we don't pop out of outer loops */
	      break_flag = 0;
              return((const char *) 0);
            }
        }
  }


/*
  writes the unsigned numeric value of the first byte in the
  argument to the output
*/
static const char *bi_numeric
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg != 2)
      return("numeric macro requires exactly 1 argument");

    return(outnum((long int) ((unsigned long int) arg[1][0])));
  }


/*
  convert a numeric expression to a single byte
*/
static const char *bi_byte
  (
    int n_arg,
    const char **arg
  )
  {
    const char *p;
    long int r;


    if (n_arg != 2)
      return("byte macro requires exactly 1 argument");

    p = calc(arg[1],&r);
    if (p != (const char *) 0)
      return(p);

    return(mcr_noeval_char((char) r));
  }


const char *bad_2nd_arg = "2nd argument is not =, >, <, <>, <= or >=";

/*
  compares stings.  compares first and third argument.  second argument
  gives relation (=, >, <, <>, <=, or >=) to use in comparison.  writes
  1 to output if comparison is true, 0 if false.
*/
static const char *bi_string_compare
  (
    int n_arg,
    const char **arg
  )
  {
    int r,c1,c2,bool_result;


    if (n_arg != 4)
      return("string_compare requires exactly 3 arguments");

    r = strcmp(arg[1],arg[3]);
    c1 = (int) arg[2][0];
    if (c1 == '\0')
      return(bad_2nd_arg);
    c2 = (int) arg[2][1];
    if (c2 == '\0')
      {
        if (c1 == '=')
          bool_result = (r == 0);
        else if (c1 == '>')
          bool_result = (r > 0);
        else if (c1 == '<')
          bool_result = (r < 0);
        else
          return(bad_2nd_arg);
      }
    else
      {
        if (arg[2][2] != (char) '\0')
          return(bad_2nd_arg);

        if (c1 == '>')
          {
            if (c2 == '=')
              bool_result = (r >= 0);
            else
              return(bad_2nd_arg);
          }
        else if (c1 == '<')
          {
            if (c2 == '>')
              bool_result = (r != 0);
            else if (c2 == '=')
              bool_result = (r <= 0);
            else
              return(bad_2nd_arg);
          }
      }

    if (bool_result)
      return(mcr_noeval_char((char) '1'));
    else
      return(mcr_noeval_char((char) '0'));
  }
    

/*
  allow a user macro to generate an error condition
*/
static const char *bi_error
  (
    int n_arg,
    const char **arg
  )
  {
    if (n_arg != 2)
      return("error macro requires exactly 1 argument");

    return(arg[1]);
  }


/*
  define the builtins in this file
*/
const char *def_builtins(void)
  {
    const char *p;


    p = mcr_def("set",(void *) bi_set,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("let",(void *) bi_let,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("calc",(void *) bi_calc,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("if",(void *) bi_if,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("repeat",(void *) bi_repeat,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("null",(void *) bi_null,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("index",(void *) bi_index,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("length",(void *) bi_length,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("break",(void *) bi_break,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("loop",(void *) bi_loop,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("substring",(void *) bi_substring,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("expand",(void *) bi_expand,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("error",(void *) bi_error,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("byte",(void *) bi_byte,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("numeric",(void *) bi_numeric,0);
    if (p != (const char *) 0)
      return(p); 

    p = mcr_def("string_compare",(void *) bi_string_compare,0);
    if (p != (const char *) 0)
      return(p); 

    return((const char *) 0);
  }
