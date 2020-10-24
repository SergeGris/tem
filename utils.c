/* Public domain.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

void *
xmalloc (size_t s)
{
  void *p = malloc (s);
  if (p == NULL)
    die (errno, "malloc()");
  return p;
}
pid_t
xfork (void)
{
  pid_t pid = fork ();
  if (pid < 0)
    die (errno, "fork()");
  return pid;
}
pid_t
xwaitpid (pid_t pid, int *status, int flags)
{
  pid = waitpid (pid, status, flags);
  if (pid < 0)
    die (errno, "waitpid()");
  return pid;
}
uid_t
xgeteuid (void)
{
  /* POSIX says identification functions (getuid, getgid, and
     others) cannot fail, but they can fail under GNU/Hurd and a
     few other systems.  Test for failure by checking errno.  */
  uid_t uid;

  errno = 0;
  uid = geteuid ();
  if (uid == (uid_t) -1 && errno != 0)
    die (errno, "cannot get effective UID");
  return uid;
}
void
xexecvp (char *file, char **argv)
{
  execvp (file, argv);
  die (errno, "execvp()"); /* Unreachable on success.  */
}
