<script lang="ts">
  import { fetchControllers } from '@handlers/controller';
  import type { Controller } from '@/types';

  let {
    open = $bindable(false),
    onConfirm,
    onCancel
  } = $props<{
    open: boolean;
    onConfirm: (payload: { deviceId: string; name: string; description?: string | null }) => void;
    onCancel: () => void;
  }>();

  let devices = $state<Controller[]>([]);
  let loading = $state(false);
  let error = $state<string | null>(null);

  let selectedDeviceId = $state('');
  let name = $state('');
  let description = $state('');

  $effect(() => {
    if (!open) return;

    loading = true;
    error = null;

    (async () => {
        try {
        devices = await fetchControllers(); // 또는 너가 만든 loadDevices 헬퍼
        } catch (e) {
        console.error(e);
        error = '디바이스 목록을 불러올 수 없습니다.';
        devices = [];
        } finally {
        loading = false;
        }
    })();
    });

  function confirm() {
    const trimmedName = name.trim();

    if (!selectedDeviceId) {
      alert('디바이스를 선택해 주세요.');
      return;
    }
    if (!trimmedName) {
      alert('워크스페이스 이름을 입력해 주세요.');
      return;
    }

    onConfirm({
      deviceId: selectedDeviceId,
      name: trimmedName,
      description: description.trim() || null
    });

    close();
  }

  function close() {
    name = '';
    description = '';
    selectedDeviceId = '';
    onCancel();
  }
</script>

{#if open}
  <div class="dialog-backdrop">
    <dialog open class="dialog-box">
      <h3>새 워크스페이스 만들기</h3>

      <!-- 디바이스 선택 -->
      <div class="form-row">
        <label for="device">디바이스 :</label>
        {#if loading}
          <span>디바이스 목록 로딩 중...</span>
        {:else if error}
          <span class="error">{error}</span>
        {:else}
          <select id="device" bind:value={selectedDeviceId}>
            <option value="">디바이스를 선택하세요</option>
            {#each devices as dev (dev.controllerMeta.serialNumber)}
              <option value={dev.controllerMeta.serialNumber}>
                {dev.controllerMeta.name ?? dev.controllerMeta.serialNumber}
              </option>
            {/each}
          </select>
        {/if}
      </div>

      <!-- 워크스페이스 이름 -->
      <div class="form-row">
        <label for="ws-name">이름 :</label>
        <input
          id="ws-name"
          type="text"
          bind:value={name}
          placeholder="워크스페이스 이름"
          onkeydown={(e) => e.key === 'Enter' && confirm()}
        />
      </div>

      <!-- 설명(옵션) -->
      <div class="form-row">
        <label for="ws-desc">설명 :</label>
        <input
          id="ws-desc"
          type="text"
          bind:value={description}
          placeholder="설명 (선택)"
          onkeydown={(e) => e.key === 'Enter' && confirm()}
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
    background: rgba(0, 0, 0, 0.35);
    display: flex;
    justify-content: center;
    align-items: center;
  }

  .dialog-box {
    width: 420px;
    border: 2px solid #4e83db;
    border-radius: 8px;
    padding: 1.4rem;
    background: white;
  }

  .form-row {
    display: flex;
    align-items: center;
    margin-top: 0.9rem;
    gap: 0.6rem;
  }

  .form-row label {
    width: 90px;
    font-weight: 500;
    color: #222;
  }

  .form-row input,
  .form-row select {
    flex: 1;
    padding: 0.45rem 0.6rem;
    border: 1px solid #ccc;
    border-radius: 6px;
    font-size: 0.95rem;
  }

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

  .error {
    color: #c00;
    font-size: 0.9rem;
  }
</style>
