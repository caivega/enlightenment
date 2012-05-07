#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "evas_cserve2.h"

#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

struct _Slave_Proc
{
   pid_t pid;
   const char *name;
   int write_fd;
   int read_fd;
   Slave_Read_Cb read_cb;
   Slave_Dead_Cb dead_cb;
   const void *data;
   Eina_Binbuf *pending;

   struct {
      int size;
      int read_size;
      Slave_Command cmd;
      char *buf;
   } read;

   Eina_Bool killed : 1;
};

static Eina_List *slaves;

static Slave_Proc *
_child_find(pid_t pid)
{
   Eina_List *l;
   Slave_Proc *s;

   EINA_LIST_FOREACH(slaves, l, s)
      if (s->pid == pid)
        return s;
   return NULL;
}

static void
_slave_free(Slave_Proc *s)
{
   if (s->write_fd)
     close(s->write_fd);
   if (s->read_fd)
     {
        cserve2_fd_watch_del(s->read_fd);
        close(s->read_fd);
     }

   free(s->read.buf);

   if (s->pending)
     eina_binbuf_free(s->pending);

   if (s->dead_cb)
     s->dead_cb(s, (void *)s->data);

   eina_stringshare_del(s->name);

   free(s);
}

static void
_slave_dead_cb(int pid, int status __UNUSED__)
{
   Slave_Proc *s;

   DBG("Child dead with pid '%d'.", pid);
   s = _child_find(pid);
   if (!s)
     {
        ERR("Unknown child dead '%d'.", pid);
        return;
     }

   slaves = eina_list_remove(slaves, s);
   _slave_free(s);
}

static size_t
_slave_write(Slave_Proc *s, const char *data, size_t size)
{
   size_t sent = 0;

   do
     {
        ssize_t ret;
        ret = write(s->write_fd, data + sent, size - sent);
        if (ret == -1)
          {
             if (errno == EAGAIN)
               break;
             if (errno == EPIPE)
               {
                  WRN("Slave unexpectedly gone.");
                  /* handle dead? */
                  break;
               }
          }
        sent += ret;
     } while(sent < size);

   return sent;
}

static void
_slave_write_cb(int fd __UNUSED__, Fd_Flags flags __UNUSED__, void *data)
{
   Slave_Proc *s = data;
   size_t sent;
   size_t size;
   const char *str;

   size = eina_binbuf_length_get(s->pending);
   str = (const char *)eina_binbuf_string_get(s->pending);
   sent = _slave_write(s, str, size);
   if (sent == size)
     {
        eina_binbuf_free(s->pending);
        s->pending = NULL;
        cserve2_fd_watch_del(s->write_fd);
        return;
     }

   eina_binbuf_remove(s->pending, 0, sent);
}

static void
_slave_read_clear(Slave_Proc *s)
{
   s->read.buf = NULL;
   s->read.cmd = 0;
   s->read.read_size = s->read.size = 0;
}

static void
_slave_read_cb(int fd, Fd_Flags flags, void *data)
{
   Slave_Proc *s = data;
   Eina_Bool done = EINA_FALSE;

   /* handle error */
   if (!(flags & FD_READ))
     return;

   if (!s->read.size)
     {
        int ints[2];
        ssize_t ret;

        ret = read(fd, ints, sizeof(int) * 2);
        if (ret < (int)sizeof(int) * 2)
          {
             return;
          }
        s->read.size = ints[0];
        s->read.cmd = ints[1];
        if (s->read.size)
          s->read.buf = malloc(s->read.size);
        else
          done = EINA_TRUE;
     }

   if (s->read.buf)
     {
        ssize_t ret;
        do {
             char *p = s->read.buf + s->read.read_size;
             int sz = s->read.size - s->read.read_size;
             ret = read(fd, p, sz);
             if (ret < 0)
               {
                  if (errno == EAGAIN)
                    break;
               }
             s->read.read_size += ret;
        } while(s->read.read_size < s->read.size);

        if (s->read.read_size == s->read.size)
          done = EINA_TRUE;
     }

   if (done)
     {
        s->read_cb(s, s->read.cmd, s->read.buf, (void *)s->data);
        _slave_read_clear(s);
     }
}

Eina_Bool
cserve2_slaves_init(void)
{
   cserve2_on_child_dead_set(_slave_dead_cb);
   return EINA_TRUE;
}

void
cserve2_slaves_shutdown(void)
{
   Slave_Proc *s;

   cserve2_on_child_dead_set(NULL);

   if (!slaves)
     return;

   DBG("Shutting down slaves subsystem with %d slaves alive!",
       eina_list_count(slaves));

   EINA_LIST_FREE(slaves, s)
     {
        kill(s->pid, SIGKILL);
        _slave_free(s);
     }
}

