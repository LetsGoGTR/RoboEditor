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

    <h3>삭제</h3>

    <!-- 삭제 선택 안내 -->
    <div class="info-box">
      <p class="warn">폴더 선택 시, 자동으로 하위에 있는 폴더 및 파일도 삭제됩니다.</p>
      <p class="warn">삭제된 항목은 되돌릴 수 없습니다.</p>
    </div>

    <!-- Folder Tree 선택 영역 -->
    <div class="tree-section">
      <span class="label">삭제 대상 선택:</span>
      {#if root}
        <DirTree root={root} mode="view" onselect={handleSelect} />
      {/if}
    </div>

    <!-- 선택된 노드 표시 -->
    <div class="selected-section">
      <div class="selected-box">
        {#if selectedNode}
          <div><strong>이름:</strong> {selectedNode.name}</div>
          <div><strong>경로:</strong> {selectedNode.path}</div>
          <div>
            <strong>유형:</strong> 
            {selectedNode.type === "file" ? "파일" : "폴더"}
          </div>
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

/* 안내 박스 */
.info-box {
  background: #fff6f6;
  border: 1px solid #e1b3b3;
  padding: 0.6rem;
  border-radius: 6px;
  margin-top: 0.5rem;
  font-size: 0.95rem;
}

.warn {
  margin-top: 0.6rem;
  color: #c33;
  font-size: 0.9rem;
  font-weight: 600;
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
