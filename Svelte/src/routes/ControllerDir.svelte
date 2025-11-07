<script lang="ts">
	import { openDirectory } from '@/utils/FSA';
	import type { FolderNode } from '@/types';
  import { fileTree } from '@/stores/fileTree';

	let backupTree: FolderNode | null = null;

	async function handleOpenBackup() {
		const tree = await openDirectory();
		if (tree && tree.type === 'folder') backupTree = tree;
	}

	function handleWorkspaceSelect(ws: FolderNode) {
    fileTree.set(ws);
    console.log('workspace selected:', ws.name);
  }
</script>

<div class="backup-browser">
	{#if backupTree}
		<ul class="controller-list" role="list">
			{#each backupTree.children as controller (controller.id)}
				{#if controller.type === 'folder'}
					<li>
						<details>
							<summary class="controller-btn">
								<span class="icon">⚙️</span> {controller.name}
							</summary>

							<ul class="workspace-list" role="list">
								{#each controller.children as ws (ws.id)}
									{#if ws.type === 'folder'}
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
		<button on:click={handleOpenBackup} class="open-btn">
			📁 Open Backup Folder
		</button>
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
</style>
