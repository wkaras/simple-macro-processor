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
  include file for macro package.  functions return
  pointer to message for error, null pointer for success.
*/

#if !defined(H_MACRO)
#define H_MACRO

#if defined(MCR_FILE)
/* in implementation file for package */

/* final result for caller */
char *mcr_result;
/* free spaces in final result area */
int mcr_n_result;

#else

/* define for caller */
extern char *mcr_result;
extern int mcr_n_result;

#endif

using Mcr_built_in_func = const char *(*)(int n_arg,const char **arg);

/*
  define a macro
*/
const char *mcr_def
  (
    /* name of macro */
    const char *name,
    /* body of definition (cannot be altered after call) */
    void *mval,
    /* if magic != 0, the body of the macro is a string.  if magic
       = 0, body is a pointer for a function whose proto-type is
       const char *f(int n_arg,const char **arg);
       this function is called when the macro is invoked, with
       the number of arguments to the macro and the
       array of arguments.  The first argument is the name
       of the macro.  f must return null for success, an error
       message string for failure. */
    int magic
  );


/*
  dump names in macro table
*/
void mcr_dump(void);


/*
  function to begin expansion
*/
void mcr_start_expand
  (
    /* number of top level arguments */
    int n_orig_arg,
    /* array of top level arguments */
    const char **orig_arg
  );


/*
  insert a character directly into the output
  stream without evaluating it
*/
const char *mcr_noeval_char
  (
    char c
  );


/*
  next character to evaluate.
*/
const char *mcr_next_char
  (
    char c
  );


/*
  boolean function - returns non-zero if in midst of
  a macro expansion.
*/
int mcr_expanding(void);

#endif
