/* slideshow.c
 *
 * Copyright (C) 1999 Tom Gilbert
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "feh.h"
#include "modify.h"

void
init_slideshow_mode (void)
{
  winwidget w = NULL;
  int success = 0;
  char *s = NULL;

  D (("In init_slideshow_mode\n"));

  actual_file_num = file_num;

  for (opt.cur_slide = 0; opt.cur_slide < file_num; opt.cur_slide++)
    {
      s = slideshow_create_name (files[opt.cur_slide]);
      if ((w = winwidget_create_from_file (files[opt.cur_slide], s)) != NULL)
	{
	  free (s);
	  success = 1;
	  if (!opt.progressive)
	    winwidget_show (w);
	  break;
	}
      else
	free (s);
    }
  if (!success)
    show_mini_usage ();
}

void
slideshow_change_image (winwidget winwid, int change)
{
  int i;
  int success = 0;

  D (("In slideshow_change_image\n"));

  /* Without this, clicking a one-image slideshow reloads it. Not very
   * intelligent behaviour :-) */
  if (actual_file_num < 2)
    return;

  /* Ok. I do this in such an odd way to ensure that if the last or first
   * image is not loadable, it will go through in the right direction to
   * find the correct one. Otherwise SLIDE_LAST would try the last file,
   * then loop forward to find a loadable one. */
  if (change == SLIDE_FIRST)
    {
      opt.cur_slide = file_num - 1;
      change = SLIDE_NEXT;
    }
  else if (change == SLIDE_LAST)
    {
      opt.cur_slide = 0;
      change = SLIDE_PREV;
    }

  /* The for loop prevents us looping infinitely */
  for (i = 0; i < file_num; i++)
    {
      if (winwid->im)
	{
	  /* I would leave these in the cache, but its a big mem 
	   * penalty for large slideshows. (In fact it brought down
	   * ljlane's box ;-) */
	  imlib_context_set_image (winwid->im);
	  imlib_free_image_and_decache ();
	}
      if (change == SLIDE_NEXT)
	{
	  opt.cur_slide++;
	  if (opt.cur_slide >= file_num)
	    opt.cur_slide = 0;
	}
      else if (change == SLIDE_PREV)
	{
	  opt.cur_slide--;
	  if (opt.cur_slide < 0)
	    opt.cur_slide = file_num - 1;
	}

      D (("file_num %d, currently %d\n", file_num, opt.cur_slide));
      if (opt.progressive)
	{
	  /* Yeah, we have to do this stuff for progressive loading, so
	   * the callback knows it's got to create a new image... */
	  progwin = winwid;
	  imlib_context_set_progress_function (progress);
	  imlib_context_set_progress_granularity (10);
	  winwid->im_w = 0;
	  winwid->im_h = 0;
	  winwid->w = 0;
	  winwid->h = 0;
	}
      if ((feh_load_image (&(winwid->im), files[opt.cur_slide])) != 0)
	{
	  success = 1;
	  winwid->zoom_mode = 0;
	  winwid->zoom = 0.0;
	  if (!opt.progressive)
	    {
	      imlib_context_set_image (winwid->im);
	      winwid->im_w = imlib_image_get_width ();
	      winwid->im_h = imlib_image_get_height ();
	      winwidget_render_image (winwid);
	      winwidget_create_blank_bg (winwid);
	    }
	  if (winwid->name)
	    {
	      free (winwid->name);
	      winwid->name = slideshow_create_name (files[opt.cur_slide]);
	      winwidget_update_title (winwid);
	    }
	  break;
	}
    }
  if (!success)
    {
      /* We didn't manage to load any files. Maybe the last one in the
       * show was deleted? */
      eprintf ("No more slides in show");
    }
}

char *
slideshow_create_name (char *filename)
{
  char *s = NULL;
  int len = 0;
  len = strlen (PACKAGE " [slideshow mode] - ") + strlen (filename) + 1;
  s = emalloc (len);
  snprintf (s, len, PACKAGE " [%d of %d] - %s", opt.cur_slide + 1,
	    file_num, filename);
  return s;
}

void
handle_keypress_event (XEvent * ev, Window win)
{
  int len;
  static char kbuf[20];
  KeySym keysym;
  XKeyEvent *kev;
  winwidget winwid = NULL;

  D (("In handle_keypress_event\n"));

  winwid = winwidget_get_from_window (win);
  if (winwid == NULL)
    return;

  kev = (XKeyEvent *) ev;
  len =
    XLookupString (&ev->xkey, (char *) kbuf, sizeof (kbuf), &keysym, NULL);

  switch (keysym)
    {
    case XK_Left:
      if (opt.slideshow)
	slideshow_change_image (winwid, SLIDE_PREV);
      break;
    case XK_Right:
      if (opt.slideshow)
	slideshow_change_image (winwid, SLIDE_NEXT);
      break;
    case XK_Delete:
      /* I could do with some confirmation here */
      /* How about holding ctrl? */
      if (opt.slideshow)
	{
	  if (kev->state & ControlMask)
	    {
	      unlink (files[opt.cur_slide]);
	      files[opt.cur_slide] = NULL;
	      actual_file_num--;
	      slideshow_change_image (winwid, SLIDE_NEXT);
	    }
	}
      break;
    case XK_Home:
    case XK_KP_Home:
      if (opt.slideshow)
	slideshow_change_image (winwid, SLIDE_FIRST);
      break;
    case XK_End:
    case XK_KP_End:
      if (opt.slideshow)
	slideshow_change_image (winwid, SLIDE_LAST);
      break;
    default:
      break;
    }

  if (len <= 0 || len > (int) sizeof (kbuf))
    return;

  kbuf[len] = '\0';

  switch (*kbuf)
    {
    case 'n':
    case 'N':
    case ' ':
      if (opt.slideshow)
	slideshow_change_image (winwid, SLIDE_NEXT);
      break;
    case 'p':
    case 'P':
    case '\b':
      if (opt.slideshow)
	slideshow_change_image (winwid, SLIDE_PREV);
      break;
    case 'q':
    case 'Q':
      winwidget_destroy_all ();
      break;
    case '1':
      feh_set_background (winwid);
      break;
    case '2':
      feh_blur (winwid, 10);
      break;
    case '3':
      feh_sharpen (winwid, 10);
      break;
    case '4':
      feh_tile_seamless_v (winwid);
      break;
    case '5':
      feh_tile_seamless_h (winwid);
      break;
    case '6':
      feh_tile_seamless (winwid);
      break;
    case '7':
      feh_rotate_clockwise (winwid);
      break;
    case '8':
      feh_flip_h (winwid);
      break;
    case '9':
      feh_flip_v (winwid);
      break;
    case '0':
      feh_modify_brightness (winwid, 1);
      break;
    case '-':
      feh_modify_gamma (winwid, 1);
      break;
    case '=':
      feh_modify_contrast (winwid, 1);
      break;
    default:
      break;
    }
}
