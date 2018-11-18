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
} pbg_number_lt;  /* PBG_LT_NUMBER */

typedef struct {
	unsigned int  _YYYY;  /* year */
	unsigned int  _MM;    /* month */
	unsigned int  _DD;    /* day */
} pbg_date_lt;  /* PBG_LT_DATE */

typedef char pbg_string_lt; /* PBG_LT_STRING */

/* ERROR REPRESENTATIONS */
typedef struct {
	int            _arity;  /* Number of arguments given to operator. */
	pbg_node_type  _type;   /* Type of operator involved in error. */
} pbg_op_arity_err;  /* PBG_ERR_OP_ARITY. */

typedef struct {
	char*  _msg;  /* Description of syntax error. */
	char*  _str;  /* String in which error occurred. */
	int    _i;    /* Index of error in string. */
} pbg_syntax_err;  /* PBG_ERR_SYNTAX */


/****************************
 *                          *
 * LOCAL FUNCTION DIRECTORY *
 *                          *
 ****************************/
 
/* NODE MANAGEMENT */
pbg_expr_node* pbg_get_node(pbg_expr* e, int index);
void pbg_free_node(pbg_expr_node* node);

/* ERROR MANAGEMENT */
void pbg_err_alloc(pbg_error* err, int line, char* file);
void pbg_err_unknown_type(pbg_error* err, int line, char* file);
void pbg_err_syntax(pbg_error* err, int line, char* file, char* str, int i, char* msg);
void pbg_err_op_arity(pbg_error* err, int line, char* file, pbg_node_type type, int arity);
void pbg_err_state(pbg_error* err, int line, char* file, char* msg);
void pbg_err_op_arg_type(pbg_error* err, int line, char* file);

char* pbg_type_str(pbg_node_type type);
char* pbg_error_str(pbg_error_type type);

/* NODE CREATION TOOLKIT */
int pbg_create_op(pbg_expr* e, pbg_error* err, pbg_node_type type, int numchildren);
int pbg_create_lt_key(pbg_expr* e, pbg_error* err, char* str, int n);
int pbg_create_lt_date(pbg_expr* e, pbg_error* err, char* str, int n);
int pbg_create_lt_number(pbg_expr* e, pbg_error* err, char* str, int n);
int pbg_create_lt_string(pbg_expr* e, pbg_error* err, char* str, int n);
int pbg_create_lt_true(pbg_expr* e, pbg_error* err, char* str, int n);
int pbg_create_lt_false(pbg_expr* e, pbg_error* err, char* str, int n);

/* NODE PARSING TOOLKIT */
int pbg_check_op_arity(pbg_node_type type, int numargs);
int pbg_parse_r(pbg_expr* e, pbg_error* err, char* str, int** fields, int** lengths, int** closings);

