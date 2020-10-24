/* Copyright (C) 2020  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of either:

   * the GNU General Public License as published by
     the Free Software Foundation; version 2.

   * the GNU General Public License as published by
     the Free Software Foundation; version 3.

   or both in parallel, as here.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copies of the GNU General Public License,
   version 2 and 3 along with this program;
   if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Sergey Sushilin.  */

#define _POSIX_C_SOURCE 1

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

static void
start_daemon (void)
{
  char *argv[] = { "/usr/local/bin/emacs", "--daemon", NULL };
  xexecvp (argv[0], argv);
}
static void
stop_daemon (void)
{
  char *argv[] = { "emacsclient", "--eval", "(progn (kill-emacs))", NULL };
  xexecvp(argv[0], argv);
}
static void
restart_damon (void)
{
  int status;
  pid_t starter;
  pid_t stopper;

  starter = xfork ();
  if (starter == 0)
    {
      raise (SIGSTOP);
      start_daemon ();
    }
  else
    {
      stopper = xfork ();
      if (stopper == 0)
        stop_daemon ();
      else
        {
          do
            {
              xwaitpid (stopper, &status, 0);

              if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
                {
                  kill (starter, SIGCONT);
                  exit (EXIT_SUCCESS);
                }
              else if (WEXITSTATUS (status) != 0 || WIFSIGNALED (status))
                die (0, "failed to stop daemon");
            }
          while (!WIFEXITED (status) && !WIFSIGNALED (status));
        }
    }
}

int
main (int argc, char **argv)
{
  int c = 0;
  char **v = xmalloc ((argc + 5) * sizeof (*v));
  pid_t pid;
  int i;
  char *tmpdir;
  char socket[512];

  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];

      if (arg[0] != '-' || arg[1] != '-' || arg[2] == '\0')
        continue;
      arg += 2;

      if (strcmp (arg, "startd") == 0)
        start_daemon ();
      if (strcmp (arg, "restartd") == 0)
        restart_damon ();
      if (strcmp (arg, "stopd") == 0)
        stop_daemon ();
    }


  if ((tmpdir = getenv ("XDG_RUNTIME_DIR")) != NULL)
    sprintf (socket, "%s/emacs/server", tmpdir);
  else if ((tmpdir = getenv ("TMPDIR")) != NULL)
    sprintf (socket, "%s/emacs%zu/server", tmpdir, (size_t) getuid ());
  else
    sprintf (socket, "/tmp/emacs%zu/server", (size_t) getuid ());

  int fd = 0;
  if (!access (socket, F_OK))
    {
      pid_t pid = xfork ();
      if (pid == 0)
        start_daemon ();
      else
        {
          int status;

          do
            xwaitpid (pid, &status, 0);
          while (!WIFEXITED (status) && !WIFSIGNALED (status));
        }
    }

  v[c++] = "emacsclient";
  v[c++] = "-t";
  if (fd >= 0)
    {
      v[c++] = "-s";
      v[c++] = socket;
    }

  for (i = 1; i < argc; i++)
    v[c++] = argv[i];
  v[c] = NULL;

  pid = xfork ();
  if (pid == 0)
    xexecvp (v[0], v);
  else
    {
      int status = 0;

      do
        {
          xwaitpid (pid, &status, 0);

          if (WIFEXITED (status) && WEXITSTATUS (status) != 0)
            {
              pid = xfork ();
              if (pid == 0)
                start_daemon ();
              else
                {
                  do
                    {
                      xwaitpid (pid, &status, 0);

                      if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
                        xexecvp (v[0], v);
                      else
                        die (0, "Failed to start daemon");
                    }
                  while (!WIFEXITED (status) && !WIFSIGNALED (status));
                }
            }
        }
      while (!WIFEXITED (status) && !WIFSIGNALED (status));
    }

  return 0;
}
