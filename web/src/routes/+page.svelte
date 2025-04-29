<script>
  import { Play, RedoDot, RotateCcw } from 'lucide-svelte';

  import Button from '$lib/Button.svelte';
  import ControlButton from '$lib/ControlButton.svelte';
  import DisassemblyView from '$lib/DisassemblyView.svelte';

  let { data } = $props();

  let module = $state();
  let module_HEAPU8 = $derived(module?.HEAPU8);
  let module_HEAPU32 = $derived(module?.HEAPU32);

  let core;
  let core_pc_ptr_u32;
  let core_ic_ptr_u32;
  let core_register_file_ptr_u32;

  let disassemblyLines = $state([]);
  let breakpoints = $state([]);

  let registersDiv;
  let pcSpan;
  let ic = $state(0);

  function toggleBreakpoint(instructionIndex) {
    if (breakpoints.includes(instructionIndex)) {
      if (core.clear_breakpoint(instructionIndex)) {
        breakpoints = breakpoints.filter(idx => idx !== instructionIndex);
      }
    } else {
      if (core.set_breakpoint(instructionIndex)) {
        breakpoints.push(instructionIndex);
      }
    }
  }

  function transformDisassemblyString(str, offsets) {
    const lines = [];
    const textLines = str.split(/\n/);

    let stringOffset = 0;
    let instructionIndex = 0;

    for (const text of textLines) {
      const line = { text: text };

      if (instructionIndex < offsets.length) {
        if (stringOffset == offsets[instructionIndex]) {
          line.instructionIndex = instructionIndex;
          instructionIndex++;
        }
      }
      stringOffset += text.length + 1;

      lines.push(line);
    }

    return lines;
  }

  function getInstructionOffsets(core) {
    const result = [];
    let memoryOffset = core.disassembly_view_map() / 4;
    while (true) {
      const v = module_HEAPU32[memoryOffset];
      if (v == 0) {
        break;
      } else {
        result.push(v);
      }
      memoryOffset++;
    }
    return result;
  }

  $effect(() => {
    if (!module) return;

    const ptr = module._malloc(data.fileDataBinary.byteLength + 3);
    const loadPtr = (ptr + 3) & (~3);

    module_HEAPU8.set(data.fileDataBinary, loadPtr);

    core = new module.Standalone_Core();
    core_pc_ptr_u32 = core.get_pc_ptr() / 4;
    core_ic_ptr_u32 = core.get_ic_ptr() / 4;
    core_register_file_ptr_u32 = core.get_register_file_ptr() / 4;
    core.init();

    core.link_flash_memory(loadPtr, data.fileDataBinary.byteLength);
    core.program_compile();

    const disassemblyString = module.UTF8ToString(core.disassembly_view());

    disassemblyLines = transformDisassemblyString(
      disassemblyString,
      getInstructionOffsets(core),
    );

    return () => {
      if (core) {
        module.destroy(core);
        core = null;
        core_pc_ptr_u32 = null;
        core_ic_ptr_u32 = null;
        core_register_file_ptr_u32 = null;
      }
      module._free(ptr);
    };
  });

  function onWasmLoad() {
    createModule().then(m => {
      module = m;
    });
  }

  function run() {
    if (core) core.run();
  }

  function step() {
    if (core) core.step();
    if (core_ic_ptr_u32) {
      ic = module_HEAPU32[core_ic_ptr_u32];
    }
  }

  function reset() {
    if (core) core.reset();
  }

  const registerNames = [
	"x0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
	"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
	"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
	"s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
  ];

  let rpage = $state(0);
  let displayMode = $state('dec');

  $effect(() => {
    let radix;

    switch (displayMode) {
    case 'hex':
      radix = 16;
      break;
    case 'dec':
    default:
      radix = 10;
      break;
    }

    let reqId;

    function onframe() {
      if (core_pc_ptr_u32) {
        pcSpan.textContent = 'PC: 0x' + module_HEAPU32[core_pc_ptr_u32].toString(16);
      }
      if (core_register_file_ptr_u32) {
        for (let i = 0; u < registersDiv.children.length; i++) {
          const spanEl = registersDiv.children[i];
          const preEl = spanEl.children[1];

          const xn = +spanEl.dataset.xn;

          const offset = core_register_file_ptr_u32 + (xn * 2);
          const regh = BigInt(module_HEAPU32[offset + 1]) << BigInt(32);
          const rehl = BigInt(module_HEAPU32[offset]);
          preEl.textContent = (regh | regl).toString(radix);
        }
      }

      reqId = requestAnimationFrame(onframe);
    }
    onframe();

    return () => {
      if (typeof reqId === 'number') cancelAnimationFrame(reqId);
    };
  });

  $effect(() => {
    let intervalId = setInterval(() => {
      if (core_ic_ptr_u32) {
        const nextIC = module_HEAPU32[core_ic_ptr_u32];
        if (ic !== nextIC) {
          ic = nextIC;
        }
      }
    }, 100);
    return () => clearInterval(intervalId);
  });
</script>

<svelte:head>
  <script async type="text/javascript" src="./riga.js"
    onload={onWasmLoad}
  ></script>
</svelte:head>

{#snippet controls()}
  <div class="flex items-center gap-1.5">
    <Button class="h-full px-4 py-0 text-sm" as="a" href="/upload">New</Button>

    <div class="w-[1px] h-full mx-2 bg-stone-200"></div>

    <ControlButton title="Run" aria-label="Run" onclick={run}>
      <Play size={20} />
    </ControlButton>

    <ControlButton title="Step" aria-label="Step" onclick={step}>
      <RedoDot size={20} />
    </ControlButton>

    <ControlButton title="Reset" aria-label="Reset" onclick={reset}>
      <RotateCcw size={20} />
    </ControlButton>

    <div class="w-[1px] h-full mx-2 bg-stone-200"></div>

    <span bind:this={pcSpan}></span>
  </div>
{/snippet}

<div class="flex flex-col gap-8 w-[90vw] max-w-[768px] h-[70vh]">
  {@render controls()}
  <div class="w-full flex-1 min-h-0 flex justify-between items-start gap-4">
    <DisassemblyView
      class="min-w-72 max-w-96 flex-1"
      ic={ic}
      lines={disassemblyLines}
      breakpoints={breakpoints}
      toggleBreakpoint={toggleBreakpoint}
    />
    <div class="flex-1 h-full px-8 pt-4 pb-6 border-1 rounded flex flex-col">
      <div class="flex justify-between gap-4 mb-4">
        <div class="flex gap-2">
          <button
            class={['cursor-pointer', rpage === 0 && 'underline']}
            onclick={() => rpage = 0}
          >x0..x15</button>
          <button
            class={['cursor-pointer', rpage === 1 && 'underline']}
            onclick={() => rpage = 1}
          >x16..x31</button>
        </div>
        <div class="flex gap-2">
          <button
            class={['cursor-pointer', displayMode === 'dec' && 'underline']}
            onclick={() => displayMode = 'dec'}
          >dec</button>
          <button
            class={['cursor-pointer', displayMode === 'hex' && 'underline']}
            onclick={() => displayMode = 'hex'}
          >hex</button>
        </div>
      </div>
      <div bind:this={registersDiv} class="flex-1 flex flex-col justify-between">
        {#each Array.from({ length: 16 }, (_, i) => i) as xn}
          <span data-xn={xn + rpage * 16} class="block flex font-mono">
            <pre>{registerNames[xn + rpage * 16].padStart(3, ' ')}:&nbsp;</pre>
            <pre>0</pre>
          </span>
        {/each}
      </div>
    </div>
  </div>
</div>
