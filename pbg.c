#include "pbg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*****************************
 *                           *
 * LOCAL STRUCTURE DIRECTORY *
 *                           *
 *****************************/

/* LITERAL REPRESENTATIONS */
typedef struct {
	double _val;
} pbg_lt_number;  /* PBG_LT_NUMBER */

typedef struct {
	unsigned int  _YYYY;  /* year */
	unsigned int  _MM;    /* month */
	unsigned int  _DD;    /* day */
} pbg_lt_date;  /* PBG_LT_DATE */

typedef char pbg_lt_string; /* PBG_LT_STRING */

/* ERROR REPRESENTATIONS */
typedef struct {
	int              _arity;  /* Number of arguments given to operator. */
	pbg_field_type  _type;    /* Type of operator involved in error. */
} pbg_op_arity_err;  /* PBG_ERR_OP_ARITY. */

typedef struct {
	char*  _msg;  /* Description of syntax error. */
	char*  _str;  /* String in which error occurred. */
	int    _i;    /* Index of error in string. */
} pbg_syntax_err;  /* PBG_ERR_SYNTAX */

typedef struct {
	char*  _field;  /* String representing the unknown type. */
	int    _n;      /* Length of field. */
} pbg_unknown_type_err;  /* PBG_ERR_UNKNOWN_TYPE */


/****************************
 *                          *
 * LOCAL FUNCTION DIRECTORY *
 *                          *
 ****************************/

/* ERROR MANAGEMENT */
void pbg_err_init(pbg_error* err, pbg_error_type type, int line, char* file, int size, void* data);
void pbg_err_alloc(pbg_error* err, int line, char* file);
void pbg_err_unknown_type(pbg_error* err, int line, char* file, char* field, int n);
void pbg_err_syntax(pbg_error* err, int line, char* file, char* str, int i, char* msg);
void pbg_err_op_arity(pbg_error* err, int line, char* file, pbg_field_type type, int arity);
void pbg_err_state(pbg_error* err, int line, char* file, char* msg);
void pbg_err_op_arg_type(pbg_error* err, int line, char* file, char* msg);
char* pbg_error_str(pbg_error_type type);
char* pbg_field_type_str(pbg_field_type type);
 
/* FIELD MANAGEMENT */
pbg_field* pbg_field_get(pbg_expr* e, int index);
void pbg_field_free(pbg_field* field);
int pbg_store_constant(pbg_expr* e, pbg_field field);
int pbg_store_variable(pbg_expr* e, pbg_field field);

/* FIELD CREATION TOOLKIT */
pbg_field pbg_make_init(pbg_field_type type, int size, void* data);
pbg_field pbg_make_op(pbg_error* err, pbg_field_type type, int numchildren);
pbg_field pbg_make_var(pbg_error* err, char* str, int n);

/* FIELD PARSING TOOLKIT */
int pbg_check_op_arity(pbg_field_type type, int numargs);
int pbg_parse_r(pbg_expr* e, pbg_error* err, char* str, 
		int** fields, int** lengths, int** closings);

