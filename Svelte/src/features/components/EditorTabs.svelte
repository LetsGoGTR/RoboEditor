<script lang="ts">
  import GroupTabs from '@layouts/GroupTabs.svelte';
  import { currentFile } from '@/stores/currentFile';
  import { onDestroy, onMount, tick } from 'svelte';
  import type { FileNode } from '@/types';

  import { getMonaco } from '@utils/monaco';
  import type * as monaco from 'monaco-editor';
	import { detectLanguage } from '@utils/nodeAction';
	import { _getFile } from '@apis/file';
	import { handleSaveFile } from '@handlers/nodeActions';

  let container: HTMLDivElement | null = null;
  let editor: monaco.editor.IStandaloneCodeEditor | null = null;
  let monacoInstance: typeof monaco | null = null;

  let state = $derived($currentFile);

  /* ------------------------------------------------------------
   * 초기화
   * ------------------------------------------------------------ */
  onMount(async () => {
    monacoInstance = await getMonaco();

    const activeFile = state.active?.file;
    if (activeFile) await loadFile(activeFile);
  });

  /* ------------------------------------------------------------ */
  async function tryInitEditor() {
    await tick();
    const file = state.active?.file;
    if (!file || !container || !monacoInstance) return;
    await loadFile(file);
  }

  /* ------------------------------------------------------------
   * 파일 로드 (서버 기반)
   * ------------------------------------------------------------ */
  async function loadFile(file: FileNode) {
    if (!monacoInstance || !container) return;

    let text = '';

    if (file.path) {
      const res = await _getFile(file.path);
      text = res?.data?.content ?? '';
    }

    const model = monacoInstance.editor.createModel(
      text,
      detectLanguage(file.name)
    );

    if (editor) {
      const prev = editor.getModel();
      prev?.dispose();
      editor.setModel(model);
    } else {
      editor = monacoInstance.editor.create(container, {
        model,
        theme: 'vs-white',
        automaticLayout: true,
        minimap: { enabled: false }
      });

      // 저장 핸들러 연결
      editor.addCommand(
        monacoInstance.KeyMod.CtrlCmd | monacoInstance.KeyCode.KeyS,
        () => handleSaveFile(editor)
      );
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

    if (file && monacoInstance) {
      tryInitEditor();
    } else if (!file && editor) {
      editor.dispose();
      editor = null;
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
