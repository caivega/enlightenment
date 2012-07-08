/*
 * Enlightement Hidden Ninjas.
 *
 * Copyright 2012 Hermet (ChunEon Park)
 *
 * This application is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This application is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

namespace ehninjas
{
   class singleton;
   class MemoryMgr;
   class PlayerChar;

   class App : public Singleton<App>
     {
      private:
         MemoryMgr *memmgr;
         PlayerChar *pc;
         Evas *e;
         Evas_Object *win;
         Evas_Object *bg;
         Eina_Bool initialized;

         Eina_Bool CreateWin(const char *, unsigned int, unsigned int);
         Eina_Bool CreateBg(int, int, int);
         Eina_Bool InitPlayerChar();

      public:
         App();
         ~App();
         Eina_Bool Initialize(int, char **);
         Eina_Bool Run();
         Eina_Bool Terminate();
         void DispatchKeyDown(const char * const);
         void DispatchKeyUp(const char *const);
         void DispatchMouseDown();
         void DispatchMouseUp();
         void DispatchMouseMove(int, int, int);
     };
}