/* FIELD EVALUATION TOOLKIT */
int pbg_evaluate_r(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_not(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_and(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_or(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_exst(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_eq(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_neq(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_lt(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_gt(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_lte(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_gte(pbg_expr* e, pbg_error* err, pbg_field* field);
int pbg_evaluate_op_typeof(pbg_expr* e, pbg_error* err, pbg_field* field);

/* JANITORIAL FUNCTIONS */
/* No local functions. */

/* CONVERSION & CHECKING TOOLKIT */
pbg_field_type pbg_gettype(char* str, int n);
int pbg_istypedate(char* str, int n);
int pbg_istypenumber(char* str, int n);
int pbg_istypebool(char* str, int n);
int pbg_istypestring(char* str, int n);
int pbg_istrue(char* str, int n);
int pbg_isfalse(char* str, int n);
int pbg_isvar(char* str, int n);
int pbg_isnumber(char* str, int n);
int pbg_isstring(char* str, int n);
int pbg_isdate(char* str, int n);

void pbg_tonumber(pbg_lt_number* ptr, char* str, int n);
void pbg_todate(pbg_lt_date* ptr, char* str, int n);

int pbg_cmpnumber(pbg_lt_number* n1, pbg_lt_number* n2);
int pbg_cmpdate(pbg_lt_date* d1, pbg_lt_date* d2);
int pbg_cmpstring(pbg_lt_string* s1, pbg_lt_string* s2, int n);

int pbg_type_isbool(pbg_field_type type);
int pbg_type_isop(pbg_field_type type);

/* HELPER FUNCTIONS */
int pbg_isdigit(char c);
int pbg_iswhitespace(char c);


/**********************
 *                    *
 * ERROR CONSTRUCTION *
 *                    *
 **********************/

void pbg_error_print(pbg_error* err)
{
	pbg_op_arity_err* arity;
	pbg_syntax_err* syntax;
	pbg_unknown_type_err* utype;
	if(err->_type == PBG_ERR_NONE)
		return;
	printf("error %s at %s:%d", 
			pbg_error_str(err->_type), err->_file, err->_line);
	switch(err->_type) {
		case PBG_ERR_OP_ARG_TYPE:
		case PBG_ERR_STATE:
			printf(": %s", (char*) err->_data);
			break;
		case PBG_ERR_OP_ARITY:
			arity = (pbg_op_arity_err*) err->_data;
			printf(": operator %s cannot take %d arguments!", 
					pbg_field_type_str(arity->_type), arity->_arity);
			break;
		case PBG_ERR_SYNTAX:
			syntax = (pbg_syntax_err*) err->_data;
			printf(": %s -> %s", (char*) syntax->_msg, syntax->_str+syntax->_i);
			break;
		case PBG_ERR_UNKNOWN_TYPE:
			utype = (pbg_unknown_type_err*) err->_data;
			printf(": failed to recognize %s (%d bytes)\n", (char*)utype->_field, utype->_n);
			break;
		default:
			break;
	}
	printf("\n");
}

void pbg_err_init(pbg_error* err, pbg_error_type type, int line, char* file, 
		int size, void* data)
{
	err->_type = type;
	err->_line = line;
	err->_file = file;
	err->_int = size;
	err->_data = data;
}

void pbg_err_alloc(pbg_error* err, int line, char* file) {
	pbg_err_init(err, PBG_ERR_ALLOC, line, file, 0, NULL);
}

void pbg_err_unknown_type(pbg_error* err, int line, char* file, 
		char* field, int n)
{
	pbg_unknown_type_err* data;
	data = malloc(n * sizeof(char));
	if(data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);  /* gah. */
		return;
	}
	data->_field = field;
	data->_n = n;
	pbg_err_init(err, PBG_ERR_UNKNOWN_TYPE, line, file, n, data);
}

void pbg_err_syntax(pbg_error* err, int line, char* file, 
		char* str, int i, char* msg)
{
	pbg_syntax_err* data;
	int size;
	data = malloc(size = sizeof(pbg_syntax_err));
	if(data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__); /* unfortunate. */
		return;
	}
	data->_msg = msg;
	data->_str = str;
	data->_i = i;
	pbg_err_init(err, PBG_ERR_SYNTAX, line, file, size, data);
}

void pbg_err_op_arity(pbg_error* err, int line, char* file, 
		pbg_field_type type, int arity)
{
	pbg_op_arity_err* data;
	int size;
	data = malloc(size = sizeof(pbg_op_arity_err));
	if(data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__); /* unfortunate. */
		return;
	}
	data->_arity = arity;
	data->_type = type;
	pbg_err_init(err, PBG_ERR_OP_ARITY, line, file, size, data);
}

void pbg_err_state(pbg_error* err, int line, char* file, char* msg) {
	pbg_err_init(err, PBG_ERR_STATE, line, file, 0, msg);
}

void pbg_err_op_arg_type(pbg_error* err, int line, char* file, char* msg) {
	pbg_err_init(err, PBG_ERR_OP_ARG_TYPE, line, file, 0, msg);
}

void pbg_error_free(pbg_error* err) {
	if(err->_int != 0) free(err->_data);
}


/********************
 *                  *
 * FIELD MANAGEMENT *
 *                  *
 ********************/

/**
 * This function returns the field identified by the given index. Constant fields
 * are identified by positive indices starting at 1. Variable fields are
 * identified by negative indices starting at -1.
 * @param e      PBG expression to get field from.
 * @param index  Index of the field to get.
 * @return Pointer to the pbg_field in e specified by the index,
 *         NULL if index is 0.
 */
pbg_field* pbg_field_get(pbg_expr* e, int index)
{
	if(index < 0) return e->_variables - (index+1);
	if(index > 0) return e->_constants + (index-1);
	return NULL;
}

/**
 * Free's the single pbg_field pointed to by the specified pointer.
 * @param field  pbg_field to free.
 */
void pbg_field_free(pbg_field* field) {
	if(field->_data != NULL) free(field->_data);
}

/**
 * This function stores the given constant field in the AST. Constant fields are
 * indexed using positive values starting at 1.
 * @param e     Abstract expression tree to store field in.
 * @param field  Field to store.
 * @return a positive index if successful,
 *         0 otherwise.
 */
int pbg_store_constant(pbg_expr* e, pbg_field field)
{
	int fieldi;
	if(field._type == PBG_NULL)
		return 0;
	fieldi = 1 + e->_numconst++;
	*pbg_field_get(e, fieldi) = field;
	return fieldi;
}

/**
 * This function stores the given variable field in the AST. Variable fields are
 * indexed using negative values starting at -1.
 * @param e     Abstract expression tree to store field in.
 * @param field  Field to store.
 * @return a negative index if successful,
 *         0 otherwise.
 */
int pbg_store_variable(pbg_expr* e, pbg_field field)
{
	int fieldi;
	fieldi = -(1 + e->_numvars++);
	*pbg_field_get(e, fieldi) = field;
	return fieldi;
}


/**************************
 *                        *
 * FIELD CREATION TOOLKIT *
 *                        *
 **************************/

/**
 * TODO
 */
pbg_field pbg_make_init(pbg_field_type type, int size, void* data)
{
	pbg_field field;
	field._type = type;
	field._int = size;
	field._data = data;
	return field;
}

/**
 * Makes a pbg_field representing the given operator type with the specified
 * number of child fields.
 * @param err   Used to store error, if any.
 * @param type  Type of the operator.
 * @param argc  Number of arguments.
 * @return the new pbg_field.
 */
pbg_field pbg_make_op(pbg_error* err, pbg_field_type type, int argc)
{
	void* data;
	data = malloc(argc * sizeof(int));
	if(data == NULL)
		pbg_err_alloc(err, __LINE__, __FILE__);
	return pbg_make_init(type, 0, data);
}

/**
 * Makes a field representing a VAR. Attempts to parse the given string as a 
 * VAR. If an error occurs during conversion, then err will be initialized with
 * the relevant error, if it is not NULL.
 * @param err  Used to store error, if any.
 * @param str  String to parse as a VAR.
 * @param n    Length of str.
 * @return a new field with the PBG_LT_VAR type if successful,
 *         a field with the PBG_NULL type otherwise.
 */
pbg_field pbg_make_var(pbg_error* err, char* str, int n)
{
	int size;
	void* data;
	data = malloc(size = (n-2) * sizeof(char));
	if(data == NULL) 
		pbg_err_alloc(err, __LINE__, __FILE__);
	else
		memcpy(data, str+1, n-2);
	return pbg_make_init(PBG_LT_VAR, size, data);
}

pbg_field pbg_make_field(pbg_field_type type) {
	return pbg_make_init(type, 0, NULL);
}

pbg_field pbg_make_date(pbg_error* err, char* str, int n)
{
	int size;
	pbg_lt_date* data;
	data = malloc(size = sizeof(pbg_lt_date));
	if(data == NULL) 
		pbg_err_alloc(err, __LINE__, __FILE__);
	else
		pbg_todate(data, str, n);
	return pbg_make_init(PBG_LT_DATE, size, data);
}

pbg_field pbg_make_number(pbg_error* err, char* str, int n)
{
	int size;
	pbg_lt_number* data;
	data = malloc(size = sizeof(pbg_lt_number));
	if(data == NULL)
		pbg_err_alloc(err, __LINE__, __FILE__);
	else
		pbg_tonumber(data, str, n);
	return pbg_make_init(PBG_LT_NUMBER, size, data);
}

pbg_field pbg_make_string(pbg_error* err, char* str, int n)
{
	int size;
	pbg_lt_string* data;
	data = malloc(size = (n-2) * sizeof(pbg_lt_string));
	if(data == NULL)
		pbg_err_alloc(err, __LINE__, __FILE__);
	else
		memcpy(data, str+1, n-2);
	return pbg_make_init(PBG_LT_STRING, size, data);
}


/*************************
 *                       *
 * FIELD PARSING TOOLKIT *
 *                       *
 *************************/

/**
 * Checks if the operator can legally take the specified number of arguments.
 * This function encodes the rules for operator arity and should be modified if
 * a new operator is added.
 * @param type     Type of operator.
 * @param numargs  Number of arguments to operator.
 * @return 1 if the number of arguments can be legally given to the operator, 
 *         0 if not or if type does not refer to an operator.
 */
int pbg_check_op_arity(pbg_field_type type, int numargs)
{
	int arity = 0;
	/* Positive arity specifies "exact arity," i.e. the number of arguments 
	 * must be exact. Negative arity specifies a "minimum arity," i.e. the 
	 * minimum number of arguments needed. */
	switch(type) {
		case PBG_OP_NOT:   arity =  1; break;
		case PBG_OP_AND:   arity = -2; break;
		case PBG_OP_OR:    arity = -2; break;
		case PBG_OP_EQ:    arity = -2; break;
		case PBG_OP_LT:    arity =  2; break;
		case PBG_OP_GT:    arity =  2; break;
		case PBG_OP_EXST:  arity =  1; break;
		case PBG_OP_NEQ:   arity =  2; break;
		case PBG_OP_LTE:   arity =  2; break;
		case PBG_OP_GTE:   arity =  2; break;
		case PBG_OP_TYPE:  arity = -2; break;
		default:
			return 0;
	}
	if((arity > 0 && numargs != arity) || 
			(-arity > 0 && numargs < -arity))
		return 0;
	return 1;
}

/**
 * TODO
 */
int pbg_parse_r(pbg_expr* e, pbg_error* err, char* str, 
		int** fields, int** lengths, int** closings)
{
	int fieldi;  /* Index of this field. This is the return value. */
	int n, strfieldstart;
	pbg_field_type type;
	pbg_field* field;
	
	int* children;  /* list of children nodes. */
	int maxc;       /* maximum number of children. */
	
	/* Cache length of field for easier referencing. */
	n = **lengths;
	strfieldstart = **fields;
	
	/* Update pointers for next field. */
	(*fields)++, (*lengths)++;
	
	/* Identify type of field. If the type cannot be resolve, throw an error. */
	type = pbg_gettype(str + strfieldstart, n);
	if(type == PBG_NULL) {
		pbg_err_unknown_type(err, __LINE__, __FILE__, str + strfieldstart, n);
		return 0;
	}
	
	/* This field is an operator. */
	if(pbg_type_isop(type)) {
		
		/* Maximum number of children this field has allocated space for. */
		maxc = 2;
		/* Initialize field and record field index. */
		fieldi = pbg_store_constant(e, pbg_make_op(err, type, maxc));
		field = pbg_field_get(e, fieldi);
		/* Propagate error back to caller, if any. */
		if(fieldi == 0) return 0;
		
		/* Recursively build subtree rooted at this operator field. pbg_evaluate 
		 * set last element in fields to -1. This is used to ensure we don't 
		 * run past the end of the expression string. */
		while(**fields != -1 && **fields < **closings) {
			/* Make recursive call to construct subtree rooted at child. */
			int childi = pbg_parse_r(e, err, str, fields, lengths, closings);
			/* Propagate error back to caller, if any. */
			if(childi == 0) return 0;
			/* Expand array of children if necessary. */
			if(field->_int == maxc) {
				maxc *= 2;  /* doubling gives amortized O(1) time insertion */
				children = (int*)realloc(field->_data, maxc * sizeof(int));
				if(children == NULL) {
					pbg_err_alloc(err, __LINE__, __FILE__);
					return 0;
				}
				field->_data = (void*) children;
			}
			/* Store index of child field. */
			((int*)field->_data)[field->_int++] = childi;
		}
		
		/* Enforce operator arity. */
		if(pbg_check_op_arity(type, field->_int) == 0) {
			pbg_err_op_arity(err, __LINE__, __FILE__, type, field->_int);
			return 0;
		}
		
		/* Tighten list of children and save it. */
		children = (int*) realloc(field->_data, field->_int * sizeof(int));
		if(children == NULL) {
			pbg_err_alloc(err, __LINE__, __FILE__);
			return 0;
		}
		field->_data = (void*) children;
		
		/* This field read all of its children until the next closing. 
		 * The parent field will need to read until the end of the next 
		 * next one. */
		(*closings)++;
		
	/* This field is a literal. */
	}else{
		/* Move str to correct starting position. */
		str += strfieldstart;
		/* Identify type of literal and initialize the field! */
		switch(type) {
			case PBG_LT_VAR:
				fieldi = pbg_store_variable(e, pbg_make_var(err, str, n));
				break;
			case PBG_LT_DATE:
				fieldi = pbg_store_constant(e, pbg_make_date(err, str, n));
				break;
			case PBG_LT_NUMBER:
				fieldi = pbg_store_constant(e, pbg_make_number(err, str, n));
				break;
			case PBG_LT_STRING:
				fieldi = pbg_store_constant(e, pbg_make_string(err, str, n));
				break;
			case PBG_LT_TRUE:
			case PBG_LT_FALSE:
			case PBG_LT_TP_DATE:
			case PBG_LT_TP_BOOL:
			case PBG_LT_TP_NUMBER:
			case PBG_LT_TP_STRING:
				fieldi = pbg_store_constant(e, pbg_make_field(type));
				break;
			default:
				pbg_err_unknown_type(err, __LINE__, __FILE__, str, n);
				return 0;
		}
	}
	
	/* Done! */
	return fieldi;
}

void pbg_parse(pbg_expr* e, pbg_error* err, char* str, int n)
{
	int i, c, f, opened;
	
	int numfields, numvars, numclosings;
	int depth, reachedend;
	int instring, invar;
	
	int numconstant, numvariable;
	int* fields;
	int* lengths;
	int* closings;
	
	int* lengths_cpy, *closings_cpy, *fields_cpy;
	int status;
	
	/* Always start with a clean error! */
	pbg_err_init(err, PBG_ERR_NONE, 0, NULL, 0, NULL);
	
	/* Set to NULL to allow for pbg_free to check if needing free. */
	e->_constants = NULL;
	e->_variables = NULL;
	
	/* These are initialized to 0 as they are used as counters for the number 
	 * of each type of field created. In the end they should be equal to the 
	 * associated local variables here. */
	e->_numconst = 0;
	e->_numvars = 0;
	
	/* Verify that all strings & variable names are bounded, that all opening 
	 * parentheses have friends, and that only a single expression is present. 
	 * Also count the number of fields, variables, and closings. */
	numfields = 0;
	numvars = 0;
	numclosings = 0;
	depth = 0, reachedend = 0;
	instring = 0, invar = 0;
	for(i = 0; i < n; i++) {
		if(pbg_iswhitespace(str[i])) continue;
		/* Count number of variables. */
		if(str[i] == '[') numvars++;
		/* It's an open! Delve deeper into the tree. */
		if(str[i] == '(') depth++;
		/* It's a close! Rise up in the tree. */
		else if(str[i] == ')') {
			numclosings++, depth--;
			/* Negative tree depth only occurs if unmatched closing 
			 * parentheses. */
			if(depth < 0) {
				pbg_err_syntax(err, __LINE__, __FILE__, str, i,
						"Too many closing parentheses.");
				return;
			}
			/* Depth zero only legally occurs at the end. */
			if(depth == 0 && !reachedend)
				reachedend = i;
			else if(depth == 0 && reachedend) {
				pbg_err_syntax(err, __LINE__, __FILE__, str, reachedend,
						"More than one complete expression.");
				return;
			}
		/* It's a field! */
		}else{
			/* It's a string! */
			if(str[i] == '\'') {
				instring = 1;
				do i++; while(i != n && !(str[i] == '\'' && str[i-1] != '\\'));
				if(i != n) instring = 0;
			/* It's a variable! */
			}else if(str[i] == '[') {
				invar = 1;
				do i++; while(!(str[i] == ']' && str[i-1] != '\\'));
				if(i != n) invar = 0;
			/* It's literally anything else! */
			}else
				while(i != n && !pbg_iswhitespace(str[i+1]) && 
						str[i+1] != '[' && str[i+1] != '(' && str[i+1] != ')') 
					i++;
			numfields++;
		}
	}
	/* Check if string is left unclosed. */
	if(instring) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, instring, 
				"Unclosed string.");
		return;
	}
	/* Check if variable is left unclosed. */
	if(invar) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, invar, 
				"Unclosed variable.");
		return;
	}
	/* Check if opening parentheses are left unclosed. */
	if(depth != 0) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, 0,
				"Unmatched opening parentheses.");
		return;
	}
	
	/* Compute sizes of constant and variable arrays. */
	numconstant = numfields - numvars;
	numvariable = numvars;
	
	/* Allocate space for needed arrays. */
	/* One extra field is allocated to be set to -1. This helps determine when 
	 * we've looped through every field. */
	fields = (int*) malloc((numfields+1) * sizeof(int));
	if(fields == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	lengths = (int*) malloc(numfields * sizeof(int));
	if(lengths == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	closings = (int*) malloc(numclosings * sizeof(int));
	if(closings == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	
	/* Identify the indices of all fields and closings as well as lengths of 
	 * the fields. */
	for(i = 0, c = 0, f = 0, opened = 0; i < n; i++) {
		/* Whitespaces are the enemy. */
		if(pbg_iswhitespace(str[i])) continue;
		/* It's a close! */
		if(str[i] == ')') {
			closings[c++] = i;
		}else if(str[i] == '(') {
			opened = 1;
		/* It's a field! */
		}else if(str[i] != '(') {
			/* Grab index of field. */
			fields[f] = i;
			/* It's a string! */
			if(str[i] == '\'') {
				do i++; while(!(str[i] == '\'' && str[i-1] != '\\'));
			/* It's a variable! */
			}else if(str[i] == '[')
				do i++; while(!(str[i] == ']' && str[i-1] != '\\'));
			/* It's literally anything else! */
			else
				while(i != n-1 && !pbg_iswhitespace(str[i+1]) && str[i+1] != '[' && 
						str[i+1] != '(' && str[i+1] != ')') i++;
			/* Compute length of field. */
			lengths[f] = i - fields[f] + 1;
			/* Record opening field index. This will be checked as an 
			 * operator later. */
			if(opened == 1) {
				pbg_field_type type = pbg_gettype(str + fields[f], lengths[f]);
				if(!pbg_type_isop(type)) {
					pbg_err_syntax(err, __LINE__, __FILE__, str, fields[f],
							"Not an operator!");
					return;
				}
				opened = 0;
			}
			f++;
		}
	}
	
	/* Needed to determine when we've reached the end of the expression string
	 * in pbg_parse_r. */
	fields[numfields] = -1;
	
	/* Allocate space for constant and variable field arrays. */
	e->_constants = (pbg_field*) malloc(numconstant * sizeof(pbg_field));
	if(e->_constants == NULL) {
		free(fields), free(lengths), free(closings);
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	e->_variables = (pbg_field*) malloc(numvariable * sizeof(pbg_field));
	if(e->_variables == NULL) {
		free(e->_constants);
		free(fields), free(lengths), free(closings);
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	
	/* Recursively parse the expression string to build the expression tree. */
	lengths_cpy = lengths;
	closings_cpy = closings;
	fields_cpy = fields;
	status = pbg_parse_r(e, err, str, &fields_cpy, 
			&lengths_cpy, &closings_cpy);
	
	/* If an error occurred, clean up. */
	if(status == 0) pbg_free(e);
	
	/* Clean up! */
	free(fields), free(lengths), free(closings);
	
	/* Do not perform sanity checks if there's already an error. */
	if(err->_type != PBG_ERR_NONE)
		return;
	
	/* Sanity check: verify we parsed everything we expected. */
	if(e->_numconst != numconstant || e->_numvars != numvariable) {
		pbg_err_state(err, __LINE__, __FILE__,
				"Not all fields were parsed?");
		return;
	}
}


/****************************
 *                          *
 * FIELD EVALUATION TOOLKIT *
 *                          *
 ****************************/

int pbg_evaluate_op_not(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0, result;
	child0 = ((int*)field->_data)[0];
	result = pbg_evaluate_r(e, err, pbg_field_get(e, child0));
	if(result == -1) return -1;  /* Pass error through. */
	return !result;
}

int pbg_evaluate_op_and(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int i, size, childi, result;
	size = field->_int;
	for(i = 0; i < size; i++) {
		childi = ((int*)field->_data)[i];
		result = pbg_evaluate_r(e, err, pbg_field_get(e, childi));
		if(result == -1) return -1;  /* Pass error through. */
		if(result == 0)  return  0;  /* FALSE! */
	}
	return 1;  /* TRUE! */
}

int pbg_evaluate_op_or(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int i, childi, result;
	for(i = 0; i < field->_int; i++) {
		childi = ((int*)field->_data)[i];
		result = pbg_evaluate_r(e, err, pbg_field_get(e, childi));
		if(result == -1) return -1;  /* Pass error through. */
		if(result == 1)  return  1;  /* TRUE! */
	}
	return 0;  /* FALSE! */
}

int pbg_evaluate_op_exst(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0;
	PBG_UNUSED(err);
	child0 = ((int*)field->_data)[0];
	return pbg_field_get(e, child0)->_type != PBG_NULL;
}

int pbg_evaluate_op_eq(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int i, child0, childi, result;
	pbg_field* c0, *ci;
	PBG_UNUSED(err);
	/* Ensure type and size of all children are identical. */
	child0 = ((int*)field->_data)[0];
	c0 = pbg_field_get(e, child0);
	if(c0->_type == PBG_NULL) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"NULL input given to EQ operator.");
		return -1;
	}
	/* We have a bunch of BOOLs! Evaluate them. */
	if(pbg_type_isbool(c0->_type)) {
		result = pbg_evaluate_r(e, err, c0);
		for(i = 1; i < field->_int; i++) {
			childi = ((int*)field->_data)[i];
			ci = pbg_field_get(e, childi);
			if(ci->_type == PBG_NULL) {
				pbg_err_op_arg_type(err, __LINE__, __FILE__, 
						"NULL input given to EQ operator.");
				return -1;
			}
			if(result != pbg_evaluate_r(e, err, ci))
				return 0;
		}
		return 1;
	/* We don't have a bunch of BOOLs! Do standard equality test. */
	}else{
		for(i = 1; i < field->_int; i++) {
			childi = ((int*)field->_data)[i];
			ci = pbg_field_get(e, childi);
			if(ci->_type == PBG_NULL) {
				pbg_err_op_arg_type(err, __LINE__, __FILE__, 
						"NULL input given to EQ operator.");
				return -1;
			}
			if(ci->_int != c0->_int || 
					ci->_type != c0->_type)
				return 0;  /* FALSE! */
			/* Ensure each data byte is identical. */
			if(memcmp(ci->_data, c0->_data, c0->_int) != 0)
				return 0;  /* FALSE! */
		}
		return 1;  /* TRUE! */
	}
}

int pbg_evaluate_op_neq(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0, child1;
	pbg_field* c0, *c1;
	PBG_UNUSED(err);
	child0 = ((int*)field->_data)[0], child1 = ((int*)field->_data)[1];
	c0 = pbg_field_get(e, child0), c1 = pbg_field_get(e, child1);
	if(c0->_type == PBG_NULL || c1->_type == PBG_NULL) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"NULL input given to NEQ operator.");
		return -1;
	}
	/* We have two BOOLs! Evaluate them, and check if they are different. */
	if(pbg_type_isbool(c0->_type) && pbg_type_isbool(c1->_type))
		return pbg_evaluate_r(e, err, c0) != pbg_evaluate_r(e, err, c1);
	/* We don't have a bunch of BOOLs! Do standard difference check. */
	else return c1->_type != c0->_type || 
			c1->_int != c0->_int || 
			memcmp(c1->_data, c0->_data, c0->_int);
}

int pbg_evaluate_op_lt(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0, child1;
	pbg_field* c0, *c1;
	child0 = ((int*)field->_data)[0], child1 = ((int*)field->_data)[1];
	c0 = pbg_field_get(e, child0), c1 = pbg_field_get(e, child1);
	if(c0->_type == PBG_NULL || c1->_type == PBG_NULL) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"NULL input given to LT operator.");
		return -1;
	}
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER)
		return pbg_cmpnumber(c0->_data, c1->_data) < 0;
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE)
		return pbg_cmpdate(c0->_data, c1->_data) < 0;
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING)
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) < 0;
	/* Both are BOOLs. */
	if(pbg_type_isbool(c0->_type) && pbg_type_isbool(c1->_type))
		return pbg_evaluate_r(e, err, c0) < pbg_evaluate_r(e, err, c1);
	pbg_err_op_arg_type(err, __LINE__, __FILE__, 
			"Unknown input type to LT operator");
	return -1;
}

