<script lang="ts">
  import { onMount, tick } from 'svelte';
  import { currentFile } from '@/stores/currentFile';
  import { readTextFile } from '@utils/FSA';
  import { detectLanguage, saveFileAndRefresh } from '@utils/fileAction';
  import type { FileNode } from '@/types';

  let container: HTMLDivElement | null = null;
  let editor: any = null;
  let monaco: any = null;

  let state = $derived($currentFile);

  /** 파일 읽기 및 에디터 초기화 */
  async function loadEditor() {
    const file = state.file as FileNode | null;
    if (!file || !container || !monaco) return;

    // 파일 읽기
    let text = '';
    if (file.handle) text = (await readTextFile(file.handle)) ?? '';
    else if (file.file) text = await file.file.text();

    const language = detectLanguage(file.name);

    // 🔹 새 model 생성
    const model = monaco.editor.createModel(text, language);

    if (editor) {
      // 기존 model dispose
      const oldModel = editor.getModel();
      if (oldModel) oldModel.dispose();

      // 새 model 교체
      editor.setModel(model);
    } else {
      // 새 editor 생성
      editor = monaco.editor.create(container, {
        model,
        theme: 'vs-white',
        automaticLayout: true,
        minimap: { enabled: false },
      });

      editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, save);
    }
  }


  /** 파일 저장 */
  async function save() {
    const file = state.file;
    if (!editor || !file) return;
    const content = editor.getValue();
    const updated = await saveFileAndRefresh(file, content);
    currentFile.open(updated);
  }

  /** onMount 후 모듈 로드 */
  onMount(async () => {
    await tick();
    console.log("🚀 onMount called, importing monaco...");
    monaco = await import('monaco-editor');
    console.log("✅ Monaco imported:", monaco);
    await loadEditor(); // import 완료 후에만 호출
  });

  /** 파일 변경 시 다시 로드 */
  $effect(() => {
    const file = $currentFile.file;
    if (monaco && file) {
      console.log('📂 File changed -> reload editor', file.name);
      loadEditor();
    }
  });
</script>

<div bind:this={container} class="editor-container"></div>
<button onclick={save} class="save-button">저장</button>

<style>
.editor-container {
  flex: 1;
  width: 100%;
  height: 100%;
  min-height: 0;
}

.save-button {
  position: absolute;
  bottom: 1rem;
  right: 2rem;
  background: #4e83db;
  color: white;
  border: none;
  border-radius: 6px;
  padding: 0.6rem 1.4rem;
  font-size: 0.95rem;
  font-weight: 500;
  cursor: pointer;
  transition: background 0.2s ease, transform 0.1s ease;
}
.save-button:hover { background: #3f6ac0; }
</style>
