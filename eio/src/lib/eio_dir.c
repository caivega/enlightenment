/* EIO - EFL data type library
 * Copyright (C) 2010 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */
#include "eio_private.h"
#include "Eio.h"

static int
eio_strcmp(const void *a, const void *b)
{
   return strcmp(a, b);
}

static Eina_Bool
_eio_dir_recursiv_ls(Ecore_Thread *thread, Eio_Dir_Copy *copy, const char *target)
{
   const Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_List *dirs = NULL;
   Eina_List *l;
   const char *dir;
   struct stat buffer;

   it = eina_file_direct_ls(target);
   if (!it)
     {
        eio_file_thread_error(&copy->progress.common, thread);
        return EINA_FALSE;
     }

   EINA_ITERATOR_FOREACH(it, info)
     {
        switch (info->dirent->d_type)
          {
           case DT_UNKNOWN:
              if (stat(info->path, &buffer) != 0)
                {
                   eio_file_thread_error(&copy->progress.common, thread);
                   goto on_error;
                }

              if (S_ISDIR(buffer.st_mode))
                dirs = eina_list_append(dirs, eina_stringshare_add(info->path));
              else
                copy->files = eina_list_append(copy->files, eina_stringshare_add(info->path));
              break;
           case DT_DIR:
              dirs = eina_list_append(dirs, eina_stringshare_add(info->path));
              break;
           default:
              copy->files = eina_list_append(copy->files, eina_stringshare_add(info->path));
              break;
          }

        if (ecore_thread_check(thread))
          goto on_error;
     }

   eina_iterator_free(it);

   EINA_LIST_FOREACH(dirs, l, dir)
     if (!_eio_dir_recursiv_ls(thread, copy, dir))
       {
          EINA_LIST_FREE(dirs, dir)
            eina_stringshare_del(dir);
          return EINA_FALSE;
       }

   copy->dirs = eina_list_merge(copy->dirs, dirs);
   return EINA_TRUE;

 on_error:
   eina_iterator_free(it);

   EINA_LIST_FREE(dirs, dir)
     eina_stringshare_del(dir);

   return EINA_FALSE;
}

static Eina_Bool
_eio_dir_init(Ecore_Thread *thread,
              off_t *step, off_t *count,
              int *length_source, int *length_dest,
              Eio_Dir_Copy *order,
              Eio_File_Progress *progress)
{
   struct stat buffer;

   /* notify main thread of the amount of work todo */
   *step = 0;
   *count = eina_list_count(order->files) + eina_list_count(order->dirs) * 2;
   eio_progress_send(thread, &order->progress, *step, *count);

   /* sort the content, so we create the directory in the right order */
   order->dirs = eina_list_sort(order->dirs, -1, eio_strcmp);
   order->files = eina_list_sort(order->files, -1, eio_strcmp);

   /* prepare stuff */
   *length_source = eina_stringshare_strlen(order->progress.source);
   *length_dest = eina_stringshare_strlen(order->progress.dest);

   memcpy(progress, &order->progress, sizeof (Eio_File_Progress));
   progress->source = NULL;
   progress->dest = NULL;

   /* create destination dir if not available */
   if (stat(order->progress.dest, &buffer) != 0)
     {
        if (stat(order->progress.source, &buffer) != 0)
          {
             eio_file_thread_error(&order->progress.common, thread);
             return EINA_FALSE;
          }

        if (mkdir(order->progress.dest, buffer.st_mode) != 0)
          {
             eio_file_thread_error(&order->progress.common, thread);
             return EINA_FALSE;
          }
     }

   return EINA_TRUE;
}

static void
_eio_dir_target(Eio_Dir_Copy *order, char *target, const char *dir, int length_source, int length_dest)
{
   int length;

   length = eina_stringshare_strlen(dir);

   memcpy(target, order->progress.dest, length_dest);
   target[length_dest] = '/';
   memcpy(target + length_dest + 1, dir + length_source, length - length_source + 1);
}

