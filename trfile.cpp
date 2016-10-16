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
  functions for reading a file and tracking the last
  character read, and to print an error message indicating
  last character read before error.
*/

#include <stdio.h>
#include <string.h>
#include "trfile.h"


/*
  function to open file for tracked reading.
  returns 0 for success, -1 for error.
*/
int tr_open
  (
    /* pointer to descriptor for file */
    TR_DESC *t,
    /* name of file to open. if null pointer, stdin used */
    const char *fn
  )
  {
    if (fn == (const char *) 0)
      {
        t->file_p = stdin;
        /* save file name */
        (void) strcpy(t->file_name,"standard input");
      }
    else
      {
        /* open file for reading */
        t->file_p = fopen(fn,"r");
        if (t->file_p == (FILE *) 0)
          return(S_TR_OPEN);

        /* save file name */
        (void) strncpy(t->file_name,fn,TR_MAX_LEN_FILE_NAME);
        /* insure proper termination */
        (t->file_name)[TR_MAX_LEN_FILE_NAME] = (char) '\0';
      }

    /* reset counters */
    t->line_no = 1;
    t->char_no = 0;

    /* create empty buffer: will trigger a read */
    (t->line)[0] = (char) '\0';

    return(S_TR_GOOD);
  }


/*
  function to get a character from file.
*/
int tr_getc
  (
    /* pointer to descriptor for file */
    TR_DESC *t,
    /* variable to put character into */
    char *c
  )
  {
    while ((t->line)[t->char_no] == (char) '\0')
      {
        if (fgets(t->line,(TR_MAX_LEN_LINE + 2),t->file_p)
              == (char *) 0)
          {
            if (feof(t->file_p))
              return(S_TR_EOF);
            else
              return(S_TR_READ);
          }

        t->char_no = 0;
      }

    *c = (t->line)[(t->char_no)++];
    if (*c == (char) '\n')
      (t->line_no)++;

    return(S_TR_GOOD);
  }

/*
  function to print error message along with number
  of current line, text of current line, pointer to
  last character read.
*/
int tr_print_error
  (
    /* pointer to descriptor for file */
    TR_DESC *t,
    /* string containing error message */
    const char *e_msg
  )
  {
    int i;


    if (fprintf(stderr,"error in line %d of %s:\n  %s\n",
                t->line_no,t->file_name,e_msg) < 0)
      return(S_TR_MESSAGE);

    /* make sure line has end-of-line */
    (t->line)[strlen(t->line) - 1] = (char) '\n';

    /* output line */
    if (fputs(t->line,stderr) != 0)
      return(S_TR_MESSAGE);

    /* output pointer to last character read */
    (t->char_no)--;
    for (i = 0; i < t->char_no; i++)
      if (fputc((char) ' ',stderr) == EOF)
        return(S_TR_MESSAGE);
    if (fputs("^\n",stderr) != 0)
      return(S_TR_MESSAGE);

    return(S_TR_GOOD);
  }

/*
  function to close file
*/
int tr_close
  (
    /* pointer to descriptor for file */
    TR_DESC *t
  )
  {
    if (t->file_p != stdin)
      if (fclose(t->file_p) == EOF)
        return(S_TR_CLOSE);

    return(S_TR_GOOD);
  }
