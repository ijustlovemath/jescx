## Calling JavaScript from C

When you search for "How to call JavaScript from C/C++", you'll find tons of results about how to write JS modules in C, and call them from your Node environment, but almost nothing for the reverse direction. Often times, you'll hear the reason is that "JavaScript is too complex to be used from C", and to extent, that's true. But if you don't mind building (relatively) big binaries, and you have a pretty good understanding of the data types required and returned by the library, you can use QuickJS and esbuild to bundle all of the functionality from your favorite Node.JS libraries into a native library, a static binary, etc.

## How it works (Linux only)

To call JS from C, the general process is:

1. Get QuickJS and esbuild
2. esbuild your desired library/script into an ESM format using CommonJS. This will output one big script with all needed dependencies included.
```bash
output=/path/to/esbuild/output
npx esbuild --bundle /path/to/original/node-library --format=esm --outfile="$output"
```
3. Patch the output of esbuild to make it compatible with QuickJS:

```bash
sed -i 's/Function(\"return this\")()/globalThis/g' $output
sed -i 's@export default@//export default@g' $output
```
4. Load the script text into an object file using your linker:

```bash
ld -r -b binary my_obj_file.o $output
```
Depending on your compiler, this will automatically create 3 symbols in the object file:

    - name_start
    - name_end
    - name_size

`name` in this context is automatically generated from the filename you provided as the last argument to ld. It replaces all non-alphanumeric characters with underscores, so `my-cool-lib.mjs` gives a `name` of  `my_cool_lib_mjs`.

You can use `ld_magic.h` for a cross platform way to access this data from your C code.

After the object file is generated, you should see the patched esbuild output if you run `strings`:

```bash
% strings foo_lib_js.o
var __getOwnPropNames = Object.getOwnPropertyNames;
var __commonJS = (cb, mod) => function __require() {
  return mod || (0, cb[__getOwnPropNames(cb)[0]])((mod = { exports: {} }).exports, mod), mod.exports;
// src/foo.js
var require_foo = __commonJS({
  "src/foo.js"(exports, module) {
    function foo(bar, baz) {
      return bar + baz;
    }
    module.exports = foo;
//export default require_foo();
_binary_my_foo_lib_mjs_end
_binary_my_foo_lib_mjs_start
_binary_my_foo_lib_mjs_size
.symtab
.strtab
.shstrtab
.data
```
5. Link the object file into your binary:
```bash
gcc my_obj_file.o <other object files> -o my_static_binary
```

You can also link the object file into a shared library, for use in other applications:
```bash
gcc -shared -o my_shared_library.so my_obj_file.o  <other object files>`
```

The source of this repo shows how to do this with a CMake project.

## How to actually call the JS functions

Let's say you have a NodeJS library with a function you want to call from C:

```js
// Let's say this lives in foo.js, and esbuild output goes in my-lib-foo.mjs
function foo(bar, baz) {
    return bar + baz
}

module.exports = foo;
```

`esbuild` creates a series of `require_thing()` functions, which can be used to get the underlying `thing(param1, param2...)` function object which you can make calls with.

A simple loader in QuickJS looks like this:

```c
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
    result = JS_Eval(ctx, data, data_length, "<embed>", JS_EVAL_TYPE_GLOBAL);

    if(JS_IsException(result)) {
        printf("failed to parse module function '%s'\n", function_name);
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
```

If you wanted to, for example, get the `foo(bar, baz)` function mentioned above, you would write a function like this:

```c
#include <stdio.h>
#include <inttypes.h>

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
        , _binary_my_foo_lib_mjs_start // gcc/Linux-specific naming
        , _binary_my_foo_lib_mjs_size
        , "foo"
    );
    
    /**
     * To create more complex objects as arguments, use 
     *   JS_ParseJSON(ctx, json_str, strlen(json_str), "<input>");
     * You can also pass callback functions by loading them just like we loaded foo_fn
     */
    JSValue args[] = {
        JS_NewInt32(ctx, bar),
        JS_NewInt32(ctx, baz)
    };

    JSValue js_result = JS_Call(ctx
        , foo_fn
        , global
        , sizeof(args)/sizeof(*args)
        , args
    );

    int32_t c_result = -1;

    JS_ToInt32(ctx, &c_result, js_result);

    return c_result;
       
}
```

## Why should anyone do this?
I'm doing this to add support for [a popular open source glucose control algorithm](https://github.com/OpenAPS/oref0) into a clinical glucose simulator, to see how state of the art controllers perform against it. Since OpenAPS is a fairly active project, and the simulator is written in C++, the two options for using it were:

1. Reimplement OpenAPS in C++ (costly in time, error prone, liable to fall behind future changes)
2. Transpile it to C++ (nothing exists that I could find easily)

So if you have a popular NodeJS library you want to integrate into some sort of native application that's not written using Electron, that's the use case I see this process covering.

Make sure you're only applying this process to source you have an appropriate license to use. I claim no responsibility for open source license violations that occur as a result of using this process.

## Does this work on Windows/Mac?
Conceivably you could use MinGW64 on Windows to have a similar build process, but that's outside my use case, so I haven't invested much time into it. Similarly, I don't have a Mac to test on.

If you want to submit a PR for how you implemented this with your platform, feel free!

## Build Instructions
To have cmake generate makefiles for you, you need to create the object file first, then subsquent times it will generate for you

```bash

# run from your build directory, then run cmake ..
ld -r -b binary -o foo_lib_js.o ../src/my-foo-lib.mjs
```
