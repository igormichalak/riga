<script>
  import { twc } from '$lib/twc.js';

  let { as = 'button', ...rest } = $props();
</script>

<svelte:element this={as} {...rest}
  class={twc('block px-5 py-2.5 bg-zinc-900 text-zinc-100 text-center rounded cursor-pointer',
    'flex justify-center items-center text-center', rest.class)}>
  {@render rest.children?.()}
</svelte:element>
