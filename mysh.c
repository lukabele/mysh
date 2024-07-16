#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <signal.h>

//DEFINES AND TYPEDEFS
#define MAX_TOKENS 20

extern int tokenize(char ** tokens, char *line, int size);
extern int find_builtin(char* line, int t, char* input, char* output);
extern void execute_builtin(char*, int, int, char*, char*);

typedef void (*cmd_operation)();

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
char** ts = tokens;
char* argnew[MAX_TOKENS];
int bg = 0;
int args = 0;
char procpath[2048] = "/proc";
char procstatpath[4096];

//OTHER FUNCTIONS
int compareStringDigits(const void *a, const void *b)
{
    const char **aa = (const char **)a;
    const char **bb = (const char **)b;
    return strcmp(*aa, *bb);
}

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

void rename_cmd()
{
    if(args == 2)
    {
        exit_status = rename(tokens[1], tokens[2]);
        if(exit_status != 0)
        {
            exit_status = errno;
            perror("rename");
            return;
        }
    }
    else
        exit_status = 1;
}

void unlink_cmd()
{
    if(args == 1)
    {
        exit_status = unlink(tokens[1]);
        if(exit_status != 0)
        {
            exit_status = errno;
            perror("unlink");
            return;
        }
    }
    else
    {
        exit_status = 1;
    }
    exit_status = 0;
}

void remove_cmd()
{
    if(args < 1)
    {
        exit_status = 1;
    }
    else
    {
        for (int i = 1; i <= args; i++)
        {
            exit_status = remove(tokens[i]);
            if(exit_status != 0)
            {
                exit_status = errno;
                perror("remove");
                return;
            } 
        }
        exit_status = 0;
    }
}

void linkhard()
{
    if(args == 2)
    {
        exit_status = link(tokens[1], tokens[2]);
        if(exit_status != 0)
        {
            exit_status = errno;
            perror("linkhard");
            return;
        }
    }
    else
    {
        exit_status = 1;
    }
}

void linksoft()
{
    if(args == 2)
    {
        exit_status = symlink(tokens[1], tokens[2]);
        if(exit_status != 0)
        {
            exit_status = errno;
            perror("linksoft");
            return;
        }
    }
    else
    {
        exit_status = 1;
    }
}

void linkread()
{
    char* path = (char*)calloc(256, sizeof(char));
    if(args == 1)
    {
        int len = (int)readlink(tokens[1], path, 256- 1);
        if(len != -1)
        {
            path[len] = '\0';
            printf("%s\n", path);
            exit_status = 0;
        }
        else
        {
            exit_status = errno;
            perror("linkread");
            free(path);
            return;
        }
    }
    else
    {
        exit_status = 1;
    }
    free(path);
}

