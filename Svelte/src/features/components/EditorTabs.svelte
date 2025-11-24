<script lang="ts">
	import GroupTabs from '@layouts/GroupTabs.svelte';
	import { currentFile } from '@/stores/currentFile';
	import { onDestroy, onMount, tick } from 'svelte';
	import type { FileNode } from '@/types';

	import { getMonaco } from '@utils/monaco';
	import type * as monaco from 'monaco-editor';
	import { _getFile } from '@apis/file';
	import { handleSaveFile, loadFile } from '@handlers/nodeActions';

	let container: HTMLDivElement | null = null;
	let editor: monaco.editor.IStandaloneCodeEditor | null = null;
	let monacoInstance: typeof monaco | null = null;

	let state = $derived($currentFile);

	/* ------------------------------------------------------------
	 * 초기화
	 * ------------------------------------------------------------ */
	onMount(async () => {
		await tick();

		monacoInstance = await getMonaco();
		const activeFile = state.active?.file;

		if (!monacoInstance || !container) return;

		editor = monacoInstance.editor.create(container, {
			theme: 'vs-white',
			automaticLayout: true,
			minimap: { enabled: false }
		});

		if (activeFile) {
			editor = await loadFile(monacoInstance, editor, activeFile);
		}

		editor?.addCommand(monacoInstance.KeyMod.CtrlCmd | monacoInstance.KeyCode.KeyS, () => {
			handleSaveFile(editor!);
		});

		editor?.onDidChangeModelContent(() => {
			currentFile.setContentManual(editor!.getValue());
		});
	});

	/* ------------------------------------------------------------ */
	async function tryInitEditor() {
		await tick();
		if (!container || !monacoInstance) return;

		if (!editor) {
			editor = monacoInstance.editor.create(container, {
				theme: 'vs-white',
				automaticLayout: true,
				minimap: { enabled: false }
			});

			editor.addCommand(monacoInstance.KeyMod.CtrlCmd | monacoInstance.KeyCode.KeyS, () => {
				handleSaveFile(editor!);
			});

			editor.onDidChangeModelContent(() => {
				currentFile.setContentManual(editor!.getValue());
			});
		}

		const file = state.active?.file;
		if (file) {
			editor = await loadFile(monacoInstance, editor, file);
		}
	}

	/* ------------------------------------------------------------
	 * Tab 전환 / 닫기
	 * ------------------------------------------------------------ */
	function handleSwitch(index: number) {
		currentFile.switchTab(index);
	}

	function handleClose(index: number) {
		currentFile.closeTab(index);
	}

	$effect(() => {
		const file = $currentFile.active?.file;

		if (!monacoInstance) return;

		// 파일이 바뀌었을 때만 모델 교체
		if (file) {
			if (editor) {
				// 현재 editor의 모델이 다른 파일이면 로드
				const currentPath = editor.getModel()?.uri.path;
				if (currentPath !== file.path) {
					tryInitEditor();
				}
			} else {
				tryInitEditor();
			}
		} else {
			// 탭이 모두 닫힌 경우만 dispose
			if (editor) {
				editor.dispose();
				editor = null;
			}
		}
	});

	onDestroy(() => {
		if (editor) {
			editor.dispose();
			editor = null;
		}
	});
</script>

<div class="editor-tabs-root">
	<GroupTabs
		tabs={state.group
			.map((g) => g.file)
			.filter((file): file is FileNode => file !== null)
			.map((file) => ({
				id: file.id,
				name: file.name,
				path: file.path
			}))}
		activeIndex={state.activeIndex}
		onSwitch={handleSwitch}
		onClose={handleClose}
	>
		<div slot="content" bind:this={container} class="editor-container"></div>
	</GroupTabs>
</div>
<button class="save-btn" onclick={() => handleSaveFile(editor!)}>저장</button>

<style>
	.editor-tabs-root {
		display: flex;
		position: relative;
		flex-direction: column;
		flex: 1;
		height: 100%;
		width: 100%;
		overflow: hidden;
	}

	.editor-container {
		flex: 1;
		width: 100%;
		height: 100%;
	}

	.save-btn {
		position: absolute;
		bottom: 24px;
		right: 24px;

		width: 72px;
		height: 36px;
		line-height: 36px;

		border: none;
		outline: none;
		border-radius: 10px;

		background-color: #4e83db;
		color: white;

		font-size: 15px;
		font-weight: 500;
		font-family:
			system-ui,
			-apple-system,
			BlinkMacSystemFont,
			'Segoe UI',
			Roboto,
			Helvetica,
			Arial,
			sans-serif;
		text-align: center;
		cursor: pointer;

		transition:
			background-color 0.2s ease,
			transform 0.15s ease;

		user-select: none;
		z-index: 999;
	}

	.save-btn:hover {
		background-color: #3b6fc5;
		transform: translateY(-2px);
	}

	.save-btn:active {
		transform: translateY(0);
	}
</style>
