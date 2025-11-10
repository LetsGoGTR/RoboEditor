<script lang="ts">
  import { onMount } from 'svelte';
  import * as monaco from 'monaco-editor';
  import { currentFile } from '@/stores/currentFile';
  import { readTextFile, writeTextFile } from '@utils/FSA';
  import type { FileNode } from '@/types';
	import { detectLanguage, saveFileAndRefresh } from '@utils/fileAction';

  let container: HTMLDivElement;
  let editor: monaco.editor.IStandaloneCodeEditor | null = null;

  // 현재 store 상태 구독
  let state = $derived($currentFile);

  /** 파일 읽기 및 에디터 초기화 */
  async function loadEditor() {
    const file = state.file as FileNode | null;
    if (!file) {
      console.warn("⚠️ 선택된 파일이 없습니다.");
      return;
    }

    let text: string | null = null;

    if (file.handle) {
      text = await readTextFile(file.handle);
    } else if (file.file) {
      text = await file.file.text();
    } else {
      // 새로 만든 파일의 경우 handle/file 없음 → 빈 내용으로 초기화
      text = "";
    }

    // --- 에디터 생성 또는 갱신
    if (!container) return;

    if (editor) {
      editor.setValue(text ?? "");
      monaco.editor.setModelLanguage(editor.getModel()!, detectLanguage(file.name));
    } else {
      editor = monaco.editor.create(container, {
        value: text ?? "",
        language: detectLanguage(file.name),
        theme: "vs-white",
        automaticLayout: true,
        minimap: { enabled: false },
      });

      editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, async () => {
        await save();
      });
    }
  }

  /** 파일 저장 */
  async function save() {
    const file = state.file;
    if (!editor || !file) return;

    const content = editor.getValue();
    const updated = await saveFileAndRefresh(file, content);

    // 변경된 handle, path를 store에 반영
    currentFile.open(updated);
  }

  /** 반응형: store 상태 변경 시 파일 다시 로드 */
  $effect(() => {
    loadEditor();
  });

  /** 마운트 후 정리 */
  onMount(() => {
    return () => editor?.dispose();
  });
</script>

<!-- ✅ 에디터 레이아웃 -->
<div bind:this={container} style="width:100%; height:100%;"></div>
<button onclick={save} class="save-button">
  저장
</button>

<style>
.save-button {
  position: absolute;
  bottom: 1rem;
  right: 2rem;

  background: #4e83db;              /* 상단 메뉴와 동일한 메인 블루 */
  color: white;
  border: none;
  border-radius: 6px;
  padding: 0.6rem 1.4rem;
  font-size: 0.95rem;
  font-weight: 500;
  cursor: pointer;
  transition: background 0.2s ease, transform 0.1s ease;
}

.save-button:hover {
  background: #3f6ac0;              /* hover 시 조금 더 짙은 블루 */
}

.save-button:active {
  background: #365ca7;              /* 클릭 시 더 어두운 블루 */
}
</style>
