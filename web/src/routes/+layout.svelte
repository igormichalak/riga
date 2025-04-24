<script>
  import '../app.css';

  let { children } = $props();
</script>

<svelte:head>
  <title>Riga</title>
</svelte:head>

{@render children()}
