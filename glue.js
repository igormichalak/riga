
// Bindings utilities

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function WrapperObject() {
}
WrapperObject.prototype = Object.create(WrapperObject.prototype);
WrapperObject.prototype.constructor = WrapperObject;
WrapperObject.prototype.__class__ = WrapperObject;
WrapperObject.__cache__ = {};
Module['WrapperObject'] = WrapperObject;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant)
    @param {*=} __class__ */
function getCache(__class__) {
  return (__class__ || WrapperObject).__cache__;
}
Module['getCache'] = getCache;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant)
    @param {*=} __class__ */
function wrapPointer(ptr, __class__) {
  var cache = getCache(__class__);
  var ret = cache[ptr];
  if (ret) return ret;
  ret = Object.create((__class__ || WrapperObject).prototype);
  ret.ptr = ptr;
  return cache[ptr] = ret;
}
Module['wrapPointer'] = wrapPointer;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function castObject(obj, __class__) {
  return wrapPointer(obj.ptr, __class__);
}
Module['castObject'] = castObject;

Module['NULL'] = wrapPointer(0);

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function destroy(obj) {
  if (!obj['__destroy__']) throw 'Error: Cannot destroy object. (Did you create it yourself?)';
  obj['__destroy__']();
  // Remove from cache, so the object can be GC'd and refs added onto it released
  delete getCache(obj.__class__)[obj.ptr];
}
Module['destroy'] = destroy;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function compare(obj1, obj2) {
  return obj1.ptr === obj2.ptr;
}
Module['compare'] = compare;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function getPointer(obj) {
  return obj.ptr;
}
Module['getPointer'] = getPointer;

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function getClass(obj) {
  return obj.__class__;
}
Module['getClass'] = getClass;

