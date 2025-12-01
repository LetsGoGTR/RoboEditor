<script lang="ts">
	import { fileTree } from '@/stores/fileTree';
	import { gotoPage } from '@/stores/currentPage';
	import {
		handleCreateFile,
		handleCreateFolder,
		handleDeleteFile,
		handleDeleteFolder
	} from '@handlers/nodeActions';
	import type { TreeNode } from '@/types';
	import NewFileDialog from './dialogs/NewFileDialog.svelte';
	import NewFolderDialog from './dialogs/NewFolderDialog.svelte';
	import DeleteDialog from './dialogs/DeleteDialog.svelte';
	import { handleDiffFiles, handleDiffWorkspaces } from '@handlers/diff';
	import CompareDialog from '@features/dialogs/CompareDialog.svelte';
	import { handleCreateWorkspace } from '@handlers/workspaces';
  	import NewWorkspaceDialog from './dialogs/NewWorkspaceDialog.svelte';

	let warningDialog: HTMLDialogElement;
	let showNewFile = $state(false);
	let showNewFolder = $state(false);
	let showDelete = $state(false);
	let showCompareFile = $state(false);
	let showCompareFolder = $state(false);
	let showNewWorkspace = $state(false);
	let root = $derived($fileTree);


	function clickApply() {
		gotoPage('apply');
	}
	function clickBackup() {
		gotoPage('backup');
	}
	function clickRegister() {
		gotoPage('register');
	}
	function clickNewFile() {
		if (!root) return alert('워크스페이스가 선택되지 않았습니다.');
		showNewFile = true;
	}

	function clickNewFolder() {
		if (!root) return alert('워크스페이스가 선택되지 않았습니다.');
		showNewFolder = true;
	}

	function clickNewWorkspace() {
		showNewWorkspace = true;
	}

	function clickDelete() {
		showDelete = true;
	}

	function confirmDelete(node: TreeNode) {
		if (node.type === 'file') handleDeleteFile(node);
		else if (node.type === 'directory') handleDeleteFolder(node);
	}

	function clickCompareFiles() {
		if (!root) return warningDialog.showModal();
		showCompareFile = true;
	}

	function clickCompareFolders() {
		if (!root) return warningDialog.showModal();
		showCompareFolder = true;
	}

	async function handleNewWorkspaceConfirm(payload: {
		deviceId: string;
		name: string;
		description?: string | null;
	}) {
		const { deviceId, name, description } = payload;

		if (!deviceId || !name.trim()) {
		alert('디바이스와 워크스페이스 이름을 확인해 주세요.');
		return;
		}

		await handleCreateWorkspace(deviceId, {
		name: name.trim(),
		description: description?.trim() || undefined
		});

		showNewWorkspace = false;
	}
</script>

<div class="full-width" role="menubar">
	<ul class="menu">
		<!-- 파일 탭 -->
		<li>
			<span>파일</span>
			<ul class="dropdown">
				<li><button onclick={clickNewFile}>새 텍스트 파일</button></li>
				<li><button onclick={clickNewFolder}>새 폴더</button></li>
				<li><button onclick={clickNewWorkspace}>새 워크스페이스</button></li>
				<li><hr /></li>
				<li><button onclick={clickDelete}>파일 또는 폴더 삭제</button></li>
				<li><hr /></li>
				<li><button>저장</button></li>
				<li><button>다른 이름으로 저장</button></li>
			</ul>
		</li>
		<!-- 도구 탭 -->
		<li>
			<span>도구</span>
			<ul class="dropdown">
				<li><button onclick={clickCompareFiles}>파일 비교</button></li>
				<li><button onclick={clickCompareFolders}>폴더 비교</button></li>
			</ul>
		</li>
		<!-- 제어기 탭 -->
		<li>
			<span>제어기</span>
			<ul class="dropdown">
				<li><button onclick={clickRegister}>제어기 등록</button></li>
				<li><button onclick={clickBackup}>제어기로부터 백업</button></li>
				<li><button onclick={clickApply}>제어기에 적용</button></li>
			</ul>
		</li>
		<li><span>설정</span></li>
	</ul>
