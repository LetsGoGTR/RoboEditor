<script lang="ts">
	import DirTree from '@components/DirectoryTree.svelte';
	import type { FolderNode, TreeNode } from '@/types';
	import { toWorkspaceDisplayPath } from '@utils/nodeAction';

	let {
		open = $bindable(false),
		root,
		onConfirm,
		onCancel
	} = $props<{
		open: boolean;
		root: FolderNode | null;
		onConfirm: (payload: { name: string; folder: FolderNode }) => void;
		onCancel: () => void;
	}>();

	let foldername = $state('');
	let selectedFolder = $state<FolderNode | null>(null);

	function handleSelect(node: TreeNode) {
		if (node.type === 'directory' && node.path) {
			selectedFolder = node as FolderNode;
		}
	}

	function confirm() {
		const name = foldername.trim();
		if (!name || !selectedFolder || !selectedFolder.path) return;

		onConfirm({ name, folder: selectedFolder });
		close();
	}

	function close() {
		foldername = '';
		selectedFolder = null;
		onCancel();
	}
</script>

{#if open}
	<div class="dialog-backdrop">
		<dialog open class="dialog-box">
			<h3>새 폴더 만들기</h3>

			<!-- 폴더명 입력 -->
			<div class="form-row">
				<label for="foldername">폴더명 :</label>
				<input
					id="foldername"
					type="text"
					bind:value={foldername}
					placeholder="새 폴더명을 입력하세요"
					onkeydown={(e) => e.key === 'Enter' && confirm()}
				/>
			</div>

			<!-- 저장 위치 선택 -->
			<div class="folder-section">
				<span class="label">상위 폴더 선택:</span>
				{#if root}
					<DirTree {root} mode="view" onselect={handleSelect} />
				{/if}
			</div>

			<!-- 선택된 폴더 경로 -->
			<div class="form-row">
				<label for="folder-path">폴더 경로 :</label>
				<input
					id="folder-path"
					type="text"
					class="path-input"
					readonly
					value={selectedFolder ? toWorkspaceDisplayPath(selectedFolder.path) : ''}
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
	/* NewFileDialog와 동일 스타일 */
	.dialog-backdrop {
		position: fixed;
		inset: 0;
		background: rgba(0, 0, 0, 0.35);
		display: flex;
		justify-content: center;
		align-items: center;
	}

	.dialog-box {
		width: 520px;
		max-height: 80vh;
		overflow: auto;
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

	.form-row input {
		flex: 1;
		padding: 0.45rem 0.6rem;
		border: 1px solid #ccc;
		border-radius: 6px;
		font-size: 0.95rem;
	}

	.folder-section {
		margin-top: 1rem;
		margin-bottom: 0.6rem;
		padding: 0.5rem 0.3rem;
		border-radius: 6px;
		max-height: 240px;
		overflow-y: auto;
		border: 1px solid #ddd;
	}

	.label {
		font-weight: 500;
		color: #222;
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
</style>
