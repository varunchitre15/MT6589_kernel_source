/*
  This file is part of ThreadSanitizer, a dynamic data race detector 
  based on Valgrind.

  Copyright (C) 2008-2009 Google Inc
     opensource@google.com 

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307, USA.

  The GNU General Public License is contained in the file COPYING.
*/

// Author: Konstantin Serebryany.
// Note: the rest of ThreadSanitizer is published under the BSD license.

#ifndef TS_VALGRIND_H_
#define TS_VALGRIND_H_

#include <stdint.h>
extern "C" {
#include "pub_tool_basics.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_vki.h"
#include "pub_tool_threadstate.h"
#include "pub_tool_errormgr.h"
#include "pub_tool_options.h"
#include "pub_tool_machine.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_seqmatch.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_options.h"
} // extern "C"
#if defined(VGP_arm_linux)
// A hacky trick to disable the inclusion of bits/string3.h on ARM.
// TODO(glider): this may be specific to Ubuntu 9.10 gcc configuration.
#define __USE_FORTIFY_LEVEL 0
#endif
#endif //  TS_VALGRIND_H_
// {{{1 end
// vim:shiftwidth=2:softtabstop=2:expandtab
