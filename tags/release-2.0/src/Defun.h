/***********************************************************************
 *
 * Copyright (c) 2006, 2010 Karoly Lorentey
 *
 * File: Defun.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***********************************************************************/

#undef DEFUN0
#undef DEFUN1
#undef DEFUN2
#undef DEFUN3
#undef DEFUNN
#undef DEFUNV
#undef DEFUNP

#ifndef ENABLE_BACKGROUND_MODE

#   if defined (GLOBALS_INCLUDE) || defined (INITIALIZER_INCLUDE)

#       define DEFUN0(res, name)
#       define DEFUN1(res, name, p1)
#       define DEFUN2(res, name, p1, p2)
#       define DEFUN3(res, name, p1, p2, p3)
#       define DEFUNN(res, name, ...)
#       define DEFUNV(res, name, ...)
#       define DEFUNP(res, name, ...)

#   elif defined (DEFUN_DEFINES)

#       define DEFUN0(res, name) @define name() f_##name (g)
#       define DEFUN1(res, name, p1) @define name(arg) f_##name (g, arg)
#       define DEFUN2(res, name, p1, p2) @define name(arg1, arg2) f_##name (g, arg1, arg2)
#       define DEFUN3(res, name, p1, p2, p3) @define name(arg1, arg2, arg3) f_##name (g, arg1, arg2, arg3)
#       define DEFUNN(res, name, ...) @define name(...) f_##name (g, %%VA%ARGS%%)
#       define DEFUNV(res, name, ...) @define name(...) f_##name (g, %%VA%ARGS%%)
#       define DEFUNP(i, c, res, name, ...) @define name(...) f_##name (g, %%VA%ARGS%%)

#   elif defined (PRIVATE_INCLUDE) || defined (NORMAL_INCLUDE)

#       define DEFUN0(res, name) res f_##name (struct Globals *g) SECTION
#       define DEFUN1(res, name, p1) res f_##name (struct Globals *g, p1) SECTION
#       define DEFUN2(res, name, p1, p2) res f_##name (struct Globals *g, p1, p2) SECTION
#       define DEFUN3(res, name, p1, p2, p3) res f_##name (struct Globals *g, p1, p2, p3) SECTION
#       define DEFUNN(res, name, ...) res f_##name (struct Globals *g, __VA_ARGS__) SECTION
#       define DEFUNV(res, name, ...) res f_##name (struct Globals *g, __VA_ARGS__,  ...) SECTION
#       define DEFUNP(i, c, res, name, ...) res f_##name (struct Globals *g, __VA_ARGS__,  ...) SECTION __attribute__ ((format (printf, i + 1, c + 1)))
#   else
#       error Missing DEFUN define
#   endif
#else  /* ENABLE_BACKGROUND_MODE */


#   define DEFUNP(i, c, res, name, ...) DEFUNV (res, name, __VA_ARGS__)

#   if defined (GLOBALS_INCLUDE)
/* When collecting globals, DEFUNs become function pointer member
 * declarations.  */
#       define DEFUN0(res, name) res (*f_ ## name) (struct Globals *g)
#       define DEFUN1(res, name, p1) res (*f_ ## name) (struct Globals *g, p1)
#       define DEFUN2(res, name, p1, p2) res (*f_ ## name) (struct Globals *g, p1, p2)
#       define DEFUN3(res, name, p1, p2, p3) res (*f_ ## name) (struct Globals *g, p1, p2, p3)
#       define DEFUNN(res, name, ...) res (*f_ ## name) (struct Globals *g, __VA_ARGS__)
#       define DEFUNV(res, name, ...) res (*f_ ## name) (struct Globals *g, __VA_ARGS__, ...)

#   elif defined (DEFUN_DEFINES)

#       define DEFUN0(res, name) @define name() g->f_##name (g)
#       define DEFUN1(res, name, p1) @define name(arg) g->f_##name (g, arg)
#       define DEFUN2(res, name, p1, p2) @define name(arg1, arg2) g->f_##name (g, arg1, arg2)
#       define DEFUN3(res, name, p1, p2, p3) @define name(arg1, arg2, arg3) g->f_##name (g, arg1, arg2, arg3)
#       define DEFUNN(res, name, ...) @define name(...) g->f_##name (g, %%VA%ARGS%%)
#       define DEFUNV(res, name, ...) @define name(...) g->f_##name (g, %%VA%ARGS%%)

#   elif defined (PRIVATE_INCLUDE)

/* When included in the implementation file, DEFUNs become function
 * declarations.  */
#       define DEFUN0(res, name) res f_##name (struct Globals *g) SECTION
#       define DEFUN1(res, name, p1) res f_##name (struct Globals *g, p1) SECTION
#       define DEFUN2(res, name, p1, p2) res f_##name (struct Globals *g, p1, p2) SECTION
#       define DEFUN3(res, name, p1, p2, p3) res f_##name (struct Globals *g, p1, p2, p3) SECTION
#       define DEFUNN(res, name, ...) res f_##name (struct Globals *g, __VA_ARGS__) SECTION
#       define DEFUNV(res, name, ...) res f_##name (struct Globals *g, __VA_ARGS__,  ...) SECTION

#   elif defined (INITIALIZER_INCLUDE)

/* When included in the initializer file, DEFUNs become function
 * slot initializations.  */
#       define DEFUN0(res, name)                g->f_##name = f_##name
#       define DEFUN1(res, name, p1)            g->f_##name = f_##name
#       define DEFUN2(res, name, p1, p2)        g->f_##name = f_##name
#       define DEFUN3(res, name, p1, p2, p3)    g->f_##name = f_##name
#       define DEFUNN(res, name, ...)           g->f_##name = f_##name
#       define DEFUNV(res, name, ...)           g->f_##name = f_##name

#   elif defined (NORMAL_INCLUDE)

/* Otherwise DEFUNs are empty.  */
#       define DEFUN0(res, name)
#       define DEFUN1(res, name, p1)
#       define DEFUN2(res, name, p1, p2)
#       define DEFUN3(res, name, p1, p2, p3)
#       define DEFUNN(res, name, ...)
#       define DEFUNV(res, name, ...)

#   else
#       error Missing DEFUN define
#   endif

#endif  /* ENABLE_BACKGROUND_MODE */
