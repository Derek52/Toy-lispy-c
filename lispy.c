#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

static char input[2048];

int main(int argc, char** argv) {
	puts("Lispy Version 0.0.0.0.1");
	puts("Press Ctrl+c to Exit\n");

	while (1) {
		//read a line of user input
		char* input = readline("lispy> ");

		//add input to history
		add_history(input);

		//repeat input back to user
		printf("No you're a %s\n", input);;

		free(input);
	}

	return 0;
}
