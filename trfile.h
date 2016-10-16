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

#if !(defined(H_TRFILE))
#define H_TRFILE

/* status codes returned by functions */

/* normal status */
#define S_TR_GOOD 0
/* error opening file */
#define S_TR_OPEN -1
/* error reading line of file */
#define S_TR_READ -2
/* end of file */
#define S_TR_EOF -3
/* line of file too long */
#define S_TR_LONG_LINE -4
/* error writing error message */
#define S_TR_MESSAGE -5
/* error closing file */
#define S_TR_CLOSE -6

/* maximum length of file name */
#define TR_MAX_LEN_FILE_NAME 32
/* maximum length of any line in file */
#define TR_MAX_LEN_LINE 79

/* descriptor for file to trace */
typedef struct
  {
    /* storage for file name */
    char file_name[TR_MAX_LEN_FILE_NAME + 1];
    /* stream object for file */
    FILE *file_p;
    /* storage for line */
    char line[TR_MAX_LEN_LINE + 2];
    /* number of current line (1 offset) */
    int line_no;
    /* number of current character in line (0 offset) */
    int char_no;
  }
TR_DESC;

/*
  function to open file for tracked reading.
*/
int tr_open
  (
    /* pointer to descriptor for file */
    TR_DESC *,
    /* name of file to open.  if null pointer, stdin used. */
    const char *
  );


/*
  function to get a character from file.
*/
int tr_getc
  (
    /* pointer to descriptor for file */
    TR_DESC *,
    /* variable to put character into */
    char *
  );


/*
  function to print error message along with number
  of current line, text of current line, pointer to
  last character read.
*/
int tr_print_error
  (
    /* pointer to descriptor for file */
    TR_DESC *,
    /* string containing error message */
    const char *
  );


/*
  function to close file
*/
int tr_close
  (
    /* pointer to descriptor for file */
    TR_DESC *
  );

#endif
