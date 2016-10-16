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
  function for evaluating numeric expression in string form
*/

#undef DEBUG

#if defined(DEBUG)
#include "stdio.h"
#endif


/*
  function checks if first string is prefix of second string.
  returns 1 if so, else 0.  comparasion case insensitive.
*/
int is_match
  (
    /* first string.  must be all lower case */
    const char *prefix,
    /* pointer to pointer to target string, updated
       to character after matched part */
    const char **str
  )
  {
    char c;


    do
      {
        c = *((*str)++);
        if ((((char) 'A') <= c) && (c <= ((char) 'Z')))
          /* convert to lower case */
          c = (char) (((int) c) - 'A' + 'a');

        if (c != *(prefix++))
          /* full prefix not present */
          {
            /* back up to last matched character */
            (*str)--;
            return(0);
          }
      }
    while (*prefix != (char) '\0');

    /* full prefix matched */
    return(1);
  }


/* types of tokens in expression.  operators are listed in
   order of ascending precedence (as binary operators, unary
   operators always have highest precidence).  it is important
   to the code logic that the numbers of T_RIGHT_PAREN and 
   T_END_STRING are lower than that of any operator. */

#define T_ERROR 0
#define T_NUMBER 1
#define T_LEFT_PAREN 2
#define T_RIGHT_PAREN 3
#define T_END_STRING 4
#define T_OR 5
#define T_AND 6
#define T_NOT 7
#define T_GT 8
#define T_LT 9
#define T_GE 10
#define T_LE 11
#define T_EQ 12
#define T_NE 13
#define T_PLUS 14
#define T_MINUS 15
#define T_TIMES 16
#define T_DIV 17
#define T_MOD 18

/* variable where value of numeric token is put */
static long int num_val;


/*
  function to get token.  returns type of token found
*/
int get_token
  (
    /* pointer to pointer to the string to extract the 
       next token from.  pointer to string is updated to
       next character after token */
    const char **str
  )
  {
    int c;


    /* skip over white space */
    c = (int) *((*str)++);
    while ((c == ' ') || (c == '\t')
           || (c == '\n'))
      c = (int) *((*str)++);

    if (('0' <= c) && (c <= '9'))
      /* token is a number */
      {
        num_val = (long int) (c - '0');
        c = (int) **str;
        while (('0' <= c) && (c <= '9'))
          {
            num_val *= 10L;
            num_val += (long int) (c - '0');
            c = (int) *(++(*str));
          }
        return(T_NUMBER);
      }

    /* token is parethesis or operator */

    /* convert to lower case if necessary */
    if (('A' <= c) && (c <= 'Z'))
      c +=  'a' - 'A';

    switch (c)
      {
        case '\0':
          /* back up pointer */
          (*str)--;
          return(T_END_STRING);

        case '(':
          return(T_LEFT_PAREN);

        case ')':
          return(T_RIGHT_PAREN);

        case 'o':
          if (is_match("r",str))
            return(T_OR);
          else
            return(T_ERROR);

        case 'a':
          if (is_match("nd",str))
            return(T_AND);
          else
            return(T_ERROR);

        case 'n':
          if (is_match("ot",str))
            return(T_NOT);
          else
            return(T_ERROR);

        case '>':
          c = (int) **str;
          if (c == '=')
            {
              (*str)++;
              return(T_GE);
            }
          else
            return(T_GT);

        case '<':
          c = (int) **str;
          if (c == '=')
            {
              (*str)++;
              return(T_LE);
            }
          else if (c == '>')
            {
              (*str)++;
              return(T_NE);
            }
          else
            return(T_LT);

        case '=':
          return(T_EQ);

        case '+':
          return(T_PLUS);

        case '-':
          return(T_MINUS);

        case '*':
          return(T_TIMES);

        case '/':
          return(T_DIV);

        case 'm':
          if (is_match("od",str))
            return(T_MOD);
          else
            return(T_ERROR);

        default:
          return(T_ERROR);
      }
  }


/* distinct unary operations which result from abitrary compositions
   or MINUS and NOT */

