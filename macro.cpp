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
  macro package.  functions return pointer to message for
  error, null pointer for success.
*/

#undef DEBUG

#if defined(DEBUG)
#include <stdio.h>
#endif

#include <string.h>
#include <limits.h> // defined INT_MAX
#include <unordered_map>
#include <string>
#include <utility>

#include "stralloc.h"

#define MCR_FILE
#include "macro.h"

/* records defining macro type and body */
class Macro_value
  {
  private:

    bool has_string_;

    union
      {
        const char *c_string_;
        Mcr_built_in_func bi_func_ptr_;
      };

    void clear_c_string()
      {
        if (has_string_)
          delete [] c_string_;
      }

    void set_c_string(const char *cs)
      {
        char *tcs = new char [strlen(cs) + 1];

        strcpy(tcs, cs);

        c_string_ = tcs;

        has_string_ = true;
      }

  public:

    Macro_value() : has_string_(false) { }

    Macro_value(const char *c_str)
      {
        set_c_string(c_str);
      }

    Macro_value(Mcr_built_in_func bi) : has_string_(false)
      {
        bi_func_ptr_ = bi;
      }

    ~Macro_value()
      {
        clear_c_string();
      }

    Macro_value(const Macro_value &src) = delete;

    Macro_value(Macro_value &&src) : has_string_(src.has_string_)
      {
        if (has_string_)
          {
            c_string_ = src.c_string_;
            src.has_string_ = false;
          }
        else
          bi_func_ptr_ = src.bi_func_ptr_;
      }

    void operator = (const Macro_value &src) = delete;

    void operator = (Macro_value &&src) = delete;

    bool has_string() const { return(has_string_); }

    const char * c_string() const { return(c_string_); }

    void c_string(const char *cs)
      {
        clear_c_string();

        set_c_string(cs);
      }

    Mcr_built_in_func bi_func_ptr() const { return(bi_func_ptr_); }

    void bi_func_ptr(Mcr_built_in_func bifp)
      {
        clear_c_string();

        has_string_ = false;
        bi_func_ptr_ = bifp;
      }
  };

using SYM_TAB = std::unordered_map<std::string, Macro_value>;

static SYM_TAB sym_tab;

/* success return value for functions */
#define SUCCESS ((const char *) 0)

/* macro identifying digit */
#define DIGIT(C) ((((char) '0') <= (C)) && ((C) <= ((char) '9')))
/* macro identifying white space character */
#define WHITE(C) (((C) == ((char) ' ')) || \
  ((C) == ((char) '\t')) || ((C) == ((char) '\n')))

/* special characters in syntax */
#define LEAD ((char) '$')
#define LEFT_DELIM ((char) '(')
#define RIGHT_DELIM ((char) ')')
#define BEGIN1_QUOTE_ARG ((char) '(')
#define BEGIN2_QUOTE_ARG ((char) '=')
#define END1_QUOTE_ARG ((char) '=')
#define END2_QUOTE_ARG ((char) ')')
#define EVAL_ARG_DELIM ((char) '!')


/*
  define a macro
*/
const char *mcr_def
  (
    /* name of macro */
    const char *name,
    /* body of definition */
    void *mval,
    /* if mgc != 0, the body of the macro is a string.  if mgc
       = 0, body is a pointer for a function whose proto-type is
       const char *f(int n_arg,const char **arg);
       this function is called when the macro is invoked, with
       the number of arguments to the macro and the
       array of arguments.  The first argument is the name
       of the macro.  f must return null for success, an error
       message string for failure. */
    int mgc
  )
  {
    const char *p;

    /* check name */
    p = name;
    if (*p == (char) '\0')
      return("empty macro name"); 
    if (DIGIT(*p))
      return("macro name cannot start with digit");
    do
      if (WHITE(*p))
        return("macro name cannot contain white space");
      else if (*p == RIGHT_DELIM)
        return("macro name cannot contain right delimeter for invocation");
    while (*(++p) != (char) '\0');

    SYM_TAB::iterator i = sym_tab.find(name);

    if ((mgc != 0) and ! *static_cast<char *>(mval))
      {
        // Macro is being deleted by setting it to the empty string.

        if (i != sym_tab.end())
          sym_tab.erase(i);

        return(SUCCESS);
      }

    if (i == sym_tab.end())
      {
        // New macro name.
        //
        if (mgc != 0)
          i = sym_tab.emplace(name, static_cast<const char *>(mval)).first;
        else
          i = sym_tab.emplace(name,
                              reinterpret_cast<Mcr_built_in_func>(mval)).first;
      }
    else
      {
        // Macro already exists, change it's value.
        //
        if (mgc != 0)
          i->second.c_string(static_cast<const char *>(mval));
        else
          i->second.bi_func_ptr(reinterpret_cast<Mcr_built_in_func>(mval));
      }

    return(SUCCESS);
  }


