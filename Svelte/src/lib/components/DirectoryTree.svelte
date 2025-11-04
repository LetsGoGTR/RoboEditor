<script lang="ts">
  import type { TreeNode } from "@/types";
  import DirectoryItem from "./DirectoryItem.svelte";

  export let root: TreeNode;
</script>

<ul class="tree-root">
  <DirectoryItem node={root} depth={0} />
</ul>

<style>
.tree-root {
  list-style: none;
  margin: 0;
  padding: 0.25rem 0.5rem;
  font-family: 'Consolas', monospace;
}
</style>
