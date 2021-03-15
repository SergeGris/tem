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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "defines.h"

#define edie(e, ...) (error (EXIT_FAILURE, e, __VA_ARGS__), assume (false))
#define die(e, s) (fputs (s, stderr), exit (e))

#define EXITED_SUCCESSFULLY(status)                                           \
  (WIFEXITED (status) && WEXITSTATUS (status) == EXIT_SUCCESS)
#define EXITED_UNSUCCESSFULLY(status)                                         \
  ((WIFEXITED (status) && WEXITSTATUS (status) != EXIT_SUCCESS)               \
   || WIFSIGNALED (status))
#define TERMINATED(status) (WIFEXITED (status) || WIFSIGNALED (status))

static uid_t uid;
static char *socket_name = NULL;

static void *xmallocarray (size_t n, size_t m) __malloc __alloc_size ((1)) __returns_nonnull __warn_unused_result;
static void *
xmalloc (size_t s)
{
  void *pointer = malloc (s);
  if (unlikely (pointer == NULL))
    edie (errno, "malloc()");
  return pointer;
}

static void *xmallocarray (size_t n, size_t m) __malloc __alloc_size ((1, 2)) __returns_nonnull __warn_unused_result;
static void *
xmallocarray (size_t n, size_t m)
{
  size_t s;
  if (unlikely (mul_overflow (n, m, &s)))
    edie (ENOMEM, "malloc()");

  return xmalloc (s);
}

#if 0
static void *xreallocarray (void *pointer, size_t n, size_t m) __malloc __nonnull ((1)) __alloc_size ((2, 3)) __returns_nonnull __warn_unused_result;
static void *
xreallocarray (void *pointer, size_t n, size_t m)
{
  size_t s;
  if (unlikely (mul_overflow (n, m, &s)))
    edie (ENOMEM, "realloc()");

  pointer = realloc (pointer, s);
  if (unlikely (pointer == NULL))
    edie (errno, "realloc()");
  return pointer;
}
#endif

static pid_t xfork (void) __warn_unused_result;
static pid_t
xfork (void)
{
  pid_t pid = fork ();
  if (unlikely (pid < 0))
    edie (errno, "fork()");
  return pid;
}

static void xwaitpid (pid_t pid, int *status, int flags) __nonnull ((2));
static void
xwaitpid (pid_t pid, int *status, int flags)
{
  while (unlikely (waitpid (pid, status, flags) < 0))
    if (unlikely (errno != EINTR))
      edie (errno, "waitpid()");
}

static void xexecvp (char *file, char **argv) __noreturn __nonnull ((1, 2));
static void
xexecvp (char *file, char **argv)
{
  execvp (file, argv);
  /* Unreachable on success.  */
  edie (errno, "execvp()");
}

static void
xraise (int signo)
{
  if (unlikely (raise (signo) != 0))
    edie (errno, "raise(%d)", signo);
}

static void
xkill (pid_t pid, int signo)
{
  if (unlikely (kill (pid, signo) != 0))
    edie (errno, "kill(%d, %d)", pid, signo);
}

static void xchmod (const char *file_name, mode_t mode) __nonnull ((1));
static void
xchmod (const char *file_name, mode_t mode)
{
  if (unlikely (chmod (file_name, mode) != 0))
    edie (errno, "chmod(%s, %o)", file_name, mode);
}

static void xmkdir (const char *directory_name, mode_t mode) __nonnull ((1));
static void
xmkdir (const char *directory_name, mode_t mode)
{
  if (unlikely (mode & ~07777))
    edie (EINVAL, "mkdir(%s, %o)", directory_name, mode);

  if (unlikely (mkdir (directory_name, mode) != 0))
    {
      if (unlikely (errno != EEXIST))
        edie (errno, "mkdir(%s, %o)", directory_name, mode);
      /* Do not die if there is directory with given name.  */

      struct stat sb;
      if (unlikely (stat (directory_name, &sb) != 0))
        edie (errno, "stat(%s)", directory_name);

      if (unlikely (!S_ISDIR (sb.st_mode)))
        edie (ENOTDIR, "%s", directory_name);

      /* If directory exists make sure that it has required mode.  */
      if (unlikely ((sb.st_mode & 07777) != mode))
        xchmod (directory_name, mode);
    }
}