/*
  dump names in macro table
*/
void mcr_dump(void)
  {
    for (SYM_TAB::const_iterator i = sym_tab.cbegin(); i != sym_tab.cend();
         ++i)
      if (i->second.has_string())
        printf("%s / %s\n", i->first.c_str(), i->second.c_string());
      else
        printf(
          "%s / BUILTIN func addr = 0x%lx\n", i->first.c_str(),
          reinterpret_cast<unsigned long>(i->second.bi_func_ptr()));
  }

/* structures for evaluation */
#define EVAL_BUF_SIZE 4*1024
#define N_EVAL_POINTERS 64
static struct
  {
    /* buffer to temporarily contain results of evaluation */
    char buf[EVAL_BUF_SIZE];
    /* pointer to next free character in buffer */
    char *buf_free;
    /* array of pointers to arguments in buffers */
    char *(ptr[N_EVAL_POINTERS]);
    /* pointer to top pointer to an string (argument) */
    char **curr_ptr;
  }
eval[2];

/* when eval[0].curr_ptr is null it is considered to be pointing
   to the results area */

/* move to a new string */
#define NEW_STRING(SELECT)  \
  {  \
    if (eval[(SELECT)].curr_ptr == (char **) 0)  \
      eval[(SELECT)].curr_ptr = eval[(SELECT)].ptr;  \
    else if (eval[(SELECT)].curr_ptr  \
             == (eval[(SELECT)].ptr + N_EVAL_POINTERS))  \
      return("buffer overflow while evaluating macro");  \
    else  \
      {  \
        eval[(SELECT)].curr_ptr++;  \
        *(eval[(SELECT)].curr_ptr) = eval[(SELECT)].buf_free;  \
      }  \
  }

/* add a character to the current string */
#define ADD_CHAR(SELECT,CH)  \
  {  \
    if (((SELECT) == 0) && (eval[0].curr_ptr == (char **) 0))  \
      {  \
        if (mcr_n_result == 0)  \
          return("result buffer overflow while evaluating macro");  \
        else  \
          {  \
            *(mcr_result++) = (CH);  \
            mcr_n_result--;  \
          }  \
      }  \
    else if (eval[(SELECT)].buf_free > (eval[(SELECT)].buf + EVAL_BUF_SIZE))  \
      return("buffer overflow while evaluating macro");  \
    else  \
      *(eval[(SELECT)].buf_free++) = (CH);  \
  }

/* get pointer to current string pointer.  */
#define CURR_PTR(SELECT) (eval[(SELECT)].curr_ptr)

/* clear N strings from the top of the stack */
#define CLEAR(SELECT,N)  \
  {  \
    if  ((eval[(SELECT)].ptr + (N)) > eval[(SELECT)].curr_ptr)  \
      {  \
        eval[(SELECT)].curr_ptr = (char **) 0;  \
        eval[(SELECT)].buf_free = eval[(SELECT)].buf;  \
      }  \
    else  \
      {  \
        eval[(SELECT)].curr_ptr -= (N);  \
        eval[(SELECT)].buf_free = *(eval[(SELECT)].curr_ptr + 1);  \
      }  \
  }

/* depth of nesting of quoted argument delimiters */
static int depth_quote_arg_nest;

/* maximum amount of nesting */
#define MAX_NEST 64

/* nesting stack for evaluation, and pointers into it */
static struct es_rec
  {
    /* state of evaluation */
    int state;
    /* select which evaluation buffer area results are going into */
    int select;
    /* number of arguments */
    int n_arg;
    /* pointer to array of arguments */
    const char **arg;
    /* flag telling if argument is being evaluated */
    int arg_eval;
  } 
eval_stack[MAX_NEST],*ep,*next_ep;

/* level of nesting */
static int nest;

#if defined(DEBUG)

