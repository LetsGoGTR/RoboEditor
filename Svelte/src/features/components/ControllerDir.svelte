<script lang="ts">
  import type { Controller } from '@/types';
  import { _listDevices, _getDevice } from '@/apis/controller';
  import { wrapDeviceAsController } from '@handlers/controller';
  import { fileTree } from '@/stores/fileTree';
  import { onMount } from 'svelte';
  import type { ApiWorkspaceMetadata } from '@/utils/workspaceApiTransport';
  import { buildWorkspaceEntry, buildControllerRoot } from '@/utils/workspaceApiTransport';

  let devices = $state<Controller[]>([]);               //디바이스 목록.
  let loadingDevices = $state(false);                   //디바이스 목록 로딩 중 여부
  let deviceError = $state<string | null>(null);        //디바이스 목록 조회 에러 메시지.

  let selectedDevice = $state<Controller | null>(null); //현재 선택된 디바이스.
  let loadingWorkspace = $state(false);                 //워크스페이스 로딩 중 여부.
  let workspaceError = $state<string | null>(null);     //워크스페이스 목록 조회 에러 메시지.


  export async function loadDevices() {
    loadingDevices = true;
    deviceError = null;
    try {
      const res: any = await _listDevices();
      if (!res.success) {
        deviceError = '디바이스 목록 조회 실패';
        devices = [];
        return;
      }
      devices = (res.data?.devices ?? []).map((raw: any) => wrapDeviceAsController(raw));
    } finally {
      loadingDevices = false;
    }
  }

  onMount(() => {
    loadDevices();
  });

  // 🔥 컨트롤러 클릭 → device-id로 workspace 가져와서 FolderNode로 만들기
  async function handleDeviceSelect(dev: Controller) {
    selectedDevice = dev;
    workspaceError = null;
    loadingWorkspace = true;

    try {
      const deviceId = dev.controllerMeta.serialNumber;
      const res: any = await _getDevice(deviceId);

      if (!res.success) {
        workspaceError = '워크스페이스 조회 실패';
        return;
      }

      const wsList: ApiWorkspaceMetadata[] = res.workspaces?.workspaces ?? [];

      dev.workspaces = wsList.map((meta) => buildWorkspaceEntry(meta));
      
      const deviceNode = buildControllerRoot(dev.controllerMeta, dev.workspaces);

      fileTree.set(deviceNode);    
    } catch (err) {
      console.error(err);
      workspaceError = '워크스페이스 조회 중 오류가 발생했습니다.';
      fileTree.set(null);
    } finally {
      loadingWorkspace = false;
    }
  }
</script>

<div class="backup-browser">
  {#if loadingDevices}
    <p class="notice">디바이스 목록을 불러오는 중입니다...</p>
  {:else if deviceError}
    <p class="notice">{deviceError}</p>
  {:else if Array.isArray(devices) && devices.length === 0}
    <div class="notice">
      <p>디바이스 목록이 없습니다.</p>
    </div>
  {:else}
    <ul class="controller-list" role="list">
      {#each devices as ctrl (ctrl.controllerMeta.serialNumber)}
        <li>
          <details
            open={selectedDevice &&
              selectedDevice.controllerMeta.serialNumber === ctrl.controllerMeta.serialNumber}
          >
            <summary
              class="controller-btn"
              onclick={(event) => {
                event.preventDefault();
                handleDeviceSelect(ctrl);
              }}
            >
              <span class="icon">⚙️</span>
              {ctrl.controllerMeta.name ?? ''}
              {` (${ctrl.controllerMeta.serialNumber})`}
            </summary>

            {#if loadingWorkspace &&
              selectedDevice &&
              selectedDevice.controllerMeta.serialNumber === ctrl.controllerMeta.serialNumber}
              <p class="notice">워크스페이스 정보 불러오는 중...</p>
            {:else if workspaceError &&
              selectedDevice &&
              selectedDevice.controllerMeta.serialNumber === ctrl.controllerMeta.serialNumber}
              <p class="notice">{workspaceError}</p>
            {/if}
          </details>
        </li>
      {/each}
    </ul>
  {/if}
</div>

<style>
  ul {
    list-style: none;
    margin: 2px 0;
    padding: 0;
  }

  details summary.controller-btn {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    cursor: pointer;
    padding: 2px 4px;
    border-radius: 4px;
    user-select: none;
    font-family: 'Consolas', monospace;
    font-size: 1rem;
  }

  details summary.controller-btn:hover {
    background: #eef4ff;
  }

  details summary::-webkit-details-marker {
    display: none;
  }
  details summary::marker {
    content: "";
  }

  .controller-btn {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    cursor: pointer;
    background: transparent;
    border: none;
    border-radius: 4px;
    padding: 2px 4px;
    text-align: left;
    font-family: 'Consolas', monospace;
    font-size: 1rem;
  }

  .controller-btn:hover {
    background: #eef4ff;
  }

  .icon {
    width: 1.2rem;
    text-align: center;
  }

  .backup-browser {
    display: flex;
    flex-direction: column;
    padding: 0.25rem 0.5rem;
    gap: 0.4rem;
  }

  .notice {
    font-size: 0.95rem;
    color: #333;
    text-align: center;
    padding: 0.8rem 0;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
    align-items: center;
  }
</style>