/* null operation */
#define UN_NULL 0
/* NOT */
#define UN_NOT 1
/* MINUS */
#define UN_MINUS 2
/* MINUS of NOT */
#define UN_MINUS_NOT 3
/* replace with boolean equivalent (n -> 1 for n != 0, 0 -> 0) */
#define UN_MAKE_BOOL 4
/* MINUS of MAKE BOOLEAN */
#define UN_MINUS_MAKE_BOOL 5

/* the index of this array is the number for an unary op
   from above.  the result of the lookup is the number of the
   op that results from applying the index op after applying
   the NOT op. */
const int compose_not[] =
  {
    UN_NOT,
    UN_MAKE_BOOL,
    UN_MINUS_NOT,
    UN_MINUS_MAKE_BOOL,
    UN_NOT,
    UN_MINUS_NOT
  };

/* same as above, except result of lookup is op from composing
   with MINUS */
const int compose_minus[] =
  {
    UN_MINUS,
    UN_NOT,
    UN_NULL,
    UN_MINUS_NOT,
    UN_MAKE_BOOL,
    UN_MINUS_MAKE_BOOL
  };

#define N_PAIRS 128
/* operand and following operator, or following right parenthesis */
static struct
  {
    /* operand */
    long int number;
    /* token code for operator/paren. */
    int op_code;
  }
pair[N_PAIRS],*pair_p;
/* index of next pair structure to fill */
static int i_pair;

/* depth of parenthesis nesting */
static int paren_depth;

    
/*
  function to get next number/op pair
*/
const char *get_num_and_op
  (
    const char **str
  )
  {
    /* a token code */
    int tok;
    /* code for unary operator to apply to number */
    int un_code;

    const char *p;

    /* prototype */
    const char *eval_expr(const char **,long int *);


    if (i_pair == N_PAIRS)
      return("buffer overflow during numeric expression evaluation");
    pair_p = pair + i_pair;
    i_pair++;

    /* get number */
    un_code = UN_NULL;

    for  ( ; ; )
      {
        tok = get_token(str);
#if defined(DEBUG)
        printf("string left after get_token: |%s|\n",*str);
#endif
        if ((tok == T_LEFT_PAREN) || (tok == T_NUMBER))
          break;
        else if (tok == T_NOT)
          un_code = compose_not[un_code];
        else if (tok == T_MINUS)
          un_code = compose_minus[un_code];
        /* ignore unary + */
        else if (tok != T_PLUS)
          return("bad syntax in numeric expression");
      }

    if (tok == T_LEFT_PAREN)
      {
        paren_depth++;
        /* recursively call evaluation function */
        p = eval_expr(str,&(pair_p->number));
        if (p != (char *) 0)
          return(p);
      }
    else
      /* number token */
      pair_p->number = num_val;

    /* apply unary operator */
    switch (un_code)
      {
        case UN_NULL:
          break;

        case UN_NOT:
          pair_p->number = !(pair_p->number);
          break;

        case UN_MINUS:
          pair_p->number = -(pair_p->number);
          break;

        case UN_MINUS_NOT:
          if (pair_p->number)
            pair_p->number = 0;
          else
            pair_p->number = -1;
          break;

        case UN_MAKE_BOOL:
          if (pair_p->number)
            pair_p->number = 1;
          break;

        case UN_MINUS_MAKE_BOOL:
          if (pair_p->number)
            pair_p->number = -1;
      }

    /* get operator */
    tok = get_token(str);
#if defined(DEBUG)
    printf("string left after get_token: |%s|\n",*str);
#endif
    if ((tok == T_LEFT_PAREN) || (tok == T_NUMBER) 
        || (tok == T_ERROR) || (tok == T_NOT))
      return("bad syntax in numeric expression");

    pair_p->op_code = tok;

#if defined(DEBUG)
    printf("pair %d: op=%d  num=%ld\n",
           i_pair,pair_p->op_code,pair_p->number);
#endif

    return((const char *) 0);
  }


