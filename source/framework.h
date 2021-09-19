#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// Einige CString-Konstruktoren sind explizit.


#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resources\resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