static int xsprintf (char *s, const char *fmt, ...) __nonnull ((1, 2)) __format_printf (2, 3);
static int
xsprintf (char *s, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  int n = vsprintf (s, fmt, ap);
  va_end (ap);

  if (n < 0)
    edie (errno, "sprintf()");

  return n;
}

#if 0
static int xasprintf (char **s, const char *fmt, ...) __nonnull ((1, 2)) __format_printf (2, 3);
static int
xasprintf (char **s, const char *fmt, ...)
{
  int n;
  va_list ap, ap2;

  va_start (ap, fmt);
  va_copy (ap2, ap);
  if (unlikely ((n = vsnprintf (NULL, 0, fmt, ap)) < 0))
    edie (errno, "vsnprintf()");
  va_end (ap);

  if (*s == NULL)
    *s = xmallocarray (n + 1, sizeof (char));
  else
    *s = xreallocarray (*s, n + 1, sizeof (char));

  if (unlikely ((n = vsnprintf (*s, n + 1, fmt, ap2)) < 0))
    edie (errno, "vsnprintf()");

  va_end (ap2);

  return n;
}

static void xastrcat (char **d, const char *s) __nonnull ((1, 2));
static void
xastrcat (char **d, const char *s)
{
  size_t s_length = strlen (s);
  size_t d_length = strlen (*d);

  *d = xreallocarray (*d, d_length + s_length + 1, sizeof (char));
  strcpy (*d + d_length, s);
}
#endif

static uid_t xgeteuid (void) __warn_unused_result;
static uid_t
xgeteuid (void)
{
  uid_t euid;
  int saved_errno = errno;

  errno = 0;
  if (unlikely ((euid = geteuid ()) == (uid_t) -1 && errno != 0))
    edie (errno, "geteuid()");
  errno = saved_errno;

  return euid;
}

poison (malloc calloc realloc fork kill raise execvp waitpid);
poison (chmod mkdir sprintf snprintf asprintf getuid geteuid);

static int wait_program_termination (pid_t pid) __warn_unused_result;
static int
wait_program_termination (pid_t pid)
{
  int status;
  do
    xwaitpid (pid, &status, 0);
  while (unlikely (!TERMINATED (status)));
  return status;
}

poison (xwaitpid);

static bool socket_exists (const char *socket_name) __nonnull ((1)) __warn_unused_result;
static bool
socket_exists (const char *socket_name)
{
  struct stat sb;

  if (stat (socket_name, &sb) != 0)
    {
      if (errno == ENOENT)
        return false;
      else
        edie (errno, "stat(%s)", socket_name);
    }

  if (unlikely (!S_ISSOCK (sb.st_mode)))
    edie (ENOTSOCK, "%s", socket_name);

  /* There is a socket in our directory,
     but this socket is not owned by us.  */
  if (unlikely (sb.st_uid != uid))
    die (EXIT_FAILURE, "The socket does not belong to us.\n");

  return (faccessat (AT_FDCWD, socket_name, X_OK, AT_EACCESS) == 0);
}

static char *get_alternate_editor (void) __returns_nonnull __warn_unused_result;
static char *
get_alternate_editor (void)
{
  char *alternate_editor = getenv ("ALTERNATE_EDITOR");
  return alternate_editor != NULL ? alternate_editor : (char *) "";
}

