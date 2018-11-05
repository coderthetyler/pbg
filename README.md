# pbg
(Prefix Boolean Grammar) A simple grammar for writing boolean expressions implemented in a small, portable C library.

[about](#about) | [goals](#goals) | [definition](#definition) | [API](#API) | [status](#status)


## about

**pbg**, an initialism for Prefix Boolean Grammar, is a simple grammar for writing boolean expressions.

The following is a PBG expression:
```
(&,(<,[start_date],[end_date]),(=,[start_date],2018-10-12))
```
This expression checks if the start date is before the end date and if the start date is October 12, 2018. It will evaluate to `TRUE` only if both conditions are `TRUE`. 

PBG is designed with databases in mind. In particular, it was devised to be used by [**tbd**, the Tiny Boolean DBMS](https://github.com/imtjd/tbd). 


## goals

pbg is built with three goals in mind.

First, it must be **simple**. It shouldn't be bogged down by redundant features, it should be easy to express thoughts with, and it must be easy to write an interpreter/compiler for.

Second, it must be **unambigous**. Any expression string must have an unambiguous truth value. Operator precedence invites bugs for the sake of better readability. This seems like a bad idea, so it is avoided.

Third, it must be **expressive**. Thoughts should be easily translated into concise expressions. The grammar falls short of Turing completeness for the sake of simplicity, but it can still go a long way. 


## definition

**pbg** is described by a set of simple recursive rules `R`. An expression is a pair `(S,D)`, where `S` is string generated by `R` and `D` is a dictionary. Each expression has a truth value, either `TRUE` or `FALSE`. This truth value is determined by a process of *expression evaluation*. Evaluation is conceptually performed with an abstract syntax tree. Internal nodes of this AST represent operators; leaves, literals.

### literals
Literals come in three flavors: `STRING`, `NUMBER`, and `DATE`. A `STRING` has the form `'.*'`, where `.` denotes any valid character constant. For example, `'Hello, grammar!'` is a valid `STRING`. As is `'123ABC'`. A `NUMBER` has the same form as a [JSON](http://json.org/) number. For example, `1` is a valid `NUMBER`. As are `1.0`, `0.314`, and `4e10`. A `DATE` can be expressed as (most) non-fractional prefixes of `YYYY-MM-DD@hh:mm:ss`. For example, `2018-10-12` is a valid `DATE` denoting October 12, 2018. As are `2018-10`, `2018-10-12@10`, `2018-10-12@10:45:13`. Notice that `2018` is a `NUMBER`, not a `DATE`.

### operators
Operators are functions that map one or more arguments to a truth value. They are either boolean or non-boolean. For example, `&` is a boolean operator as it acts on a list of expressions, but neither `=` nor `?` are non-boolean operators as they act on literals which do not themselves have truth values. All implemented operators are outlined in the rule set below.

### dictionary
The dictionary `D` is a set of `(KEY,VALUE)` pairs, where each `KEY` is unique. Prior to expression evaluation, every occurrence of `[KEY]` in the string `S` is replaced with its corresponding `VALUE` in `D`. If `KEY` is not in `D`, then `[KEY]` evaluates to the special literal `NULL`. Similarly to `STRING`, `NUMBER`, and `DATE`, `NULL` has no truth value. Currently, only the existence operator `?` can take `NULL` as an argument. (`?` evaluates to `TRUE` only if its argument is not `NULL`.)

### rules
The following is the set of rules `R` used for generating **pbg** expressions:
```
EXPR
  = (!,EXPR)           -> TRUE if expression is FALSE (Boolean NOT).
  = (&,EXPR,EXPR,...)  -> TRUE if every expression is TRUE (Boolean AND).
  = (|,EXPR,EXPR,...)  -> TRUE if at least one expression is TRUE (Boolean OR).
  = (=,ANY,ANY,...)    -> TRUE if equal.
  = (!=,ANY,ANY)       -> TRUE if not equal.
  = (<,VALUE,VALUE)    -> TRUE if first less than second.
  = (>,VALUE,VALUE)    -> TRUE if first greater than second.
  = (<=,VALUE,VALUE)   -> TRUE if first less than or equal to second.
  = (>=,VALUE,VALUE)   -> TRUE if first greater than or equal to second.
  = (?,[KEY])          -> TRUE if argument is not NULL, i.e. KEY exists in the dictionary.
  = TRUE               -> True!
  = FALSE              -> False!
ANY
  = VALUE
  = STRING
VALUE
  = NUMBER
  = DATE
[KEY]
  = ANY if KEY in D
  = NULL if KEY not in D
```


## API

This repository provides a lightweight implementation of a PBG interpreter. It can be incorporated into an existing project by including the single header file `pbg.h`. Complete documentation is provided in `pbg.h` but is partially reproduced here for visibility. The library reserves the `pbg_` and `PBG_` prefixes.

Given a string `S` generated using the [set of rules `R`](#rules), the library converts `S` into an instance of the `pbg_expr` struct.

### functions

#### int pbg_istrue(char* str, int n)
Checks if the given string represents `TRUE`. Returns 1 if so, 0 otherwise.

#### int pbg_isfalse(char* str, int n)
Checks if the given string represents `FALSE`. Returns 1 if so, 0 otherwise.

#### int pbg_isnumber(char* str, int n)
Checks if the given string represents a `NUMBER` literal. Returns 1 if so, 0 otherwise.

#### int pbg_iskey(char* str, int n)
Checks if the given string represents `[KEY]`. Returns 1 if so, 0 otherwise.

#### int pbg_isstring(char* str, int n)
Checks if the given string represents a `STRING` literal. Returns 1 if so, 0 otherwise.

#### int pbg_isdate(char* str, int n)
Checks if the given string represents a `DATE` literal. Returns 1 if so, 0 otherwise.

#### int pbg_parse(pbg_expr* e, char* str, int n)
Parses the string as a PBG expression. Returns 0 if successful, an error code if unsuccessful.

#### void pbg_free(pbg_expr* e)
Destroys the PBG expression instance and `free`'s all associated resources.

#### int pbg_evaluate(pbg_expr* e)
Evaluates the PBG expression using the provided dictionary.

#### char* pbg_gets(pbg_expr* e, char** bufptr, int n)
Prints the PBG expression to the char pointer provided. If ptr is NULL and n is 0, then this function will allocate memory on the heap which must then be `free`'d by the caller. In this way, this function behaves similarly to `fgets`.


## status

This project is still a work-in-progress. Updates will be forthcoming. 