static Eina_Bool
_eio_dir_mkdir(Ecore_Thread *thread, Eio_Dir_Copy *order,
               off_t *step, off_t count,
               int length_source, int length_dest)
{
   const char *dir;
   Eina_List *l;
   char target[PATH_MAX];

   /* create all directory */
   EINA_LIST_FOREACH(order->dirs, l, dir)
     {
        /* build target dir path */
        _eio_dir_target(order, target, dir, length_source, length_dest);

        /* create the directory (we will apply the mode later) */
        if (mkdir(target, 0777) != 0)
          {
             eio_file_thread_error(&order->progress.common, thread);
             return EINA_FALSE;
          }

        /* inform main thread */
        (*step)++;
        eio_progress_send(thread, &order->progress, *step, count);

        /* check for cancel request */
        if (ecore_thread_check(thread))
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool
_eio_dir_chmod(Ecore_Thread *thread, Eio_Dir_Copy *order,
               off_t *step, off_t count,
               int length_source, int length_dest,
               Eina_Bool rmdir_source)
{
   const char *dir;
   char target[PATH_MAX];
   struct stat buffer;

   while(order->dirs)
     {
        /* destroy in reverse order so that we don't prevent change of lower dir */
        dir = eina_list_data_get(eina_list_last(order->dirs));
        order->dirs = eina_list_remove_list(order->dirs, eina_list_last(order->dirs));

        /* build target dir path */
        _eio_dir_target(order, target, dir, length_source, length_dest);

        /* FIXME: in some case we already did a stat call, so would be nice to reuse previous result here */
        /* stat the original dir for mode info */
        if (stat(dir, &buffer) != 0)
          goto on_error;

        /* set the orginal mode to the newly created dir */
        if (chmod(target, buffer.st_mode) != 0)
          goto on_error;

        /* if required destroy original directory */
        if (rmdir_source)
          {
             if (rmdir(dir) != 0)
               goto on_error;
          }

        /* inform main thread */
        (*step)++;
        eio_progress_send(thread, &order->progress, *step, count);

        /* check for cancel request */
        if (ecore_thread_check(thread))
          goto on_cancel;

        eina_stringshare_del(dir);
     }

   return EINA_TRUE;

 on_error:
   eio_file_thread_error(&order->progress.common, thread);
 on_cancel:
   if (dir) eina_stringshare_del(dir);
   return EINA_FALSE;
}

static void
_eio_dir_copy_heavy(Ecore_Thread *thread, void *data)
{
   Eio_Dir_Copy *copy = data;
   const char *file = NULL;
   const char *dir = NULL;

   Eio_File_Progress file_copy;
   char target[PATH_MAX];

   int length_source = 0;
   int length_dest = 0;
   off_t count;
   off_t step;

   /* list all the content that should be copied */
   if (!_eio_dir_recursiv_ls(thread, copy, copy->progress.source))
     return ;

   /* init all structure needed to copy the file */
   if (!_eio_dir_init(thread, &step, &count, &length_source, &length_dest, copy, &file_copy))
     goto on_error;

   /* suboperation is a file copy */
   file_copy.op = EIO_FILE_COPY;

   /* create all directory */
   if (!_eio_dir_mkdir(thread, copy, &step, count, length_source, length_dest))
     goto on_error;

   /* copy all files */
   EINA_LIST_FREE(copy->files, file)
     {
        /* build target file path */
        _eio_dir_target(copy, target, file, length_source, length_dest);

        file_copy.source = file;
        file_copy.dest = eina_stringshare_add(target);

        /* copy the file */
        if (!eio_file_copy_do(thread, &file_copy))
          {
             copy->progress.common.error = file_copy.common.error;
             goto on_error;
          }

        /* notify main thread */
        step++;
        eio_progress_send(thread, &copy->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(file_copy.dest);
        eina_stringshare_del(file);
     }
   file_copy.dest = NULL;
   file = NULL;

   /* set directory right back */
   if (!_eio_dir_chmod(thread, copy, &step, count, length_source, length_dest, EINA_FALSE))
     goto on_error;

 on_error:
   /* cleanup the mess */
   if (file_copy.dest) eina_stringshare_del(file_copy.dest);
   if (file) eina_stringshare_del(file);

   EINA_LIST_FREE(copy->files, file)
     eina_stringshare_del(file);
   EINA_LIST_FREE(copy->dirs, dir)
     eina_stringshare_del(dir);

   if (!ecore_thread_check(thread))
     eio_progress_send(thread, &copy->progress, count, count);

   return ;
}

static void
_eio_dir_copy_notify(Ecore_Thread *thread __UNUSED__, void *msg_data, void *data)
{
   Eio_Dir_Copy *copy = data;
   Eio_Progress *progress = msg_data;

   eio_progress_cb(progress, &copy->progress);
}

static void
_eio_dir_copy_free(Eio_Dir_Copy *copy)
{
   eina_stringshare_del(copy->progress.source);
   eina_stringshare_del(copy->progress.dest);
   free(copy);
}

static void
_eio_dir_copy_end(void *data)
{
   Eio_Dir_Copy *copy = data;

   copy->progress.common.done_cb((void*) copy->progress.common.data);

   _eio_dir_copy_free(copy);
}

static void
_eio_dir_copy_error(void *data)
{
   Eio_Dir_Copy *copy = data;

   eio_file_error(&copy->progress.common);

   _eio_dir_copy_free(copy);
}

static void
_eio_dir_move_heavy(Ecore_Thread *thread, void *data)
{
   Eio_Dir_Copy *move = data;
   const char *file = NULL;
   const char *dir = NULL;

   Eio_File_Progress file_move;
   char target[PATH_MAX];

   int length_source;
   int length_dest;
   off_t count;
   off_t step;

   /* just try a rename, maybe we are lucky... */
   if (rename(move->progress.source, move->progress.dest) == 0)
     {
        /* we are really lucky */
        eio_progress_send(thread, &move->progress, 1, 1);
        return ;
     }

   /* list all the content that should be moved */
   if (!_eio_dir_recursiv_ls(thread, move, move->progress.source))
     return ;

   /* init all structure needed to move the file */
   if (!_eio_dir_init(thread, &step, &count, &length_source, &length_dest, move, &file_move))
     goto on_error;

   /* sub operation is a file move */
   file_move.op = EIO_FILE_MOVE;

   /* create all directory */
   if (!_eio_dir_mkdir(thread, move, &step, count, length_source, length_dest))
     goto on_error;

   /* move file around */
   EINA_LIST_FREE(move->files, file)
     {
        /* build target file path */
        _eio_dir_target(move, target, file, length_source, length_dest);

        file_move.source = file;
        file_move.dest = eina_stringshare_add(target);

        /* first try to rename */
        if (rename(file_move.source, file_move.dest) < 0)
          {
             if (errno != EXDEV)
               {
                  eio_file_thread_error(&move->progress.common, thread);
                  goto on_error;
               }

             /* then try real copy */
             if (!eio_file_copy_do(thread, &file_move))
               {
                  move->progress.common.error = file_move.common.error;
                  goto on_error;
               }

             /* and unlink the original */
             if (unlink(file) != 0)
               {
                  eio_file_thread_error(&move->progress.common, thread);
                  goto on_error;
               }
          }

        step++;
        eio_progress_send(thread, &move->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(file_move.dest);
        eina_stringshare_del(file);
     }
   file_move.dest = NULL;
   file = NULL;

   /* set directory right back */
   if (!_eio_dir_chmod(thread, move, &step, count, length_source, length_dest, EINA_TRUE))
     goto on_error;

   if (rmdir(move->progress.source) != 0)
     goto on_error;

 on_error:
   /* cleanup the mess */
   if (file_move.dest) eina_stringshare_del(file_move.dest);
   if (file) eina_stringshare_del(file);

   EINA_LIST_FREE(move->files, file)
     eina_stringshare_del(file);
   EINA_LIST_FREE(move->dirs, dir)
     eina_stringshare_del(dir);

   if (!ecore_thread_check(thread))
     eio_progress_send(thread, &move->progress, count, count);

   return;
}

static void
_eio_dir_rmrf_heavy(Ecore_Thread *thread, void *data)
{
   Eio_Dir_Copy *rmrf = data;
   const char *file = NULL;
   const char *dir = NULL;

   off_t count;
   off_t step;

   /* list all the content that should be moved */
   if (!_eio_dir_recursiv_ls(thread, rmrf, rmrf->progress.source))
     return ;

   /* init counter */
   step = 0;
   count = eina_list_count(rmrf->files) + eina_list_count(rmrf->dirs) + 1;

   EINA_LIST_FREE(rmrf->files, file)
     {
        if (unlink(file) != 0)
          {
             eio_file_thread_error(&rmrf->progress.common, thread);
             goto on_error;
          }

        eina_stringshare_replace(&rmrf->progress.dest, file);

        step++;
        eio_progress_send(thread, &rmrf->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(file);
     }
   file = NULL;

   EINA_LIST_FREE(rmrf->dirs, dir)
     {
        if (rmdir(dir) != 0)
          {
             eio_file_thread_error(&rmrf->progress.common, thread);
             goto on_error;
          }

        eina_stringshare_replace(&rmrf->progress.dest, dir);

        step++;
        eio_progress_send(thread, &rmrf->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(dir);
     }
   dir = NULL;

   if (rmdir(rmrf->progress.source) != 0)
     goto on_error;
   step++;

 on_error:
   if (dir) eina_stringshare_del(dir);
   if (file) eina_stringshare_del(file);

   EINA_LIST_FREE(rmrf->dirs, dir)
     eina_stringshare_del(dir);
   EINA_LIST_FREE(rmrf->files, file)
     eina_stringshare_del(file);

   if (!ecore_thread_check(thread))
     eio_progress_send(thread, &rmrf->progress, count, count);

   return;
}

/**
 * @addtogroup Eio_Group Asynchronous Inout/Output library
 *
 * @{
 */

/**
 * @brief Copy a directory and it's content asynchronously
 * @param source Should be the name of the directory to copy the data from.
 * @param dest Should be the name of the directory to copy the data to.
 * @param progress_cb Callback called to know the progress of the copy.
 * @param done_cb Callback called when the copy is done.
 * @param error_cb Callback called when something goes wrong.
 * @param data Private data given to callback.
 *
 * This function will copy a directory and all it's content from source to dest.
 * It will try to use splice if possible, if not it will fallback to mmap/write.
 * It will try to preserve access right, but not user/group identity.
 */
EAPI Eio_File *
eio_dir_copy(const char *source,
             const char *dest,
             Eio_Progress_Cb progress_cb,
             Eio_Done_Cb done_cb,
             Eio_Error_Cb error_cb,
             const void *data)
{
   Eio_Dir_Copy *copy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(source, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dest, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   copy = malloc(sizeof(Eio_Dir_Copy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(copy, NULL);

   copy->progress.op = EIO_DIR_COPY;
   copy->progress.progress_cb = progress_cb;
   copy->progress.source = eina_stringshare_add(source);
   copy->progress.dest = eina_stringshare_add(dest);
   copy->files = NULL;
   copy->dirs = NULL;

   if (!eio_long_file_set(&copy->progress.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_copy_heavy,
                          _eio_dir_copy_notify,
                          _eio_dir_copy_end,
                          _eio_dir_copy_error))
     return NULL;

   return &copy->progress.common;
}

/**
 * @brief Move a directory and it's content asynchronously
 * @param source Should be the name of the directory to copy the data from.
 * @param dest Should be the name of the directory to copy the data to.
 * @param progress_cb Callback called to know the progress of the copy.
 * @param done_cb Callback called when the copy is done.
 * @param error_cb Callback called when something goes wrong.
 * @param data Private data given to callback.
 *
 * This function will move a directory and all it's content from source to dest.
 * It will try first to rename the directory, if not it will try to use splice
 * if possible, if not it will fallback to mmap/write.
 * It will try to preserve access right, but not user/group identity.
 */
EAPI Eio_File *
eio_dir_move(const char *source,
             const char *dest,
             Eio_Progress_Cb progress_cb,
             Eio_Done_Cb done_cb,
             Eio_Error_Cb error_cb,
             const void *data)
{
   Eio_Dir_Copy *move;

   EINA_SAFETY_ON_NULL_RETURN_VAL(source, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dest, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   move = malloc(sizeof(Eio_Dir_Copy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(move, NULL);

   move->progress.op = EIO_DIR_MOVE;
   move->progress.progress_cb = progress_cb;
   move->progress.source = eina_stringshare_add(source);
   move->progress.dest = eina_stringshare_add(dest);
   move->files = NULL;
   move->dirs = NULL;

   if (!eio_long_file_set(&move->progress.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_move_heavy,
                          _eio_dir_copy_notify,
                          _eio_dir_copy_end,
                          _eio_dir_copy_error))
     return NULL;

   return &move->progress.common;
}

/**
 * @brief Remove a directory and it's content asynchronously
 * @param path Should be the name of the directory to destroy.
 * @param progress_cb Callback called to know the progress of the copy.
 * @param done_cb Callback called when the copy is done.
 * @param error_cb Callback called when something goes wrong.
 * @param data Private data given to callback.
 *
 * This function will move a directory and all it's content from source to dest.
 * It will try first to rename the directory, if not it will try to use splice
 * if possible, if not it will fallback to mmap/write.
 * It will try to preserve access right, but not user/group identity.
 */
EAPI Eio_File *
eio_dir_unlink(const char *path,
               Eio_Progress_Cb progress_cb,
               Eio_Done_Cb done_cb,
               Eio_Error_Cb error_cb,
               const void *data)
{
   Eio_Dir_Copy *rmrf;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   rmrf = malloc(sizeof(Eio_Dir_Copy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(rmrf, NULL);

   rmrf->progress.op = EIO_UNLINK;
   rmrf->progress.progress_cb = progress_cb;
   rmrf->progress.source = eina_stringshare_add(path);
   rmrf->progress.dest = NULL;
   rmrf->files = NULL;
   rmrf->dirs = NULL;

   if (!eio_long_file_set(&rmrf->progress.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_rmrf_heavy,
                          _eio_dir_copy_notify,
                          _eio_dir_copy_end,
                          _eio_dir_copy_error))
     return NULL;

   return &rmrf->progress.common;
}

/**
 * @}
 */
