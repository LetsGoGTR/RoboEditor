<script lang="ts">
	import {  fileTree, insertFolderNode } from '@/stores/fileTree';
	import { gotoPage } from '@/stores/currentPage';
	import { _createFolder } from '@apis/folder';
	import { selectedDirectory } from '@/stores/selectedDirectory';
	import { get } from 'svelte/store';
	import SaveDialog from './NewFileDialog.svelte';
	import { _createFile } from '@apis/file';
	import { handleCreateFile, handleDeleteFile, handleDeleteFolder } from '@handlers/nodeActions';
	import DeleteDialog from './DeleteDialog.svelte';
	import type { TreeNode } from '@/types';

	let dialog: HTMLDialogElement;
	let showNewFile = $state(false);
	let showDelete = $state(false);
	let root = $derived(fileTree);

	// async function handleCompare() {

	// 	// ✅ diff 모드 진입
	// 	currentFile.openDiff(left, right);
	// 	gotoPage('compare');
	// }

	function handleApply() { gotoPage('apply'); }
	function handleBackup() { gotoPage('backup'); }
	function handleRegister() { gotoPage('register'); }
	function handleNewFile() {
    if (!root) return alert("워크스페이스가 없습니다.");
    showNewFile = true;
  }

	function handleDelete() {
    showDelete = true;
  }

	function confirmDelete(node: TreeNode) {
    if (node.type === "file") handleDeleteFile(node);
    else if (node.type === "directory") handleDeleteFolder(node);
  }

	// create a new folder
	export async function handleNewFolder() {
		const parent = get(selectedDirectory);
		if (!parent) return alert('상위 폴더를 선택하세요.');

		const name = prompt('새 폴더 이름:');
		if (!name?.trim()) return;

		const clean = name.trim();

		// 경로 구성 시 trailing slash는 시스템 규칙에 맞게 유지
		const newPath = `${parent.path}/${clean}/`;

		// 1) 서버 폴더 생성
		await _createFolder(newPath);

		// 2) store 내부 트리 부분 갱신
		if (!parent.path) return console.error('상위 폴더 경로 관련 오류가 발생하였습니다.');
		insertFolderNode(parent.path, {
			id: crypto.randomUUID(),
			name: clean,
			type: 'directory',
			path: newPath,
			children: []
		});
	}

	// 백업 폴더 불러오기
	// async function handleOpenBackup() {
	// 	const tree = await openDirectory();
	// 	if (tree && tree.type === 'folder') fileTree.set(tree);
	// 	console.log('workspace selected:', fileTree);
	// }
</script>

<!-- 비교 대상 폴더 없음 -->
<dialog bind:this={dialog}>
	<h3>경고</h3>
	<p>비교할 대상이 선택되지 않았습니다. 먼저 항목을 선택하세요.</p>
	<button class="confirm-btn" onclick={() => dialog.close()}>확인</button>
</dialog>

<div class="full-width" role="menubar">
	<ul class="menu">
		<li>
			<span>파일</span>
			<ul class="dropdown">
				<li><button onclick={handleNewFile}>새 텍스트 파일</button></li>
				<li><button onclick={handleNewFolder}>새 폴더</button></li>
				<li><hr /></li>
				<li><button onclick={handleNewFolder}>파일 열기</button></li>
				<li><button >백업 폴더 열기</button></li>
				<li><hr /></li>
				<li><button onclick={handleDelete}>파일 또는 폴더 삭제</button></li>
				<li><hr /></li>
				<li><button>저장</button></li>
				<li><button>다른 이름으로 저장</button></li>
			</ul>
		</li>
		<li>
			<span>도구</span>
			<ul class="dropdown">
				<li><button >비교</button></li>
			</ul>
		</li>
		<li>
			<span>제어기</span>
			<ul class="dropdown">
				<li><button onclick={handleRegister}>제어기 등록</button></li>
				<li><button onclick={handleBackup}>제어기로부터 백업</button></li>
				<li><button onclick={handleApply}>제어기에 적용</button></li>
			</ul>
		</li>
		<li><span>설정</span></li>
	</ul>
</div>

<SaveDialog
  bind:open={showNewFile}
  root={$fileTree}
  onConfirm={handleCreateFile}
  onCancel={() => (showNewFile = false)}
/>

<DeleteDialog
  bind:open={showDelete}
  root={$fileTree}
  onConfirm={confirmDelete}
  onCancel={() => (showDelete = false)}
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
