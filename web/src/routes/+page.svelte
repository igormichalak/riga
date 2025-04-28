<script>
  let { data } = $props();
  let module = $state();

  $effect(() => {
    if (!module) return;

    const ptr = module._malloc(data.fileDataBinary.byteLength + 3);
    const loadPtr = (ptr + 3) & (~3);

    module.HEAPU8.set(data.fileDataBinary, loadPtr);

    const core = new module.Standalone_Core();
    core.link_flash_memory(loadPtr, data.fileDataBinary.byteLength);
    core.program_compile();

    const disassembly_str_ptr = core.disassembly_view();
    console.log(module.UTF8ToString(disassembly_str_ptr));
    module._free(disassembly_str_ptr);

    return () => {
      module.destroy(core);
      module._free(ptr);
    };
  });

  function onWasmLoad() {
    createModule().then(m => {
      module = m;
    });
  }
</script>

<svelte:head>
  <script async type="text/javascript" src="./riga.js"
    onload={onWasmLoad}
  ></script>
</svelte:head>

<div class="flex flex-col gap-1">
  <a href="/upload">New</a>
  <p>Simulator</p>
</div>
