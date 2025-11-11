<script lang="ts">
  import { onMount } from 'svelte';
  import { browser } from '$app/environment';
  import * as monaco from 'monaco-editor/esm/vs/editor/editor.api'; // ✅ 직접 API 로드
  import { currentFile } from '@/stores/currentFile';
  import { readTextFile } from '@/utils/FSA';
  import { detectLanguage } from '@/utils/fileAction';
  import type { FileNode } from '@/types';
  import HorizontalSplit from '@layouts/HorizontalSplit.svelte';

  let diffContainer: HTMLDivElement | null = null;
  let diffEditor: monaco.editor.IStandaloneDiffEditor | null = null;

  let state = $derived($currentFile);

  /** 파일 내용을 읽고 모델 생성 */
  async function createModel(file: FileNode | null) {
    if (!file) return monaco.editor.createModel('', 'plaintext');
    let text = '';
    if (file.handle) text = (await readTextFile(file.handle)) ?? '';
    else if (file.file) text = await file.file.text();
    const lang = detectLanguage(file.name);
    return monaco.editor.createModel(text, lang);
  }

  onMount(() => {
    if (!browser) return;

    // 🔹 DOM이 완전히 렌더된 다음 프레임까지 대기
    requestAnimationFrame(async () => {
      if (!diffContainer) {
        console.error('❌ diffContainer is null — cannot mount editor');
        return;
      }

      try {
        const original = await createModel(state.active?.file ?? null);
        const modified = await createModel(state.right?.file ?? null);

        diffEditor = monaco.editor.createDiffEditor(diffContainer, {
          theme: 'vs-white',
          readOnly: true,
          renderSideBySide: true,
          automaticLayout: true,
          originalEditable: false,
          scrollBeyondLastLine: false,
          minimap: { enabled: false }
        });

        // ✅ 반드시 original=왼쪽, modified=오른쪽 순서
        diffEditor.setModel({ original, modified });

        // 레이아웃 재계산 (한 프레임 뒤)
        setTimeout(() => diffEditor?.layout(), 50);
      } catch (err) {
        console.error('❌ Diff Editor 초기화 실패:', err);
      }
    });

    return () => {
      diffEditor?.dispose();
    };
  });
</script>

<HorizontalSplit initialLeftRatio={75}>
  <div slot="left" class="compare-body" bind:this={diffContainer}></div>
</HorizontalSplit>

<style>
.compare-body {
  height: 100%;
  width: 100%;
  display: block;
  overflow: hidden;
}

:global(.monaco-editor) {
  border-radius: 4px;
}
</style>
