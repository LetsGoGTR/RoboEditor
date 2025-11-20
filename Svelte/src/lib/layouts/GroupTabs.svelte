<script lang="ts">
  interface TabItem {
    id: string;
    name: string;
    path?: string | null;
  }

  export let tabs: TabItem[] = [];
  export let activeIndex: number | null = null;

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

  /** 키보드 접근성: Enter/Space 지원 */
  function handleKeydown(e: KeyboardEvent, index: number) {
    if (e.key === "Enter" || e.key === " ") {
      e.preventDefault();
      handleSwitch(index);
    }
  }
</script>

<!-- 전체 컨테이너 -->
<div class="group-tabs-root">
  <!-- 탭 헤더 -->
  <nav class="tab-bar" aria-label="Opened files">
    {#each tabs as view, i (view.id)}
      <div
        class="tab-btn {i === activeIndex ? 'active' : ''}"
        role="tab"
        tabindex={i === activeIndex ? 0 : -1}
        aria-selected={i === activeIndex}
        title={view.path ?? ''}
        aria-label={`Open file: ${view.name ?? 'Untitled'}`}
        on:click={() => handleSwitch(i)}
        on:keydown={(e) => handleKeydown(e, i)}
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

  <!-- 탭 콘텐츠 -->
  <div class="tab-content">
    {#if activeIndex !== null && tabs[activeIndex]}
      <div class="tab-pane" role="tabpanel" tabindex="0">
        <slot name="content" {activeIndex} tab={tabs[activeIndex]} />
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
  background-color: #fff;
}

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
  white-space: nowrap;
}

.tab-btn {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  font-size: 0.9rem;
  background: #fafbfc;
  border: 1px solid #d1d5db;
  border-bottom: none;
  border-radius: 6px 6px 0 0;
  padding: 0.45rem 0.9rem;
  cursor: pointer;
  user-select: none;
  will-change: background-color, color, border-color;
}

.tab-btn + .tab-btn {
  margin-left: -1px;
}

.tab-btn:hover {
  background: #f1f3f5;
  color: #222;
}

/* 활성 탭 */
.tab-btn.active {
  background: #fff;
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
  border: none;
  background: transparent;
  color: #666;
  font-size: 0.85rem;
  font-weight: 700;
  cursor: pointer;
  margin-left: 0.4rem;
  transition: color 0.15s ease;
}

.close-btn:hover {
  color: #e11d48;
}

.close-btn:active {
  color: #be123c;
}

/* ───────────── 콘텐츠 영역 ───────────── */
.tab-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  background-color: #fff;
  width: 100%;
  height: 100%;
  position: relative;
}

.tab-pane {
  flex: 1;
  display: flex;
  width: 100%;
  height: 100%;
  overflow: hidden;
}

/* 비어 있는 상태 */
.empty-state {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  text-align: center;
  color: #999;
  font-size: 1rem;
  line-height: 1.4;
  white-space: nowrap;
  pointer-events: none;
}
</style>
