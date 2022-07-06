/*
  GROUP: 18
  Roll No.: 18CS10043, 19CS30004
*/

#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>

using namespace std;

const int MAX = 1024;

// parse acc. delimiter
int sh_parse(char **args, char *input, const char *delimiter)
{
  int position = 0;
  args[position] = strtok(input, delimiter);
  while (args[position] != NULL)
  {
    position++;
    args[position] = strtok(NULL, delimiter);
  }
  return position;
}

int sh_cd(char **args)
{
  if (args[1] == NULL) 
    fprintf(stderr, "sh: expected argument to \"cd\"\n");
  else 
  {
    if (chdir(args[1]) != 0)
      perror("sh");
  }
  return 1;
}

// execute basic sh cmd's
void sh_execute(char *input)
{
  char *args[MAX];

  sh_parse(args, input, " \n");

  if (!strcmp(input, "exit"))
    exit(0);

  execvp(args[0], args);
  perror("sh");
}

void sh_redirecting(char *input)
{ 
  char *args[MAX];
  char *files[MAX];

  int i = 0;

  // find the index of <
  auto get_in_idx = [&](char *input)
  {
    for (int i = 0; input[i] != '\0'; i++)
    {
      if (input[i] == '<')
        return i;
    }
    return -1;
  };

  // find the index of >
  auto get_out_idx = [&](char *input)
  {
    for (int i = 0; input[i] != '\0'; i++)
    {
      if (input[i] == '>')
        return i;
    }
    return -1;
  };

  // in is the index of < character
  int in = get_in_idx(input);

  //out is the index of > character
  int out = get_out_idx(input);
    
  // break input according &, <, > and \n and get size of args in i
  i = sh_parse(args, input, "&<>\n");
    
  pid_t p = 0;
    
  if (p == 0)
  {
    if (in == -1 && out == -1)
    {
      if (i <= 1)
        sh_execute(args[0]);
      else
        perror("sh");
    }
    else if (in > 0 && out == -1)
    {
      if (i == 2)
      {
        int f = sh_parse(files, args[1], " \n");
        if (f != 1)
        {
          perror("sh");
          return;
        }
        int ifd = open(files[0], O_RDONLY);
        if (ifd < 0)
        {
          perror("sh");
          return;
        }

        // redirect STDIN to the mentioned input file
        close(0);
        dup(ifd);
        close(ifd);
      }
      else 
        perror("sh");
    }
    else if (in == -1 && out > 0)
    {
      if (i == 2)
      {
        int f = sh_parse(files, args[1], " \n");
        if (f != 1)
        {
          perror("sh");
          return;
        }
        int ofd = open(files[0], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (ofd < 0)
        {
          perror("sh");
          return;
        }

        // redirect STDOUT to the mentioned output file
        close(1);
        dup(ofd);
        close(ofd);
      }
    }
    else if (in > 0 && out > 0 && i == 3)
    {
      int ifd, ofd, f;
      if (in > out)
      {
        f = sh_parse(files,args[1]," \n");
        if (f != 1)
        {
          perror("sh");
          return;
        }
        ofd = open(files[0], O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        f = sh_parse(files,args[2]," \n");
        if (f != 1)
        {
          perror("sh");
          return;
        }
        ifd = open(files[0], O_RDONLY);
      }
      else if (in < out)
      {
        f = sh_parse(files, args[2], " \n");
        if (f != 1)
        {
          perror("sh");
          return;
        }
        ofd = open(files[0], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        f = sh_parse(files,args[1]," \n");
        if (f != 1) 
        {
          perror("sh");
          return;
        }
        ifd = open(files[0], O_RDONLY);             
      }

      // redirect STDIN and STDOUT to mentioned files

      close(0);
      dup(ifd);
      close(1);
      dup(ofd);
      close(ifd);
      close(ofd);
    }
    else 
      return;
    sh_execute(args[0]);
  }
  return;
  exit(0);
}

void sh_pipe(char** args, int N)
{
  int p[N-1][2]; // N - 1 pipes

  for (int i = 0; i < N-1; i++)
  {
    if (pipe(p[i]) < 0)
    {
      perror("sh");
      return;
    }   
  }

  pid_t pid;
  for (int i = 0; i < N; i++) 
  { 
    if ((pid = fork()) == 0) 
    { 
      // read from the previous pipe 
      if (i != 0)
      {
        // it doesn't write to the previous pipe
        close(p[i - 1][1]);

        // redirect STDIN to read end of the previous pipe
        dup2(p[i - 1][0], 0);
                
        // close the read pipe end
        close(p[i - 1][0]); 
      }
            
      // write to the next pipe 
      if (i != N-1)
      {
        // it doesn't read from the next pipe
        close(p[i][0]);
                
        // redirects STDOUT to write end of the next pipe
        dup2(p[i][1], 1);

        // close the write pipe end
        close(p[i][1]); 
      }
      
      for (int j = 0; j < N - 1; j++)
      { 
        // closes all pipes
        close(p[j][0]);
        close(p[j][1]);
      }

      sh_redirecting(args[i]);

      exit(0); 
    }
    else 
    {
      // wait for a while
      usleep(10000);

      for (int j = 0; j < i; j++)
      {   
        // closes all pipes
        close(p[j][0]);
        close(p[j][1]);
      }
    }
  }
  while (wait(NULL) > 0);
  exit(0);
}

void sh_loop()
{
  while (true)
  {
    printf(">> ");
    char input[MAX] = ""; 
    fgets(input, MAX, stdin);  

    if ((strlen(input) >= 4) && (input[0] == 'e') && (input[1] == 'x') && (input[2] == 'i') && (input[3] == 't')) // exit
      exit(0);

    if ((strlen(input) >= 2) && (input[0] == 'c') && (input[1] == 'd'))
    {
      char *args[MAX];
      sh_parse(args, input, " \n");
      sh_cd(args);
      continue;
    }

    int background = 0; // & flag

    for (int i = 0; input[i] != '\0'; i++)
    {
      if (input[i] == '&') // truncating 'cmd' here
      {
        int j = i;
        j++;
        while (input[j] == ' ')
          j++;
        if (input[j]=='\n') 
        {
          background = 1;
          input[i] = '\n';
          i++; 
          input[i] = '\0';
        }
      }
    }

    char *ps_args[MAX];
    int N = sh_parse(ps_args, input, "|");
    
    int status;
    pid_t pid;
    pid = fork();
    if (pid == 0)
      sh_pipe(ps_args, N);
    else if (pid < 0)
      perror("sh");
    else 
    { 
      if (background == 0) // no &
      {
        do 
        {
          // wait for child process to complete
          waitpid(pid, &status, WUNTRACED);
        } 
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
      }
      else
        continue;
    }
  }
}

int main()
{
  sh_loop();

  return 0;
}