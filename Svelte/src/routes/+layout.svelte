<script lang="ts">
	let { children } = $props();
</script>

<svelte:head>
	<title>RoboEditor</title>
</svelte:head>

{@render children()}