void linklist()
{
    DIR* d;
    struct dirent* entry;
    struct stat fstat;
    struct stat tgtstat;

    if (stat(tokens[1], &tgtstat) == -1) {
        exit_status = errno;
        perror("linklist");
        return;
    }

    if(args == 1)
        d = opendir(".");
    else
    {
        exit_status = 1;
        return;
    }

    if(d == NULL)
    {
        exit_status = errno;
        perror("linklist");
        return;
    }
    
    int di = 0;
    while((entry = readdir(d)) != NULL)
    {
        if(stat(entry->d_name, &fstat) != -1 && fstat.st_ino == tgtstat.st_ino)
        {
            if(di > 0)
                printf("  ");
            printf("%s", entry->d_name);
            di++;
        }
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

void cpcat()
{
    fflush(stdout);
    int in = 0;
    int out = 1;
    if(args > 2)
    {
        exit_status = 1;
        return;
    }
    else if(args == 2)
    {
        if(strcmp(tokens[1], "-") != 0)
        {
            in = open(tokens[1], O_RDONLY);
            if(in == -1)
            {
                exit_status = errno;
                perror("cpcat");
                return;
            }
        } 
        out = open(tokens[2], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if(out == -1)
            {
                exit_status = errno;
                perror("cpcat");
                if(in != 0)
                    close(in);
                return;
            }
    }
    else if(args == 1)
    {
        if(strcmp(tokens[1], "-") != 0)
        {
            in = open(tokens[1], O_RDONLY);
            if(in == -1)
            {
                exit_status = errno;
                perror("cpcat");
                return;
            }
        }
    }

    char* buff = (char*)calloc(4096, sizeof(char));
    ssize_t rbytes, wbytes;

    while( (rbytes = read(in, buff, 4096)) > 0)
    {
        wbytes = write(out, buff, rbytes);
        if( wbytes != rbytes)
        {
            exit_status = errno;
            perror("cpcat");
            return;
        }
    }

    if(rbytes == -1)
    {
        exit_status = errno;
        perror("cpcat");
        return;
    }

    if(in != 0)
        close(in);
    if(out != 1)
        close(out);
    free(buff);
    exit_status = 0;
}

void pid()
{
    pid_t id = getpid();
    printf("%d\n", id);
    exit_status = 0;
}

void ppid()
{
    pid_t id = getppid();
    printf("%d\n", id);
    exit_status = 0;
}

void uid()
{
    uid_t id = getuid();
    printf("%d\n", id);
    exit_status = 0;
}

void euid()
{
    uid_t id = geteuid();
    printf("%d\n", id);
    exit_status = 0;
}

void gid()
{
    gid_t id = getgid();
    printf("%d\n", id);
    exit_status = 0;
}

void egid()
{
    gid_t id = getegid();
    printf("%d\n", id);
    exit_status = 0;
}

void sysinfo()
{
    struct utsname udata;
    if(uname(&udata) != 0)
    {
        exit_status = errno;
        perror("sysinfo");
        return;
    
    }
    printf("Sysname: %s\n", udata.sysname);
    printf("Nodename: %s\n", udata.nodename);
    printf("Release: %s\n", udata.release);
    printf("Version: %s\n", udata.version);
    printf("Machine: %s\n", udata.machine);
    exit_status = 0;
}

void proc()
{
    if(args > 1)
    {
        exit_status = 1;
        return;
    }
    else if(args == 1)
    {
        exit_status = access(tokens[1], F_OK|R_OK);
        if(exit_status != 0)
        {
            exit_status = 1;
            return;
        }
        else
        {
            strncpy(procpath, tokens[1], sizeof(procpath));
            procpath[sizeof(procpath) - 1] = '\0';
        }
    }   
    else
    {
        printf("%s\n", procpath);
        exit_status = 0;
    }
}

void pids()
{
    struct dirent* entry;
    DIR* d = opendir(procpath);
    char** pids = NULL;
    int np = 0;
    
    if(d == NULL)
    {
        exit_status = errno;
        perror("pids");
        return;
    }

    if(args != 0)
    {
        exit_status = 1;
        return;
    }

    while((entry = readdir(d)) != NULL)
    {
        bool num = true;
        if(isdigit(entry->d_name[0]))
        {
            int i = 1;
            num = true;
            while(entry->d_name[i] != '\0')
            {
                if(isdigit(entry->d_name[i]) == 0)
                {
                    num = false;
                    break;
                }
                i++;
            }
            if(num)
            {
                pids = realloc(pids, sizeof(char *) * (np + 1));
                if(pids == NULL)
                {
                    exit_status = errno;
                    perror("pids");
                    return;
                }
                pids[np] = strdup(entry->d_name);
                if(pids[np] == NULL)
                {
                    exit_status = errno;
                    perror("pids");
                    return;
                }
                np++;
            }
        }   
    }

    qsort(pids, np, sizeof(char*), compareStringDigits);

    exit_status = closedir(d);
    if(exit_status != 0)
    {
        exit_status = errno;
        printf("dirls: %s\n", strerror(exit_status));
        return;
    }

    for (int i = 0; i < np; i++)
    {
        printf("%s\n", pids[i]);
        free(pids[i]);
    }
    free(pids);
    exit_status = 0;
}

void pinfo()
{
    struct dirent* entry;
    DIR* d = opendir(procpath);
    char** pids = NULL;
    int np = 0;
    if(d == NULL)
    {
        exit_status = errno;
        perror("pids");
        return;
    }
    if(args != 0)
    {
        exit_status = 1;
        return;
    }
    while((entry = readdir(d)) != NULL)
    {
        bool num = true;
        if(isdigit(entry->d_name[0]))
        {
            int i = 1;
            num = true;
            while(entry->d_name[i] != '\0')
            {
                if(isdigit(entry->d_name[i]) == 0)
                {
                    num = false;
                    break;
                }
                i++;
            }
            if(num)
            {
                pids = realloc(pids, sizeof(char *) * (np + 1));
                if(pids == NULL)
                {
                    exit_status = errno;
                    perror("pids");
                    return;
                }
                pids[np] = strdup(entry->d_name);
                if(pids[np] == NULL)
                {
                    exit_status = errno;
                    perror("pids");
                    return;
                }
                np++;
            }
        }   
    }

    qsort(pids, np, sizeof(char*), compareStringDigits);
    
    exit_status = closedir(d);
    if(exit_status != 0)
    {
        exit_status = errno;
        printf("dirls: %s\n", strerror(exit_status));
        return;
    }

    int pid;
    int ppid;
    char name[256];
    char state;

    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
    for (int i = 0; i < np; i++)
    {
        sprintf(procstatpath, "%s/%s/stat", procpath, pids[i]);
        FILE *f = fopen(procstatpath, "r");
        if(f == NULL)
        {
            exit_status = errno;
            perror("pids");
            return;
        }
        fscanf(f, "%d %255s %c %d", &pid, name, &state, &ppid);
        name[strlen(name) - 1] = '\0';
        printf("%5d %5d %6c %s\n", pid, ppid, state, &name[1]);
        free(pids[i]);
    }
    free(pids);
    exit_status = 0;
}

void waitone()
{

    int status;
    int pid;

    if(args == 1)
    {
        int child = atoi(tokens[1]);
        pid = waitpid(child, &status, 0);
            if(WIFEXITED(status))
                exit_status = WEXITSTATUS(status);
        if(pid == -1)
            exit_status = 0;
    }
    else if(args == 0)
    {
        pid = waitpid(-1, &status, 0);
        if(pid > 0)
        {
            if(WIFEXITED(status))
                exit_status = WEXITSTATUS(status);
            else
                exit_status = 127;
        }
        else if(pid == -1)
            exit_status = 0;
    }
    else
        exit_status = 1;
}

void waitall()
{
    int status;
    int pid;

    if(args == 0)
    {
        while((pid = waitpid(-1, &status, 0)) != -1)
        {
            if (WIFEXITED(status))
                exit_status = WEXITSTATUS(status);   
        }

        if(errno == ECHILD)
            exit_status = 0;
    }
    else
        exit_status = 1;
}

void pipes()
{
    
    if(args >= 2)
    {
        fflush(stdin);
        int fds[2 * (args-1)];
        
        for(int i = 0; i < args - 1; i++)
            pipe(fds + i*2);
        
        for(int i = 0; i < args; i++) 
        {
            
            if(!fork())
            {
                //CHILD
                if(i != 0)
                    dup2(fds[(i-1) * 2], 0);
                if(i != args - 1)
                    dup2(fds[i*2 + 1], 1);
                for(int j = 0; j < 2*(args - 1); j++)
                {
                    close(fds[j]);
                }

                int len = strlen(tokens[i+1]);
                char* line =(char*)calloc(len + 1, sizeof(char)); // Creates a string that is alreaddy NULL terminated
                strcpy(line, tokens[i+1]);
                args = tokenize(tokens, line, len + 1);
                tokens[args + 1] = NULL;

                int ix = find_builtin(line, args, NULL, NULL);

                if(ix != -1)
                    execute_builtin(line, args, ix, NULL, NULL);
                else
                    execvp(tokens[0], tokens);

                free(line);
                exit(exit_status);
            }
            
        }
        
        //PARENT
        for(int i = 0; i < 2*(args-1); i++)
            close(fds[i]);
        for(int i = 0; i < args; i++)
            wait(NULL);
        
    }
    else
        exit_status = 1;
    
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
    {"rename", &rename_cmd, "rename opis"},
    {"unlink", &unlink_cmd, "unlink opis"},
    {"remove", &remove_cmd, "remove opis"},
    {"linkhard", &linkhard, "linkhard opis"},
    {"linksoft", &linksoft, "linksoft opis"},
    {"linkread", &linkread, "linkread opis"},
    {"linklist", &linklist, "linklist opis"},
    {"cpcat", &cpcat, "cpcat opis"},
    {"pid", &pid, "cmpidd opis"},
    {"ppid", &ppid, "ppid opis"},
    {"uid", &uid, "uid opis"},
    {"euid", &euid, "euid opis"},
    {"gid", &gid, "gid opis"},
    {"egid", &egid, "egid opis"},
    {"sysinfo", &sysinfo, "sysinfo opis"},
    {"proc", &proc, "proc opis"},
    {"pids", &pids, "pids opis"},
    {"pinfo", &pinfo, "pinfo opis"},
    {"waitone", &waitone, "waitone opis"},
    {"waitall", &waitall, "waitall opis"},
    {"pipes", &pipes, "pipes opis"}
};


//SIGNAL HANDLERS
void sigchld_handler(int signum) {
    int pid, status, serrno;
    serrno = errno;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        if (WIFEXITED(status))
            exit_status = WEXITSTATUS(status);
    errno = serrno;
}

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

void redirect(const char* input, const char* output)
{
    if(input != NULL)
    {
        int infd = open(input, O_RDONLY);
        if(infd < 0)
        {
            exit_status = errno;
            perror("redirect");
            return;
        }
        if(dup2(infd, 0) < 0)
        {
            exit_status = errno;
            perror("redirect");
            return;
        }
        if(close(infd) < 0)
        {
            exit_status = errno;
            perror("redirect");
            return;
        }
    }

    if(output != NULL)
    {
        fflush(stdout);
        int outfd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(outfd < 0)
        {
            exit_status = errno;
            perror("redirect");
            return;
        }
        if(dup2(outfd, 1) < 0)
        {
            exit_status = errno;
            perror("redirect");
            return;
        }
        if(close(outfd) < 0)
        {
            exit_status = errno;
            perror("redirect");
            return;
        }
    }
}

void execute_builtin(char* line, int t, int ix, char* input, char* output)
{
        if(debug_level > 0)
        {
            info_print(line, t, input, output);
            printf("Executing builtin '%s' in %s\n", tokens[0], (bg) ? "background" : "foreground");
        }

        if(bg == 1)
        {
            fflush(stdin);
            fflush(stdout);
            redirect(input, output);
            int pid = fork();
            if(pid < 0)
            {
                exit_status = errno;
                perror("fork");
                return;
            }
            else if(pid == 0)
            {
                //CHILD
                builtin_commands[ix].operation();
            }
        }
        else
        {
            int oldin, oldout;
            if(input != NULL)
                oldin = dup(0);
            if(output != NULL)
                oldout = dup(1);
            redirect(input, output);
            builtin_commands[ix].operation();
            if(input != NULL)
            {
                dup2(oldin, 0);
                close(oldin);
            }  
            if(output != NULL)
            {
                fflush(stdout);
                dup2(oldout, 1);
                close(oldout);
            }
                
        }
            
}

void execute_external(char* line, int t, char* input, char* output)
{
    fflush(stdin);
    fflush(stdout);
    redirect(input, output);
    int pid = fork();
    if(pid < 0)
    {
        exit_status = errno;
        perror("fork");
        return;
    }
    else if(pid == 0)
    {
        //CHILD
        char* arge[args + 2];
        for(int i = 0; i <= args; i++)
        {
            arge[i] = tokens[i];
        }
        arge[args + 1] = NULL;
        execvp(tokens[0], tokens);
        perror("exec");
        exit(127);
    }
    else
    {
        //PARENT
        if(bg == 0)
        {
            int status;
            if(waitpid(pid, &status, 0) < 0)
            {
                exit_status = errno;
                perror("waitpid");
                return;
            }
            if(WIFEXITED(status))
                exit_status = WEXITSTATUS(status);
            else
                exit_status = 127;
        }
    }
}

int find_builtin(char* line, int t, char* input, char* output)
{
    int builtin_size = (int)(sizeof(builtin_commands) / sizeof(builtin_commands[0]));
    for(int i = 0; i < builtin_size; i++)
    {
        if(strcmp(builtin_commands[i].name, tokens[0]) == 0)
        {
            return i;
        }
    }
    return -1;
}

//LINE TOKENIZATION
int tokenize(char ** t, char *line, int size)
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
                        t[ti++] = &line[++i];
                    }
                    else
                    {
                        word = true;
                        t[ti++] = &line[i];
                    }
                }
            }
        }   
    }    
    return --ti;
}

//MAIN
int main(int argc, char *argv[])
{
    int mode = isatty(STDIN_FILENO); 

    signal(SIGCHLD, sigchld_handler);
    
    while(1)
    {
        fflush(stdout);
        
        int ogin = dup(0);
        int ogout = dup(1);

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
            int t = tokenize(tokens, line, line_read);
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

            int ix = find_builtin(lline, t, input, output);
            if(ix == -1)
                execute_external(line, t, input, output);
            else
                execute_builtin(line, t, ix, input, output);
        }

        dup2(ogin, 0);
        dup2(ogout, 1);
        close(ogin);
        close(ogout);
        
        free(input);
        free(output);
        free(line);
        free(lline);
    
    }

    return exit_status;
}