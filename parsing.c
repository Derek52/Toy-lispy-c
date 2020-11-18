#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

long eval(mpc_ast_t* t);
long eval_op(long x, char* op, long y);

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
			 long result = eval(r.output);
	 		printf("%li\n", result);
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

long eval(mpc_ast_t* t) {

	//If the node is a number, return the number
	if (strstr(t->tag, "number")) {
		return atoi(t->contents);
	}

	//the operator is always the node's 2nd child
	char* op = t->children[1]->contents;

	//store the third child in 'x'
	long x = eval(t->children[2]);

	//iterate over the remaining children
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	return x;
}

//use operator string to determine what operation to perform
long eval_op(long x, char* op, long y) {
	if (strcmp(op, "+") == 0) { return x + y; }
	if (strcmp(op, "-") == 0) { return x - y; }
	if (strcmp(op, "/") == 0) { return x / y; }
	if (strcmp(op, "*") == 0) { return x * y; }
	return 0;
}






