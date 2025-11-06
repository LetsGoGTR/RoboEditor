<script lang="ts">
  import ControllerList from '../apply/ControllerList.svelte';
  import { dummyController } from '@/testData';
  import type { PageProps } from './$types';
	import FileExplorer from './FileExplorer.svelte';
	import { fileTree } from '@/stores/fileTree';
	import type { TreeNode } from '@/types';

  let { data }: PageProps = $props();
  let tree: TreeNode = $derived($fileTree);

  let selected = $state<string[]>([]);
  let showBackupOnly = $state(false);

  function handleSelectChange(ids: string[]) {
    selected = ids;
  }

  const filteredControllers = $derived(
    showBackupOnly
      ? dummyController.filter(
          (c) => c.state === 'idle' || c.state === 'error'
        )
      : dummyController
  );

  function handleConfirm() {
    alert(`선택된 제어기: ${selected.join(', ')}`);
  }

  function toggleBackupFilter() {
    showBackupOnly = !showBackupOnly;
  }
</script>

<main>
  <div class="header">
    <h1>제어기에 적용하기</h1>
    <label class="filter-toggle">
      <input
        type="checkbox"
        checked={showBackupOnly}
        onchange={toggleBackupFilter}
      />
      적용 가능한 제어기만 보기
    </label>
  </div>

  <ControllerList
    controllers={filteredControllers}
    onSelectChange={handleSelectChange}
  />

  <button disabled={selected.length === 0} onclick={handleConfirm}>
    적용하기
  </button>
</main>

<FileExplorer />

<style>
main {
  margin: 20px;
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

/* 상단 제목과 토글 버튼 정렬 */
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.header h1 {
  margin: 0;
  font-size: 1.4rem;
  font-weight: 600;
}

/* 토글 영역 */
.filter-toggle {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.9rem;
  color: #333;
  user-select: none;
}

.filter-toggle input[type='checkbox'] {
  width: 1rem;
  height: 1rem;
  accent-color: #0078d7;
  cursor: pointer;
}

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
