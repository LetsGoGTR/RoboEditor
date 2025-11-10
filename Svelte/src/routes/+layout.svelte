<script lang="ts">
  import Header from '@features/Header.svelte';
  import MenuBar from '@features/MenuBar.svelte';
  import WorkspaceSection from '@features/WorkspaceSection.svelte';
  import HorizontalSplit from '@layouts/HorizontalSplit.svelte';

  export const ssr = false;
</script>

<svelte:head>
	<title>RoboEditor</title>
  <link rel="icon" type="image/svg+xml" href="/roboeditor-icon.svg" />
</svelte:head>

<div class="app-container">
  <header class="app-header">
    <Header />
    <MenuBar />
  </header>

  <main class="app-main">
    <HorizontalSplit initialLeftRatio={20}>
      <WorkspaceSection slot="left" />
      <slot slot="right" />
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