/* NODE EVALUATION TOOLKIT */
int pbg_evaluate_r(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_not(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_and(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_or(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_exst(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_eq(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_neq(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_lt(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_gt(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_lte(pbg_expr* e, pbg_error* err, pbg_expr_node* node);
int pbg_evaluate_op_gte(pbg_expr* e, pbg_error* err, pbg_expr_node* node);

/* JANITORIAL FUNCTIONS */
// No local functions.

/* CONVERSION & CHECKING TOOLKIT */
int pbg_istrue(char* str, int n);
int pbg_isfalse(char* str, int n);
int pbg_iskey(char* str, int n);
int pbg_isnumber(char* str, int n);
int pbg_isstring(char* str, int n);
int pbg_isdate(char* str, int n);

void pbg_tonumber(pbg_number_lt* ptr, char* str, int n);
void pbg_todate(pbg_date_lt* ptr, char* str, int n);

int pbg_cmpnumber(pbg_number_lt* n1, pbg_number_lt* n2);
int pbg_cmpdate(pbg_date_lt* d1, pbg_date_lt* d2);
int pbg_cmpstring(pbg_string_lt* s1, pbg_string_lt* s2, int n);

/* HELPER FUNCTIONS */
int pbg_isdigit(char c);
int pbg_iswhitespace(char c);


/*******************
 *                 *
 * NODE MANAGEMENT *
 *                 *
 *******************/

/**
 * This function returns the node identified by the given index. Static nodes
 * are identified by positive indices starting at 1. Dynamic nodes are
 * identified by negative indices starting at -1.
 * @param e      PBG expression to get node from.
 * @param index  Index of the node to get.
 * @return Pointer to the pbg_expr_node in e specified by the index,
 *         NULL if index is 0.
 */
pbg_expr_node* pbg_get_node(pbg_expr* e, int index)
{
	if(index < 0) return e->_dynamic - (index+1);
	if(index > 0) return e->_static + (index-1);
	return NULL;
}

/**
 * Free's the single pbg_expr_node pointed to by the specified pointer.
 * @param node  pbg_expr_node to free.
 */
void pbg_free_node(pbg_expr_node* node)
{
	if(node->_data != NULL) free(node->_data);
}


/**********************
 *                    *
 * ERROR CONSTRUCTION *
 *                    *
 **********************/

void pbg_error_print(pbg_error* err)
{
	if(err->_type == PBG_ERR_NONE)
		return;
	pbg_op_arity_err* arity;
	pbg_syntax_err* syntax;
	printf("error %s at %s:%d", 
			pbg_error_str(err->_type), err->_file, err->_line);
	switch(err->_type) {
		case PBG_ERR_STATE:
			printf(": %s", (char*) err->_data);
			break;
		case PBG_ERR_OP_ARITY:
			arity = (pbg_op_arity_err*) err->_data;
			printf(": operator %s cannot take %d arguments!", 
					pbg_type_str(arity->_type), arity->_arity);
			break;
		case PBG_ERR_SYNTAX:
			syntax = (pbg_syntax_err*) err->_data;
			printf(": %s -> %s", (char*) syntax->_msg, syntax->_str+syntax->_i);
			break;
		default:
			break;
	}
	printf("\n");
}

void pbg_err_alloc(pbg_error* err, int line, char* file)
{
	err->_type = PBG_ERR_ALLOC;
	err->_line = line;
	err->_file = file;
	err->_int = 0;
	err->_data = NULL;
}

void pbg_err_unknown_type(pbg_error* err, int line, char* file)
{
	err->_type = PBG_ERR_UNKNOWN_TYPE;
	err->_line = line;
	err->_file = file;
	err->_int = 0;
	err->_data = NULL;
}

void pbg_err_syntax(pbg_error* err, int line, char* file, 
		char* str, int i, char* msg)
{
	err->_type = PBG_ERR_SYNTAX;
	err->_line = line;
	err->_file = file;
	err->_int = sizeof(pbg_syntax_err);
	err->_data = malloc(err->_int);
	if(err->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__); /* unfortunate. */
		return;
	}
	*((pbg_syntax_err*)err->_data) = (pbg_syntax_err) { msg, str, i };
}

void pbg_err_op_arity(pbg_error* err, int line, char* file, 
		pbg_node_type type, int arity)
{
	err->_type = PBG_ERR_OP_ARITY;
	err->_line = line;
	err->_file = file;
	err->_int = sizeof(pbg_op_arity_err);
	err->_data = malloc(err->_int);
	if(err->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__); /* unfortunate. */
		return;
	}
	*((pbg_op_arity_err*)err->_data) = (pbg_op_arity_err) { arity, type };
}

void pbg_err_state(pbg_error* err, int line, char* file, char* msg)
{
	err->_type = PBG_ERR_STATE;
	err->_line = line;
	err->_file = file;
	err->_int = 0;
	err->_data = msg;
}

void pbg_err_op_arg_type(pbg_error* err, int line, char* file)
{
	err->_type = PBG_ERR_OP_ARG_TYPE;
	err->_line = line;
	err->_file = file;
	err->_int = 0;
	err->_data = NULL;
}

void pbg_error_free(pbg_error* err)
{
	if(err->_int != 0)
		free(err->_data);
}


/*************************
 *                       *
 * NODE CREATION TOOLKIT *
 *                       *
 *************************/

int pbg_create_op(pbg_expr* e, pbg_error* err, pbg_node_type type, int numchildren)
{
	/* Static nodes have positive indices. */
	int nodei = 1 + e->_staticsz++;
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = type;
	node->_int = 0;
	node->_data = malloc(numchildren * sizeof(int));
	if(node->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return 0;
	}
	/* Done! */
	return nodei;
}

int pbg_create_lt_key(pbg_expr* e, pbg_error* err, char* str, int n)
{
	/* Dynamic nodes have negative indices. 
	 * Subtract 1 to offset first element to -1 from 0. */
	int nodei = -(1 + e->_dynamicsz++);
	/* Initialize node! */
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = PBG_LT_KEY;
	node->_int = (n-2) * sizeof(char);
	node->_data = malloc(node->_int);
	if(node->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return 0;
	}
	memcpy(node->_data, str+1, n-2);
	// TODO ensure each key node in e->_dynamic is unique?
	/* Done! */
	return nodei;
}

int pbg_create_lt_date(pbg_expr* e, pbg_error* err, char* str, int n)
{
	/* Static nodes have positive indices. */
	int nodei = 1 + e->_staticsz++;
	/* Initialize node! */
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = PBG_LT_DATE;
	node->_int = sizeof(pbg_date_lt);
	node->_data = malloc(node->_int);
	if(node->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return 0;
	}
	pbg_todate((pbg_date_lt*)node->_data, str, n);
	/* Done! */
	return nodei;
}

int pbg_create_lt_number(pbg_expr* e, pbg_error* err, char* str, int n)
{
	/* Static nodes have positive indices. */
	int nodei = 1 + e->_staticsz++;
	/* Initialize node! */
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = PBG_LT_NUMBER;
	node->_int = sizeof(pbg_number_lt);
	node->_data = malloc(node->_int);
	if(node->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return 0;
	}
	pbg_tonumber((pbg_number_lt*)node->_data, str, n);
	/* Done! */
	return nodei;
}

int pbg_create_lt_string(pbg_expr* e, pbg_error* err, char* str, int n)
{
	/* Static nodes have positive indices. */
	int nodei = 1 + e->_staticsz++;
	/* Initialize node! */
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = PBG_LT_STRING;
	node->_int = (n-2) * sizeof(pbg_string_lt);
	node->_data = malloc(node->_int);
	if(node->_data == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return 0;
	}
	memcpy(node->_data, str+1, n-2);
	/* Done! */
	return nodei;
}

int pbg_create_lt_true(pbg_expr* e, pbg_error* err, char* str, int n)
{
	PBG_UNUSED(err); PBG_UNUSED(str); PBG_UNUSED(n);
	/* Static nodes have positive indices. */
	int nodei = 1 + e->_staticsz++;
	/* Initialize node! */
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = PBG_LT_TRUE;
	node->_int = 0;
	node->_data = NULL;
	/* Done! */
	return nodei;
}

int pbg_create_lt_false(pbg_expr* e, pbg_error* err, char* str, int n)
{
	PBG_UNUSED(err); PBG_UNUSED(str); PBG_UNUSED(n);
	/* Static nodes have positive indices. */
	int nodei = 1 + e->_staticsz++;
	/* Initialize node! */
	pbg_expr_node* node = pbg_get_node(e, nodei);
	node->_type = PBG_LT_FALSE;
	node->_int = 0;
	node->_data = NULL;
	/* Done! */
	return nodei;
}


/************************
 *                      *
 * NODE PARSING TOOLKIT *
 *                      *
 ************************/

/**
 * Checks if the operator can legally take the specified number of arguments.
 * This function encodes the rules for operator arity and should be modified if
 * a new operator is added.
 * @param type     Type of operator.
 * @param numargs  Number of arguments to operator.
 * @return 1 if the number of arguments can be legally given to the operator, 
 *         0 if not or if type does not refer to an operator.
 */
inline int pbg_check_op_arity(pbg_node_type type, int numargs)
{
	int arity = 0;
	/* Positive arity specifies "exact arity," i.e. the number of arguments 
	 * must be exact. Negative arity specifies a "minimum arity," i.e. the 
	 * minimum number of arguments needed. */
	switch(type) {
		case PBG_OP_NOT:  arity =  1; break;
		case PBG_OP_AND:  arity = -2; break;
		case PBG_OP_OR:   arity = -2; break;
		case PBG_OP_EQ:   arity = -2; break;
		case PBG_OP_LT:   arity =  2; break;
		case PBG_OP_GT:   arity =  2; break;
		case PBG_OP_EXST: arity =  1; break;
		case PBG_OP_NEQ:  arity =  2; break;
		case PBG_OP_LTE:  arity =  2; break;
		case PBG_OP_GTE:  arity =  2; break;
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
	int nodei;  /* Index of this node. This is the return value. */
	
	/* Cache length of field for easier referencing. */
	int n = **lengths;
	int fieldi = **fields;
	
	/* Update pointers for next node. */
	(*fields)++, (*lengths)++;
	
	/* Identify type of field. If the type cannot be resolve, throw an error. */
	pbg_node_type type = pbg_gettype(str + fieldi, n);
	if(type == PBG_UNKNOWN) {
		pbg_err_unknown_type(err, __LINE__, __FILE__);
		return 0;
	}
	
	/* This field is an operator. */
	if(type > PBG_MAX_LT && type < PBG_MAX_OP) {
		pbg_expr_node* node;
		
		/* Maximum number of children this node has allocated space for. */
		int maxc = 2;
		/* Initialize node and record node index. */
		nodei = pbg_create_op(e, err, type, maxc);
		node = pbg_get_node(e, nodei);
		/* Propagate error back to caller, if any. */
		if(nodei == 0) return 0;
		
		/* Recursively build subtree rooted at this operator node. pbg_evaluate 
		 * set last element in fields to -1. This is used to ensure we don't 
		 * run past the end of the expression string. */
		while(**fields != -1 && **fields < **closings) {
			/* Make recursive call to construct subtree rooted at child. */
			int childi = pbg_parse_r(e, err, str, fields, lengths, closings);
			/* Propagate error back to caller, if any. */
			if(childi == 0) return 0;
			/* Expand array of children if necessary. */
			if(node->_int == maxc) {
				maxc *= 2;  // doubling gives amortized O(1) time insertion
				int* children = (int*) realloc(node->_data, maxc * sizeof(int));
				if(children == NULL) {
					pbg_err_alloc(err, __LINE__, __FILE__);
					return 0;
				}
				node->_data = (void*) children;
			}
			/* Store index of child node. */
			((int*)node->_data)[node->_int++] = childi;
		}
		
		/* Enforce operator arity. */
		if(pbg_check_op_arity(type, node->_int) == 0) {
			pbg_err_op_arity(err, __LINE__, __FILE__, type, node->_int);
			return 0;
		}
		
		/* Tighten list of children and save it. */
		int* children = (int*) realloc(node->_data, node->_int * sizeof(int));
		if(children == NULL) {
			pbg_err_alloc(err, __LINE__, __FILE__);
			return 0;
		}
		node->_data = (void*) children;
		
		/* This node read all of its children until the next closing. 
		 * The parent node will need to read until the end of the next 
		 * next one. */
		(*closings)++;
		
	/* This field is a literal. */
	}else{
		/* Move str to correct starting position. */
		str += fieldi;
		/* Identify type of literal and initialize the node! */
		if(type == PBG_LT_KEY)         nodei = pbg_create_lt_key(e, err, str, n);
		else if(type == PBG_LT_DATE)   nodei = pbg_create_lt_date(e, err, str, n);
		else if(type == PBG_LT_NUMBER) nodei = pbg_create_lt_number(e, err, str, n);
		else if(type == PBG_LT_STRING) nodei = pbg_create_lt_string(e, err, str, n);
		else if(type == PBG_LT_TRUE)   nodei = pbg_create_lt_true(e, err, str, n);
		else if(type == PBG_LT_FALSE)  nodei = pbg_create_lt_false(e, err, str, n);
		else{
			pbg_err_unknown_type(err, __LINE__, __FILE__);
			return 0;
		}
	}
	
	/* Done! */
	return nodei;
}

void pbg_parse(pbg_expr* e, pbg_error* err, char* str, int n)
{
	// TODO verify str is an element of PBG (syntactically, not semantically)
	
	/* Set to NULL to allow for pbg_free to check if needing free. */
	e->_static = NULL;
	e->_dynamic = NULL;
	
	/* These are initialized to 0 as they are used as counters for the number 
	 * of each type of node created. In the end they should be equal to the 
	 * associated local variables here. */
	e->_staticsz = 0;
	e->_dynamicsz = 0;
	
	/* Verify that all strings & keys are bounded, that all opening parentheses
	 * have friends, and that only a single expression is present. Also count 
	 * the number of fields, keys, and closings. */
	int numfields = 0;
	int numkeys = 0;
	int numclosings = 0;
	int depth = 0, reachedend = 0;
	int instring = 0, inkey = 0;
	for(int i = 0; i < n; i++) {
		if(pbg_iswhitespace(str[i])) continue;
		/* Count number of keys. */
		if(str[i] == '[') numkeys++;
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
			}
			/* It's a key! */
			else if(str[i] == '[') {
				inkey = 1;
				do i++; while(!(str[i] == ']' && str[i-1] != '\\'));
				if(i != n) inkey = 0;
			}
			/* It's literally anything else! */
			else
				while(!pbg_iswhitespace(str[i+1]) && str[i+1] != '[' && 
						str[i+1] != '(' && str[i+1] != ')') i++;
			numfields++;
		}
	}
	/* Check if string is left unclosed. */
	if(instring) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, instring, 
				"Unclosed string.");
		return;
	}
	/* Check if key is left unclosed. */
	if(inkey) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, inkey, 
				"Unclosed key.");
		return;
	}
	/* Check if there are any parentheses at all . */
	if(numclosings == 0) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, 0,
				"Every PBG expression must be bound with parentheses.");
		return;
	}
	/* Check if opening parentheses are left unclosed. */
	if(depth != 0) {
		pbg_err_syntax(err, __LINE__, __FILE__, str, 0,
				"Unmatched opening parentheses.");
		return;
	}
	
	/* Compute sizes of static and dynamic arrays. */
	int numstatic = numfields - numkeys;
	int numdynamic = numkeys;
	
	/* Allocate space for needed arrays. */
	int* fields = (int*) malloc((numfields+1) * sizeof(int));
	if(fields == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	int* lengths = (int*) malloc(numfields * sizeof(int));
	if(lengths == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	int* closings = (int*) malloc(numclosings * sizeof(int));
	if(closings == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	
	/* Identify the indices of all fields and closings as well as lengths of 
	 * the fields. */
	for(int i = 0, c = 0, f = 0; i < n; i++) {
		/* Whitespaces are the enemy. */
		if(pbg_iswhitespace(str[i])) continue;
		/* It's a close! */
		if(str[i] == ')') {
			closings[c++] = i;
		/* It's a field! */
		}else if(str[i] != '(') {
			/* Grab index of field. */
			fields[f] = i;
			/* It's a string! */
			if(str[i] == '\'') {
				do i++; while(!(str[i] == '\'' && str[i-1] != '\\'));
			/* It's a key! */
			}else if(str[i] == '[')
				do i++; while(!(str[i] == ']' && str[i-1] != '\\'));
			/* It's literally anything else! */
			else
				while(!pbg_iswhitespace(str[i+1]) && str[i+1] != '[' && 
						str[i+1] != '(' && str[i+1] != ')') i++;
			/* Compute length of field. */
			lengths[f] = i - fields[f] + 1, f++;
		}
	}
	
	/* Needed to determine when we've reached the end of the expression string
	 * in pbg_parse_r. */
	fields[numfields] = -1;
	
	/* Allocate space for static and dynamic node arrays. */
	e->_static = (pbg_expr_node*) malloc(numstatic * sizeof(pbg_expr_node));
	if(e->_static == NULL) {
		free(fields), free(lengths), free(closings);
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	e->_dynamic = (pbg_expr_node*) malloc(numdynamic * sizeof(pbg_expr_node));
	if(e->_dynamic == NULL) {
		free(e->_static);
		free(fields), free(lengths), free(closings);
		pbg_err_alloc(err, __LINE__, __FILE__);
		return;
	}
	
	/* Recursively parse the expression string to build the expression tree. */
	int* lengths_cpy = lengths, *closings_cpy = closings, *fields_cpy = fields;
	int status = pbg_parse_r(e, err, str, &fields_cpy, &lengths_cpy, &closings_cpy);
	
	/* If an error occurred, clean up. */
	if(status == 0) pbg_free(e);
	
	/* Clean up! */
	free(fields), free(lengths), free(closings);
	
	/* Do not perform sanity checks if there's already an error. */
	if(err->_type != PBG_ERR_NONE)
		return;
	
	/* Sanity check: verify we parsed everything we expected. */
	if(e->_staticsz != numstatic || e->_dynamicsz != numdynamic) {
		pbg_err_state(err, __LINE__, __FILE__,
				"Not all fields were parsed?");
		return;
	}
}


/***************************
 *                         *
 * NODE EVALUATION TOOLKIT *
 *                         *
 ***************************/

int pbg_evaluate_op_not(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int child0 = ((int*)node->_data)[0];
	int result = pbg_evaluate_r(e, err, pbg_get_node(e, child0));
	if(result == -1) return -1;  /* Pass error through. */
	return !result;
}

int pbg_evaluate_op_and(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int size = node->_int;
	for(int i = 0; i < size; i++) {
		int childi = ((int*)node->_data)[i];
		int result = pbg_evaluate_r(e, err, pbg_get_node(e, childi));
		if(result == -1) return -1;  /* Pass error through. */
		if(result == 0)  return  0;  /* FALSE! */
	}
	return 1;  /* TRUE! */
}

int pbg_evaluate_op_or(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int size = node->_int;
	for(int i = 0; i < size; i++) {
		int childi = ((int*)node->_data)[i];
		int result = pbg_evaluate_r(e, err, pbg_get_node(e, childi));
		if(result == -1) return -1;  /* Pass error through. */
		if(result == 1)  return  1;  /* TRUE! */
	}
	return 0;  /* FALSE! */
}

int pbg_evaluate_op_exst(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	PBG_UNUSED(err);
	int child0 = ((int*)node->_data)[0];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	return c0->_type != PBG_UNKNOWN;
}

int pbg_evaluate_op_eq(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	PBG_UNUSED(err);
	/* Ensure type and size of all children are identical. */
	int size = node->_int;
	int child0 = ((int*)node->_data)[0];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	for(int i = 1; i < size; i++) {
		int childi = ((int*)node->_data)[i];
		pbg_expr_node* childni = pbg_get_node(e, childi);
		if(childni->_int != c0->_int || 
				childni->_type != c0->_type)
			return 0;  /* FALSE! */
		/* Ensure each data byte is identical. */
		if(memcmp(childni->_data, c0->_data, c0->_int) != 0)
			return 0;  /* FALSE! */
	}
	return 1;  /* TRUE! */
}

int pbg_evaluate_op_neq(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	PBG_UNUSED(err);
	int child0 = ((int*)node->_data)[0], child1 = ((int*)node->_data)[1];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	pbg_expr_node* c1 = pbg_get_node(e, child1);
	return c1->_type != c0->_type || 
			c1->_int != c0->_int || 
			memcmp(c1->_data, c0->_data, c0->_int);
}

int pbg_evaluate_op_lt(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int child0 = ((int*)node->_data)[0], child1 = ((int*)node->_data)[1];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	pbg_expr_node* c1 = pbg_get_node(e, child1);
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER) {
		return pbg_cmpnumber(c0->_data, c1->_data) < 0;
	}
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE) {
		return pbg_cmpdate(c0->_data, c1->_data) < 0;
	}
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING) {
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) < 0;
	}
	pbg_err_op_arg_type(err, __LINE__, __FILE__);
	return -1;
}

