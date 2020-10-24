
/* Public domain.  */

#ifndef _UTILS
#define _UTILS 1

#include <stddef.h>
#include <sys/types.h>

#if defined(__FreeBSD__)
# include <err.h>
# define die(errno, ...) err (errno, __VA_ARGS__), exit (EXIT_FAILURE)
#else
# include <error.h>
# define die(errno, ...) error (EXIT_FAILURE, errno, __VA_ARGS__)
#endif

extern void *xmalloc (size_t s);
extern pid_t xfork (void);
extern pid_t xwaitpid (pid_t pid, int *status, int flags);
extern uid_t xgeteuid (void);
extern void xexecvp (char *file, char **argv);

#endif /* _UTILS */
