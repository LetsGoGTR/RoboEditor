<!-- Layout.svelte -->
<script lang="ts">
  import Header from '@routes/Header.svelte';
  import MenuBar from '@routes/MenuBar.svelte';
  import WorkspaceSection from './WorkspaceSection.svelte';
  import HorizontalSplit from '@layouts/HorizontalSplit.svelte';
  import Editor from '@components/Editor.svelte';
</script>

<div class="app-container">
  <header class="app-header">
    <Header />
    <MenuBar />
  </header>

  <main class="app-main">
    <HorizontalSplit>
      <WorkspaceSection slot="left" />
      <Editor slot="right" />
    </HorizontalSplit>
  </main>
</div>

<style>
.app-container {
  display: flex;
  flex-direction: column;
  height: 100vh;
  overflow: hidden;
}

.app-header {
  flex: 0 0 auto;
  display: flex;
  flex-direction: column;
  z-index: 10;
  background: #fff;
  border-bottom: 1px solid #ddd;
}

.app-main {
  flex: 1 1 auto;
  overflow: hidden;
  display: flex;
  height: 100%;
  min-height: 0;
}
</style>