int pbg_evaluate_op_gt(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int child0 = ((int*)node->_data)[0], child1 = ((int*)node->_data)[1];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	pbg_expr_node* c1 = pbg_get_node(e, child1);
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER) {
		return pbg_cmpnumber(c0->_data, c1->_data) > 0;
	}
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE) {
		return pbg_cmpdate(c0->_data, c1->_data) > 0;
	}
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING) {
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) > 0;
	}
	pbg_err_op_arg_type(err, __LINE__, __FILE__);
	return -1;
}

int pbg_evaluate_op_lte(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int child0 = ((int*)node->_data)[0], child1 = ((int*)node->_data)[1];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	pbg_expr_node* c1 = pbg_get_node(e, child1);
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER) {
		return pbg_cmpnumber(c0->_data, c1->_data) <= 0;
	}
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE) {
		return pbg_cmpdate(c0->_data, c1->_data) <= 0;
	}
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING) {
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) <= 0;
	}
	pbg_err_op_arg_type(err, __LINE__, __FILE__);
	return -1;
}

int pbg_evaluate_op_gte(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	int child0 = ((int*)node->_data)[0], child1 = ((int*)node->_data)[1];
	pbg_expr_node* c0 = pbg_get_node(e, child0);
	pbg_expr_node* c1 = pbg_get_node(e, child1);
	/* Both are NUMBERs. */
	if(c0->_type == PBG_LT_NUMBER &&
			c1->_type == PBG_LT_NUMBER) {
		return pbg_cmpnumber(c0->_data, c1->_data) >= 0;
	}
	/* Both are DATEs. */
	if(c0->_type == PBG_LT_DATE &&
			c1->_type == PBG_LT_DATE) {
		return pbg_cmpdate(c0->_data, c1->_data) >= 0;
	}
	/* Both are STRINGs. */
	if(c0->_type == PBG_LT_STRING &&
			c1->_type == PBG_LT_STRING) {
		return pbg_cmpstring(c0->_data, c1->_data, c0->_int) >= 0;
	}
	pbg_err_op_arg_type(err, __LINE__, __FILE__);
	return -1;
}