// Converts big (string or array) values into a C-style storage, in temporary space

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
var ensureCache = {
  buffer: 0,  // the main buffer of temporary storage
  size: 0,   // the size of buffer
  pos: 0,    // the next free offset in buffer
  temps: [], // extra allocations
  needed: 0, // the total size we need next time

  prepare() {
    if (ensureCache.needed) {
      // clear the temps
      for (var i = 0; i < ensureCache.temps.length; i++) {
        Module['_webidl_free'](ensureCache.temps[i]);
      }
      ensureCache.temps.length = 0;
      // prepare to allocate a bigger buffer
      Module['_webidl_free'](ensureCache.buffer);
      ensureCache.buffer = 0;
      ensureCache.size += ensureCache.needed;
      // clean up
      ensureCache.needed = 0;
    }
    if (!ensureCache.buffer) { // happens first time, or when we need to grow
      ensureCache.size += 128; // heuristic, avoid many small grow events
      ensureCache.buffer = Module['_webidl_malloc'](ensureCache.size);
      assert(ensureCache.buffer);
    }
    ensureCache.pos = 0;
  },
  alloc(array, view) {
    assert(ensureCache.buffer);
    var bytes = view.BYTES_PER_ELEMENT;
    var len = array.length * bytes;
    len = alignMemory(len, 8); // keep things aligned to 8 byte boundaries
    var ret;
    if (ensureCache.pos + len >= ensureCache.size) {
      // we failed to allocate in the buffer, ensureCache time around :(
      assert(len > 0); // null terminator, at least
      ensureCache.needed += len;
      ret = Module['_webidl_malloc'](len);
      ensureCache.temps.push(ret);
    } else {
      // we can allocate in the buffer
      ret = ensureCache.buffer + ensureCache.pos;
      ensureCache.pos += len;
    }
    return ret;
  },
  copy(array, view, offset) {
    offset /= view.BYTES_PER_ELEMENT;
    for (var i = 0; i < array.length; i++) {
      view[offset + i] = array[i];
    }
  },
};

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureString(value) {
  if (typeof value === 'string') {
    var intArray = intArrayFromString(value);
    var offset = ensureCache.alloc(intArray, HEAP8);
    ensureCache.copy(intArray, HEAP8, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureInt8(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAP8);
    ensureCache.copy(value, HEAP8, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureInt16(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAP16);
    ensureCache.copy(value, HEAP16, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureInt32(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAP32);
    ensureCache.copy(value, HEAP32, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureFloat32(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAPF32);
    ensureCache.copy(value, HEAPF32, offset);
    return offset;
  }
  return value;
}

/** @suppress {duplicate} (TODO: avoid emitting this multiple times, it is redundant) */
function ensureFloat64(value) {
  if (typeof value === 'object') {
    var offset = ensureCache.alloc(value, HEAPF64);
    ensureCache.copy(value, HEAPF64, offset);
    return offset;
  }
  return value;
}

// Interface: VoidPtr

/** @suppress {undefinedVars, duplicate} @this{Object} */
function VoidPtr() { throw "cannot construct a VoidPtr, no constructor in IDL" }
VoidPtr.prototype = Object.create(WrapperObject.prototype);
VoidPtr.prototype.constructor = VoidPtr;
VoidPtr.prototype.__class__ = VoidPtr;
VoidPtr.__cache__ = {};
Module['VoidPtr'] = VoidPtr;

/** @suppress {undefinedVars, duplicate} @this{Object} */
VoidPtr.prototype['__destroy__'] = VoidPtr.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_VoidPtr___destroy___0(self);
};

// Interface: Standalone_Core

/** @suppress {undefinedVars, duplicate} @this{Object} */
function Standalone_Core() {
  this.ptr = _emscripten_bind_Standalone_Core_Standalone_Core_0();
  getCache(Standalone_Core)[this.ptr] = this;
};

Standalone_Core.prototype = Object.create(WrapperObject.prototype);
Standalone_Core.prototype.constructor = Standalone_Core;
Standalone_Core.prototype.__class__ = Standalone_Core;
Standalone_Core.__cache__ = {};
Module['Standalone_Core'] = Standalone_Core;
/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['init'] = Standalone_Core.prototype.init = function() {
  var self = this.ptr;
  _emscripten_bind_Standalone_Core_init_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['program_compile'] = Standalone_Core.prototype.program_compile = function() {
  var self = this.ptr;
  _emscripten_bind_Standalone_Core_program_compile_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['link_flash_memory'] = Standalone_Core.prototype.link_flash_memory = function(p, len) {
  var self = this.ptr;
  if (p && typeof p === 'object') p = p.ptr;
  if (len && typeof len === 'object') len = len.ptr;
  _emscripten_bind_Standalone_Core_link_flash_memory_2(self, p, len);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['define_symbol'] = Standalone_Core.prototype.define_symbol = function(value, name) {
  var self = this.ptr;
  ensureCache.prepare();
  if (value && typeof value === 'object') value = value.ptr;
  if (name && typeof name === 'object') name = name.ptr;
  else name = ensureString(name);
  _emscripten_bind_Standalone_Core_define_symbol_2(self, value, name);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['disassembly_view_map'] = Standalone_Core.prototype.disassembly_view_map = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_disassembly_view_map_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['disassembly_view'] = Standalone_Core.prototype.disassembly_view = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_disassembly_view_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['get_memory_size'] = Standalone_Core.prototype.get_memory_size = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_get_memory_size_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['get_memory_ptr'] = Standalone_Core.prototype.get_memory_ptr = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_get_memory_ptr_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['get_register_file_ptr'] = Standalone_Core.prototype.get_register_file_ptr = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_get_register_file_ptr_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['get_pc_ptr'] = Standalone_Core.prototype.get_pc_ptr = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_get_pc_ptr_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['get_ic_ptr'] = Standalone_Core.prototype.get_ic_ptr = function() {
  var self = this.ptr;
  return _emscripten_bind_Standalone_Core_get_ic_ptr_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['set_breakpoint'] = Standalone_Core.prototype.set_breakpoint = function(instr_idx) {
  var self = this.ptr;
  if (instr_idx && typeof instr_idx === 'object') instr_idx = instr_idx.ptr;
  return !!(_emscripten_bind_Standalone_Core_set_breakpoint_1(self, instr_idx));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['clear_breakpoint'] = Standalone_Core.prototype.clear_breakpoint = function(instr_idx) {
  var self = this.ptr;
  if (instr_idx && typeof instr_idx === 'object') instr_idx = instr_idx.ptr;
  return !!(_emscripten_bind_Standalone_Core_clear_breakpoint_1(self, instr_idx));
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['reset'] = Standalone_Core.prototype.reset = function() {
  var self = this.ptr;
  _emscripten_bind_Standalone_Core_reset_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['run'] = Standalone_Core.prototype.run = function() {
  var self = this.ptr;
  _emscripten_bind_Standalone_Core_run_0(self);
};

/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['step'] = Standalone_Core.prototype.step = function() {
  var self = this.ptr;
  _emscripten_bind_Standalone_Core_step_0(self);
};


/** @suppress {undefinedVars, duplicate} @this{Object} */
Standalone_Core.prototype['__destroy__'] = Standalone_Core.prototype.__destroy__ = function() {
  var self = this.ptr;
  _emscripten_bind_Standalone_Core___destroy___0(self);
};