int pbg_evaluate_op_gt(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0, child1;
	pbg_field* c0, *c1;
	child0 = ((int*)field->_data)[0], child1 = ((int*)field->_data)[1];
	c0 = pbg_field_get(e, child0), c1 = pbg_field_get(e, child1);
	if(c0->_type == PBG_NULL || c1->_type == PBG_NULL) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"NULL input given to GT operator.");
		return -1;
	}
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER)
		return pbg_cmpnumber(c0->_data, c1->_data) > 0;
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE)
		return pbg_cmpdate(c0->_data, c1->_data) > 0;
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING)
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) > 0;
	/* Both are BOOLs. */
	if(pbg_type_isbool(c0->_type) && pbg_type_isbool(c1->_type))
		return pbg_evaluate_r(e, err, c0) > pbg_evaluate_r(e, err, c1);
	pbg_err_op_arg_type(err, __LINE__, __FILE__, 
			"Unknown input type to GT operator");
	return -1;
}

int pbg_evaluate_op_lte(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0, child1;
	pbg_field* c0, *c1;
	child0 = ((int*)field->_data)[0], child1 = ((int*)field->_data)[1];
	c0 = pbg_field_get(e, child0), c1 = pbg_field_get(e, child1);
	if(c0->_type == PBG_NULL || c1->_type == PBG_NULL) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"NULL input given to LTE operator.");
		return -1;
	}
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER)
		return pbg_cmpnumber(c0->_data, c1->_data) <= 0;
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE)
		return pbg_cmpdate(c0->_data, c1->_data) <= 0;
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING)
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) <= 0;
	/* Both are BOOLs. */
	if(pbg_type_isbool(c0->_type) && pbg_type_isbool(c1->_type))
		return pbg_evaluate_r(e, err, c0) <= pbg_evaluate_r(e, err, c1);
	pbg_err_op_arg_type(err, __LINE__, __FILE__, 
			"Unknown input type to LTE operator");
	return -1;
}

