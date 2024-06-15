#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define MAX_TOKENS 20

char* SHNAME = "mischell";

char* tokens[MAX_TOKENS];

int parse(int t){
    bool in = false;
    bool out = false;
    char* input = NULL;
    char* output = NULL;
    int bg = 0;
    if(*tokens[t] == '&'){
        bg = 1;
        t--;
    }
    if(*tokens[t] == '>'){
        out = true;
        output = (char*)calloc(256, sizeof(char));
        strcpy(output, ++tokens[t]);
        t--;
    }
    if(*tokens[t] == '<'){
        in = true;
        input = (char*)calloc(256, sizeof(char));
        strcpy(input, ++tokens[t]);
        t--;
    }
    if(in)
        printf("Input redirect: '%s'\n", input);
    if(out)
        printf("Output redirect: '%s'\n", output);
    if(bg)
        printf("Background: %d\n", bg);

    free(input);
    free(output);

    return t;
}

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
    
    return --ti;
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

        int args = parse(t);

        free(line);
    
    }
    

    return 0;
}