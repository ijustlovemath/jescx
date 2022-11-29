#pragma once

#include "quickjs.h"
extern int call_foo(int, int);

void list_properties (JSContext *ctx, JSValue map, const char *comment);
