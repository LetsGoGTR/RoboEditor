<script lang="ts">
  import DirectoryTree from "@components/DirectoryTree.svelte";
  import type { TreeNode } from "@/types";

  let { root }: { root?: TreeNode | null } = $props();
  const rootSelect = $derived(root ?? null);
</script>

<div class="explorer">
  <div>
    <h4>Select Workspace</h4>
    {#if rootSelect}
      <DirectoryTree root={rootSelect} mode="view" />
    {:else}
      <p>표시할 트리가 없습니다.</p>
    {/if}
  </div>
</div>


<style>
.explorer {
  display: flex;
  gap: 1rem;
  height: 100%;
}
</style>
