#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, 
	   LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR};
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };


typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval {
	int type;
	long num;

	//error and symbol types have string data
	char* err;
	char* sym;

	lbuiltin fun;
	//count and pointer to a list of lval*
	int count;
	struct lval** cell;
};

struct lenv {
	int count;
	char** syms;
	lval** vals;
};


lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
lval* lval_num(long x);
lval* lval_err(char* m);
lval* lval_sym(char* s);
lval* lval_fun(lbuiltin func);
lval* lval_sexpr(void); 
lval* lval_qexpr(void); 
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_add(lval* v, lval* x);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);
lval* lval_copy(lval* v);
lval* builtin(lval* a, char* func);
lval* builtin_op(lval* a, char* op);
lval* builtin_head(lval* a);
lval* builtin_tail(lval* a);
lval* builtin_list(lval* a);
lval* builtin_eval(lval* a);
lval* builtin_join(lval* a);
lval* lval_join(lval* x, lval* y);
void lval_expr_print(lval* v, char open, char close);
void lval_print(lval* l);
void lval_println(lval* v);
void lval_del(lval* v);

lenv* lenv_new(void);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_del(lenv* e);

int main(int argc, char** argv) {
	mpc_parser_t* Number    = mpc_new("number");
	mpc_parser_t* Symbol    = mpc_new("symbol");
	mpc_parser_t* Sexpr      = mpc_new("sexpr");
	mpc_parser_t* Qexpr      = mpc_new("qexpr");
	mpc_parser_t* Expr      = mpc_new("expr");
	mpc_parser_t* Lispy     = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT,
	  "\
	  number   : /-?[0-9]+/ ;            \
	  symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;  \
	  | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" ; \
	  sexpr    : '(' <expr>* ')' ;       \
	  qexpr    : '{' <expr>* '}' ;       \
	  expr     : <number> | <symbol> | <sexpr> | <qexpr> ; \
	  lispy    : /^/ <expr>* /$/ ; \
	  ",
	  Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


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
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
			//lval result = eval(r.output);
			//lval_println(result);
		 	mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	return 0;
}



lval* lval_eval_sexpr(lval* v) {
	//evaluate children
	for (int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	//Error checking
	for (int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	//Empty expression
	if (v->count == 0) { return 0; }

	//single expression
	if (v->count == 1) { 
		return lval_take(v, 0);
	}

	//ensure first element is a function after evaluation
	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_FUN) {
		lval_del(f);
		lval_del(v);
		return lval_err("First element is not a function");
	}

	//call function to get result
	lval* result = f->fun(e, v);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v) {

	if (v->type == LVAL_SYM) {
		lval* x = lenv_get(e, v);
		lval_del(v);
		return x;
	}

	//evaluate Sexpressions
	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	}
	return v;
}

//construct a pointer to a new number lval
lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_fun(lbuiltin func) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->fun = func;
	return v;
}

//construct a pointer to a new error lval
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

//construct a pointer to a new symbol lval
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

//a pointer to a new empty Sexpr lval
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

//a pointer to a new empty Qexpr lval
lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
	//if symbol or number return our token converted to that type
	if (strstr(t->tag, "number")) { return lval_read_num(t); }
	if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	//if root (>) or sexpr then create empty list
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
	if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

	//fill this list with any valid expression contained within
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_pop(lval* v, int i) {
	//find item at i
	lval* x = v->cell[i];

	//shift memory after the item at i
	memmove(&v->cell[i], &v->cell[i+1],
			sizeof(lval*) * (v->count-i-1));

	//decrease the count of items in the list
	v->count--;

	//reallocate the memory used
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

lval* builtin(lval* a, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(a); }
	if (strcmp("head", func) == 0) { return builtin_head(a); }
	if (strcmp("tail", func) == 0) { return builtin_tail(a); }
	if (strcmp("join", func) == 0) { return builtin_join(a); }
	if (strcmp("eval", func) == 0) { return builtin_eval(a); }
	if (strcmp("+-/*", func)) { return builtin_op(a, func); }
	lval_del(a);
	return lval_err("Unknown Function!");
}

lval* builtin_op(lval* a, char* op) {
	//ensure all arguments are numbers
	for (int i = 0; i < a->count; i++) {
		 if (a->cell[i]->type != LVAL_NUM) {
			 lval_del(a);
			 return lval_err("Cannon operate on non-number");
		 }
	}

	//pop first element
	lval* x = lval_pop(a, 0);

	//if no arguments and sub, then perform unary negation
	if ((strcmp(op, "-") == 0) && a->count == 0) {
		x->num = -x->num;
	}

	//while there are still elements remaining
	while (a->count > 0) {
		//pop the next element
		lval* y = lval_pop(a, 0);

		if (strcmp(op, "+") == 0) { x->num += y->num; }
		if (strcmp(op, "-") == 0) { x->num -= y->num; }
		if (strcmp(op, "*") == 0) { x->num *= y->num; }
		if (strcmp(op, "/") == 0) {
			if (y->num == 0) {
				lval_del(x);
				lval_del(y);
				x = lval_err("Division By Zero!");
				break;
			}
			x->num /= y->num;
		}
		lval_del(y);
	}

	lval_del(a);
	return x;
}

