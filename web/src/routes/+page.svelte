<script>
  let files = $state();

  $effect(() => {
    if (!files || files.length == 0) return;

    files[0].arrayBuffer().then(buf => new Uint8Array(buf)).then(buf => {
      const core = new Module.Standalone_Core();

      const ptr = Module._malloc(buf.byteLength + 3);
      const loadAddress = (ptr + 3) & (~3);

      Module.HEAPU8.set(buf, loadAddress);

      core.link_flash_memory(loadAddress, buf.byteLength);
      core.program_compile();

      Module._free(ptr);
      Module.destroy(core);
    });
  });
</script>

<svelte:head>
  <script type="text/javascript" src="./riga.js"></script>
</svelte:head>

<div>
  <label for="program">Upload program</label>
  <input bind:files type="file" id="program" name="program">
</div>
