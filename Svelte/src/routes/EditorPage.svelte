<script lang="ts">
  import { onMount } from 'svelte';
  import EditorTabs from '@features/EditorTabs.svelte'; // ✅ 올바른 feature component

  onMount(() => {
    console.log('🧩 EditorPage mounted with EditorTabs feature');
  });
</script>

<svelte:head>
  <title>RoboEditor – Editor</title>
</svelte:head>

<main class="editor-page">
  <div class="editor-wrapper">
    <EditorTabs />
  </div>
</main>

<style>
.editor-page {
  display: flex;
  flex-direction: column;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  padding: 0;
  margin: 0;
}

.editor-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  height: 100%;
  width: 100%;
  overflow: hidden;
}
</style>