/*
  function to do evaluation up to closing parenthesis
  or end of string.  returns string describing error,
  null if no error.
*/
const char *eval_expr
  (
    /* pointer to pointer to string to evaluate.  pointer
       to string updated to character after evaluated portion. */
    const char **str,
    /* result of evaluation */
    long int *e_res
  )
  {
    /* number of active number/op pairs in current evaluation */
    int n_local_pairs;

    const char *p;


    n_local_pairs = 0;
    for ( ; ; )
      {
        /* get a number/op pair */
        p = get_num_and_op(str);
        if (p != (const char *) 0)
          return(p);
        n_local_pairs++;

        while (n_local_pairs >= 2)
          {
            if (pair_p->op_code > (pair_p - 1)->op_code)
              /* second operator of greater precedence */
              break;

            /* the op in the current pair is of lower precedence
               than the op in the previous pair, so simplify by
               performing the op in the previous pair */
            switch((pair_p - 1)->op_code)
              {
                case T_OR:
                  (pair_p - 1)->number = (pair_p - 1)->number ||
                                         pair_p->number;
                  break;

                case T_AND:
                  (pair_p - 1)->number = (pair_p - 1)->number &&
                                         pair_p->number;
                  break;

                case T_GT:
                  (pair_p - 1)->number = (pair_p - 1)->number >
                                         pair_p->number;
                  break;

                case T_LT:
                  (pair_p - 1)->number = (pair_p - 1)->number <
                                         pair_p->number;
                  break;

                case T_GE:
                  (pair_p - 1)->number = (pair_p - 1)->number >=
                                         pair_p->number;
                  break;

                case T_LE:
                  (pair_p - 1)->number = (pair_p - 1)->number <=
                                         pair_p->number;
                  break;

                case T_EQ:
                  (pair_p - 1)->number = (pair_p - 1)->number ==
                                         pair_p->number;
                  break;

                case T_NE:
                  (pair_p - 1)->number = (pair_p - 1)->number !=
                                         pair_p->number;
                  break;

                case T_PLUS:
                  (pair_p - 1)->number = (pair_p - 1)->number +
                                         pair_p->number;
                  break;

                case T_MINUS:
                  (pair_p - 1)->number = (pair_p - 1)->number -
                                         pair_p->number;
                  break;

                case T_TIMES:
                  (pair_p - 1)->number = (pair_p - 1)->number *
                                         pair_p->number;
                  break;

                case T_DIV:
                  (pair_p - 1)->number = (pair_p - 1)->number /
                                         pair_p->number;
                  break;

                case T_MOD:
                  (pair_p - 1)->number = (pair_p - 1)->number %
                                         pair_p->number;

              }
            (pair_p - 1)->op_code = pair_p->op_code;
            pair_p--;
            i_pair--;
            n_local_pairs--;
#if defined(DEBUG)
            printf("pair %d: op=%d  num=%ld\n",
                   i_pair,pair_p->op_code,pair_p->number);
#endif
          }

        if (n_local_pairs == 1)
          if ((pair_p->op_code == T_RIGHT_PAREN) ||
              (pair_p->op_code == T_END_STRING))
            {
              /* if end of string, make sure not inside parentheses */ 
              if ((pair_p->op_code == T_END_STRING) &&
                  (paren_depth != 0))
                return("missing right parenthesis in numeric expression");
              /* if not end of string make sure in parentheses */
              if (pair_p->op_code == T_RIGHT_PAREN)
                {
                  if (paren_depth == 0)
                    return(
                    "extra right parenthesis in numeric expression");
                  paren_depth--;
                }

              *e_res = pair_p->number;
              pair_p--;
              i_pair--;
              return((const char *) 0);
            }

      }
  }


/*
  returns pointer to message for error, null otherwise
*/
const char *calc
  (
    /* expresion to evaluate */
    const char *expr,
    /* result of evaluation if no error */
    long int *result
  )
  {
    const char *p,*q;


    i_pair = 0;
    paren_depth = 0;

    p = expr;
    q = eval_expr(&p,result);
    if (q != (const char *) 0)
      return(q);

    if (get_token(&p) != T_END_STRING)
      return("junk follows numeric expression");

    return((const char *) 0);
  }
