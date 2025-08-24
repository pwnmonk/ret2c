#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/limits.h>

#define JSH_RL_BUFSZ 1024
#define JSH_TOK_BUFSZ 512
#define JSH_TOK_DELIM " \t\r\n\a"

//
// Fucntion declarations for builtin shell commands
// 
int jsh_cd(char **args);
int jsh_help(char **args);
int jsh_exit(char **args);

char *jsh_getcwd(void);

//
// List of builtin commands. Followed by corresponding functions
//
char *builtin_str[] = {
	"cd",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
	&jsh_cd,
	&jsh_help,
	&jsh_exit
};

int jsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

//
// Builtin function implementations
//
int jsh_cd(char **args) {
	if (args[1] == NULL) {
		fprintf(stderr, "jsh:expected argument to 'cd'\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("jsh");
		}
	}
	return 1;
}

int jsh_help(char **args) {
	int i;
	printf("JSH\n");
	printf("Type program names and arguments, and hit enter\n");
	printf("The following are builtin: \n");

	for (i = 0; i < jsh_num_builtins(); i++) {
		printf("\t%s\n", builtin_str[i]);
	}

	printf("Use the man command for info on other programs\n");
	return 1;
}

int jsh_exit(char **args) {
	return 0;
}

// Function for getting cwd as string (for prompt):
char *jsh_getcwd(void) {
    static char cwd[PATH_MAX];  // Static so it persists after function returns
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return cwd;
    } else {
        return ":(";  // Fallback if getcwd fails
    }
}

int jsh_launch(char **args) {
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid == 0) {
		// child process
		if (execvp(args[0], args) == -1) {
			perror("jsh");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// error forking
		perror("jsh");
	} else {
		// parent process
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int jsh_execute(char **args) {
	int i;

	if (args[0] == NULL) {
		// no command was entered
		return 1;
	}

	for (i = 0; i < jsh_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}
	return jsh_launch(args);
}

char *jsh_read_line(void) {
	int bufsize = JSH_RL_BUFSZ;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "jsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character
		c = getchar();

		// If we hit EOF, replace it with a null character and return.
		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If we have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += JSH_RL_BUFSZ;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "jsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

char **jsh_split_line(char *line) {
	int bufsz = JSH_TOK_BUFSZ, position = 0;
	char **tokens = malloc(bufsz * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, "jsh: allocation error!\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, JSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsz) {
			bufsz += JSH_TOK_BUFSZ;
			tokens = realloc(tokens, bufsz * sizeof(char*));
			
		if (!tokens) {
			fprintf(stderr, "jsh: allocation error!\n");
			exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, JSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

void jsh_loop(void) {
	char *line;
	char **args;
	int status;
	
	do {
		printf("[%s] ", jsh_getcwd());
		line = jsh_read_line();
		args = jsh_split_line(line);
		status = jsh_execute(args);
		
		free(line);
		free(args);
	} while (status);
}

int main(int argc, char **argv)
{
	jsh_loop();
	return EXIT_SUCCESS;
}
