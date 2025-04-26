
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

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_define_symbol_2(Standalone_Core* self, unsigned int value, const char* name) {
  self->define_symbol(value, name);
}

const char* EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_disassembly_view_0(Standalone_Core* self) {
  return self->disassembly_view();
}

unsigned int EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_get_memory_size_0(Standalone_Core* self) {
  return self->get_memory_size();
}

void* EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_get_memory_ptr_0(Standalone_Core* self) {
  return self->get_memory_ptr();
}

void* EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_get_register_file_ptr_0(Standalone_Core* self) {
  return self->get_register_file_ptr();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_run_0(Standalone_Core* self) {
  self->run();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core_step_0(Standalone_Core* self) {
  self->step();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Standalone_Core___destroy___0(Standalone_Core* self) {
  delete self;
}

}

