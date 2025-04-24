
#include <emscripten.h>
#include <stdlib.h>

EM_JS_DEPS(webidl_binder, "$intArrayFromString,$UTF8ToString,$alignMemory,$addOnInit");

extern "C" {

// Define custom allocator functions that we can force export using
// EMSCRIPTEN_KEEPALIVE.  This avoids all webidl users having to add
// malloc/free to -sEXPORTED_FUNCTIONS.
EMSCRIPTEN_KEEPALIVE void webidl_free(void* p) { free(p); }
EMSCRIPTEN_KEEPALIVE void* webidl_malloc(size_t len) { return malloc(len); }


// Interface: VoidPtr


void EMSCRIPTEN_KEEPALIVE emscripten_bind_VoidPtr___destroy___0(void** self) {
  delete self;
}

// Interface: Standalone_Core


Standalone_Core* EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_Standalone_Core_0() {
  return new Standalone_Core();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_program_compile_0(Standalone_Core* self) {
  self->program_compile();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_link_flash_memory_2(Standalone_Core* self, void* p, unsigned int len) {
  self->link_flash_memory(p, len);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core___destroy___0(Standalone_Core* self) {
  delete self;
}

}

