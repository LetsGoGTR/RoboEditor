<script lang="ts">
  import DirectoryTree from "@components/DirectoryTree.svelte";
  import type { TreeNode } from "@/types";

  export let workspace: TreeNode; // 루트 노드 데이터

  let selected: TreeNode | null = null;

  function handleSelect(node: TreeNode) {
    selected = node;
  }
</script>

<div class="explorer">
  <DirectoryTree root={workspace} onselect={handleSelect} />
  {#if selected}
    <div class="info">
      <p><strong>{selected.name}</strong></p>
      <p>Type: {selected.type}</p>
    </div>
  {/if}
</div>

<style>
.explorer {
  display: flex;
  flex-direction: column;
  height: 100%;
}
.info {
  margin-top: 0.5rem;
  font-size: 0.9rem;
  color: #444;
}
</style>
