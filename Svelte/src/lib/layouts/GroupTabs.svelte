<script lang="ts">
  // Tab 데이터 타입 정의 (FileNode 구조와 호환되게)
  interface TabItem {
    id: string;
    name: string;
    path?: string | null;
  }

  // 🔹 부모 컴포넌트로부터 주입받는 props
  export let tabs: TabItem[] = [];
  export let activeIndex: number | null = null;

  // 🔹 이벤트: 탭 전환 / 닫기
  export let onSwitch: (index: number) => void = () => {};
  export let onClose: (index: number) => void = () => {};

  /** 탭 전환 */
  function handleSwitch(index: number) {
    onSwitch(index);
  }

  /** 탭 닫기 */
  function handleClose(event: MouseEvent, index: number) {
    event.stopPropagation();
    onClose(index);
  }
</script>

<!-- ✅ 탭 전체 컨테이너 -->
<div class="group-tabs-root">
  <!-- 🔹 탭 헤더 -->
  <nav class="tab-bar" aria-label="Opened files">
    {#each tabs as view, i (view.id)}
      <div
        class="tab-btn"
        class:active={i === activeIndex}
        role="button"
        tabindex="0"
        title={view.path ?? ''}
        aria-label={`Open file: ${view.name ?? 'Untitled'}`}
        on:click={() => handleSwitch(i)}
        on:keydown={(e) => e.key === 'Enter' && handleSwitch(i)}
      >
        <span class="tab-title">{view.name ?? 'Untitled'}</span>
        <button
          type="button"
          class="close-btn"
          aria-label="Close tab"
          on:click={(e) => handleClose(e, i)}
        >
          ✕
        </button>
      </div>
    {/each}
  </nav>

  <!-- 🔹 탭 콘텐츠 (slot을 내부에 wrap) -->
  <div class="tab-content">
    {#if activeIndex !== null && tabs[activeIndex]}
      <div class="tab-pane">
        <!-- ✅ Monaco Editor나 다른 콘텐츠가 이 영역에 삽입됨 -->
        <slot
          name="content"
          {activeIndex}
          tab={tabs[activeIndex]}
        />
      </div>
    {:else}
      <div class="empty-state">열린 파일이 없습니다.</div>
    {/if}
  </div>
</div>

<style>
.group-tabs-root {
  display: flex;
  flex-direction: column;
  height: 100%;
  width: 100%;
  overflow: hidden;
  background-color: #ffffff;
}

/* ──────────────── 탭 헤더 ──────────────── */
.tab-bar {
  display: flex;
  align-items: center;
  gap: 0.25rem;
  background-color: #f5f6f7;
  border-bottom: 1px solid #d1d5db;
  padding: 0 0.5rem;
  height: 2.3rem;
  flex-shrink: 0;
  overflow-x: auto;
}

/* 개별 탭 버튼 */
.tab-btn {
  position: relative;
  display: flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.9rem;
  background: #fafbfc;
  color: #555;
  border: 1px solid #d1d5db;
  border-bottom: none;
  border-radius: 6px 6px 0 0;
  padding: 0.45rem 0.9rem;
  cursor: pointer;
  white-space: nowrap;
  transition: background 0.15s ease, color 0.15s ease, border-color 0.15s ease;
}

/* 탭 간 구분선 명확하게 (아래 제외 3면) */
.tab-btn + .tab-btn {
  margin-left: -1px;
}

.tab-btn:hover {
  background: #f1f3f5;
  color: #222;
}

/* 활성 탭 */
.tab-btn.active {
  background: #ffffff;
  color: #000;
  border-color: #c5c9cf;
  border-bottom: 2px solid #2563eb;
  font-weight: 600;
  z-index: 1;
}

/* 탭 제목 */
.tab-title {
  max-width: 140px;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* 닫기 버튼 */
.close-btn {
  border: none;                /* 🔹 테두리 제거 */
  background: transparent;     /* 🔹 배경 제거 */
  color: #666;                 /* 기본 회색 */
  font-size: 0.85rem;
  font-weight: 700;            /* 🔹 항상 굵게 표시 */
  cursor: pointer;
  user-select: none;
  margin-left: 0.4rem;
  transition: color 0.15s ease;
}

.close-btn:hover {
  color: #e11d48;              /* hover 시 강조 (붉은색 계열) */
}

/* 클릭 시 색상 살짝 어둡게 */
.close-btn:active {
  color: #be123c;
}

/* ──────────────── 탭 본문 ──────────────── */
.tab-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  background-color: #ffffff;
  width: 100%;
  height: 100%;
  position: relative; /* 중심 정렬을 위해 relative 필요 */
}

.tab-pane {
  flex: 1;
  display: flex;
  width: 100%;
  height: 100%;
  overflow: hidden;
}

.empty-state {
  position: absolute;
  top: 40%;
  left: 45%;
  transform: translate(-50%, -50%); /* 정확한 중앙 */
  text-align: center;
  color: #999;
  font-size: 1rem;
  line-height: 1.4;
  white-space: nowrap;
  pointer-events: none;
}
</style>

