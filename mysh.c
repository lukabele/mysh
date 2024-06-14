#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_TOKENS 20

char* SHNAME = "mischell";

char* tokens[MAX_TOKENS];

int tokenize(char *line, int size)
{
    int ti = 0;
    bool word = false;
    bool niz = false;
    bool whitespace = false;
    line[size - 1] = '\0';
    printf("Input line: '%s'\n", line);
    for(int i = 0; i < size; i++)
    {
        char c = line[i];
        
        
        if(c == '"')
            niz = !niz;
        if(!niz){
            if(!word && c == '#'){
                line[i] = '\0';
                break;
            }
            if(word && (c == ' ' || c == '"')){
                word = false;
                line[i] = '\0';
            }
        }
        if(!word && c != ' ' && c != '\n' && c != '\0' && c != '"'){
            word = true;
            tokens[ti++] = &line[i];
        }
        
    }
    
    for(int i = 0; i < ti; i++){
        printf("Token %d: '%s'\n", i, tokens[i]);
    }
    
    return ti;
}

int main()
{

    int mode = isatty(STDIN_FILENO);
    
    while(1)
    {
        size_t line_size = 0;
        char *line = NULL;
        if(mode)
            printf(">%s ", SHNAME);
        int line_read = getline(&line, &line_size, stdin);
        if(line_read < 0)
            break;
        int t = tokenize(line, line_read);
        free(line);
    }

    return 0;
}