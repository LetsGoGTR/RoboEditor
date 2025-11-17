<script lang="ts">
  import { onMount } from "svelte";
  import { browser } from "$app/environment";
  import * as monaco from "monaco-editor/esm/vs/editor/editor.api";

  import HorizontalSplit from "@layouts/HorizontalSplit.svelte";
  import FileDiffResult from "@features/FileDiffResult.svelte";

  import { fileDiffStore } from "@/stores/fileDiff";
  import { toMonacoUri, getOrCreateModel } from "@/utils/monaco";

  /** 🔥 store → Svelte 최신 문법으로 반응형 연결 */
  let diff = $derived($fileDiffStore);

  let diffContainer: HTMLDivElement | null = null;
  let diffEditor: monaco.editor.IStandaloneDiffEditor | null = null;

  /** DiffEditor 모델 갱신 */
  function applyDiff() {
    if (!diffEditor) return;

    // 모든 데이터가 준비된 뒤에만 실행
    if (!diff.leftFilePath || !diff.rightFilePath) return;
    if (!diff.leftContent || !diff.rightContent) return;

    const original = getOrCreateModel(
      toMonacoUri(diff.leftFilePath),
      diff.leftContent
    );
    const modified = getOrCreateModel(
      toMonacoUri(diff.rightFilePath),
      diff.rightContent
    );

    diffEditor.setModel({ original, modified });

    // Layout 보정
    queueMicrotask(() => diffEditor?.layout());
  }

  /** mount 시 에디터 생성 */
  onMount(() => {
    if (!browser) return;

    const wait = setInterval(() => {
      if (diffContainer) {
        clearInterval(wait);

        diffEditor = monaco.editor.createDiffEditor(diffContainer, {
          theme: "vs-white",
          renderSideBySide: true,
          readOnly: true,
          automaticLayout: true,
          minimap: { enabled: false },
          scrollBeyondLastLine: false
        });

        applyDiff();
      }
    }, 30);
  });

  /** 🔥 store 변화 시 applyDiff() 다시 실행 */
  $effect(() => {
    if (diffEditor) applyDiff();
  });
</script>

<HorizontalSplit initialLeftRatio={75}>
  <div
    slot="left"
    class="compare-body"
    bind:this={diffContainer}
  ></div>

  <FileDiffResult
    slot="right"
  />
</HorizontalSplit>

<style>
.compare-body {
  height: 100%;
  width: 100%;
  display: block;
  overflow: hidden;
}
</style>
