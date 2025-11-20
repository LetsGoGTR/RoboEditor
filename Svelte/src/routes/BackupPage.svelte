<script lang="ts">
  import ControllerList from '@features/ApplyControllerList.svelte';
  import type { Controller, ControllerMeta } from '@/types';
  import { _listDevices } from '@/apis/controller';
  import { _ftBackup } from '@/apis/sftp';
  import { onMount } from 'svelte';

  let selected = $state<string[]>([]);
  let showBackupOnly = $state(false);

  let controllers = $state<Controller[]>([]);
  let loading = $state(false);
  let error = $state<string | null>(null);

  function handleSelectChange(ids: string[]) {
    selected = ids;
  }

  function wrapDeviceAsController(raw: any): Controller {
    const meta: ControllerMeta = {
      serialNumber: raw.serialNumber,
      name: raw.name,
      description: raw.description ?? null,
      api: raw.api,
      sftpHost: raw.sftpHost,
      sftpPort: raw.sftpPort,
      sftpUser: raw.sftpUser,
      sftpPassword: raw.sftpPassword,
      createdAt: raw.createdAt,
      updatedAt: raw.updatedAt,
      state: raw.state ?? 'idle'
    };

    return {
      controllerMeta: meta,
      workspaces: []
    };
  }

  async function loadControllers() {
    loading = true;
    error = null;
    try {
      const res: any = await _listDevices();

      if (!res?.success) {
        error = '제어기 목록 조회 실패';
        controllers = [];
        return;
      }

      const rawList = res.data?.devices ?? res.devices ?? [];
      controllers = rawList.map(wrapDeviceAsController);
    } catch (e) {
      console.error(e);
      error = '제어기 목록 조회 중 오류가 발생했습니다.';
      controllers = [];
    } finally {
      loading = false;
    }
  }

  onMount(loadControllers);

  const filteredControllers = $derived(
    showBackupOnly
      ? controllers.filter(
          (c) =>
            c.controllerMeta.state === 'idle' ||
            c.controllerMeta.state === 'error'
        )
      : controllers
  );


  async function handleConfirm() {
    if (selected.length === 0) return;
    alert(`선택된 제어기: ${selected.join(', ')}`);
    const res = await _ftBackup({ deviceId: selected[0] });
    console.log('백업 결과:', res);
  }

  function toggleBackupFilter() {
    showBackupOnly = !showBackupOnly;
  }
</script>

<main>
  <div class="header">
    <h1>제어기로부터 백업</h1>
    <label class="filter-toggle">
      <input
        type="checkbox"
        checked={showBackupOnly}
        onchange={toggleBackupFilter}
      />
      백업 가능한 제어기만 보기
    </label>
  </div>

  {#if loading}
    <p>제어기 목록을 불러오는 중입니다...</p>
  {:else if error}
    <p>{error}</p>
  {:else}
    <ControllerList
      controllers={filteredControllers}
      onSelectChange={handleSelectChange}
    />
  {/if}

  <button disabled={selected.length === 0} onclick={handleConfirm}>
    백업하기
  </button>
</main>

<style>
main {
  margin: 20px;
  display: flex;
  flex-direction: column;
  gap: 1rem;
  min-width: 500px;
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