int pbg_evaluate_op_gte(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int child0, child1;
	pbg_field* c0, *c1;
	child0 = ((int*)field->_data)[0], child1 = ((int*)field->_data)[1];
	c0 = pbg_field_get(e, child0), c1 = pbg_field_get(e, child1);
	if(c0->_type == PBG_NULL || c1->_type == PBG_NULL) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"NULL input given to GTE operator.");
		return -1;
	}
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER)
		return pbg_cmpnumber(c0->_data, c1->_data) >= 0;
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE)
		return pbg_cmpdate(c0->_data, c1->_data) >= 0;
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING)
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) >= 0;
	/* Both are BOOLs. */
	if(pbg_type_isbool(c0->_type) && pbg_type_isbool(c1->_type))
		return pbg_evaluate_r(e, err, c0) >= pbg_evaluate_r(e, err, c1);
	pbg_err_op_arg_type(err, __LINE__, __FILE__, 
			"Unknown input type to GTE operator");
	return -1;
}

int pbg_evaluate_op_typeof(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	int i, child0, childi;
	pbg_field* c0, *ci;
	pbg_field_type type;
	child0 = ((int*)field->_data)[0];
	c0 = pbg_field_get(e, child0);
	type = c0->_type;
	/* Ensure the first argument is a type literal. */
	if(type < PBG_MIN_LT_TP || type > PBG_MAX_LT_TP) {
		pbg_err_op_arg_type(err, __LINE__, __FILE__, 
				"First input to TYPE operator must be a type literal.");
		return -1;
	}
	/* Verify types of all trailing arguments. */
	for(i = 1; i < field->_int; i++) {
		childi = ((int*)field->_data)[i];
		ci = pbg_field_get(e, childi);
		if(type == PBG_LT_TP_BOOL && !pbg_type_isbool(ci->_type))
			return 0;  /* FALSE */
		if(type == PBG_LT_TP_DATE && ci->_type != PBG_LT_DATE)
			return 0;  /* FALSE */
		if(type == PBG_LT_TP_NUMBER && ci->_type != PBG_LT_NUMBER)
			return 0;  /* FALSE */
		if(type == PBG_LT_TP_STRING && ci->_type != PBG_LT_STRING)
			return 0;  /* FALSE */
	}
	return 1;  /* TRUE! */
}

