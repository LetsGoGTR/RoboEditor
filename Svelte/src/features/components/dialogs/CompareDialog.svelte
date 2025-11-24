<script lang="ts">
  import DirTree from "@components/DirectoryTree.svelte";
  import type { FolderNode, FileNode, TreeNode, NodeType } from "@/types";

  // Props 정의
  let {
    open = $bindable(false),
    root,
    mode = "file", // "file" | "directory"
    onConfirm,
    onCancel
  } = $props<{
    open: boolean;
    root: FolderNode | null;
    mode: NodeType;
    onConfirm: (payload: {
      left: FileNode | FolderNode;
      right: FileNode | FolderNode;
    }) => void;
    onCancel: () => void;
  }>();

  // 선택된 좌/우 파일(or 폴더)
  let selectedLeft = $state<TreeNode | null>(null);
  let selectedRight = $state<TreeNode | null>(null);

  /* =============================
        선택 로직 (좌측/우측)
     ============================= */
  function selectLeft(node: TreeNode) {
    if (mode === "file" && node.type !== "file") return;
    if (mode === "folder" && node.type !== "directory") return;
    selectedLeft = node;
  }

  function selectRight(node: TreeNode) {
    if (mode === "file" && node.type !== "file") return;
    if (mode === "folder" && node.type !== "directory") return;
    selectedRight = node;
  }

  /* =============================
        확인 버튼 클릭 처리
     ============================= */
  function confirm() {
    if (!selectedLeft || !selectedRight) return;

    if (mode === "file") {
      onConfirm({
        left: selectedLeft as FileNode,
        right: selectedRight as FileNode
      });
    } else {
      onConfirm({
        left: selectedLeft as FolderNode,
        right: selectedRight as FolderNode
      });
    }

    close();
  }

  function close() {
    selectedLeft = null;
    selectedRight = null;
    onCancel();
  }
</script>

{#if open}
<div class="dialog-backdrop">
  <dialog open class="dialog-box">

    <h3>{mode === "file" ? "파일 비교" : "폴더 비교"}</h3>

    <div class="container">
      <!-- 좌측 선택 -->
      <div class="panel">
        <span class="label">{mode === "file" ? "좌측 파일 선택" : "좌측 폴더 선택"}</span>
        {#if root}
          <DirTree root={root} mode="view" onselect={selectLeft} />
        {/if}

        <div class="selected-box">
          <strong>좌측:</strong>
          {selectedLeft ? selectedLeft.path : "선택 없음"}
        </div>
      </div>

      <!-- 우측 선택 -->
      <div class="panel">
        <span class="label">{mode === "file" ? "우측 파일 선택" : "우측 폴더 선택"}</span>
        {#if root}
          <DirTree root={root} mode="view" onselect={selectRight} />
        {/if}

        <div class="selected-box">
          <strong>우측:</strong>
          {selectedRight ? selectedRight.path : "선택 없음"}
        </div>
      </div>
    </div>

    <!-- 액션 버튼 -->
    <div class="actions">
      <button
        onclick={confirm}
        disabled={!selectedLeft || !selectedRight}>
        비교
      </button>

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
  width: 780px;
  max-height: 85vh;
  overflow: auto;
  border: 2px solid #4e83db;
  border-radius: 10px;
  padding: 1.4rem;
  background: white;
}

h3 {
  margin-top: 0;
  text-align: center;
  color: #4e83db;
}

/* 좌/우 2패널 */
.container {
  display: flex;
  gap: 1rem;
  margin-top: 1rem;
}

.panel {
  flex: 1;
  border: 1px solid #ddd;
  padding: 0.6rem;
  border-radius: 6px;
  background: #fafafa;
  max-height: 50vh;
  overflow-y: auto;
}

.label {
  font-weight: 600;
  color: #222;
}

.selected-box {
  margin-top: 0.6rem;
  border: 1px solid #ccc;
  padding: 0.5rem;
  border-radius: 6px;
  background: #fff;
  font-size: 0.9rem;
}

/* 버튼 영역 */
.actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1.4rem;
  gap: 0.7rem;
}

button {
  padding: 0.55rem 1.2rem;
  border: none;
  background: #4e83db;
  color: white;
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
