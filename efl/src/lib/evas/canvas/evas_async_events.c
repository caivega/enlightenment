#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _MSC_VER
# include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include "evas_common.h"
#include "evas_private.h"

static int _fd_write = -1;
static int _fd_read = -1;
static pid_t _fd_pid = 0;

static int _init_evas_event = 0;

typedef struct _Evas_Event_Async	Evas_Event_Async;

struct _Evas_Event_Async
{
   const void		    *target;
   void			    *event_info;
   Evas_Async_Events_Put_Cb  func;
   Evas_Callback_Type	     type;
};

Eina_Bool
_evas_fd_close_on_exec(int fd)
{
#ifdef HAVE_EXECVP
   int flags;

   flags = fcntl(fd, F_GETFD);
   if (flags == -1)
     return EINA_FALSE;

   flags |= FD_CLOEXEC;
   if (fcntl(fd, F_SETFD, flags) == -1)
     return EINA_FALSE;
   return EINA_TRUE;
#else
   (void) fd;
   return EINA_FALSE;
#endif
}

int
evas_async_events_init(void)
{
   int filedes[2];

   _init_evas_event++;
   if (_init_evas_event > 1) return _init_evas_event;

   _fd_pid = getpid();
   
   if (pipe(filedes) == -1)
     {
	_init_evas_event = 0;
	return 0;
     }

   _evas_fd_close_on_exec(filedes[0]);
   _evas_fd_close_on_exec(filedes[1]);

   _fd_read = filedes[0];
   _fd_write = filedes[1];

   fcntl(_fd_read, F_SETFL, O_NONBLOCK);

   return _init_evas_event;
}

int
evas_async_events_shutdown(void)
{
   _init_evas_event--;
   if (_init_evas_event > 0) return _init_evas_event;

   close(_fd_read);
   close(_fd_write);
   _fd_read = -1;
   _fd_write = -1;

   return _init_evas_event;
}

static void
_evas_async_events_fork_handle(void)
{
   int i, count = _init_evas_event;
   
   if (getpid() == _fd_pid) return;
   for (i = 0; i < count; i++) evas_async_events_shutdown();
   for (i = 0; i < count; i++) evas_async_events_init();
}

EAPI int
evas_async_events_fd_get(void)
{
   _evas_async_events_fork_handle();
   return _fd_read;
}

static int
_evas_async_events_process_single(void)
{
   Evas_Event_Async *ev;
   int ret;

   ret = read(_fd_read, &ev, sizeof(Evas_Event_Async *));
   if (ret == sizeof(Evas_Event_Async *))
     {
        if (ev->func) ev->func((void *)ev->target, ev->type, ev->event_info);
        free(ev);
        return 1;
     }
   else if (ret < 0)
     {
        switch (errno)
          {
           case EBADF:
           case EINVAL:
           case EIO:
           case EISDIR:
              _fd_read = -1;
          }
     }

   return ret;
}

EAPI int
evas_async_events_process(void)
{
   int count = 0;

   if (_fd_read == -1) return 0;

   _evas_async_events_fork_handle();

   while (_evas_async_events_process_single() > 0) count++;

   evas_cache_image_wakeup();

   return count;
}

static void
_evas_async_events_fd_blocking_set(Eina_Bool blocking)
{
   long flags = fcntl(_fd_read, F_GETFL);

   if (blocking) flags &= ~O_NONBLOCK;
   else flags |= O_NONBLOCK;

   fcntl(_fd_read, F_SETFL, flags);
}

int
evas_async_events_process_blocking(void)
{
   int ret;

   _evas_async_events_fork_handle();

   _evas_async_events_fd_blocking_set(EINA_TRUE);
   ret = _evas_async_events_process_single();
   evas_cache_image_wakeup(); /* FIXME: is this needed ? */
   _evas_async_events_fd_blocking_set(EINA_FALSE);

   return ret;
}

EAPI Eina_Bool
evas_async_events_put(const void *target, Evas_Callback_Type type, void *event_info, Evas_Async_Events_Put_Cb func)
{
   Evas_Event_Async *ev;
   ssize_t check;
   Eina_Bool result = EINA_FALSE;

   if (!func) return 0;
   if (_fd_write == -1) return 0;

   _evas_async_events_fork_handle();
   
   ev = calloc(1, sizeof (Evas_Event_Async));
   if (!ev) return 0;

   ev->func = func;
   ev->target = target;
   ev->type = type;
   ev->event_info = event_info;

   do
     {
        check = write(_fd_write, &ev, sizeof (Evas_Event_Async*));
     }
   while ((check != sizeof (Evas_Event_Async*)) &&
          ((errno == EINTR) || (errno == EAGAIN)));

   evas_cache_image_wakeup();

   if (check == sizeof (Evas_Event_Async*))
     result = EINA_TRUE;
   else
     {
        switch (errno)
          {
           case EBADF:
           case EINVAL:
           case EIO:
           case EPIPE:
             _fd_write = -1;
          }
     }

   return result;
}
