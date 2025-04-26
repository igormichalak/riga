<script>
  let { as = 'button', children, ...rest } = $props();
</script>

<svelte:element this={as} {...rest}
  class={['block px-5 py-2.5 bg-zinc-900 text-zinc-100 text-center rounded cursor-pointer', rest?.class]}>
  {@render children?.()}
</svelte:element>
