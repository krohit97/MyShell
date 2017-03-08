#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#define CMD_LENGTH 64
#define line_SIZE 1000
int command_size=0;

//Declaring built-in commands
char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "history",
  "clear"
};

void sigkill();
//When build-in command clear is called
int built_in_clear(char **args)
{
 printf("\033c");
  return 1;
}
//When build-in command cd is called
int built_in_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "Error: expected argument for cd \n");
  } else {
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "Error: Directory does not found \n");
    }
  }
  return 1;
}
//When build-in command help is called
int built_in_help(char **args)
{
  int i;
  printf("The following are built-in commands :\n");

  for (i = 0; i < 5; i++) {
    printf("  %s\n", builtin_str[i]);
  }
  return 1;
}
//When build-in command exit is called
int built_in_exit(char **args)
{
  return 0;
}
//When build-in command history is called
int built_in_history(char **args)
{
  int c;
  FILE *file;
  file = fopen("history.txt", "r");
  if (file) {
    while ((c = getc(file)) != EOF)
        putchar(c);
    fclose(file);
}
  return 1;
}
//This method will execute the command input by the user
int cmd_execute(char **args)
{
  int i;
  pid_t pid, wpid;
  int status;

  if (args[0] == NULL) {
    return 1;
  }
  //Checking if build-in command is entered
  for (i = 0; i < 5; i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      if(strcmp(builtin_str[i],"cd") == 0) {
          return built_in_cd(args);
      }
      else if(strcmp(builtin_str[i],"help") == 0) {
          return built_in_help(args);
      }
      else if(strcmp(builtin_str[i],"exit") == 0) {
          return built_in_exit(args);
      }
      else if(strcmp(builtin_str[i],"history") == 0) {
          return built_in_history(args);
      }
       else if(strcmp(builtin_str[i],"clear") == 0) {
          return built_in_clear(args);
      }
    }
  }
  //If build-in command is not entered then check for redirection 
  for(int j=0; j<command_size;j++) {

    if(strcmp(args[j],">") == 0 || strcmp(args[j],"<") == 0)
    {
      i=0;
    for(i=0; i<2; i++) {  

    if(strcmp(args[i],">") == 0) {
      args[i] = NULL;
      FILE *test=fopen(args[i+1], "w");
      if(test == NULL) {
        fprintf(stderr, "File name not entered\n");
        return 1;
      }
      int fd=fileno(test);
      int pid;
      args[i+1] = NULL;
      
      pid=fork();
      if (pid==0) {
        dup2(fd,1);
        if (execvp(args[0], args) == -1) {
          fprintf(stderr, "Error: Command execution \n");
        }
        return 1;
    }
    } 
    else if(strcmp(args[i],"<") == 0) {
      args[i] = NULL;
      FILE *test=fopen(args[i+1], "r");
      int fd=fileno(test);
      int pid;
      args[i+1] = NULL;
      
      pid=fork();
      if (pid==0) {
        dup2(fd,0);
        if (execvp(args[0], args) == -1) {
          fprintf(stderr, "Error: Command execution \n");
        }
        return 1;
    } 

    
  }
  }
  break;
}//If build-in command and redirection both are not entered then check for piping 
else if(strcmp(args[j],"|") == 0) {

    
        FILE *pipein_fp, *pipeout_fp;
        char readbuf[80];
        if (( pipein_fp = popen(args[0], "r")) == NULL)
        {
                perror("popen");
                exit(1);
        }
        if (( pipeout_fp = popen(args[2], "w")) == NULL)
        {
                perror("popen");
                exit(1);
        }
        while(fgets(readbuf, 80, pipein_fp))
                fputs(readbuf, pipeout_fp);

        pclose(pipein_fp);
        pclose(pipeout_fp);

        return 1;
}
else
{
  if(j == command_size-1)
    break;
}
}
  pid = fork();
  if (pid == 0) {
    signal(SIGINT, &sigkill);
    if (execvp(args[0], args) == -1) {
      fprintf(stderr, "Error: Command execution \n");
    }
    exit(0);
  } else if (pid < 0) {
    fprintf(stderr, "Error: Forking \n");
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}
//Reading input line-by-line
char *read_line(void)
{
  int bufsize = line_SIZE;
  int position = 0;
  char *line = malloc(sizeof(char) * bufsize);
  int c;

  if (!line) {
    fprintf(stderr, "Error: size allocation error\n");
    exit(0);
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      line[position] = '\0';
      return line;
    } else {
      line[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += line_SIZE;
      line = realloc(line, bufsize);
      if (!line) {
        fprintf(stderr, "Error: size allocation error\n");
        exit(0);
      }
    }
  }
}
//Writing command to history
void write_to_history(char *line) {

  FILE *f = fopen("history.txt", "a");
  if (f == NULL)
  {
      printf("Error opening file!\n");
      exit(1);
  }
  
  fprintf(f, "%s\n", line);

  fclose(f);
}
//Split the inputted line into array to words
char **split_line(char *line)
{
  command_size = 0;
  int bufsize = CMD_LENGTH, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "Error: size allocation error\n");
    exit(0);
  }

  token = strtok(line, " ");
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += CMD_LENGTH;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "Error: size allocation error\n");
        exit(0);
      }
    }
    command_size++;
    token = strtok(NULL, " ");
  }
  tokens[position] = NULL;
 // command_size++;
  return tokens;
}

int main()
{
  char *line;
  char **args;
  int status;

  signal(SIGINT, &sigkill);
  do {
    printf("shell> ");
    line = read_line();

    if(!(strcmp(line,"\n") == 0))
      write_to_history(line);

    args = split_line(line);
    status = cmd_execute(args);

    free(line);
    free(args);
  } while (status);

  return 0;
}
//To sustain the new window even if ctrl+C is entered
void sigkill() {
  signal(SIGINT, &sigkill);
  fflush(stdout);
  main();
}