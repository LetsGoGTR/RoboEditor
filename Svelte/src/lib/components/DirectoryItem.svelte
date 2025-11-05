<script lang="ts">
  import type { TreeNode, FolderNode } from "@/types";

  export let node: TreeNode;
  export let depth = 0;
  export let onselect: (node: TreeNode) => void;

  function sortedChildren(children: TreeNode[]) {
    return [...children].sort((a, b) => {
      if (a.type === b.type) return a.name.localeCompare(b.name);
      return a.type === "folder" ? -1 : 1;
    });
  }

  function handleSelect() {
    onselect?.(node);
  }
</script>

<li class="tree-item">
  {#if node.type === "folder"}
    <details class="folder" open={depth === 0}>
      <summary class="node" on:click={handleSelect}>
        <span class="icon">📁</span>
        <span class="name">{node.name}</span>
      </summary>

      <ul class="children">
        {#each sortedChildren((node as FolderNode).children) as child (child.id)}
          <svelte:self node={child} depth={depth + 1} onselect={onselect} />
        {/each}
      </ul>
    </details>
  {:else}
    <div
      class="file node"
      role="button"
      tabindex="0"
      on:click={handleSelect}
      on:keydown={(e) => (e.key === "Enter" || e.key === " ") && handleSelect()}
    >
      <span class="icon">📄</span>
      <span class="name">{node.name}</span>
    </div>
  {/if}
</li>

<style>
.tree-item {
  list-style: none;
  margin: 1px 0;
  padding: 0;
}

/* 폴더 / 파일 선택 시 영역 */
.node {
  transition: background-color 0.15s ease;
  padding-left: 0;
  margin: 1px 0;
}

/* summary (폴더명 줄) */
summary {
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 0.35rem;
  padding: 2px 4px;
  border-radius: 4px;
  list-style: none;
}

.node:hover {
  background-color: rgba(0, 0, 0, 0.06);
}

/* summary 왼쪽 삼각형 마커 제거 */
summary::-webkit-details-marker {
  display: none;
}

/* 파일 아이템 */
.file {
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 0.35rem;
  padding: 2px 4px;
  border-radius: 4px;
  margin: 0 0;
}

/* 공통 아이콘 정렬 */
.icon {
  width: 1.2rem;
  text-align: center;
}

/* 자식 목록 들여쓰기 최소화 */
.children {
  margin: 0;
  padding-left: 20px;
}
</style>