<script>
  import { X } from 'lucide-svelte';

  import { goto } from '$app/navigation';
  import Button from '$lib/Button.svelte';

  let filesBinary = $state();
  let filesSymbols = $state();

  async function navigateToSimulator() {
    const params = new URLSearchParams();

    if (filesBinary?.length > 0) {
      const buf = await filesBinary[0].arrayBuffer();
      const v = btoa(String.fromCharCode(...new Uint8Array(buf)));
      params.set('pb', v);
    }

    if (filesSymbols?.length > 0) {
      const buf = await filesSymbols[0].arrayBuffer();
      const v = btoa(String.fromCharCode(...new Uint8Array(buf)));
      params.set('ps', v);
    }

    goto(`/?${params.toString()}`);
  }
</script>

<main class="w-84 flex flex-col gap-3 items-start">
  <div class="w-full flex gap-0.5">
    <div class="relative grow">
      <Button as="label" for="program-binary">Upload binary</Button>
      <input bind:files={filesBinary} type="file"
        id="program-binary" name="program-binary"
        class="absolute opacity-0 pointer-events-none"
      />
    </div>
    <div class="relative grow">
      <Button as="label" for="program-symbols">Upload symbols</Button>
      <input bind:files={filesSymbols} type="file"
        id="program-symbols" name="program-symbols"
        class="absolute opacity-0 pointer-events-none"
      />
    </div>
  </div>
  <div class="w-full overflow-hidden">
    {#if filesBinary?.length > 0}
      <div class="flex items-center gap-1.5">
        <button class="cursor-pointer"
          onclick={() => filesBinary = new DataTransfer().files}
        >
          <X size={16} />
        </button>
        <span class="whitespace-nowrap">Program binary: {filesBinary[0].name}</span>
      </div>
    {/if}
    {#if filesSymbols?.length > 0}
      <div class="flex items-center gap-1.5">
        <button class="cursor-pointer"
          onclick={() => filesSymbols = new DataTransfer().files}
        >
          <X size={16} />
        </button>
        <span class="whitespace-nowrap">Program symbols: {filesSymbols[0].name}</span>
      </div>
    {/if}
  </div>
  {#if filesBinary?.length > 0}
    <Button class="w-full" onclick={navigateToSimulator}>Simulate</Button>
  {/if}
</main>
