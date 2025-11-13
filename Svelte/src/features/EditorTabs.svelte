<script lang="ts">
  import GroupTabs from '@layouts/GroupTabs.svelte';
  import { currentFile } from '@/stores/currentFile';
  import { onDestroy, onMount, tick } from 'svelte';
  import type { FileNode } from '@/types';
  import { readTextFile } from '@utils/FSA';
  import { detectLanguage, saveFileAndRefresh } from '@utils/fileAction';
  import { getMonaco } from '@utils/monaco';
  import type * as monaco from 'monaco-editor';

  // --- 상태 ---
  let container: HTMLDivElement | null = null;
  let editor: monaco.editor.IStandaloneCodeEditor | null = null;
  let monacoInstance: typeof monaco | null = null;
  let state = $derived($currentFile);

  // --- 초기화 ---
  onMount(async () => {
    monacoInstance = await getMonaco();
    const activeFile = state.active.file;
    if (activeFile) await loadFile(activeFile);
  });

  // --- 안전한 초기화 보장 ---
  async function tryInitEditor() {
    await tick(); // DOM 렌더 보장
    const file = state.active.file;
    if (!file || !container || !monacoInstance) return;
    await loadFile(file);
  }

  // --- 파일 로드 ---
  async function loadFile(file: FileNode) {
    if (!monacoInstance || !container) return;

    const text =
      file.handle ? (await readTextFile(file.handle)) ?? '' : (await file.file?.text()) ?? '';

    const language = detectLanguage(file.name);
    const model = monacoInstance.editor.createModel(text, language);

    if (editor) {
      const prev = editor.getModel();
      if (prev) prev.dispose();
      editor.setModel(model);
    } else {
      editor = monacoInstance.editor.create(container, {
        model,
        theme: 'vs-white',
        automaticLayout: true,
        minimap: { enabled: false }
      });

      editor.addCommand(monacoInstance.KeyMod.CtrlCmd | monacoInstance.KeyCode.KeyS, save);
    }
  }

  // --- 파일 저장 ---
  async function save() {
    const file = state.active.file;
    if (!editor || !file) return;

    const content = editor.getValue();
    const updated = await saveFileAndRefresh(file, content);

    currentFile.setContent(content);
    currentFile.open(updated);
  }

  // --- 탭 전환 ---
  function handleSwitch(index: number) {
    currentFile.switchTab(index);
  }

  // --- 탭 닫기 ---
  function handleClose(index: number) {
    currentFile.closeTab(index);
  }

  // --- 파일 변경 감시 ---
  $effect(() => {
    const file = $currentFile.active.file;
    if (file && monacoInstance) tryInitEditor();
    else if (!file && editor) {
      editor.dispose();
      editor = null;
    }
  });

  // --- 컴포넌트 종료 ---
  onDestroy(() => {
    if (editor) {
      editor.dispose();
      editor = null;
    }
  });
</script>

<!-- Wrapping 구조 -->
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
