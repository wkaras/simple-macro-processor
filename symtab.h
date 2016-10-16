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
  Header file for symbol table library.
  init_str_alloc() must be called before using this package.
*/

#if !defined(H_SYMTAB)
#define H_SYMTAB

/* status codes returned by functions */

/* good status */
#define S_SYM_TAB_GOOD 0
/* error while allocating storage */
#define S_SYM_TAB_ALLOC -1
/* attempt to insert duplicate symbol */
#define S_SYM_TAB_DUP -2
/* attempt to lookup non-existent symbol */
#define S_SYM_TAB_NO_SYM -3
/* symbol table is full */
#define S_SYM_TAB_FULL -4

/* symbol table enty */
typedef struct
  {
    /* pointer to symbol string */
    char *symbol;
    /* result of lookup */
    void *assoc;
  }
SYM_TAB_ENT;

/* symbol table data structure */
typedef struct SYM_TAB_STRUCT
  {
    /* maximum number of entries allowed */
    unsigned int max_entries;
    /* pointer to array of entries */
    SYM_TAB_ENT *entry;
  }
*SYM_TAB;


/*
  function to initialize a symbol table.
  returns status code.
*/
int init_sym_tab
  (
    /* symbol table to initialize */
    SYM_TAB *,
    /* maximum number of entries */
    int
  );


/*
  function to insert a symbol.  returns status code.
*/
int insert_sym
  (
    /* symbol table to put symbol into */
    SYM_TAB,
    /* symbol string */
    const char *,
    /* value to associate with symbol */
    void *
  );


/*
  function to lookup symbol.  returns 0 for success, -1 if
  symbol cannot be found.
*/
int lookup_sym
  (
    /* symbol table to do lookup into */
    SYM_TAB,
    /* string to look for */
    const char *,
    /* pointer variable to load with value associated
       with symbol (undefined if search fails) */ 
    void **
  );


/*
  function to delete symbol.  returns status code.  storage
  for symbol value is lost
*/
int delete_sym
  (
    /* symbol table to do lookup into */
    SYM_TAB,
    /* string to delete */
    char *
  );


/*
  debug function to dump symbol table
*/
void dump_sym_tab
  (
    /* table to dump */
    SYM_TAB
  );

#endif