/**
 * TODO
 */
int pbg_evaluate_r(pbg_expr* e, pbg_error* err, pbg_field* field)
{
	if(pbg_type_isbool(field->_type))
		switch(field->_type) {
			case PBG_OP_NOT:   return pbg_evaluate_op_not(e, err, field);
			case PBG_OP_AND:   return pbg_evaluate_op_and(e, err, field);
			case PBG_OP_OR:    return pbg_evaluate_op_or(e, err, field);
			case PBG_OP_EXST:  return pbg_evaluate_op_exst(e, err, field);
			case PBG_OP_EQ:    return pbg_evaluate_op_eq(e, err, field);
			case PBG_OP_NEQ:   return pbg_evaluate_op_neq(e, err, field);
			case PBG_OP_LT:    return pbg_evaluate_op_lt(e, err, field);
			case PBG_OP_GT:    return pbg_evaluate_op_gt(e, err, field);
			case PBG_OP_LTE:   return pbg_evaluate_op_lte(e, err, field);
			case PBG_OP_GTE:   return pbg_evaluate_op_gte(e, err, field);
			case PBG_OP_TYPE:  return pbg_evaluate_op_typeof(e, err, field);
			case PBG_LT_TRUE:  return 1;
			case PBG_LT_FALSE: return 0;
			default: pbg_err_state(err, __LINE__, __FILE__,
							"Unsupported operation.");
		}
	else
		pbg_err_state(err, __LINE__, __FILE__, 
				"Cannot evaluate a non-BOOL value.");
	return -1;
}

