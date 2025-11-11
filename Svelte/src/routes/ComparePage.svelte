<script lang="ts">
  import { onMount } from "svelte";
  import * as monaco from "monaco-editor";
  import { currentFile } from "@/stores/currentFile";
  import { readTextFile } from "@utils/FSA";
  import { detectLanguage } from "@utils/fileAction";
  import type { FileNode } from "@/types";

  // --- DOM refs ---
  let diffContainer: HTMLDivElement;
  let diffEditor: monaco.editor.IStandaloneDiffEditor | null = null;
  let leftModel: monaco.editor.ITextModel | null = null;
  let rightModel: monaco.editor.ITextModel | null = null;

  // --- Store 구독 ---
  let current = $currentFile;

  // --- 로컬 상태 ---
  let rightFile = $state<FileNode | null>(null);

  /** 안전하게 모델 해제 */
  function safeDisposeModels() {
    if (diffEditor) diffEditor.setModel(null); // 반드시 먼저 detach
    leftModel?.dispose();
    rightModel?.dispose();
    leftModel = null;
    rightModel = null;
  }

  /** 좌측(현재 파일) 로드 */
  async function loadLeftModel() {
    const file = current.active.file as FileNode | null;
    if (!file) return null;

    let text = "";
    if (file.handle) text = (await readTextFile(file.handle)) ?? "";
    else if (file.file) text = await file.file.text();

    // 기존 모델 제거
    if (leftModel) leftModel.dispose();

    leftModel = monaco.editor.createModel(
      text,
      detectLanguage(file.name),
      monaco.Uri.parse(`inmemory://left/${file.name}`)
    );

    return leftModel;
  }

  /** 우측 파일 선택 및 로드 */
  async function selectRightFile(autoOpen = false) {
    try {
      const [handle] = await (window as any).showOpenFilePicker({
        types: [{ description: "모든 파일", accept: { "*/*": [".*"] } }],
      });
      const file = await handle.getFile();
      const text = await file.text();

      rightFile = {
        id: crypto.randomUUID(),
        name: file.name,
        type: "file",
        handle,
        size: file.size,
        lastModified: file.lastModified,
      } as FileNode;

      if (rightModel) rightModel.dispose();
      rightModel = monaco.editor.createModel(
        text,
        detectLanguage(file.name),
        monaco.Uri.parse(`inmemory://right/${file.name}`)
      );

      updateDiffEditor();
    } catch (err) {
      if (!autoOpen) console.warn("❗ 파일 선택 취소 또는 오류:", err);
    }
  }

  /** DiffEditor 모델 갱신 */
  function updateDiffEditor() {
    if (!diffEditor) return;
    if (!leftModel || leftModel.isDisposed()) return;
    if (!rightModel || rightModel.isDisposed()) return;

    diffEditor.setModel({ original: leftModel, modified: rightModel });
  }

  /** --- 마운트 시 초기화 --- */
  onMount(() => {
    // DiffEditor 생성 (모델은 나중에 적용)
    diffEditor = monaco.editor.createDiffEditor(diffContainer, {
      theme: "vs-dark",
      automaticLayout: true,
      renderSideBySide: true,
      originalEditable: false,
      readOnly: false,
      minimap: { enabled: false },
      scrollBeyondLastLine: false,
    });

    // 비동기 초기화
    (async () => {
      const left = await loadLeftModel();

      if (left) {
        diffEditor.setModel({
          original: left,
          modified: monaco.editor.createModel(
            "// 비교할 파일을 선택하세요.",
            "plaintext",
            monaco.Uri.parse("inmemory://empty/right")
          ),
        });
      }

      // mount 후 자동으로 오른쪽 파일 선택 창 열기
      await selectRightFile(true);
    })();

    // 언마운트 시 안전하게 정리
    return () => {
      safeDisposeModels();
      diffEditor?.dispose();
      diffEditor = null;
    };
  });

  /** --- currentFile 변경 시 좌측 모델 재로딩 --- */
  $effect(() => {
    (async () => {
      if (!diffEditor) return;

      // 기존 모델 안전 해제
      diffEditor.setModel(null);
      leftModel?.dispose();

      const left = await loadLeftModel();
      if (!left) return;

      // 오른쪽 모델이 이미 있으면 즉시 갱신
      if (rightModel && !rightModel.isDisposed()) {
        diffEditor.setModel({ original: left, modified: rightModel });
      } else {
        diffEditor.setModel({
          original: left,
          modified: monaco.editor.createModel(
            "// 비교할 파일을 선택하세요.",
            "plaintext",
            monaco.Uri.parse("inmemory://empty/right")
          ),
        });
      }
    })();
  });
</script>

<!-- ✅ 레이아웃 -->
<div class="diff-container" bind:this={diffContainer}></div>

{#if rightFile}
  <div class="toolbar">
    <span>비교 중: {rightFile.name}</span>
  </div>
{/if}

<style>
.diff-container {
  width: 100%;
  height: 100%;
}

.toolbar {
  position: absolute;
  top: 1rem;
  right: 1rem;
  display: flex;
  align-items: center;
  gap: 1rem;
  background: rgba(30, 30, 30, 0.8);
  color: white;
  padding: 0.5rem 1rem;
  border-radius: 8px;
  z-index: 10;
}
</style>
