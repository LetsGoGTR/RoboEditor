<script lang="ts">
  import type { TreeNode, FolderNode } from "@/types";
  export let node: TreeNode;
  export let depth = 0;

	// 폴더 -> 파일, 이름순(ㄱ-ㅎ) 정렬
	function sortedChildren(children: TreeNode[]) {
    return [...children].sort((a, b) => {
      if (a.type === b.type) return a.name.localeCompare(b.name);
      return a.type === "folder" ? -1 : 1;
    });
  }
</script>

<li class="tree-item">
  {#if node.type === "folder"}
    <details class="folder" open={depth === 0}>
      <summary>
        <span class="icon">{'📁'}</span>
        <span class="name">{node.name}</span>
      </summary>

      <ul class="children">
        {#each sortedChildren((node as FolderNode).children) as child (child.id)}
          <svelte:self node={child} depth={depth + 1} />
        {/each}
      </ul>
    </details>
  {:else}
    <div class="file">
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
  /* depth 기반 왼쪽 들여쓰기 (기존보다 얕게 10px 단위) */
  margin-left: calc(var(--depth) * 10px);
}

/* 폴더 섹션 */
details.folder {
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
  transition: background-color 0.15s ease;
  list-style: none;
}

summary:hover {
  background-color: rgba(0, 0, 0, 0.06);
}

/* summary 왼쪽 삼각형 마커 제거 */
summary::-webkit-details-marker {
  display: none;
}

/* 파일 아이템 */
.file {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  padding: 2px 4px;
  margin: 1px 0;
}

/* 공통 아이콘 정렬 */
.icon {
  width: 1.2rem;
  text-align: center;
}

/* 자식 목록 들여쓰기 최소화 */
.children {
  margin: 0;
  padding-left: 20px; /* 기본 16px → 절반 수준으로 축소 */
}
</style>