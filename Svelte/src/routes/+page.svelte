<script lang="ts">
	import Header from '@routes/Header.svelte';
	import MenuBar from '@routes/MenuBar.svelte';
	import WorkspaceSection from './WorkspaceSection.svelte';
	import HorizontalSplit from '@layouts/HorizontalSplit.svelte';
</script>

<Header />
<MenuBar />
<HorizontalSplit>
	<WorkspaceSection slot="left" />
</HorizontalSplit>

<style>
</style>