/**
 * TODO
 */
int pbg_evaluate_r(pbg_expr* e, pbg_error* err, pbg_expr_node* node)
{
	/* This is a literal node! */
	if(node->_type < PBG_MAX_LT) {
		if(node->_type == PBG_LT_TRUE)  return 1;
		if(node->_type == PBG_LT_FALSE) return 0;
		
		/* TRUE and FALSE are the only literals with truth values. 
		 * If we get here, there's a bug, and we need to throw an error. */
		pbg_err_state(err, __LINE__, __FILE__, 
				"Cannot evaluate a literal without a truth value.");
		return -1;
		
	/* This is an operator node! */
	}else {
		switch(node->_type) {
			case PBG_OP_NOT:  return pbg_evaluate_op_not(e, err, node);
			case PBG_OP_AND:  return pbg_evaluate_op_and(e, err, node);
			case PBG_OP_OR:   return pbg_evaluate_op_or(e, err, node);
			case PBG_OP_EXST: return pbg_evaluate_op_exst(e, err, node);
			case PBG_OP_EQ:   return pbg_evaluate_op_eq(e, err, node);
			case PBG_OP_NEQ:  return pbg_evaluate_op_neq(e, err, node);
			case PBG_OP_LT:   return pbg_evaluate_op_lt(e, err, node);
			case PBG_OP_GT:   return pbg_evaluate_op_gt(e, err, node);
			case PBG_OP_LTE:  return pbg_evaluate_op_lte(e, err, node);
			case PBG_OP_GTE:  return pbg_evaluate_op_gte(e, err, node);
			default:
				pbg_err_unknown_type(err, __LINE__, __FILE__);
				return -1;
		}
	}
	
	
}

