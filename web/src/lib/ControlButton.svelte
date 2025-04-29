<script>
  let { ...rest} = $props();
</script>

<button {...rest}
  class="size-8 hover:bg-stone-200 border-1 rounded flex justify-center items-center cursor-pointer"
>
  {@render rest.children?.()}
</button>