static void print_es_rec(void)
  {
    int i;


    (void) fprintf(stderr,"nest=%d\n",nest); 
    (void) fprintf(stderr,"state=%d\n",ep->state);
    (void) fprintf(stderr,"select=%d\n",ep->select);
    for (i = 0; i < ep->n_arg; i++)
      (void) fprintf(stderr,"arg%d=<%s>\n",i,(ep->arg)[i]);
    if (eval[ep->select].curr_ptr == (char **) 0)
      fprintf(stderr,"no active strings\n");
    else
      fprintf(stderr,"%ld active strings\n",
        ((long int) (eval[ep->select].curr_ptr - eval[ep->select].ptr)) + 1L);
    i = 1 - ep->select;
    if (eval[i].curr_ptr == (char **) 0)
      fprintf(stderr,"other: no active strings\n");
    else
      fprintf(stderr,"other: %ld active strings\n",
        ((long int) (eval[i].curr_ptr - eval[i].ptr)) + 1L);

    return;
  }

#endif


/* states for finite state machine */
#define NORMAL 0
#define LEAD_SEEN 1
#define LEAD_AGAIN 2
#define WAIT_NAME 3
#define GETTING_ARG_NO 4
#define WAIT_ARG_END 5
#define GETTING_NAME 6
#define DELIM_SEEN_EVAL_ARG 7
#define WAIT_ARG_OR_MACRO_END 8
#define BEGIN1_QUOTE_ARG_SEEN 9
#define GETTING_QUOTED_ARG 10
#define BEGIN1_SEEN_WITHIN_ARG 11
#define END1_QUOTE_ARG_SEEN 12

/* argument number being read */
static unsigned int arg_no;

/* body of macro which is "evaluated" when an 
   undefined macro is referenced */
const Macro_value mcr_empty("");


/*
  function to begin expansion
*/
void mcr_start_expand
  (
    /* number of top level arguments */
    int n_orig_arg,
    /* array of top level arguments */
    const char **orig_arg
  )
  {
    nest = 0;

    ep = eval_stack;
    next_ep = eval_stack + 1;

    ep->state = NORMAL;
    ep->select = 0;
    ep->n_arg = n_orig_arg;
    ep->arg = orig_arg;
    ep->arg_eval = 0;

    eval[0].buf_free =  eval[0].buf;
    /* make results area current string */
    eval[0].curr_ptr = (char **) 0;
    eval[0].ptr[0] = eval[0].buf;
    eval[1].buf_free = eval[1].buf;
    eval[1].curr_ptr = (char **) 0;
    eval[1].ptr[0] = eval[1].buf;

    return;
  }

/*
  insert a character directly into the output
  stream without evaluating it
*/
const char *mcr_noeval_char
  (
    char c
  )
  {
    ADD_CHAR(ep->select, c);
    return((const char *) 0);
  }