int pbg_evaluate(pbg_expr* e, pbg_error* err, pbg_expr_node (*dict)(char*, int))
{
	/* KEY resolution. Lookup every key in provided dictionary. */
	pbg_expr_node* dynamics;
	dynamics = (pbg_expr_node*) malloc(e->_dynamicsz * sizeof(pbg_expr_node));
	if(dynamics == NULL) {
		pbg_err_alloc(err, __LINE__, __FILE__);
		return -1;
	}
	for(int i = 0; i < e->_dynamicsz; i++) {
		pbg_expr_node* keylt = e->_dynamic+i;
		dynamics[i] = dict((char*)(keylt->_data), keylt->_int);
	}
	
	/* Swap out KEY literals with dictionary equivalents. */
	pbg_expr_node* old = e->_dynamic;
	e->_dynamic = dynamics;
	
	/* Evaluate expression! */
	int result = pbg_evaluate_r(e, err, e->_static);
	
	/* Restore old KEY literal array. */
	e->_dynamic = old;
	
	/* Clean up malloc'd memory. */
	for(int i = 0; i < e->_dynamicsz; i++)
		pbg_free_node(dynamics+i);
	free(dynamics);
	
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
	/* Free individual static nodes. Some do not have _data malloc'd. */
	for(int i = e->_staticsz-1; i >= 0; i--)
		pbg_free_node(e->_static+i);
	
	/* Free individual dynamic nodes. All have _data malloc'd. */
	for(int i = 0; i < e->_dynamicsz; i++)
		pbg_free_node(e->_dynamic+i);
	
	/* Free internal node arrays. */
	if(e->_static != NULL) free(e->_static);
	if(e->_static != NULL) free(e->_dynamic);
}


