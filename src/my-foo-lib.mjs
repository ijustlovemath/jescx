var __getOwnPropNames = Object.getOwnPropertyNames;
var __commonJS = (cb, mod) => function __require() {
  return mod || (0, cb[__getOwnPropNames(cb)[0]])((mod = { exports: {} }).exports, mod), mod.exports;
};

// src/foo.js
var require_foo = __commonJS({
  "src/foo.js"(exports, module) {
    function foo(bar, baz) {
      return bar + baz;
    }
    module.exports = foo;
  }
});
//export default require_foo();
