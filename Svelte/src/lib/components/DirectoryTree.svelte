<script lang="ts">
  import DirectoryItem from "./DirectoryItem.svelte";
  import type { TreeNode } from "@/types";

  export let root: TreeNode;
  export let onselect: (node: TreeNode) => void;
</script>

<ul class="tree-root">
  <DirectoryItem node={root} depth={0} onselect={onselect} />
</ul>

<style>
.tree-root {
  list-style: none;
  margin: 0;
  padding: 0.25rem 0.5rem;
  font-family: 'Consolas', monospace;
}
</style>
