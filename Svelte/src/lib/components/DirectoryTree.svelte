<script lang="ts">
  import DirectoryItem from "./DirectoryItem.svelte";
  import type { TreeNode } from "@/types";

  export let root: TreeNode;
  export let mode: "view" | "select" = "view";
  export let onselect: ((node: TreeNode) => void) | undefined = undefined;

  function handleToggle(id: string, checked: boolean) {
    toggleCheckRecursive(root, id, checked);
  }

  function toggleCheckRecursive(node: TreeNode, id: string, checked: boolean): boolean {
    if (node.id === id) node.checked = checked;
    else if (node.type === "folder" && node.children)
      for (const child of node.children)
        toggleCheckRecursive(child, id, checked);
    return node.checked ?? false;
  }
</script>

<ul class="tree-root">
  <DirectoryItem node={root} depth={0} mode={mode} onToggleCheck={handleToggle} onselect={onselect}/>
</ul>

<style>
.tree-root {
  list-style: none;
  margin: 0;
  padding: 0.25rem 0.5rem;
  font-family: 'Consolas', monospace;
}
</style>
