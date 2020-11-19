#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

typedef struct {
	int type;
	long num;
	int err;
} lval;

lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);
lval lval_num(long x);
lval lval_err(int x);
void lval_print(lval l);
void lval_println(lval v);


enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

int main(int argc, char** argv) {
	mpc_parser_t* Number    = mpc_new("number");
	mpc_parser_t* Operator  = mpc_new("operator");
	mpc_parser_t* Expr      = mpc_new("expr");
	mpc_parser_t* Lispy     = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT,
	  "\
	  number   : /-?[0-9]+/ ;            \
	  operator : '+' | '-' | '*' | '/' ; \
	  expr     : <number> | '(' <operator> <expr>+ ')' ; \
	  lispy    : /^/ <operator> <expr>+ /$/ ; \
	  ",
	  Number, Operator, Expr, Lispy);


	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	while (1) {
		//read a line of user input
		char* input = readline("lispy> ");

		//add input to history
		add_history(input);

		//repeat input back to user
		mpc_result_t r;

		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_println(result);
		 	mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;
}

lval eval(mpc_ast_t* t) {

	//If the node is a number, return the number
	if (strstr(t->tag, "number")) {
		//check if there was an error in the conversion
		errno = 0;//errno is a c preprocessor macro. 
		long x = strtol(t->contents, NULL, 10);
		if (errno != ERANGE) {
			return lval_num(x);
		} else {
			return lval_err(LERR_BAD_NUM);
		}
	}

	//the operator is always the node's 2nd child
	char* op = t->children[1]->contents;
	//store the third child in 'x'
	lval x = eval(t->children[2]);

	//iterate over the remaining children
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	return x;
}

//use operator string to determine what operation to perform
lval eval_op(lval x, char* op, lval y) {
	//if either value is an error, just return it
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	//do math if both values are actually numbers
	if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "/") == 0) { 
		//if the divisor is 0, return the div by zero error
		if (y.num == 0) {
			return lval_err(LERR_DIV_ZERO);
		} else {
			 return lval_num(x.num / y.num); 
		}
	}

	return lval_err(LERR_BAD_OP);
}

lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}


void lval_print(lval v) {
	 switch (v.type) {
		 case LVAL_NUM: 
			 printf("%li", v.num);
			 break;
		 case LVAL_ERR:
			 if (v.err == LERR_DIV_ZERO) {
				 printf("Error: Division By Zero!");
			 }
			 if (v.err == LERR_BAD_OP) {
				 printf("Error: Invalid Operation!");
			 }
			 if (v.err == LERR_BAD_NUM) {
				 printf("Error: Invalid Number!");
			 }
			 break;
	 }
}

void lval_println(lval v) {
	 lval_print(v);
	 putchar('\n');
}
