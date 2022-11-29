#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "quickjs.h"
#include "foo_lib.h"
#include "ld_magic.h"

EXTLD(my_foo_lib_mjs)

JSValue commonjs_module_data_to_function(JSContext *ctx, const uint8_t *data, size_t data_length, const char *function_name)
{
    JSValue result = JS_UNDEFINED;
    char * module_function_name = NULL;

    // Make sure you properly free all JSValues created from this procedure

    if(data == NULL) {
        goto done;
    }

    /**
     * To pull the script objects, including require_thing() etc, into global scope,
     * load the patched NodeJS script from the object file embedded in the binary
     */
    result = JS_Eval(ctx, data, data_length-1, "<embed>", JS_EVAL_TYPE_GLOBAL);

    if(JS_IsException(result)) {
        printf("failed to parse module function '%s'\n", function_name);
        printf("exception: %s\n", JS_ToCString(ctx, JS_GetException(ctx)));
        goto cleanup_fail;
    }

    JSValue global = JS_GetGlobalObject(ctx);

    /**
     * Automatically create the require_thing() function name
     */
    asprintf(&module_function_name, "require_%s", function_name);
    JSValue module = JS_GetPropertyStr(ctx, global, module_function_name);
    if(JS_IsException(module)) {
        printf("failed to find %s module function\n", function_name);
        goto cleanup_fail;
    }
    result = JS_Call(ctx, module, global, 0, NULL);
    if(JS_IsException(result)) {
        goto cleanup_fail;
    }

    /* don't lose the object we've built by passing over failure case */
    goto done;

cleanup_fail:
    /* nothing to do, cleanup context elsewhere */
    result = JS_UNDEFINED;

done:
    free(module_function_name);
    return result;
}

// A simple helper for getting a JSContext
JSContext * easy_context(void)
{
    JSRuntime *runtime = JS_NewRuntime();
    if(runtime == NULL) {
        puts("unable to create JS Runtime");
        goto cleanup_content_fail;
    }

    JSContext *ctx = JS_NewContext(runtime);
    if(ctx == NULL) {
        puts("unable to create JS context");
        goto cleanup_runtime_fail;
    }
    return ctx;

cleanup_runtime_fail:
    free(runtime);

cleanup_content_fail:
    return NULL;

}


int call_foo(int bar, int baz)
{
    JSContext *ctx = easy_context();
    JSValue global = JS_GetGlobalObject(ctx);

    /**
     * esbuild output was to my-foo-lib.mjs, so symbols will be named with my_foo_lib_mjs
     */
    JSValue foo_fn = commonjs_module_data_to_function(
        ctx
        , LDVAR(my_foo_lib_mjs) // gcc/Linux-specific naming
        , LDLEN(my_foo_lib_mjs)
        , "foo"
    );

    if(JS_IsUndefined(foo_fn)) {
        printf("couldn't find foo to call\n");
        return -1;
    }
    
    /**
     * To create more complex objects as arguments, use 
     *   JS_ParseJSON(ctx, json_str, strlen(json_str), "<input>");
     */
    JSValue args[] = {
        JS_NewInt32(ctx, (int32_t) bar),
        JS_NewInt32(ctx, (int32_t) baz)
    };

    JSValue js_result = JS_Call(ctx
        , foo_fn
        , global
        , sizeof(args)/sizeof(*args)
        , args
    );

    if(JS_IsUndefined(js_result)) {
        puts("function call failed\n");
        return -1;
    }

    int32_t c_result = -1;

    JS_ToInt32(ctx, &c_result, js_result);

    return c_result;
       
}
