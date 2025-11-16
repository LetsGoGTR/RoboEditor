<script lang="ts">
  import DirTree from "@components/DirectoryTree.svelte";
  import type { FolderNode, TreeNode } from "@/types";

  let {
    open = $bindable(false),
    root,
    initialTarget = null,
    onConfirm,
    onCancel
  } = $props<{
    open: boolean;
    root: FolderNode | null;
    onConfirm: (node: TreeNode) => void;
    onCancel: () => void;
  }>();

  let selectedNode = $state<TreeNode | null>(initialTarget);

  function handleSelect(node: TreeNode) {
    selectedNode = node;
  }

  function confirm() {
    if (!selectedNode) return;
    onConfirm(selectedNode);
    close();
  }

  function close() {
    selectedNode = null;
    onCancel();
  }
</script>

{#if open}
<div class="dialog-backdrop">
  <dialog open class="dialog-box">

    <h3>Workspace 비교</h3>

    <!-- Folder Tree 선택 영역 -->
    <div class="tree-section">
      <span class="label">첫 번째 workspace 선택</span>
      {#if root}
        <DirTree root={root} mode="view" onselect={handleSelect} />
      {/if}
    </div>

    <!-- 선택된 노드 표시 -->
    <div class="selected-section">
      <div class="selected-box">
        {#if selectedNode}
          <div><strong>경로:</strong> {selectedNode.path}</div>
        {:else}
          <span class="none">선택된 항목 없음</span>
        {/if}
      </div>
    </div>

        <!-- Folder Tree 선택 영역 -->
    <div class="tree-section">
      <span class="label">두 번째 workspace 선택</span>
      {#if root}
        <DirTree root={root} mode="view" onselect={handleSelect} />
      {/if}
    </div>

    <!-- 선택된 노드 표시 -->
    <div class="selected-section">
      <div class="selected-box">
        {#if selectedNode}
          <div><strong>경로:</strong> {selectedNode.path}</div>
        {:else}
          <span class="none">선택된 항목 없음</span>
        {/if}
      </div>
    </div>

    <div class="actions">
      <button onclick={confirm} disabled={!selectedNode}>삭제</button>
      <button class="cancel" onclick={close}>취소</button>
    </div>

  </dialog>
</div>
{/if}

<style>
.dialog-backdrop {
  position: fixed;
  inset: 0;
  background: rgba(0,0,0,0.35);
  display: flex;
  justify-content: center;
  align-items: center;
}

.dialog-box {
  width: 520px;
  max-height: 80vh;
  overflow: auto;
  border: 2px solid #d26a6a;
  border-radius: 8px;
  padding: 1.4rem;
  background: white;
}

/* 트리 영역 */
.tree-section {
  margin-top: 1rem;
  border: 1px solid #ddd;
  padding: 0.6rem;
  border-radius: 6px;
  max-height: 260px;
  overflow-y: auto;
}

.label {
  font-weight: 500;
  color: #222;
}

/* 선택 결과 표시 */
.selected-section {
  margin-top: 1rem;
}

.selected-box {
  margin-top: 0.5rem;
  border: 1px solid #ddd;
  border-radius: 6px;
  padding: 0.7rem;
  background: #fafafa;
  font-size: 0.92rem;
}

.none {
  color: #888;
}

/* 버튼 */
.actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1.4rem;
  gap: 0.7rem;
}

button {
  padding: 0.5rem 1rem;
  border: none;
  background: #d24949;
  color: #fff;
  border-radius: 6px;
  cursor: pointer;
}

button:disabled {
  background: #ccc;
  cursor: not-allowed;
}

.cancel {
  background: #777;
}
</style>
