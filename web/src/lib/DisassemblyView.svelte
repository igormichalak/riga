<script>
  import { twc } from '$lib/twc.js';

  let { ic, lines, breakpoints, toggleBreakpoint, ...rest } = $props();

  function hasInstruction(line) {
    return typeof line.instructionIndex === 'number' && line.instructionIndex >= 0;
  }

  function hasBreakpoint(line) {
    if (hasInstruction(line)) {
      return breakpoints.includes(line.instructionIndex);
    }
    return false;
  }
</script>

{#snippet item(line, active)}
  <div class={[
    'flex items-center gap-2 pl-1.5 rounded group',
    active ? 'bg-red-100' : (hasInstruction(line) && 'hover:bg-stone-100'),
  ]}>
    {#if hasInstruction(line)}
      <button
        class={twc(
          'size-3.5 hover:bg-red-400 rounded-full cursor-pointer',
          hasBreakpoint(line) ? 'bg-red-400' : 'group-hover:bg-red-200',
        )}
        title="Toggle breakpoint"
        aria-label="Toggle breakpoint"
        onclick={() => toggleBreakpoint(line.instructionIndex)}
      ></button>
    {:else}
      <div class="size-3.5"></div>
    {/if}
    <pre class="block font-mono">{line.text}</pre>
  </div>
{/snippet}

<div class={twc('flex flex-col w-full h-full overflow-scroll', rest?.class)}>
  {#each lines as line}
    {@render item(line, ic === line?.instructionIndex)}
  {/each}
</div>
