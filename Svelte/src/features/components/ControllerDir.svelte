<script lang="ts">
  import type { Controller, Workspace } from '@/types';
  import { _getDevice } from '@/apis/controller';
  import { _getWorkspace } from '@apis/workspace';
  import { fetchControllers } from '@handlers/controller';
  import { fileTree } from '@/stores/fileTree';
  import { workspaceContext } from '@/stores/selectedDirectory';
  import { onMount } from 'svelte';
  import type { ApiWorkspaceMetadata } from '@/utils/workspaceApiTransport';
  import { buildWorkspaceEntry, buildWorkspaceFromApi } from '@/utils/workspaceApiTransport';

  let devices = $state<Controller[]>([]);               //디바이스 목록.
  let loadingDevices = $state(false);                   //디바이스 목록 로딩 중 여부
  let deviceError = $state<string | null>(null);        //디바이스 목록 조회 에러 메시지.

  let selectedDevice = $state<Controller | null>(null); //현재 선택된 디바이스.
  let loadingWorkspace = $state(false);                 //워크스페이스 로딩 중 여부.
  let workspaceError = $state<string | null>(null);     //워크스페이스 목록 조회 에러 메시지.


  export async function loadDevices() {
    loadingDevices = true;
    deviceError = null;
    const prevSelected = selectedDevice;
    try {
      devices = await fetchControllers();
      if (prevSelected) {
        const found = devices.find(
          (d) => d.controllerMeta.serialNumber === prevSelected.controllerMeta.serialNumber
        );

        if (found) {
          await handleDeviceSelect(found);
        } else {
          selectedDevice = null;
        }
      }
    } catch (err) {
      console.error(err);
      deviceError = '디바이스 목록 조회 실패';
      devices = [];
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
      const entries: Workspace[] = wsList.map((meta) => buildWorkspaceEntry(meta));
      dev.workspaces = entries;

      devices = devices.map((d) =>
        d.controllerMeta.serialNumber === dev.controllerMeta.serialNumber
          ? { ...d, workspaces: entries }
          : d
      );   
    } catch (err) {
      console.error(err);
      workspaceError = '워크스페이스 조회 중 오류가 발생했습니다.';
      fileTree.set(null);
    } finally {
      loadingWorkspace = false;
    }
  }

  async function handleWorkspaceSelect(dev: Controller, ws: Workspace) {
    workspaceError = null;
    loadingWorkspace = true;

    try {
      const deviceId = dev.controllerMeta.serialNumber;
      const deviceName = dev.controllerMeta.name ?? deviceId;
      const workspaceId = ws.workspaceMeta.uuid;  // buildWorkspaceEntry 에서 넣어둔 uuid

      const res: any = await _getWorkspace(deviceId, workspaceId);

      if (!res.success) {
        workspaceError = '워크스페이스 상세 조회 실패';
        return;
      }

      // 응답 전체를 실제 Workspace 트리로 변환
      const workspaceTree = buildWorkspaceFromApi(res);

      // 오른쪽 탭에서 쓰는 트리 스토어에 세팅
      fileTree.set(workspaceTree);
      workspaceContext.set({
        deviceId,
        deviceName,
        workspaceUuid: workspaceId,
        workspaceName: ws.name
      });
    } catch (err) {
      console.error(err);
      workspaceError = '워크스페이스 상세 조회 중 오류가 발생했습니다.';
      fileTree.set(null);
      workspaceContext.set(null);
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

            <!-- 🔽 디바이스 밑 워크스페이스 목록 -->
            {#if ctrl.workspaces && ctrl.workspaces.length > 0}
              <ul class="workspace-list">
                {#each ctrl.workspaces as ws (ws.id)}
                  <li>
                    <button
                      class="workspace-btn"
                      onclick={() => handleWorkspaceSelect(ctrl, ws)}
                    >
                      <span class="icon">📁</span>
                      {ws.name}
                    </button>
                  </li>
                {/each}
              </ul>
            {:else if selectedDevice &&
              selectedDevice.controllerMeta.serialNumber === ctrl.controllerMeta.serialNumber &&
              !loadingWorkspace && !workspaceError}
              <p class="notice">등록된 워크스페이스가 없습니다.</p>
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

  /* 워크스페이스 목록 스타일 */
  .workspace-list {
    list-style: none;
    margin: 0.2rem 0 0.4rem;
    padding-left: 1.2rem;
    display: flex;
    flex-direction: column;
    gap: 0.1rem;
  }

  .workspace-btn {
    border: none;
    background: transparent;
    padding: 2px 4px;
    font-size: 0.9rem;
    cursor: pointer;
    font-family: 'Consolas', monospace;
    text-align: left;
    display: block;
    width: 100%;
  }

  .workspace-btn:hover {
    background: #f3f5ff;
  }
</style>