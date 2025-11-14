<script lang="ts">
  import type { TreeNode, FolderNode } from "@/types";

  export let node: TreeNode;
  export let depth = 0;
  export let mode: "view" | "select" = "view";
  export let onToggleCheck: (id: string, checked: boolean) => void;
  export let onselect: ((node: TreeNode) => void) | undefined = undefined;

  function sortedChildren(children: TreeNode[]) {
    return [...children].sort((a, b) => {
      if (a.type === b.type) return a.name.localeCompare(b.name);
      return a.type === "directory" ? -1 : 1;
    });
  }

  function handleCheck(event: Event) {
    const target = event.target as HTMLInputElement;
    onToggleCheck(node.id, target.checked);
  }

  function handleClick() {
    if (mode === "view" && node.type === "file") onselect?.(node);
  }
</script>

<li class="tree-item" style={`--depth: ${depth}`}>
  {#if node.type === "directory"}
    <details class="folder" open={depth === 0}>
      <summary on:click={() => onselect?.(node)}>
        {#if mode === "select"}
          <input
            type="checkbox"
            checked={node.checked}
            on:change={handleCheck}
          />
        {/if}
        <span class="icon">📁</span>
        <span class="name">{node.name}</span>
      </summary>

      <ul class="children">
        {#each sortedChildren((node as FolderNode).children) as child (child.id)}
          <svelte:self
            node={child}
            depth={depth + 1}
            mode={mode}
            onToggleCheck={onToggleCheck}
            onselect={onselect}
          />
        {/each}
      </ul>
    </details>
  {:else}
    <button class="file" on:click={handleClick}>
      {#if mode === "select"}
        <input
          type="checkbox"
          checked={node.checked}
          on:change={handleCheck}
        />
      {/if}
      <span class="icon">📄</span>
      <span class="name">{node.name}</span>
    </button>
  {/if}
</li>

<style>
.tree-item {
  list-style: none;
  margin: 2px 0;
}
summary, .file {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  cursor: pointer;
  padding: 2px 4px;
  border-radius: 4px;
}
.icon {
  width: 1.2rem;
  text-align: center;
}
.children {
  margin: 0;
  padding-left: 1rem;
}
button.file {
  all: unset;
  display: flex;
  align-items: center;
  gap: 0.4rem;
  cursor: pointer;
}
</style>