int pbg_evaluate(pbg_expr* e, pbg_error* err, pbg_field (*dict)(char*, int))
{
	int i, result;
	pbg_field* newvars, *var, *oldvars;
	
	/* Always start with a clean error! */
	pbg_err_init(err, PBG_ERR_NONE, 0, NULL, 0, NULL);
	
	/* Variable resolution. Lookup every variable in provided dictionary. */
	newvars = (pbg_field*) malloc(e->_numvars * sizeof(pbg_field));
	if(newvars == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return -1;
	}
	for(i = 0; i < e->_numvars; i++) {
		var = e->_variables+i;
		newvars[i] = dict((char*)(var->_data), var->_int);
	}
	
	/* Swap out variable literals with dictionary equivalents. */
	oldvars = e->_variables;
	e->_variables = newvars;
	
	/* Evaluate expression! */
	result = pbg_evaluate_r(e, err, e->_constants);
	
	/* Restore old variable literal array. */
	e->_variables = oldvars;
	
	/* Clean up malloc'd memory. */
	for(i = 0; i < e->_numvars; i++)
		pbg_field_free(newvars+i);
	free(newvars);
	
	/* Done! */
	return result;
}


/************************
 *                      *
 * JANITORIAL FUNCTIONS *
 *                      *
 ************************/

