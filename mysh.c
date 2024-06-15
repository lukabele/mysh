    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <stdbool.h>
    #include <string.h>
    #include <ctype.h>
    #include <errno.h>
    #include <dirent.h>
    #include <sys/stat.h>

    //DEFINES AND TYPEDEFS
    #define MAX_TOKENS 20

    typedef void (*cmd_operation)(int);

    typedef struct cmd_
    {
        char* name;
        cmd_operation operation;
        char* description;
    } cmd;

    //GLOBAL VARIABLES
    char SHNAME[9] = "mysh";
    int exit_status = 0;
    int debug_level = 0;
    char* tokens[MAX_TOKENS];
    int bg = 0;
    int args = 0;

    //BUILT-IN COMMAND FUNCIONS
    void debug()
    {
        if(args == 0)
        {
            printf("%d\n", debug_level);
            return;
        }
        else
        {
            debug_level = atoi(tokens[1]);;
        }
        exit_status = 0;
    }

    void prompt()
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

    void status()
    {
        if(args > 0)
        {
            exit_status = 1;
            return;
        }
        printf("%d\n", exit_status);
    }

    void exit_cmd()
    {
        if(args == 0)
        {
            exit(exit_status);
        }
        int exit_with = atoi(tokens[1]);
        exit(exit_with);
    }

    void help()
    {
        if(args > 0)
        {
            exit_status = 1;
            return;
        }
        printf("help\n");
    }

    void print()
    {
        for(int i = 1; i <= args; i++)
        {
            if(i > 1)
                printf(" ");
            printf("%s", tokens[i]);
        }
        exit_status = 0;
    }

    void echo()
    {
        for(int i = 1; i <= args; i++)
        {
            if(i > 1)
                printf(" ");
            printf("%s", tokens[i]);
        }
        printf("\n");
        exit_status = 0;
    }

    void len()
    {
        int ln = 0;
        for(int i = 1; i <= args; i++){
            ln += strlen(tokens[i]);
        }
        printf("%d\n", ln);
        exit_status = 0;
    }

    void sum()
    {
        int suma = 0;
        for(int i = 1; i <= args; i++){
            suma += atoi(tokens[i]);
        }
        printf("%d\n", suma);
        exit_status = 0;
    }

    void calc()
    {
        int a1 = atoi(tokens[1]);
        int a2 = atoi(tokens[3]);
        int op = *tokens[2];
        exit_status = 0;
        switch (op)
        {
        case '+':
            printf("%d\n", a1 + a2);
            break;
        case '-':
            printf("%d\n", a1 - a2);
            break;
        case '*':
            printf("%d\n", a1 * a2);
            break;
        case '/':
            printf("%d\n", a1 / a2);
            break;
        case '%':
            printf("%d\n", a1 % a2);
            break;
        default:
            exit_status = 1;
            break;
        }
    }

    void basename()
    {
        if(args < 1)
        {
            exit_status = 1;
            return;
        }
        for(int i = 1; i <= args; i++){
            char* r = tokens[i];
            char* s = tokens[i];
            char* d = NULL;
            while(*r != '\0')
            {
                if(*r == '/')
                    s = r;
                if(*r == '.')
                    d = r;
                r++;
            }
            if(d != NULL)
                *d = '\0';
            if(i > 1)
                printf(" ");
            printf("%s\n", ++s);
        }
        exit_status = 0;
    }   

    void dirname()
    {
        if(args < 1)
        {
            exit_status = 1;
            return;
        }
        for(int i = 1; i <= args; i++){
            char* r = tokens[i];
            char* s = tokens[i] + 1;
            while(*r != '\0')
            {
                if(*r == '/')
                    s = r;
                r++;
            }
                *s = '\0';
            if(i > 1)
                printf(" ");
            printf("%s\n", tokens[i]);
        }
        exit_status = 0;
    }

    void dirch()
    {
        if(args == 0)
        {
            exit_status = chdir("/");
        }else if(args == 1){
            exit_status = chdir(tokens[1]);
        }
        if(exit_status != 0)
        {
            exit_status = errno;
            printf("dirch: %s\n", strerror(exit_status));
        }    
    }

    void dirwd()
    {
        char* wd = (char *)calloc(1024, sizeof(char));
        getcwd(wd, 1024);
        if(wd == NULL)
        {
            exit_status = errno;
            free(wd);
            return;
        }
        else
            exit_status = 0;
        if(args < 1 || strcmp(tokens[1], "base") == 0)
        {
            char* r = wd;
            char* s = wd;
            char* d = NULL;
            while(*r != '\0')
            {
                if(*r == '/')
                    s = r;
                if(*r == '.')
                    d = r;
                r++;
            }
            if(d != NULL)
                *d = '\0';
            if(strlen(wd) == 1)
                printf("%s\n", s);
            else
                printf("%s\n", ++s);
        }else if(strcmp(tokens[1], "full") == 0)
        {
            printf("%s\n", wd);
        }else
            exit_status = 1;
        free(wd);
    }

    void dirmk()
    {
        if(args < 1)
            exit_status = 1;
        else
        {
            for (int i = 1; i <= args; i++)
            {
                exit_status = mkdir(tokens[i], 0777);
                if(exit_status != 0)
                {
                    exit_status = errno;
                    printf("dirmk: %s\n", strerror(exit_status));
                    return;
                } 
            }
            exit_status = 0;
        }
    }

    void dirrm()
    {
        if(args < 1)
            exit_status = 1;
        else
        {
            for (int i = 1; i <= args; i++)
            {
                exit_status = rmdir(tokens[i]);
                if(exit_status != 0)
                {
                    exit_status = errno;
                    printf("dirrm: %s\n", strerror(exit_status));
                    return;
                } 
            }
            exit_status = 0;
        }
    }

    void dirls()
    {
        DIR* d;
        struct dirent* entry;
        if(args == 0)
            d = opendir(".");
        else if (args == 1)
        {
            d = opendir(tokens[1]);
        }
        else
        {
            exit_status = 1;
            return;
        }

        if(d == NULL)
        {
            exit_status = errno;
            printf("dirls: %s\n", strerror(exit_status));
            return;
        }
        
        int di = 0;
        while((entry = readdir(d)) != NULL)
        {
            if(di > 0)
                printf("  ");
            printf("%s", entry->d_name);
            di++;
        }
        printf("\n");

        exit_status = closedir(d);
        if(exit_status != 0)
        {
            exit_status = errno;
            printf("dirls: %s\n", strerror(exit_status));
            return;
        }

        exit_status = 0;
    }

    
    //EXTERNAL COMMAND FUNCIONS
     

    //BUILT-IN COMMANDS ARRAY
    cmd builtin_commands[] = 
    {
        {"debug", &debug, "debug opis"},
        {"prompt", &prompt, "prompt opis"},
        {"status", &status, "status opis"},
        {"exit", &exit_cmd, "exit opis"},
        {"help", &help, "help opis"},
        {"print", &print, "debug opis"},
        {"echo", &echo, "echo opis"},
        {"len", &len, "len opis"},
        {"sum", &sum, "sum opis"},
        {"calc", &calc, "help opis"},
        {"basename", &basename, "basename opis"},
        {"dirname", &dirname, "dirname opis"},
        {"dirch", &dirch, "dirch opis"},
        {"dirwd", &dirwd, "dirwd opis"},
        {"dirmk", &dirmk, "dirmk opis"},
        {"dirrm", &dirrm, "dirrm opis"},
        {"dirls", &dirls, "dirls opis"},

    };

    /*
    //EXTERNAL COMMANDS ARRAY
    cmd external_commands[] = 
    {
        
    };
    */

    //COMMAND EVAL FUNCTIONS
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

    void execute_builtin(char* line, int t, int ix, char* input, char* output)
    {
            if(debug_level > 0)
            {
                info_print(line, t, input, output);
                printf("Executing builtin '%s' in %s\n", tokens[0], (bg) ? "background" : "foreground");
            }
            builtin_commands[ix].operation(args);
    }

    void execute_external(char* line, int t, char* input, char* output)
    {
        
        info_print(line, t, input, output);
        printf("External command '%s", tokens[0]);
        for(int i = 1; i <= args; i++)
        {
            printf(" %s", tokens[i]);
        }
        printf("'\n");
        /*
        external_commands[ix].operation(args);
        */
    }

    void find_builtin(char* line, int t, char* input, char* output)
    {
        int builtin_size = (int)(sizeof(builtin_commands) / sizeof(builtin_commands[0]));
        for(int i = 0; i < builtin_size; i++)
        {
            if(strcmp(builtin_commands[i].name, tokens[0]) == 0)
            {
                execute_builtin(line, t, i, input, output);
                return;
            }
        }
        execute_external(line, t, input, output);
    }

    //LINE TOKENIZATION
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
            
            if(niz)
            {
                if(c == '"')
                {
                    niz = false;
                    line[i] = '\0'; 
                }
            }
            else
            {
                if(word)
                {
                    if(c == ' ')
                    {
                        word = false;
                        line[i] = '\0';
                    }
                }
                else
                {
                    if(c == '#')
                    {
                        line[i] = '\0';
                        break;
                    }
                    if(c != ' ' && c != '\n' && c != '\0')
                    {
                        if(c == '"')
                        {
                            niz = true;
                            tokens[ti++] = &line[++i];
                        }
                        else
                        {
                            word = true;
                            tokens[ti++] = &line[i];
                        }
                    }
                }
            }
        }    
        return --ti;
    }

    //MAIN
    int main()
    {
        int mode = isatty(STDIN_FILENO);
        
        while(1)
        {
            size_t line_size = 0;
            char *line = NULL;
            bool blank = true;
            bool comment = false;

            char* input = NULL;
            char* output = NULL;
            bg = 0;
            args = 0;

            //INTERACTIVE MODE
            if(mode)
                printf(">%s ", SHNAME);
            int line_read = getline(&line, &line_size, stdin);
            
            char* lline = (char*)calloc(strlen(line) + 1, sizeof(char));
            strcpy(lline, line);
            lline[line_read - 1] = '\0';

            for(int i = 0; i < line_read; i++)
            {
                if(blank && line[i] == '#')
                {
                    comment == true;
                    break;
                }
                if(!isspace(line[i])){
                    blank = false;
                    break;
                }
                
            }
            if(line_read <= 0)
                break;
            if(!comment && !blank)
            {
                int t = tokenize(line, line_read);
                args = t;

                //PARSING
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

                find_builtin(lline, t, input, output);
            }

            free(input);
            free(output);
            free(line);
            free(lline);
        
        }

        return exit_status;
    }