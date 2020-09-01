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

#include <errno.h>
#include <error.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <spawn.h>
#include <unistd.h>

#include "utils.h"

int
main (int argc, char **argv)
{
  int c = 0;
  char **v = xmalloc ((argc + 2) * sizeof (*v));
  pid_t pid;

  v[c++] = "emacsclient";
  v[c++] = "-t";
  do
    v[c] = argv[c - 1];
  while (++c <= argc);

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
                {
                  v[0] = "/usr/local/bin/emacs";
                  v[1] = "--daemon";
                  v[2] = NULL;

                  xexecvp (v[0], v);
                }
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
