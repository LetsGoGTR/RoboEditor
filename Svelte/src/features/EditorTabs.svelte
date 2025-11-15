<script lang="ts">
  import GroupTabs from '@layouts/GroupTabs.svelte';
  import { currentFile } from '@/stores/currentFile';
  import { onDestroy, onMount, tick } from 'svelte';
  import type { FileNode } from '@/types';

  import { getMonaco } from '@utils/monaco';
  import type * as monaco from 'monaco-editor';

  // 🔥 서버 기반 파일 API
  import { _createFile, _getFile, _updateFile } from '@apis/file';
  import { detectLanguage } from '@utils/nodeAction';

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

  /* ------------------------------------------------------------
   * 안전한 초기화 (Tab 전환 시 호출)
   * ------------------------------------------------------------ */
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

    // 신규 파일: 서버 로딩 금지
    if (!file.path) {
      text = '';
    } else {
      // 기존 파일: 서버에서 내용 가져오기
      const res = await _getFile(file.path);
      text = res?.data?.content ?? '';
    }

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

      editor.addCommand(
        monacoInstance.KeyMod.CtrlCmd | monacoInstance.KeyCode.KeyS,
        save
      );
    }
  }

  /* ------------------------------------------------------------
   * 파일 저장 (서버)
   * ------------------------------------------------------------ */
  async function save() {
    const active = $currentFile.active;
    if (!editor || !active) return;

    const file = active.file;
    const content = editor.getValue();

    let path = file.path;

    try {
      /* ----------------------------------------------------------
      * 1) 신규 파일 여부 확인
      * -------------------------------------------------------- */
      if (!path) {
        console.warn('[save] path가 없어 신규 파일로 생성합니다:', file.name);

        const resp = await _createFile(file.name, content);

        // 실제 생성된 path 반환
        const createdPath = resp?.path ?? file.name;

        // Store에 path 반영
        currentFile.update((s) => {
          if (!s.active) return s;

          // active.file.path 반영
          s.active.file.path = createdPath;

          // group 내의 동일 파일 갱신
          const idx = s.group.findIndex((g) => g.file.id === s.active!.file.id);
          if (idx !== -1) s.group[idx].file.path = createdPath;

          return s;
        });

        currentFile.setContent(content);
        return;
      }

      /* ----------------------------------------------------------
      * 2) 기존 경로가 실제 존재하는지 확인
      * (서버 파일 삭제/이동 등으로 인해 path만 남은 경우 대응)
      * -------------------------------------------------------- */
      let exists = true;
      const check = await _getFile(path).catch(() => (exists = false));

      if (!exists || !check?.data) {
        console.warn('[save] 파일이 존재하지 않아 신규 생성으로 대체:', path);

        const resp = await _createFile(path, content);
        const createdPath = resp?.path ?? path;

        // Store 업데이트
        currentFile.update((s) => {
          if (!s.active) return s;
          s.active.file.path = createdPath;

          const idx = s.group.findIndex((g) => g.file.id === s.active!.file.id);
          if (idx !== -1) s.group[idx].file.path = createdPath;
          return s;
        });

        currentFile.setContent(content);
        return;
      }

      /* ----------------------------------------------------------
      * 3) 정상적으로 파일이 존재 → update
      * -------------------------------------------------------- */
      await _updateFile(path, { content });

      // store 동기화
      currentFile.setContent(content);

    } catch (err) {
      console.error('[save] 파일 저장 중 오류 발생:', err);
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

  // 파일 변경 감지
  $effect(() => {
    const file = $currentFile.active?.file;

    if (file && monacoInstance) {
      tryInitEditor();
    } else if (!file && editor) {
      editor.dispose();
      editor = null;
    }
  });

  // 컴포넌트 제거
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
