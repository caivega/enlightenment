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

#ifndef __DEFINES__H__
#define __DEFINES__H__

//MACRO FUNCTIONS
#define PRINT_DBG(string) \
   do \
     { \
        fprintf(stderr, "%s - %s(%d)\n", string, __func__, __LINE__); \
     } \
   while(0)

//Singleton Instances
#define MEMMGR MemoryMgr ::GetInstance()

//ID VALUES
#define ID_BLOCK         1
#define ID_BOMB          2
#define ID_PLAYER        3

//CONST VARIABLES
#define GRID_SIZE        50
#define MEMPOOL_SIZE     (sizeof(int) * 10000)

//MOUSE BUTTON FIELDS
#define MOUSE_BUTTON1 0x01
#define MOUSE_BUTTON2 0x02

#define SCRN_WIDTH 800
#define SCRN_HEIGHT 800

typedef float ELEMENT_TYPE;
template<typename T> struct Vector2;
typedef Vector2<ELEMENT_TYPE> VECTOR2;

#endif

