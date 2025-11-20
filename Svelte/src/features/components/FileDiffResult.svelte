<script lang="ts">
  import { fileDiffStore } from '@/stores/fileDiff';
  import type { FileDiffItem, DiffFilter } from '@/types';

  // 🔥 store 값을 runes 방식으로 구독
  const state = $derived($fileDiffStore);

  // 필터 버튼 목록 (store.filter 도 이 값들 중 하나라고 가정)
  const FILTERS: DiffFilter[] = ['all', 'added', 'removed', 'modified'];

  // 필터 변경
  function changeFilter(next: DiffFilter) {
    fileDiffStore.setFilter(next);
  }

  // 서버에서 오는 type 이 대소문자 섞여 있어도 안전하게 처리
  function normalizeType(t: FileDiffItem['type']): DiffFilter {
    return (String(t).toLowerCase() as DiffFilter);
  }

  // ✅ 실제로 화면에 쓸 리스트: 함수로 단순하게 만든다
  function getFilteredDiffs(): FileDiffItem[] {
    const diffs = state.diffs ?? [];
    const current = state.filter ?? 'all';

    if (current === 'all') return diffs;

    return diffs.filter((d) => normalizeType(d.type) === current);
  }

  // 디버깅용 로그 — 지금 상태를 정확히 확인하기 위해 유지하는 것을 추천
  $effect(() => {
    console.log('[FileDiffResult] raw diffs =', state.diffs);
    console.log('[FileDiffResult] filter =', state.filter);
    console.log('[FileDiffResult] filtered length =', getFilteredDiffs().length);
  });

  const stateColor = (s: 'added' | 'removed' | 'modified') =>
    ({
      added: '#28a745',
      removed: '#d73a49',
      modified: '#f0ad4e'
    }[s] ?? '#888');
</script>

<section class="diff-section">
  <div class="toolbar">
    <div class="filter-group">
      {#each FILTERS as f}
        <button
          class="btn filter {state.filter === f ? 'active' : ''}"
          onclick={() => changeFilter(f)}
        >
          {f.charAt(0).toUpperCase() + f.slice(1)}
        </button>
      {/each}
    </div>
  </div>

  <div class="diff-container">
    <div class="diff-header">
      <span class="col line">Line</span>
      <span class="col left">Old Value</span>
      <span class="col right">New Value</span>
      <span class="col state">State</span>
    </div>

    <div class="diff-body">
      {#each getFilteredDiffs() as d}
        <div class="diff-row">
          <span class="col line">
            {d.newLineNumber ?? d.oldLineNumber ?? 0}
          </span>
          <span class="col left">{d.oldValue}</span>
          <span class="col right">{d.newValue}</span>
          <span class="col state" style={`color:${stateColor(normalizeType(d.type) as any)};`}>
            {normalizeType(d.type)}
          </span>
        </div>
      {:else}
        <!-- diff 없을 때 확인용 -->
        <div class="diff-row">
          <span class="col line">-</span>
          <span class="col left" style="grid-column: span 3;">
            No changes to display
          </span>
          <span class="col state"></span>
        </div>
      {/each}
    </div>
  </div>
</section>


<style>
.diff-section {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-width: 400px;
  background: #fafafa;
  border-left: 1px solid #ddd;
  font-family: "Consolas", monospace;
  font-size: 0.85rem;
  overflow: hidden;
}

/* Toolbar */
.toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: #f7f7f7;
  border-bottom: 1px solid #ccc;
  padding: 0.4rem 0.75rem;
  gap: 0.5rem;
  flex-wrap: wrap;
}

.filter-group {
  display: flex;
  align-items: center;
  gap: 0.4rem;
}

.btn {
  padding: 0.25rem 0.6rem;
  border: 1px solid #bbb;
  border-radius: 0.3rem;
  background: #fff;
  cursor: pointer;
  transition: background 0.15s ease, border-color 0.15s ease;
}

.btn:hover { background: #f0f0f0; border-color: #999; }
.btn.active { background: #e2e6ff; border-color: #5b73e8; font-weight: 600; }

/* Diff Rows */
.diff-container {
  flex: 1 1 auto;
  overflow: auto;
}

.diff-header {
  display: grid;
  grid-template-columns: 0.5fr 2fr 2.5fr 2.5fr 1fr;
  font-weight: 600;
  background: #f0f0f0;
  border-bottom: 1px solid #ccc;
  padding: 0.5rem 0.75rem;
}

.diff-row {
  display: grid;
  grid-template-columns: 0.5fr 2fr 2.5fr 2.5fr 1fr;
  padding: 0.4rem 0.75rem;
  border-bottom: 1px solid #eee;
  transition: background 0.2s ease;
}

.diff-row:hover { background: #f9f9f9; }
.col { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; min-width: 60px; }
.state { font-weight: 600; }
</style>
