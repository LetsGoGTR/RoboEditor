<script lang="ts">
	import GroupTabs from '@layouts/GroupTabs.svelte';
	import { currentFile } from '@/stores/currentFile';
	import { onMount, onDestroy, tick } from 'svelte';
	import type { FileNode } from '@/types';

	import { getMonaco } from '@utils/monaco';
	import type * as monaco from 'monaco-editor';
	import { handleSaveFile } from '@handlers/nodeActions';

	import { loadFile } from '@/features/handlers/nodeActions';

	let container: HTMLDivElement | null = null;
	let editor: monaco.editor.IStandaloneCodeEditor | null = null;
	let monacoInstance: typeof monaco | null = null;

	let state = $derived($currentFile);

	/* ------------------------------------------------------------
	 * Editor 최초 초기화 (이벤트 1회 등록)
	 * ------------------------------------------------------------ */
	function initEditor(model) {
		editor = monacoInstance!.editor.create(container!, {
			model,
			theme: 'vs-white',
			automaticLayout: true,
			minimap: { enabled: false }
		});

		// Ctrl+S 저장
		editor.addCommand(monacoInstance!.KeyMod.CtrlCmd | monacoInstance!.KeyCode.KeyS, () =>
			handleSaveFile(editor!)
		);
	}

	/* ------------------------------------------------------------
	 * active file 변경 감지 → loadFileToEditor 실행
	 * ------------------------------------------------------------ */
	$effect(async () => {
		const file = $currentFile.active?.file;
		if (!file || !monacoInstance || !container) return;

		await tick();

		const updated = await loadFile(monacoInstance, editor, file);

		// 처음 설정한 경우 updated=null → 모델은 있지만 editor가 없음
		if (updated === null) {
			const model = monacoInstance!.editor.getModels().find((m) => m.uri.path === file.path);
			model && initEditor(model);
		}
	});

	/* ------------------------------------------------------------
	 * mount
	 * ------------------------------------------------------------ */
	onMount(async () => {
		monacoInstance = await getMonaco();

		const active = state.active?.file;
		if (active) {
			await tick();
			const updated = await loadFile(monacoInstance, editor, active);

			if (!updated) {
				const model = monacoInstance!.editor.getModels().find((m) => m.uri.path === active.path);
				model && initEditor(model);
			}
		}
	});

	onDestroy(() => {
		editor?.dispose();
	});

	function handleSwitch(i: number) {
		currentFile.switchTab(i);
	}

	function handleClose(i: number) {
		currentFile.closeTab(i);
	}
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

<style>
	.editor-tabs-root {
		display: flex;
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
</style>