lval* builtin_head(lval* a) {
	//check error conditions
	if (a->count != 1) {
		return lval_err("Function 'head' passed too many arguments!");
	}
	if (a->cell[0]->type != LVAL_QEXPR) {
		return lval_err("Function 'head' passed incorrect types!");
	}
	if (a->cell[0]->count == 0) {
		return lval_err("Function 'head' passed {}!");
	}

	//otherwise take first argument
	lval* v = lval_take(a, 0);

	//delete all elements that are not head and return
	while (v->count > 1) {
		lval_del(lval_pop(v, 1));
	}
	return v;
}

lval* builtin_tail(lval* a) {
	//check error conditions
	if (a->count != 1) {
		return lval_err("Function 'tail' passed too many arguments!");
	}
	if (a->cell[0]->type != LVAL_QEXPR) {
		return lval_err("Function 'tail' passed incorrect types!");
	}
	if (a->cell[0]->count == 0) {
		return lval_err("Function 'tail' passed {}!");
	}

	//otherwise take first argument
	lval* v = lval_take(a, 0);

	//delete all elements that are not head and return
	lval_del(lval_pop(v, 0));
	return v;
}


lval* builtin_list(lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lval* a) {
	 if (a->count != 1) {
		 return lval_err("Function 'eval' passed too many arguments!");
	 }

	 if (a->cell[0]->type != LVAL_QEXPR) {
		 return lval_err("Function 'eval' passed incorrect type!");
	 }

	 lval* x = lval_take(a, 0);
	 x->type = LVAL_SEXPR;
	 return lval_eval(x);
}

lval* builtin_join(lval* a) {
	for (int i = 0; i < a->count; i++) {
		if(a->cell[i]->type != LVAL_QEXPR) {
			return lval_err("Function 'join' passed incorrect type.");
		}
	}

	lval* x = lval_pop(a, 0);

	while (a->count) {
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_del(a);
	return x;
}

lval* lval_join(lval* x, lval* y) {
	//for each cell in 'y' join it with 'x'
	while (y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	//delete the empty list 'y' and return 'x'
	lval_del(y);
	return x;
}

void lval_print(lval* v) {
	 switch (v->type) {
		 case LVAL_NUM: 
			 printf("%li", v->num);
			 break;
		 case LVAL_ERR:
			 printf("Error: %s", v->err);
			 break;
		 case LVAL_SYM:
			 printf("%s", v->sym);
			 break;
		 case LVAL_FUN: 
			 printf("<function>");
			 break;
		 case LVAL_SEXPR:
			 lval_expr_print(v, '(', ')');
			 break;
		 case LVAL_QEXPR:
			 lval_expr_print(v, '{', '}');
			 break;
	 }
}

void lval_println(lval* v) {
	 lval_print(v);
	 putchar('\n');
}

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; i++) {
		//print value containined in this cell
		lval_print(v->cell[i]);

		//don't print trailing spaces
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

lval* lval_copy(lval* v) {
	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch (v->type) {
		//copy functions and numbers directly
		case LVAL_FUN:
			x->fun = v->fun;
			break;
		case LVAL_NUM:
			x->num = v->num;
			break;

		//copy strings using malloc and strcpy
		case LVAL_ERR:
			x->err = malloc(strlen(v->err) + 1);
			strcpy(x->err, v->err);
			break;
		case LVAL_SYM:
			x->sym = malloc(strlen(v->sym) + 1);
			strcpy(x->sym, v->sym);
			break;

		//copy lists by copying each sub-expression
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->count = v->count;
			x->cell = malloc(sizeof(lval*) * x->count);
			for (int i = 0; i < x->count; i++) {
				x->cell[i] = lval_copy(v->cell[i]);
			}
			break;
	}
	return x;
}

//cleanup
void lval_del(lval* v) {
	switch (v->type) {
		case LVAL_NUM: break;

		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		case LVAL_FUN: break;

		//if qexpr or sexpr then delete all elements in the expression
		case LVAL_QEXPR:
		case LVAL_SEXPR:
		   for (int i = 0; i < v->count; i++) {
			   lval_del(v->cell[i]);
		   }
		   free(v->cell);
		   break;
	}

	free(v);
}

lenv* lenv_new(void) {
	lenv* e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;
	return e;
}

lval* lenv_get(lenv* e, lval* k) {
	//iterate over all items in environment
	for (int i = 0; i < e->count; i++) {
		//return a copy of value, if the symbol matches a stored string
		if(strcmp(e->syms[i], k->sym) == 0) {
			return lval_copy(e->vals[i]);
		}
	}
	//if no symbol is found, return error
	return lval_err("Unbound symbol!");
}

void lenv_put(lenv* e, lval* k, lval* v) {
	//iterate over all items in environment
	for (int i = 0; i< e->count; i++) {
		//if variable is found delete item at position
		//and replace with new variable
		if (strcmp(e->syms[i], k->sym) == 0) {
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	//if variable does not already exist, make space for a new variable
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);

	//copy contents of lval and symbol string into new location
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym)+1);
	strcpy(e->syms[e->count-1], k->sym);
}

void lenv_del(lenv* e) {
	for (int i = 0; i< e->count; i++) {
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}