void pbg_free(pbg_expr* e)
{
	int i;
	
	/* Free individual constant fields. Some do not have _data malloc'd. */
	for(i = e->_numconst-1; i >= 0; i--)
		pbg_field_free(e->_constants+i);
	
	/* Free individual variable fields. All have _data malloc'd. */
	for(i = 0; i < e->_numvars; i++)
		pbg_field_free(e->_variables+i);
	
	/* Free internal field arrays. */
	if(e->_constants != NULL) free(e->_constants);
	if(e->_variables != NULL) free(e->_variables);
}


/*********************************
 *                               *
 * CONVERSION & CHECKING TOOLKIT *
 *                               *
 *********************************/
 
/**
 * Translates the given pbg_field_type to a human-readable string.
 * @param type  PBG field type to translate.
 * @return String representation of the given field type.
 */
char* pbg_field_type_str(pbg_field_type type)
{
	switch(type) {
		case PBG_LT_TRUE: return "PBG_LT_TRUE";
		case PBG_LT_FALSE: return "PBG_LT_FALSE";
		case PBG_LT_NUMBER: return "PBG_LT_NUMBER";
		case PBG_LT_STRING: return "PBG_LT_STRING";
		case PBG_LT_DATE: return "PBG_LT_DATE";
		case PBG_LT_VAR: return "PBG_LT_VAR";
		case PBG_OP_NOT: return "PBG_OP_NOT";
		case PBG_OP_AND: return "PBG_OP_AND";
		case PBG_OP_OR: return "PBG_OP_OR";
		case PBG_OP_EQ: return "PBG_OP_EQ";
		case PBG_OP_LT: return "PBG_OP_LT";
		case PBG_OP_GT: return "PBG_OP_GT";
		case PBG_OP_EXST: return "PBG_OP_EXST";
		case PBG_OP_NEQ: return "PBG_OP_NEQ";
		case PBG_OP_LTE: return "PBG_OP_LTE";
		case PBG_OP_GTE: return "PBG_OP_GTE";
		case PBG_LT_TP_DATE: return "PBG_LT_TP_DATE";
		case PBG_LT_TP_BOOL: return "PBG_LT_TP_BOOL";
		case PBG_LT_TP_NUMBER: return "PBG_LT_TP_NUMBER";
		case PBG_LT_TP_STRING: return "PBG_LT_TP_STRING";
		default: return "PBG_NULL";
	}
}

/**
 * Translates the given pbg_error_type to a human-readable string.
 * @param type  PBG error type to translate.
 * @return String representation of the given error type.
 */
char* pbg_error_str(pbg_error_type type)
{
	switch(type) {
		case PBG_ERR_NONE:         return "PBG_ERR_NONE";
		case PBG_ERR_ALLOC:        return "PBG_ERR_ALLOC";
		case PBG_ERR_STATE:        return "PBG_ERR_STATE";
		case PBG_ERR_SYNTAX:       return "PBG_ERR_SYNTAX";
		case PBG_ERR_UNKNOWN_TYPE: return "PBG_ERR_UNKNOWN_TYPE";
		case PBG_ERR_OP_ARITY:     return "PBG_ERR_OP_ARITY";
		case PBG_ERR_OP_ARG_TYPE:  return "PBG_ERR_OP_ARG_TYPE";
	}
	return "PBG_ERR_???";
}

int pbg_iserror(pbg_error* err) {
	return err->_type != PBG_ERR_NONE;
}

pbg_field_type pbg_gettype(char* str, int n)
{
	/* Is it a literal? */
	if(pbg_istrue(str, n))   return PBG_LT_TRUE;
	if(pbg_isfalse(str, n))  return PBG_LT_FALSE;
	if(pbg_isnumber(str, n)) return PBG_LT_NUMBER;
	if(pbg_isstring(str, n)) return PBG_LT_STRING;
	if(pbg_isdate(str, n))   return PBG_LT_DATE;
	if(pbg_isvar(str, n))    return PBG_LT_VAR;
	if(pbg_istypedate(str, n))    return PBG_LT_TP_DATE;
	if(pbg_istypebool(str, n))    return PBG_LT_TP_BOOL;
	if(pbg_istypenumber(str, n))  return PBG_LT_TP_NUMBER;
	if(pbg_istypestring(str, n))  return PBG_LT_TP_STRING;
	
	/* Is it an operator? */
	if(n == 1) {
		if(str[0] == '!') return PBG_OP_NOT;
		if(str[0] == '&') return PBG_OP_AND;
		if(str[0] == '|') return PBG_OP_OR;
		if(str[0] == '=') return PBG_OP_EQ;
		if(str[0] == '<') return PBG_OP_LT;
		if(str[0] == '>') return PBG_OP_GT;
		if(str[0] == '?') return PBG_OP_EXST;
		if(str[0] == '@') return PBG_OP_TYPE;
	}
	if(n == 2) {
		if(str[0] == '!' && str[1] == '=') return PBG_OP_NEQ;
		if(str[0] == '<' && str[1] == '=') return PBG_OP_LTE;
		if(str[0] == '>' && str[1] == '=') return PBG_OP_GTE;
	}
	
	/* It isn't anything! */
	return PBG_NULL;
}

int pbg_istypedate(char* str, int n) {
	return n == 4 && 
		str[0]=='D' && 
		str[1]=='A' && 
		str[2]=='T' && 
		str[3]=='E';
}

