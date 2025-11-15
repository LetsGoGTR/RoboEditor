<script lang="ts">
  import DirTree from "@components/DirectoryTree.svelte";
  import type { FolderNode, TreeNode } from "@/types";

  let {
    open = $bindable(false),
    root,
    onConfirm,
    onCancel
  } = $props<{
    open: boolean;
    root: FolderNode | null;
    onConfirm: (payload: { name: string; folder: FolderNode }) => void;
    onCancel: () => void;
  }>();

  let filename = $state("");
  let selectedFolder = $state<FolderNode | null>(null);

  function handleSelect(node: TreeNode) {
    if (node.type === "directory") selectedFolder = node;
  }

  function confirm() {
    const name = filename.trim();
    if (!name || !selectedFolder) return;
    onConfirm({ name, folder: selectedFolder });
    close();
  }

  function close() {
    filename = "";
    selectedFolder = null;
    onCancel();
  }
</script>

{#if open}
<div class="dialog-backdrop">
  <dialog open class="dialog-box">

    <h3>새 파일 만들기</h3>

    <!-- 파일명 입력 -->
    <div class="form-row">
      <label for="filename">파일명 :</label>
      <input
        id="filename"
        type="text"
        bind:value={filename}
        placeholder="새 파일명을 입력하세요"
        onkeydown={(e) => e.key === "Enter" && confirm()}
      />
    </div>

    <!-- 폴더 선택 안내 -->
    <div class="folder-section">
      <span class="label">저장 위치 선택:</span>
      {#if root}
        <DirTree root={root} mode="view" onselect={handleSelect} />
      {/if}
    </div>

    <!-- 선택된 폴더 경로 표시 -->
    <div class="form-row">
      <label for="foder-path">폴더 경로 :</label>
      <input
        id="foder-path"
        type="text"
        class="path-input"
        readonly
        value={selectedFolder ? selectedFolder.path : ""}
      />
    </div>

    <div class="actions">
      <button onclick={confirm}>확인</button>
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
  border: 2px solid #4e83db;
  border-radius: 8px;
  padding: 1.4rem;
  background: white;
}

/* 라벨 + 인풋 한 줄 정렬 */
.form-row {
  display: flex;
  align-items: center;
  margin-top: 0.9rem;
  gap: 0.6rem;
}

.form-row label {
  width: 90px; /* 정렬용 고정폭 */
  font-weight: 500;
  color: #222;
}

.form-row input {
  flex: 1;
  padding: 0.45rem 0.6rem;
  border: 1px solid #ccc;
  border-radius: 6px;
  font-size: 0.95rem;
}

/* 폴더 선택 트리 영역 */
.folder-section {
  margin-top: 1rem;
}

.label {
  font-weight: 500;
  color: #222;
}

.folder-section {
  margin-bottom: 0.6rem;
  padding: 0.5rem 0.3rem;
  border-radius: 6px;
  max-height: 240px;
  overflow-y: auto;
  border: 1px solid #ddd;
}

/* 버튼 영역 */
.actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1.4rem;
  gap: 0.7rem;
}

button {
  padding: 0.5rem 1rem;
  border: none;
  background: #4e83db;
  color: #fff;
  border-radius: 6px;
  cursor: pointer;
}

.cancel {
  background: #777;
}
</style>
