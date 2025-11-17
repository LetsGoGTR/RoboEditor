<script lang="ts">
  import ControllerList from '@features/ApplyControllerList.svelte';
  import { dummyController } from '@/testData';
	import type { TreeNode } from '@/types';
  import FileExplorer from '@features/FileExplorer.svelte';
  import ApplyDialog from '@features/dialogs/ApplyDialog.svelte';
	import { get } from 'svelte/store';
	import { fileTree } from '@/stores/fileTree';
  
  let selected = $state<string[]>([]);
  let showBackupOnly = $state(false);
  let selectTree = $state<TreeNode | null>(null);

  function handleSelectChange(ids: string[]) {
    selected = ids;
  }

  function toggleBackupFilter() {
    showBackupOnly = !showBackupOnly;
  }

  const filteredControllers = $derived(
    showBackupOnly
      ? dummyController.filter(
          (c) => c.state === 'idle' || c.state === 'error'
        )
      : dummyController
  );

  // button
  const hasControllers = $derived(selected.length > 0);
  const hasFolder = $derived(!!selectTree);
  const primaryDisabled = $derived(!hasControllers);
  const primaryLabel = $derived(
    !hasControllers ? '제어기를 선택하세요'
    : !hasFolder    ? '폴더 선택'
                    : '폴더 교체'
  );

  // dialog
  let applyDlg: any;
  async function handleConfirm() {
    try {
      if (tree && tree.type === 'folder') {
        selectTree = tree;
      }
    } catch (err) {
      console.error('openDirectory 실패:', err);
    }
    return;
  }

  async function handleApply() {
    applyDlg?.open();
  }

  async function handleApplyDialog(event: CustomEvent<{ password: string }>) {
    const { password } = event.detail;
    console.log('적용 실행', { selected, selectTree, password });
    // TODO: 실제 적용 로직
    if (!selectTree) return;

    const archiveName = 'app.tar.gz';
    const localPath = `${selectTree.path}/${archiveName}`;
    const remotePath = `/home/default/${archiveName}`;
    try {
      const targets = filteredControllers
        .filter((c) => selected.includes(c.serialNumber))
        .map((c) => ({
          host: c.ipAddress,           // 또는 c.host / c.address 등 실제 필드에 맞게
          port: 22, // 이미 number면 Number() 없어도 됨
          username: 'default', // 또는 c.user / c.id 등 실제 계정 필드
          password: '1234',             // 다이얼로그에서 받은 비번
        }));

      const res = await fetch('/api', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          targets,
          localPath,
          remotePath
        }),
      });

      if (!res.ok) {
        const msg = await res.text();
        throw new Error(`서버 에러(${res.status}): ${msg}`);
      }

      const data = await res.json();
      console.log('적용 성공 :', data);
    } catch (err) {
      console.error('요청 실패:', err);
    }
  }
</script>

<main>
  <div class="header">
    <h1>제어기에 적용하기</h1>
  </div>

  <div class="workspace">
    <div class="left-panel">
      <label class="filter-toggle">
        <input
          type="checkbox"
          checked={showBackupOnly}
          onchange={toggleBackupFilter}
        />
        적용 가능한 제어기만 보기
      </label>

      <ControllerList
        controllers={filteredControllers}
        onSelectChange={handleSelectChange}
      />

      <button disabled={primaryDisabled} onclick={handleConfirm}>
        {primaryLabel}
      </button>

      {#if hasControllers && hasFolder}
        <button class="apply" onclick={handleApply}>적용하기</button>
      {/if}
    </div>

    <div class="right-panel">
      {#if selectTree}
        <FileExplorer root={selectTree} />
      {/if}
    </div>
  </div>
  <ApplyDialog
    bind:this={applyDlg}
    {selected}
    folderName={selectTree?.name ?? ''}
    on:apply={handleApplyDialog}
  />
</main>

<style>
main {
  margin: 20px;
  display: flex;
  flex-direction: column;
  gap: 1rem;
  height: 90vh;
}

/* ─────────────────────────────── */
/* 상단 제목 */
.title {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.title h1 {
  margin: 0;
  font-size: 1.4rem;
  font-weight: 600;
}

/* ─────────────────────────────── */
/* 전체 좌우 영역 */
.workspace {
  flex: 1;
  display: flex;
  flex-direction: row;
  gap: 1.5rem;
  overflow: hidden;
}

/* 왼쪽: 체크박스 + 리스트 + 버튼 */
.left-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 1rem;
  overflow-y: auto;
}

/* 오른쪽: 파일 탐색기 */
.right-panel {
  flex: 1.2;
  border: 1px solid #ddd;
  border-radius: 0.4rem;
  padding: 0.5rem;
  overflow-y: auto;
}

.empty {
  color: #777;
  text-align: center;
  margin-top: 2rem;
}

/* 체크박스 토글 스타일 */
.filter-toggle {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.9rem;
  color: #333;
  user-select: none;
  justify-content: flex-end;
}

.filter-toggle input[type='checkbox'] {
  width: 1rem;
  height: 1rem;
  accent-color: #0078d7;
  cursor: pointer;
}

/* 버튼 */
button {
  padding: 0.6rem 1rem;
  font-size: 1rem;
  background: #0078d7;
  color: #fff;
  border: none;
  border-radius: 0.4rem;
  cursor: pointer;
  transition: background 0.2s ease;
}

button:hover:enabled {
  background: #005fa3;
}

button:disabled {
  background: #ccc;
  cursor: not-allowed;
}
</style>