int pbg_istypenumber(char* str, int n) {
	return n == 6 && 
		str[0]=='N' && 
		str[1]=='U' && 
		str[2]=='M' && 
		str[3]=='B' && 
		str[4]=='E' && 
		str[5]=='R';
}

int pbg_istypebool(char* str, int n) {
	return n == 4 && 
		str[0]=='B' && 
		str[1]=='O' && 
		str[2]=='O' && 
		str[3]=='L';
}

int pbg_istypestring(char* str, int n) {
	return n == 6 && 
		str[0]=='S' && 
		str[1]=='T' && 
		str[2]=='R' && 
		str[3]=='I' && 
		str[4]=='N' && 
		str[5]=='G';
}

int pbg_istrue(char* str, int n) {
	return n == 4 && 
		str[0]=='T' && 
		str[1]=='R' && 
		str[2]=='U' && 
		str[3]=='E';
}

int pbg_isfalse(char* str, int n) {
	return n == 5 && 
		str[0]=='F' && 
		str[1]=='A' && 
		str[2]=='L' && 
		str[3]=='S' && 
		str[4]=='E';
}

int pbg_isnumber(char* str, int n)
{
	int i;
	
	/* Start at the beginning of the string. */
	i = 0;
	
	/* Check if negative or positive */
	if(str[i] == '-' || str[i] == '+') i++;
	/* Otherwise, ensure first character is a digit. */
	else if(!pbg_isdigit(str[i]))
		return 0;
	
	/* Parse everything before the dot. */
	if(str[i] != '0' && pbg_isdigit(str[i])) {
		while(i != n && pbg_isdigit(str[i])) i++;
		if(i != n && !pbg_isdigit(str[i]) && str[i] != '.') return 0;
	}else if(str[i] == '0') {
		if(++i != n && !(str[i] == '.' || str[i] == 'e' || str[i] == 'E')) return 0;
	}
	
	/* Parse everything after the dot. */
	if(str[i] == '.') {
		/* Last character must be a digit. */
		if(i++ == n-1) return 0;
		/* Exhaust all digits. */
		while(i != n && pbg_isdigit(str[i])) i++;
		if(i != n && !pbg_isdigit(str[i]) && str[i] != 'e' && str[i] != 'E') return 0;
	}
	
	/* Parse everything after the exponent. */
	if(str[i] == 'e' || str[i] == 'E') {
		/* Last character must be a digit. */
		if(i++ == n-1) return 0;
		/* Parse positive or negative sign. */
		if(str[i] == '-' || str[i] == '+') i++;
		/* Exhaust all digits. */
		while(i != n && pbg_isdigit(str[i])) i++;
		if(i != n && !pbg_isdigit(str[i])) return 0;
	}
	
	/* Probably a number! */
	return 1;
}

void pbg_tonumber(pbg_lt_number* ptr, char* str, int n) {
	PBG_UNUSED(n);
	ptr->_val = atof(str);
}

int pbg_cmpnumber(pbg_lt_number* n1, pbg_lt_number* n2) {
	if(n1->_val < n2->_val) return -1;
	if(n1->_val > n2->_val) return 1;
	return 0;
}

int pbg_cmpdate(pbg_lt_date* n1, pbg_lt_date* n2) {
	if(n1->_YYYY < n2->_YYYY) return -1;
	if(n1->_YYYY > n2->_YYYY) return 1;
	if(n1->_MM < n2->_MM) return -1;
	if(n1->_MM > n2->_MM) return 1;
	if(n1->_DD < n2->_DD) return -1;
	if(n1->_DD > n2->_DD) return 1;
	return 0;
}

int pbg_cmpstring(pbg_lt_string* s1, pbg_lt_string* s2, int n) {
	return strncmp(s1, s2, n);
}

int pbg_isvar(char* str, int n) {
	return str[0] == '[' && str[n-1] == ']';
}

int pbg_isstring(char* str, int n) {
	/* TODO ensure I don't contain any unescaped single quotes! */
	return str[0] == '\'' && str[n-1] == '\'';
}

int pbg_isdate(char* str, int n) {
	return n == 10 && 
		pbg_isdigit(str[0]) && pbg_isdigit(str[1]) && 
		pbg_isdigit(str[2]) && pbg_isdigit(str[3]) &&
		str[4] == '-' && 
		pbg_isdigit(str[5]) && pbg_isdigit(str[6]) && 
		str[7] == '-' && 
		pbg_isdigit(str[8]) && pbg_isdigit(str[9]);
}

void pbg_todate(pbg_lt_date* ptr, char* str, int n) {
	if(n != 10) return;
	ptr->_YYYY = (str[0]-'0')*1000 + (str[1]-'0')*100 + (str[2]-'0')*10 + (str[3]-'0');
	ptr->_MM = (str[5]-'0')*10 + (str[6]-'0');
	ptr->_DD = (str[8]-'0')*10 + (str[9]-'0');
	/* TODO enforce ranges on months and days */
}

/**
 * Checks if the given type is an operator, TRUE, or FALSE. Useful for checking 
 * if both arguments will have a valid return value from pbg_evaluate_r.
 * @param type  Type to check.
 * @return 1 if the given type is TRUE, FALSE, or an operator,
 *         0 otherwise.
 */
int pbg_type_isbool(pbg_field_type type) {
	return type == PBG_LT_TRUE || type == PBG_LT_FALSE || 
			(type < PBG_MAX_OP && type > PBG_MIN_OP);
}

/**
 * Checks if the given type is an operator.
 * @param type  Type to check.
 * @return 1 if the given type is an operator,
 *         0 otherwise.
 */
int pbg_type_isop(pbg_field_type type) {
	return type > PBG_MIN_OP && type < PBG_MAX_OP;
}


/********************
 *                  *
 * HELPER FUNCTIONS *
 *                  *
 ********************/

/**
 * Checks if the given character is a digit.
 * @param c  Character to check.
 */
int pbg_isdigit(char c) { return c >= '0' && c <= '9'; }

/**
 * Checks if the given character is whitespace.
 * @param c  Character to check.
 */
int pbg_iswhitespace(char c) { return c==' ' || c=='\t' || c=='\n'; }