/*
  next character to evaluate.
*/
const char *mcr_next_char
  (
    char c
  )
  {
    if (c == (char) '\0')
      return("null character in input to macro processor");

    switch (ep->state)
      {
        case NORMAL:
          if (c == LEAD)
            ep->state = LEAD_SEEN;
          else if ((ep->arg_eval) && (c == EVAL_ARG_DELIM))
            ep->state = DELIM_SEEN_EVAL_ARG;
          else
            ADD_CHAR(ep->select,c);

          break;

        case DELIM_SEEN_EVAL_ARG:
          if (c == EVAL_ARG_DELIM)
            /* escape of delimiter */
            {
              ADD_CHAR(ep->select,EVAL_ARG_DELIM)
              ep->state = NORMAL;
            }
          else
            /* done evaluating an argument */
            {
              /* terminate string */
              ADD_CHAR(ep->select,(char) '\0')

              /* go back to pointing to level below macro */
              nest -= 2;
              ep -= 2;
              next_ep -=2;

              /* process current charater */
              return(mcr_next_char(c));
            }

          break;

        case LEAD_SEEN:
          if (c == LEAD)
            /* not a macro invocation, may be an escape */
            ep->state = LEAD_AGAIN;
          else if (c == LEFT_DELIM)
            /* macro invocation */
            ep->state = WAIT_NAME;
          else
            /* isolated lead character */
            {
              ADD_CHAR(ep->select,LEAD)
              ep->state = NORMAL;
              /* process current charater */
              return(mcr_next_char(c));
            }

          break;

        case LEAD_AGAIN:
          if (c == LEFT_DELIM)
            /* escape sequence to result */
            {
              ADD_CHAR(ep->select,LEAD)
              ADD_CHAR(ep->select,LEFT_DELIM)
              ep->state = NORMAL;
            }
          else
            {
              /* not escape of lead-in sequence; put first lead
                 characters in result */
              ADD_CHAR(ep->select,LEAD)
              ep->state = LEAD_SEEN;
              return(mcr_next_char(c));
            }
          break;

        case WAIT_NAME:
          if (DIGIT(c))
            {
              ep->state = GETTING_ARG_NO;
              arg_no = ((unsigned int) c) - ((unsigned int) '0');
            }
          else if (!WHITE(c))
            {
              ep->state = GETTING_NAME;

              /* initialize argument information */
              next_ep->n_arg = 1;
              NEW_STRING(1 - ep->select)
              next_ep->arg =
                const_cast<const char **>(CURR_PTR(1 - ep->select));

              ADD_CHAR(1 - ep->select,c)
            }

          break;

        case GETTING_ARG_NO:
          if (DIGIT(c))
            {
              arg_no *= 10;
              arg_no += ((unsigned int) c) - ((unsigned int) '0');

              if (arg_no > (INT_MAX / 10))
                return("ridiculous macro argument number");
            }
          else
            {
              const char *p;

              if (int(arg_no) < ep->n_arg)
                {
                  /* copy value of argument into result */
                  p = (ep->arg)[arg_no];
                  while (*p != (char) '\0')
                    {
                      ADD_CHAR(ep->select,*p)
                      p++;
                    }
                }

              /* if argument number too large, argument considered
                 to be null */

              ep->state = WAIT_ARG_END;
              return(mcr_next_char(c));
            }
          break;

        case WAIT_ARG_END:
          if (c == RIGHT_DELIM)
            /* argument invocation has ended */
            ep->state = NORMAL;
          else if (!WHITE(c))
            return("unexpected garbage in macro argument reference");

          break;

        case GETTING_NAME:
          if ((!WHITE(c)) && (c != RIGHT_DELIM))
            ADD_CHAR(1 - ep->select,c)
          else
            /* good termination of name, get definition */
            {
              /* null terminate name */
              ADD_CHAR(1 - ep->select,(char) '\0')

#if defined(DEBUG)

              (void) fprintf(stderr,"\nMACRO %s SEEN\n",
                             *CURR_PTR(1 - ep->select));

#endif

              ep->state = WAIT_ARG_OR_MACRO_END;
              return(mcr_next_char(c));
            }

          break;

        case WAIT_ARG_OR_MACRO_END:
          if (c == EVAL_ARG_DELIM)
            {
              struct es_rec *tmp_ep;

              next_ep->n_arg++;
              NEW_STRING(1 - ep->select)

              if (nest >= MAX_NEST - 2)
                return("macro nesting level too deep");

              /* record two above current one is for evaluation
                 of macro argument */
              tmp_ep = ep + 2;

              /* fill in record for argument evaluation */
              tmp_ep->state = NORMAL;
              tmp_ep->select = 1 - ep->select;
              tmp_ep->n_arg = ep->n_arg;
              tmp_ep->arg = ep->arg;
              tmp_ep->arg_eval = 1;

              /* now evaluating the argument, make its evaluation
                 stack record current */
              ep += 2;
              next_ep += 2;
              nest += 2;

#if defined(DEBUG)

              (void) fprintf(stderr,"\nEVAL MACRO ARG\n");
              print_es_rec();

#endif
            }
          else if (c == RIGHT_DELIM)
            /* macro invocation completed; time to evaluate it */
            {
              if (nest == (MAX_NEST - 1))
                return("macro nesting level too deep");

              /* lookup name */
              auto i = sym_tab.find(next_ep->arg[0]);

	      const Macro_value *to_eval;

              if (i == sym_tab.end())
                to_eval = &mcr_empty;
              else
                to_eval = &(i->second);

              const char *p,*rv;

              if (to_eval->has_string())
                /* normal evaluation */
                {
                  /* finalize record for macro evaluation */
                  next_ep->state = NORMAL;
                  next_ep->select = ep->select;
                  next_ep->arg_eval = 0;

                  p = to_eval->c_string();

                  /* now evaluating the macro body, make its evaluation
                     stack record current */
                  ep++;
                  next_ep++;
                  nest++;

#if defined(DEBUG)

                  (void) fprintf(stderr,"\nEVAL MACRO BODY %s\n",p);
                  print_es_rec();

#endif

                  while (*p != (char) '\0')
                    {
                      rv = mcr_next_char(*(p++));
                      if (rv != SUCCESS)
                        return(rv);
                    }

                  CLEAR(1 - ep->select,ep->n_arg)
                  ep--;
                  next_ep--;
                  nest--;
                }
              else
                /* magic macro, call its function */
                {
                  struct es_rec *tmp_ep;

                  /* re-create argument evaluation environment, so
                     macros like "if" can evaluate arguments that
                     were quoted */

                  if (nest >= MAX_NEST - 2)
                    return("macro nesting level too deep");

                  /* record two above current one is for evaluation
                     of macro argument */
                  tmp_ep = ep;
                  ep += 2;
                  next_ep += 2;
                  nest += 2;
                  ep->state = NORMAL;
                  ep->select = tmp_ep->select;
                  ep->n_arg = tmp_ep->n_arg;
                  ep->arg = tmp_ep->arg;
                  ep->arg_eval = 0;
#if defined(DEBUG)

                  (void) fprintf(stderr,"\nEVAL MAGIC MACRO\n");
                  print_es_rec();

#endif
                  rv = (to_eval->bi_func_ptr())(
                         (tmp_ep + 1)->n_arg,(tmp_ep + 1)->arg);
                  if (rv != SUCCESS)
                    return(rv);

                  nest -= 2;
                  ep -= 2;
                  next_ep -=2;

                  /* clear arguments */
                  CLEAR(1 - ep->select,next_ep->n_arg)
                }

              ep->state = NORMAL;
            }
          else if (c == BEGIN1_QUOTE_ARG)
            ep->state = BEGIN1_QUOTE_ARG_SEEN;
          else if (!WHITE(c))
            return("unexpected garbage in macro invocation");
          
          break;

        case BEGIN1_QUOTE_ARG_SEEN:
          if (c == BEGIN2_QUOTE_ARG)
            {
              next_ep->n_arg++;
              NEW_STRING(1 - ep->select)

              depth_quote_arg_nest = 1;

              ep->state = GETTING_QUOTED_ARG;
            }
          else
            return("unexpected garbage in macro invocation");

          break;

        case GETTING_QUOTED_ARG:
          if (c == END1_QUOTE_ARG)
            ep->state = END1_QUOTE_ARG_SEEN;
          else
            {
              if (c == BEGIN1_QUOTE_ARG)
                ep->state = BEGIN1_SEEN_WITHIN_ARG;

              ADD_CHAR(1 - ep->select,c)
            }

          break;

        case BEGIN1_SEEN_WITHIN_ARG:
          ADD_CHAR(1 - ep->select,c)
          if (c == BEGIN2_QUOTE_ARG)
            depth_quote_arg_nest++;
          if (c != BEGIN1_QUOTE_ARG)
            ep->state = GETTING_QUOTED_ARG;

          break;

        case END1_QUOTE_ARG_SEEN:
          if (c == END2_QUOTE_ARG)
            {
              if (depth_quote_arg_nest == 1)
                /* argument has been terminated */
                {
                  /* terminate argument string */
                  ADD_CHAR(1 - ep->select,(char) '\0')

                  ep->state = WAIT_ARG_OR_MACRO_END;
                }
              else
                {
                  ADD_CHAR(1 - ep->select,END1_QUOTE_ARG)
                  ADD_CHAR(1 - ep->select,END2_QUOTE_ARG)
                  depth_quote_arg_nest--;
                  ep->state = GETTING_QUOTED_ARG;
                }
            }
          else
            /* false alarm */
            {
              ADD_CHAR(1 - ep->select,END1_QUOTE_ARG)
              ADD_CHAR(1 - ep->select,c)
              if (c != END1_QUOTE_ARG)
                ep->state = GETTING_QUOTED_ARG;
            }
          break;
      } /* switch */

    return(SUCCESS);
  }


/*
  boolean function - returns non-zero if in midst of
  a macro expansion.
*/
int mcr_expanding(void)
  {
    return(eval_stack[0].state != NORMAL);
  }
