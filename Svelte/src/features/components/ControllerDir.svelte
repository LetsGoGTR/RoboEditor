<script lang="ts">
<<<<<<< HEAD
	import type { FolderNode } from '@/types';
  import { fileTree } from '@/stores/fileTree';

	let backupTree: FolderNode | null = null;

	/** config/controllers.json 파일 존재 여부 확인 */
	function hasConfigControllers(folder: FolderNode): boolean {
		const configFolder = folder.children?.find(
			(child) => child.type === 'directory' && child.name === 'config'
		);

		if (!configFolder) return false;

		const controllersFile = configFolder.children?.find(
			(file) => file.type === 'file' && file.name === 'controllers.json'
		);

		return !!controllersFile;
	}

	function handleWorkspaceSelect(ws: FolderNode) {
    fileTree.set(ws);
    console.log('workspace selected:', ws.name);
=======
  import type { Controller, ControllerMeta, Workspace, WorkspaceMeta, FolderNode } from '@/types';
  import { _listDevices, _getDevice } from '@/apis/controller';
  import { fileTree } from '@/stores/fileTree';
  import { onMount } from 'svelte';

  let devices = $state<Controller[]>([]);               //디바이스 목록.
  let loadingDevices = $state(false);                   //디바이스 목록 로딩 중 여부
  let deviceError = $state<string | null>(null);        //디바이스 목록 조회 에러 메시지.

  let selectedDevice = $state<Controller | null>(null); //현재 선택된 디바이스.
  let loadingWorkspace = $state(false);                 //워크스페이스 로딩 중 여부.
  let workspaceError = $state<string | null>(null);     //워크스페이스 목록 조회 에러 메시지.

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
      state: 'idle' // 일단 기본값
    };

    return {
      controllerMeta: meta,
      workspaces: []
    };
  }

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
      const rawDevices = res.data?.devices ?? [];
      devices = rawDevices.map(wrapDeviceAsController);
    } catch (err) {
      console.error(err);
      deviceError = '디바이스 목록 조회 중 오류가 발생했습니다.';
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

      const wsList: WorkspaceMeta[] = res.workspaces?.workspaces ?? [];

      dev.workspaces = wsList.map((ws) => ({
        id: ws.uuid,
        name: ws.name,
        type: 'directory',
        path: null,
        children: [],
        workspaceMeta: ws
      }));
      
      const deviceNode: FolderNode = {
        id: `device:${deviceId}`,
        name: dev.controllerMeta.name ?? deviceId,
        type: 'directory',
        path: `${deviceId}`,
        children: dev.workspaces
      };

      fileTree.set(deviceNode);    
    } catch (err) {
      console.error(err);
      workspaceError = '워크스페이스 조회 중 오류가 발생했습니다.';
      fileTree.set(null);
    } finally {
      loadingWorkspace = false;
    }
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
  }
</script>

<div class="backup-browser">
<<<<<<< HEAD
	{#if backupTree}
		<ul class="controller-list" role="list">
			{#each backupTree.children as controller (controller.id)}
				{#if controller.type === 'directory'}
					<li>
						<details>
							<summary class="controller-btn">
								<span class="icon">⚙️</span> {controller.name}
							</summary>

							<ul class="workspace-list" role="list">
								{#each controller.children as ws (ws.id)}
									{#if ws.type === 'directory'}
										<li>
											<button
												type="button"
												class="workspace-btn"
												on:click={() => handleWorkspaceSelect(ws)}
											>
												<span class="icon">📁</span> {ws.name}
											</button>
										</li>
									{/if}
								{/each}
							</ul>
						</details>
					</li>
				{/if}
			{/each}
		</ul>
	{:else}
    <p class="notice">아직 백업 폴더를 열지 않았습니다.</p>
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
	padding: 2px 4px;              /* DirectoryTree의 summary, .file과 동일 */
	border-radius: 4px;
	user-select: none;
	font-family: 'Consolas', monospace;
	font-size: 1rem;
}

details summary.controller-btn:hover {
	background: #eef4ff;
}

/* 기본 삼각형 아이콘 제거 */
details summary::-webkit-details-marker {
	display: none;
}
details summary::marker {
	content: "";
}

/* Workspace 목록 들여쓰기 */
.workspace-list {
	margin: 0;
	padding-left: 1rem;
}

/* Controller / Workspace 버튼 공통 */
.controller-btn,
.workspace-btn {
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

.controller-btn:hover,
.workspace-btn:hover {
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

/* 열기 버튼 */
.open-btn {
	align-self: center;
	background: #0077cc;
	color: white;
	padding: 0.6rem 1.2rem;
	border: none;
	border-radius: 0.4rem;
	cursor: pointer;
}
.open-btn:hover {
	background: #005fa3;
}

.notice {
  font-size: 0.95rem;
	color: #333;
	text-align: center;
	padding: 0.8rem 0;
}
=======
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
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
</style>
