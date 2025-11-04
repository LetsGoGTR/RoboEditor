<script lang="ts">
	import { dummyDeepTree } from '@/testData';
	import DirectoryTree from '@components/DirectoryTree.svelte';
	import Header from '@components/Header.svelte';
	import MenuBar from '@components/MenuBar.svelte';
</script>

<Header />
<MenuBar />
<main>
  <DirectoryTree root={dummyDeepTree} />
</main>

<style>
</style>