static void
start_daemon (bool do_fork, int argc, char **argv)
{
  pid_t pid;
  int status;

  pid = do_fork ? xfork () : 0;
  if (pid == 0)
    {
      int i;
      int c;
      char **v;
      char *d;

      d = xmalloc (strlen ("--daemon=") + strlen (socket_name) + 1);
      xsprintf (d, "--daemon=%s", socket_name);

      c = 0;
      v = xmallocarray (argc + 2, sizeof (*v));

      v[c++] = "emacs";
      v[c++] = d;

      i = 1;
      while (i < argc)
        v[c++] = argv[i++];
      v[c] = NULL;

      setsid ();

      xexecvp (v[0], v);
    }

  status = wait_program_termination (pid);

  if (unlikely (EXITED_UNSUCCESSFULLY (status)))
    die (EXIT_FAILURE, "Failed to start daemon.\n");
}
static void
start_client (int argc, char **argv)
{
  int i;
  int c;
  char **v;

  c = 0;
  v = xmallocarray (argc + 7, sizeof (*v));

  v[c++] = "emacsclient";
  v[c++] = "-t";
  v[c++] = "-q";
  v[c++] = "-s";
  v[c++] = socket_name;
  v[c++] = "-a";
  v[c++] = get_alternate_editor ();

  /* Copy the rest arguments.  */
  i = 1;
  while (i < argc)
    v[c++] = argv[i++];
  v[c] = NULL;

  xexecvp (v[0], v);
}
static void
stop_daemon (bool do_fork)
{
  pid_t pid;
  int status;

  pid = do_fork ? xfork () : 0;
  if (pid == 0)
    {
      char *v[6];
      v[0] = "emacsclient";
      v[1] = "-s";
      v[2] = socket_name;
      v[3] = "--eval";
      v[4] = "(progn (kill-emacs))";
      v[5] = NULL;

      if (unlikely (!socket_exists (socket_name)))
        die (EXIT_SUCCESS, "Emacs daemon is not running.\n");

      xexecvp (v[0], v);
    }

  status = wait_program_termination (pid);

  if (unlikely (EXITED_UNSUCCESSFULLY (status)))
    die (EXIT_FAILURE, "Failed to stop Emacs daemon.\n");
}
static __noreturn void
restart_daemon (int argc, char **argv)
{
  pid_t pid;
  int status;

  pid = xfork ();
  if (pid == 0)
    {
      /* Stop executing until we are allowed to start daemon.  */
      xraise (SIGSTOP);
      start_daemon (false, argc, argv);
    }

  stop_daemon (true);

  xkill (pid, SIGCONT); /* Allow starter to start doing his job.  */
  status = wait_program_termination (pid);

  if (unlikely (EXITED_UNSUCCESSFULLY (status)))
    die (EXIT_FAILURE, "Failed to start Emacs daemon.\n");

  exit (EXIT_SUCCESS);
}

static __noreturn void
usage (int status)
{
  fputs ("\
Usage: tes [OPTIONS] FILE...\n\
Tiny Emacs Manager.\n\
Every FILE can be either just a FILENAME or [+LINE[:COLUMN]] FILENAME.\n\
\n\
The following OPTIONS are accepted:\n\
  --help                  Print this usage information message.\n\
  --version               Print version.\n\
  --startd                Start the emacs daemon.\n\
  --restartd              Restart the already runned emacs daemon.\n\
  --stopd                 Stop the emacs daemon.\n\
\n\
All other options will be passed to emacsclient.\n\
",
         (status == EXIT_SUCCESS ? stdout : stderr));

  exit (status);
}

static __noreturn void
version (void)
{
  puts ("Tiny Emacs Manager 1.0");
  exit (EXIT_SUCCESS);
}

int
main (int argc, char **argv)
{
  pid_t pid;
  int i;
  int status;

  uid = xgeteuid ();

#define EMACS_SOCKET_DIRECTORY "/tmp/.emacs-sockets"

  socket_name = xmalloc (strlen (EMACS_SOCKET_DIRECTORY)
                         + 1
                         + INT_STRLEN_BOUND (uid_t)
                         + 1
                         + strlen ("socket")
                         + 1);

  strcpy (socket_name, EMACS_SOCKET_DIRECTORY);
  xmkdir (socket_name, 00777);
  xsprintf (socket_name + strlen (socket_name), "/%u", uid);
  xmkdir (socket_name, 00700);
  strcpy (socket_name + strlen (socket_name), "/socket");

  for (i = 1; i < argc; i++)
    {
      char *arg = argv[i];

      if (unlikely (arg[0] != '-' || arg[1] != '-' || arg[2] == '\0'))
        continue;
      arg += 2;

      if (streq (arg, "help"))
        usage (EXIT_SUCCESS);
      if (streq (arg, "version"))
        version ();
      if (streq (arg, "startd"))
        start_daemon (false, argc - i, argv + i);
      if (streq (arg, "restartd"))
        restart_daemon (argc - i, argv + i);
      if (streq (arg, "stopd"))
        stop_daemon (false);
    }

  if (unlikely (!socket_exists (socket_name)))
    {
      start_daemon (true, 0, NULL);

      if (unlikely (!socket_exists (socket_name)))
        die (EXIT_FAILURE, "Can not find socket.\n");
    }

  pid = xfork ();
  if (pid == 0)
    start_client (argc, argv);

  free (socket_name);

  status = wait_program_termination (pid);
  return EXITED_SUCCESSFULLY (status) ? EXIT_SUCCESS : EXIT_FAILURE;
}
