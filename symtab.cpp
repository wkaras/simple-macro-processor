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
  functions implemeting symbol table package.
  init_str_alloc() must be called before using this package.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "symtab.h"
#include "stralloc.h"

extern uint32_t crc32(uint32_t init, const void *data, int bytes_of_data);


/*
  function to return hash value for a string
*/
inline uint32_t str_hash
  (
    const char *s
      /* string to find hash value for */
  )
  {
    return(crc32(~uint32_t(0), s, strlen(s)));
  }


/* dummy value for entries whose symbols have been deleted */
static char deleted[] = "";


/*
  function to initialize a symbol table.  returns status code.
*/
int init_sym_tab
  (
    /* symbol table to initialize */
    SYM_TAB *st,
    /* maximum number of entries */
    int me
  )
  {
    SYM_TAB_ENT *p;

    /* allocate storage for symbol table */
    (*st)->max_entries = (unsigned int) me;
    (*st)->entry = (SYM_TAB_ENT *)
                   malloc(((size_t) me) * sizeof(SYM_TAB_ENT));

    p = (*st)->entry;
    if (p == (SYM_TAB_ENT *) 0)
      return(S_SYM_TAB_ALLOC);

    /* mark all entries as empty */
    while (me-- > 0)
      (p++)->symbol = (char *) 0;

    return(S_SYM_TAB_GOOD);
  }


/*
  function to insert a symbol.  returns status code.
*/
int insert_sym
  (
    /* symbol table to put symbol into */
    SYM_TAB st,
    /* symbol string */
    const char *sym,
    /* value to associate with symbol */
    void *val
  )
  {
    /* hash value for string */
    unsigned int h;
    /* number of tries left in search for open entry */
    unsigned int search_cnt;
    /* pointer to entry */
    SYM_TAB_ENT *ep;

    /* take modulus of hash to make it a valid index */
    h = static_cast<unsigned int>(str_hash(sym) % st->max_entries);

    search_cnt = st->max_entries;
    while (search_cnt-- > 0U)
      {
        ep = st->entry + h;
        if ((ep->symbol == (char *) 0) || (ep->symbol == deleted))
          /* available entry */
          {
            /* get space for symbol (including null) */
            ep->symbol = str_alloc(strlen(sym) + 1);
            if (ep->symbol == (char *) 0)
              return(S_SYM_TAB_ALLOC);
            (void) strcpy(ep->symbol,sym);

            ep->assoc = val;

            return(S_SYM_TAB_GOOD);
          }
        else if (strcmp(ep->symbol,sym) == 0)
          /* attempt to duplicate current entry */
          return(S_SYM_TAB_DUP);

        h++;
        if (h == st->max_entries)
          h = 0U;
      }

    /* entire table has been searched, no entries free */
    return(S_SYM_TAB_FULL);
  }


/*
  function to lookup symbol.  returns status code.
*/
int lookup_sym
  (
    /* symbol table to do lookup into */
    SYM_TAB st,
    /* string to look for */
    const char *sym,
    /* pointer variable to load with value associated
       with symbol (undefined if search fails) */ 
    void **val
  )
  {
    /* hash value for string */
    int h;
    /* number of tries left in search for matching entry */
    int search_cnt;
    /* pointer to entry */
    SYM_TAB_ENT *ep;


    /* take modulus of hash to make it a valid index */
    h = static_cast<unsigned int>(str_hash(sym) % st->max_entries);

    search_cnt = st->max_entries;
    while (search_cnt-- > 0)
      {
        ep = st->entry + h;
        if (ep->symbol != (char *) 0)
          if (strcmp(ep->symbol,sym) == 0)
            /* a match is found */
            {
              *val = ep->assoc;
              return(S_SYM_TAB_GOOD);
            }

        h++;
        h %= st->max_entries;
      }

    /* entire table has been searched, no matches */
    return(S_SYM_TAB_NO_SYM);
  }


/*
  function to delete symbol.  returns status code.  storage
  for symbol value is lost
*/
int delete_sym
  (
    /* symbol table to do lookup into */
    SYM_TAB st,
    /* string to delete */
    char *sym
  )
  {
    /* hash value for string */
    int h;
    /* number of tries left in search for matching entry */
    int search_cnt;
    /* pointer to entry */
    SYM_TAB_ENT *ep;


    h = str_hash(sym);

    /* take modulus of h to make it a valid index */
    h %= st->max_entries;

    search_cnt = st->max_entries;
    while (search_cnt-- > 0)
      {
        ep = st->entry + h;
        if (ep->symbol != (char *) 0)
          if (strcmp(ep->symbol,sym) == 0)
            /* a match is found */
            {
              ep->symbol = deleted;
              return(S_SYM_TAB_GOOD);
            }

        h++;
        h %= st->max_entries;
      }

    /* entire table has been searched, no matches */
    return(S_SYM_TAB_NO_SYM);
  }


/*
  debug function to dump symbol table
*/
void dump_sym_tab
  (
    /* table to dump */
    SYM_TAB st
  )
  {
    int i;
    SYM_TAB_ENT *ep;


    ep = st->entry;
    for (i = 0; i < (int) st->max_entries; i++)
      {
        if (ep->symbol == deleted)
          printf("entry %d: DELETED\n",i);
        else if (ep->symbol != (char *) 0)
          printf("entry %d: %s (hash=%u)\n",i,ep->symbol,
                 (static_cast<unsigned>(
                   str_hash(ep->symbol) % st->max_entries)));
        ep++;
      }

    return;
  }
