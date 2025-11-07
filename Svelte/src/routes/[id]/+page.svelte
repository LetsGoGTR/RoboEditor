<script lang="ts">
  import { onMount } from 'svelte';
  import * as monaco from 'monaco-editor';
  import { readTextFile, writeTextFile } from '@/utils/FSA';

  export let fileHandle: FileSystemFileHandle;

  let container: HTMLDivElement;
  let editor: monaco.editor.IStandaloneCodeEditor;

  // Monaco Editor 초기화
  async function initEditor() {
    const text = await readTextFile(fileHandle);

    editor = monaco.editor.create(container, {
      value: text ?? '',
      language: 'yaml',          // 확장자에 따라 동적 설정 가능
      theme: 'vs-dark',
      automaticLayout: true
    });
  }

  // 파일 저장
  async function save() {
    if (!editor) return;
    const content = editor.getValue();
    await writeTextFile(fileHandle, content);
  }

  // Svelte 생명주기 진입
  onMount(() => {
    // 내부에서 async IIFE로 처리하여 타입 충돌 방지
    (async () => {
      await initEditor();
    })();

    // cleanup
    return () => editor?.dispose();
  });
</script>

<!-- 레이아웃 -->
<div bind:this={container} style="width:100%; height:100%;"></div>
