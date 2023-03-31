#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

int toysh_cd(char **args);
int toysh_help(char **args);
int toysh_exit(char **args);
int toysh_ls(char **args);
int toysh_echo(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "ls",
    "echo"
};

int (*builtin_func[]) (char **) = {
    &toysh_cd,
    &toysh_help,
    &toysh_exit,
    &toysh_ls,
    &toysh_echo
};

int toysh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

int toysh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "toysh: expected argument to \"cd\"\n");
    }
    else {
        if (chdir(args[1]) != 0) {
            perror("toysh");
        }
    }

    return 1;
}

int toysh_help(char **args) {
    int i;

    printf("toysh - Toy Shell\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in commands: \n");

    for (i = 0; i < toysh_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");

    return 1;
}

int toysh_exit(char **args) {
    return 0;
}

int toysh_ls(char **args) {
    DIR *dir;
    struct dirent *entry;
    char *path;

    if (args[1] == NULL) {
        path = ".";
    }
    else {
        path = args[1];
    }
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("toysh");
    }
    else {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue;
            }
            printf("%s    ", entry->d_name);
        }
        printf("\n");

        closedir(dir);
    }

    return 1;
}

int toysh_echo(char **args) {
    char **p;
    
    for (p = args + 1; *p != NULL; p++) {
        printf("%s ", *p);
    }
    printf("\n");

    return 1;
}

int toysh_launch(char **args) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("toysh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        // Error forking
        perror("toysh");
    }
    else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int toysh_execute(char **args) {
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < toysh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return toysh_launch(args);
}

#define TOYSH_RL_BUFSIZE 1024
char *toysh_read_line(void) {
    int bufsize = TOYSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "toysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        }
        else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += TOYSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "toysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define TOYSH_TOK_BUFSIZE 64
#define TOYSH_TOK_DELIM " \t\r\n\a"
char **toysh_split_line(char *line) {
    int bufsize = TOYSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(sizeof(char *) * bufsize);
    char *token;

    if (!tokens) {
        fprintf(stderr, "toysh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOYSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOYSH_TOK_BUFSIZE;
            tokens = realloc(tokens, sizeof(char *) * bufsize);
            if (!tokens) {
                fprintf(stderr, "toysh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOYSH_TOK_DELIM);
    }
    tokens[position] = NULL;

    return tokens;
}

void toysh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = toysh_read_line();
        args = toysh_split_line(line);
        status = toysh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {
    // Load config files, if any.

    // Run command loop.
    toysh_loop();

    // Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}