</div>

<!-- 비교 대상 폴더 없음 -->
<dialog bind:this={warningDialog}>
	<h3>경고</h3>
	<p>먼저 백업 폴더를 선택하여 주십시요.</p>
	<button class="confirm-btn" onclick={() => warningDialog.close()}>확인</button>
</dialog>

<NewFileDialog
	bind:open={showNewFile}
	root={$fileTree}
	onConfirm={handleCreateFile}
	onCancel={() => (showNewFile = false)}
/>

<NewFolderDialog
	bind:open={showNewFolder}
	root={$fileTree}
	onConfirm={handleCreateFolder}
	onCancel={() => (showNewFolder = false)}
/>

<NewWorkspaceDialog
	bind:open={showNewWorkspace}
	onConfirm={handleNewWorkspaceConfirm}
	onCancel={() => (showNewWorkspace = false)}
/>

<DeleteDialog
	bind:open={showDelete}
	root={$fileTree}
	onConfirm={confirmDelete}
	onCancel={() => (showDelete = false)}
/>

<CompareDialog
	bind:open={showCompareFile}
	root={$fileTree}
	mode="file"
	onConfirm={async ({ left, right }) => {
		if (left.type !== "file" || right.type !== "file") return;
		await handleDiffFiles(left, right);
		gotoPage('compare');
	}}
	onCancel={() => (showCompareFile = false)}
/>

<CompareDialog
	bind:open={showCompareFolder}
	root={$fileTree}
	mode="directory"
	onConfirm={async ({ left, right }) => {
		if (left.type !== "directory" || right.type !== "directory") return;
		await handleDiffWorkspaces(left, right);
		gotoPage('compare');
	}}
	onCancel={() => (showCompareFolder = false)}
/>

<style>
	/* dialog styles */
	dialog {
		border: #4e83db solid 2px;
		border-radius: 8px;
		padding: 1.2rem 1.5rem;
		text-align: center;
	}

	dialog::backdrop {
		background: rgba(0, 0, 0, 0.3);
	}

	dialog > h3 {
		margin-top: 0;
		color: #3f51b5; /* 상단 메뉴와 동일한 블루톤 */
		font-size: 1.1rem;
	}

	dialog > p {
		margin: 1rem 0;
		font-size: 0.95rem;
		line-height: 1.4;
		color: #444;
	}

	.confirm-btn {
		background: #4e83db;
		color: white;
		border: none;
		border-radius: 6px;
		padding: 0.5rem 1.2rem;
		font-size: 0.9rem;
		cursor: pointer;
		transition: background 0.2s ease;
	}

	.confirm-btn:hover {
		background: #32419c;
	}

	/* nav styles */
	.full-width {
		display: block; /* 인라인 요소라면 block으로 변경 */
		width: 100%; /* 부모 너비의 100% 차지 */
		background: #4e83db;
	}

	.menu {
		list-style: none;
		margin: 0;
		padding: 0;
		display: flex;
	}

	.menu > li {
		position: relative;
	}

	ul > li > span {
		font-size: 16px;
		cursor: pointer;
		display: block;
		padding: 10px 20px;
		color: #fff;
		text-decoration: none;
		background: none;
		border: none;
	}

	ul > li > button {
		font-size: 14px;
		cursor: pointer;
		display: block;
		padding: 6px 12px;
		color: #fff;
		text-decoration: none;
		background: none;
		border: none;
	}

	ul > li:hover {
		background: #fff;
	}

	ul > li:hover > span,
	ul > li:hover > button {
		color: #000;
	}

	ul > li > hr {
		margin: 1px 0;
		padding: 0;
	}

	/* 드롭다운 숨김 */
	.dropdown {
		display: none;
		position: absolute;
		top: 100%;
		left: 0;

		min-width: 200px;

		list-style: none;
		margin: 0;
		padding: 0;
		background: #4675c7;
	}

	/* 마우스 오버 시 표시 */
	.menu > li:hover .dropdown {
		display: block;
	}
</style>
