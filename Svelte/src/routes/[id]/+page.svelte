<script lang="ts">
  import { onMount } from 'svelte';
  import * as monaco from 'monaco-editor';
  import { currentFile } from '@/stores/currentFile';
  import { readTextFile, writeTextFile } from '@/utils/FSA';
  import type { FileNode } from '@/types';
	import { detectLanguage } from '@/utils/fileConfig';

  let container: HTMLDivElement;
  let editor: monaco.editor.IStandaloneCodeEditor | null = null;

  // 현재 store 상태 구독
  let state = $derived($currentFile);

  /** 파일 읽기 및 에디터 초기화 */
  async function loadEditor() {
    const file = state.file as FileNode | null;
    if (!file) {
      console.warn('⚠️ 선택된 파일이 없습니다.');
      return;
    }

    // 파일 내용 읽기
    let text: string | null = null;
    if (file.handle) {
      text = await readTextFile(file.handle);
    } else if (file.file) {
      text = await file.file.text();
    } else {
      console.warn('⚠️ 파일 내용을 읽을 수 없습니다.');
      return;
    }

    // 에디터 생성 또는 갱신
    if (!container) return;

    if (editor) {
      editor.setValue(text ?? '');
      monaco.editor.setModelLanguage(editor.getModel()!, detectLanguage(file.name));
    } else {
      editor = monaco.editor.create(container, {
        value: text ?? '',
        language: detectLanguage(file.name),
        theme: 'vs-white',
        automaticLayout: true,
        minimap: { enabled: false },
      });
    }
  }

  /** 파일 저장 */
  async function save() {
    const file = state.file;
    if (!editor || !file || !file.handle) return;
    const content = editor.getValue();
    await writeTextFile(file.handle, content);
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
<button onclick={save} style="position:absolute; bottom:1rem; right:1rem;">
  💾 저장
</button>
