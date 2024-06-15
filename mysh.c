#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// DEFINES AND TYPEDEFS
#define MAX_TOKENS 20

typedef void (*cmd_operation)(int);

typedef struct cmd_
{
    char* name;
    cmd_operation operation;
    char* description;
} cmd;

// GLOBAL VARIABLES
char SHNAME[9] = "mysh";
int exit_status = 0;
int debug_level = 0;
char* tokens[MAX_TOKENS];
int bg = 0;

// BUILT-IN COMMAND FUNCTIONS
void debug(int args)
{
    if(args == 0)
    {
        printf("%d\n", debug_level);
        return;
    }
    else
    {
        debug_level = atoi(tokens[1]);
    }
    exit_status = 0;
}

void prompt(int args)
{
    if(args == 0)
    {
        printf("%s\n", SHNAME);
        exit_status = 0;
    }
    else if(args == 1)
    {
        int plen = strlen(tokens[1]);
        if(plen > 8)
        {
            exit_status = 1;
            return;
        }
        else
        {
            strcpy(SHNAME, tokens[1]);
            exit_status = 0;
        }
    }
    else
    {
        exit_status = 1;
        return;
    }
}

void status(int args)
{
    if(args > 0)
    {
        exit_status = 1;
        return;
    }
    printf("%d\n", exit_status);
}

void exit_cmd(int args)
{
    if(args == 0)
    {
        exit(exit_status);
    }
    int exit_with = atoi(tokens[1]);
    exit(exit_with);
}

void help(int args)
{
    if(args > 0)
    {
        exit_status = 1;
        return;
    }
    printf("help\n");
}

// BUILT-IN COMMANDS ARRAY
cmd builtin_commands[] =
{
    {"debug", &debug, "debug opis"},
    {"prompt", &prompt, "prompt opis"},
    {"status", &status, "status opis"},
    {"exit", &exit_cmd, "exit opis"},
    {"help", &help, "help opis"}
};

// COMMAND EVAL FUNCTIONS
void info_print(char* line, int t, char* input, char* output)
{
    printf("Input line: '%s'\n", line);
    for(int i = 0; i <= t; i++)
        printf("Token %d: '%s'\n", i, tokens[i]);
    if(input != NULL)
        printf("Input redirect: '%s'\n", input);
    if(output != NULL)
        printf("Output redirect: '%s'\n", output);
}

void execute_builtin(char* line, int t, int args, int ix, char* input, char* output)
{
    if(debug_level > 0)
    {
        info_print(line, t, input, output);
        printf("Executing builtin '%s' in %s\n", tokens[0], (bg) ? "background" : "foreground");
    }
    builtin_commands[ix].operation(args);
}

void execute_external(char* line, int t, int args, char* input, char* output)
{
    info_print(line, t, input, output);
    printf("External command '%s", tokens[0]);
    for(int i = 1; i <= args; i++)
    {
        printf(" %s", tokens[i]);
    }
    printf("'\n");
}

void find_builtin(char* line, int t, int args, char* input, char* output)
{
    for(int i = 0; i < 5; i++)
    {
        if(strcmp(builtin_commands[i].name, tokens[0]) == 0)
        {
            execute_builtin(line, t, args, i, input, output);
            return;
        }
    }
    execute_external(line, t, args, input, output);
}

// LINE TOKENIZATION
int tokenize(char *line, int size)
{
    int ti = 0;
    bool word = false;
    bool niz = false;
    bool whitespace = false;
    line[size - 1] = '\0';
    for(int i = 0; i < size; i++)
    {
        char c = line[i];

        if(c == '"')
            niz = !niz;
        if(!niz){
            if(!word && c == '#')
            {
                line[i] = '\0';
                break;
            }
            if(word && (c == ' ' || c == '"'))
            {
                word = false;
                line[i] = '\0';
            }
        }
        if(!word && c != ' ' && c != '\n' && c != '\0' && c != '"')
        {
            word = true;
            tokens[ti++] = &line[i];
        }

    }
    return --ti;
}

// MAIN
int main()
{
    int mode = isatty(STDIN_FILENO);

    while(1)
    {
        size_t line_size = 0;
        char *line = NULL;
        bool blank = true;

        char* input = NULL;
        char* output = NULL;
        bg = 0;

        // INTERACTIVE MODE
        if(mode)
            printf(">%s ", SHNAME);
        int line_read = getline(&line, &line_size, stdin);

        char* lline = (char*)calloc(strlen(line) + 1, sizeof(char));
        strcpy(lline, line);
        lline[line_read - 1] = '\0';

        for(int i = 0; i < line_read; i++)
        {
            if(!isspace(line[i])){
                blank = false;
                break;
            }
        }
        if(line_read <= 0 || blank)
            break;

        int t = tokenize(line, line_read);
        int args = t;

        // PARSING
        if(*tokens[args] == '&')
        {
            bg = 1;
            args--;
        }
        if(*tokens[args] == '>')
        {
            output = (char*)calloc(strlen(tokens[args]) + 1, sizeof(char));
            char* ptr = tokens[args];
            strcpy(output, ++ptr);
            args--;
        }
        if(*tokens[args] == '<')
        {
            input = (char*)calloc(strlen(tokens[args]) + 1, sizeof(char));
            char* ptr = tokens[args];
            strcpy(input, ++ptr);
            args--;
        }

        find_builtin(lline, t, args, input, output);

        free(input);
        free(output);
        free(line);
        free(lline);

    }

    return 0;
}