/*********************************
 *                               *
 * CONVERSION & CHECKING TOOLKIT *
 *                               *
 *********************************/
 
/**
 * Translates the given pbg_node_type to a human-readable string.
 * @param type  PBG node type to translate.
 * @return String representation of the given node type.
 */
char* pbg_type_str(pbg_node_type type)
{
	switch(type) {
		case PBG_LT_TRUE: return "PBG_LT_TRUE";
		case PBG_LT_FALSE: return "PBG_LT_FALSE";
		case PBG_LT_NUMBER: return "PBG_LT_NUMBER";
		case PBG_LT_STRING: return "PBG_LT_STRING";
		case PBG_LT_DATE: return "PBG_LT_DATE";
		case PBG_LT_KEY: return "PBG_LT_KEY";
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
		default: return "PBG_UNKNOWN";
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

pbg_node_type pbg_gettype(char* str, int n)
{
	/* Is it a literal? */
	if(pbg_istrue(str, n))   return PBG_LT_TRUE;
	if(pbg_isfalse(str, n))  return PBG_LT_FALSE;
	if(pbg_isnumber(str, n)) return PBG_LT_NUMBER;
	if(pbg_isstring(str, n)) return PBG_LT_STRING;
	if(pbg_isdate(str, n))   return PBG_LT_DATE;
	if(pbg_iskey(str, n))    return PBG_LT_KEY;
	
	/* Is it an operator? */
	if(n == 1) {
		if(str[0] == '!') return PBG_OP_NOT;
		if(str[0] == '&') return PBG_OP_AND;
		if(str[0] == '|') return PBG_OP_OR;
		if(str[0] == '=') return PBG_OP_EQ;
		if(str[0] == '<') return PBG_OP_LT;
		if(str[0] == '>') return PBG_OP_GT;
		if(str[0] == '?') return PBG_OP_EXST;
	}
	if(n == 2) {
		if(str[0] == '!' && str[1] == '=') return PBG_OP_NEQ;
		if(str[0] == '<' && str[1] == '=') return PBG_OP_LTE;
		if(str[0] == '>' && str[1] == '=') return PBG_OP_GTE;
	}
	
	/* It isn't anything! */
	return PBG_UNKNOWN;
}

int pbg_istrue(char* str, int n)
{
	return n == 4 && 
		str[0]=='T' && str[1]=='R' && str[2]=='U' && str[3]=='E';
}

int pbg_isfalse(char* str, int n)
{
	return n == 5 && 
		str[0]=='F' && str[1]=='A' && str[2]=='L' && str[3]=='S' && str[4]=='E';
}

int pbg_isnumber(char* str, int n)
{
	int i = 0;
	
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

void pbg_tonumber(pbg_number_lt* ptr, char* str, int n)
{
	PBG_UNUSED(n);
	ptr->_val = atof(str);
}

int pbg_cmpnumber(pbg_number_lt* n1, pbg_number_lt* n2)
{
	if(n1->_val < n2->_val) return -1;
	if(n1->_val > n2->_val) return 1;
	return 0;
}

int pbg_cmpdate(pbg_date_lt* n1, pbg_date_lt* n2)
{
	if(n1->_YYYY < n2->_YYYY) return -1;
	if(n1->_YYYY > n2->_YYYY) return 1;
	if(n1->_MM < n2->_MM) return -1;
	if(n1->_MM > n2->_MM) return 1;
	if(n1->_DD < n2->_DD) return -1;
	if(n1->_DD > n2->_DD) return 1;
	return 0;
}

int pbg_cmpstring(pbg_string_lt* s1, pbg_string_lt* s2, int n)
{
	return strncmp(s1, s2, n);
}

int pbg_iskey(char* str, int n)
{
	return str[0] == '[' && str[n-1] == ']';
}

int pbg_isstring(char* str, int n)
{
	// TODO ensure I don't contain any unescaped single quotes!
	return str[0] == '\'' && str[n-1] == '\'';
}

int pbg_isdate(char* str, int n)
{
	return n == 10 && 
		pbg_isdigit(str[0]) && pbg_isdigit(str[1]) && 
		pbg_isdigit(str[2]) && pbg_isdigit(str[3]) &&
		str[4] == '-' && 
		pbg_isdigit(str[5]) && pbg_isdigit(str[6]) && 
		str[7] == '-' && 
		pbg_isdigit(str[8]) && pbg_isdigit(str[9]);
}

void pbg_todate(pbg_date_lt* ptr, char* str, int n)
{
	if(n != 10) return;
	ptr->_YYYY = (str[0]-'0')*1000 + (str[1]-'0')*100 + (str[2]-'0')*10 + (str[3]-'0');
	ptr->_MM = (str[5]-'0')*10 + (str[6]-'0');
	ptr->_DD = (str[8]-'0')*10 + (str[9]-'0');
	// TODO enforce ranges on months and days
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
inline int pbg_isdigit(char c) { return c >= '0' && c <= '9'; }

/**
 * Checks if the given character is whitespace.
 * @param c  Character to check.
 */
inline int pbg_iswhitespace(char c) { return c==' ' || c=='\t' || c=='\n'; }