<script lang="ts">
  import GroupTabs from '@layouts/GroupTabs.svelte';
  import { currentFile } from '@/stores/currentFile';
  import { onMount } from 'svelte';
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

  // --- 파일 로드 ---
  async function loadFile(file: FileNode) {
    if (!monacoInstance || !container) return;

    const text = file.handle
      ? (await readTextFile(file.handle)) ?? ''
      : (await file.file?.text()) ?? '';

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

      // ⌨️ Ctrl+S 단축키
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
    if (file && monacoInstance) loadFile(file);
  });
</script>

<!-- ✅ Wrapping 구조 -->
<div class="editor-tabs-root">
  <GroupTabs
    tabs={state.group
      .map((g) => g.file)
      .filter((file): file is FileNode => file !== null) // 🔹 null 제거 타입가드
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
/* 루트는 부모 컨테이너의 flex 전파를 보장 */
.editor-tabs-root {
  display: flex;
  flex-direction: column;
  flex: 1;
  height: 100%;
  width: 100%;
  overflow: hidden;
}

/* Monaco 영역은 남은 공간 전부 차지 */
.editor-container {
  flex: 1;
  width: 100%;
  height: 100%;
  background: #1e1e1e;
}
</style>