static const char *
_slave_path_get(const char *name)
{
   char buf[PATH_MAX], cwd[PATH_MAX];

   if (name[0] == '/')
     {
        if (access(name, X_OK))
          return NULL;
        return eina_stringshare_add(name);
     }

   getcwd(cwd, sizeof(cwd));
   snprintf(buf, sizeof(buf), "%s/%s", cwd, name);
   if (!access(buf, X_OK))
     return eina_stringshare_add(buf);

   snprintf(buf, sizeof(buf), PACKAGE_LIBEXEC_DIR"/%s", name);
   if (!access(buf, X_OK))
     return eina_stringshare_add(buf);

   return NULL;
}

Slave_Proc *
cserve2_slave_run(const char *exe, Slave_Read_Cb read_cb, Slave_Dead_Cb dead_cb, const void *data)
{
   Slave_Proc *s;
   pid_t pid;
   int child[2], parent[2];
   int flags;
   const char *name;

   name = _slave_path_get(exe);
   if (!name)
     {
        ERR("Cannot execute slave '%s'. Not found or not executable.", exe);
        return NULL;
     }
   DBG("Running slave '%s', resolved to path: %s", exe, name);

   s = calloc(1, sizeof(Slave_Proc));
   if (!s)
     {
        ERR("Could not create Slave_Proc handler.");
        eina_stringshare_del(name);
        return NULL;
     }

   if (pipe(child))
     {
        ERR("Could not create pipes for child.");
        eina_stringshare_del(name);
        free(s);
        return NULL;
     }

   if (pipe(parent))
     {
        ERR("Could not create pipes for parent.");
        eina_stringshare_del(name);
        free(s);
        close(child[0]);
        close(child[1]);
        return NULL;
     }

   pid = fork();
   if (pid < 0)
     {
        ERR("Could not create sub process.");
        eina_stringshare_del(name);
        close(child[0]);
        close(child[1]);
        close(parent[0]);
        close(parent[1]);
        free(s);
        return NULL;
     }

   if (!pid)
     {
        char *args[4], readfd[12], writefd[12];

        close(child[1]);
        close(parent[0]);

        sprintf(readfd, "%d", child[0]);
        sprintf(writefd, "%d", parent[1]);
        args[0] = (char *)name;
        args[1] = writefd;
        args[2] = readfd;
        args[3] = NULL;
        execvp(name, args);
        /* we only get here if execvp fails, which should not
         * happen and if it does, it's baaaaaaaaad */
        ERR("execvp() for slave at: '%s' failed! '%m'", name);
        exit(1);
     }

   s->pid = pid;
   s->name = name;
   s->write_fd = child[1];
   flags = fcntl(s->write_fd, F_GETFL);
   flags |= O_NONBLOCK;
   fcntl(s->write_fd, F_SETFL, flags);
   s->read_fd = parent[0];
   flags = fcntl(s->read_fd, F_GETFL);
   flags |= O_NONBLOCK;
   fcntl(s->read_fd, F_SETFL, flags);
   s->read_cb = read_cb;
   s->dead_cb = dead_cb;
   s->data = data;
   cserve2_fd_watch_add(s->read_fd, FD_READ, _slave_read_cb, s);

   close(child[0]);
   close(parent[1]);

   slaves = eina_list_append(slaves, s);

   return s;
}

static void
_slave_send_aux(Slave_Proc *s, const char *data, size_t size)
{
   size_t sent;

   if (s->pending)
     {
        eina_binbuf_append_length(s->pending, (unsigned char *)data, size);
        return;
     }

   sent = _slave_write(s, data, size);
   if (sent < size)
     {
        s->pending = eina_binbuf_new();
        eina_binbuf_append_length(s->pending, (unsigned char *)data + sent,
                                  size - sent);
        cserve2_fd_watch_add(s->write_fd, FD_WRITE, _slave_write_cb, s);
     }
}

void
cserve2_slave_send(Slave_Proc *s, Slave_Command cmd, const char *data, size_t size)
{
   int ints[2];

   ints[0] = size;
   ints[1] = cmd;
   _slave_send_aux(s, (char *)ints, sizeof(int) * 2);
   if (size)
     _slave_send_aux(s, (char *)data, size);
}

void
cserve2_slave_kill(Slave_Proc *s)
{
   if (s->killed)
     {
        if (!kill(s->pid, 0))
          DBG("Slave %p(%d) requested to kill, but it's still alive.",
              s, s->pid);
     }

   s->killed = EINA_TRUE;
   kill(s->pid, SIGTERM);
}
