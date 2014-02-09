// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

#define STRICT
// Change these values to use different versions
#define WINVER		0x0500
#define _WIN32_WINNT	0x0501
#define _WIN32_IE	0x0500
#define _RICHEDIT_VER	0x0100
#define _ATL_SINGLE_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
// ATL
#include <atlcom.h>
//#include <atlhost.h>
#include <atlctl.h>
#include <atlwin.h>

// WTL
#include <atlapp.h>
//#include <atlframe.h>
#include <atlcrack.h>
#include <atlmisc.h>
#include <atldlgs.h>
#include <atlctrls.h>
//#include <atlctrlw.h>
//#include <atlctrlx.h>
//#include <atlsplit.h>

#include "foobar2000/SDK/foobar2000.h"
#include "foobar2000/helpers/helpers.h"

#undef BEGIN_MSG_MAP
#define BEGIN_MSG_MAP(x) BEGIN_MSG_MAP_EX(x)
