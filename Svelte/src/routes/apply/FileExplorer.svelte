<script lang="ts">
	import { fileTree } from "@/stores/fileTree";
  import DirectoryTree from "@components/DirectoryTree.svelte";

  let rootSelect = $derived($fileTree);
</script>

<div class="explorer">
  <div class="panel">
    <h4>Select Workspace</h4>
    <DirectoryTree root={rootSelect} mode="select" />
  </div>
</div>

<style>
.explorer {
  display: flex;
  gap: 1rem;
  height: 100%;
}
.panel {
  flex: 1;
  border: 1px solid #ddd;
  padding: 0.5rem;
}
</style>
