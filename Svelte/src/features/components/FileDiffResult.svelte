<script lang="ts">
  import { fileDiffStore } from "@/stores/fileDiff";
  import type { DiffFilter, FileDiffItem } from "@/types";

  // diff store 전체를 반응형으로
  const diff = $derived($fileDiffStore);

  // literal tuple (타입 자동 생성)
  const FILTERS = ["all", "added", "removed", "changed"] as const;

  // 필터 상태
  let filter = $state<DiffFilter>("all");

  // filtered list
  const filteredDiffs = $derived<FileDiffItem[]>(() => {
    const diffs = diff.diffs ?? [];

    return filter === "all"
      ? diffs
      : diffs.filter((d) => d.state === filter);
  });


  const stateColor = (s: "added" | "removed" | "changed") =>
    ({
      added: "#28a745",
      removed: "#d73a49",
      changed: "#f0ad4e",
    }[s] ?? "#888");
</script>

<section class="diff-section">
  <div class="toolbar">
    <div class="filter-group">
      {#each FILTERS as f}
        <button
          class="btn filter {filter === f ? 'active' : ''}"
          onclick={() => (filter = f)}
        >
          {f.charAt(0).toUpperCase() + f.slice(1)}
        </button>
      {/each}
    </div>
  </div>

  <div class="diff-container">
    <div class="diff-header">
      <span class="col line">Line</span>
      <span class="col path">Path</span>
      <span class="col left">Left Value</span>
      <span class="col right">Right Value</span>
      <span class="col state">State</span>
    </div>

    <div class="diff-body">
      {#each filteredDiffs as d}
        <div class="diff-row">
          <span class="col line">{d.line}</span>
          <span class="col path" title={d.path}>{d.path}</span>
          <span class="col left" title={d.oldValue}>{d.oldValue}</span>
          <span class="col right" title={d.newValue}>{d.newValue}</span>
          <span class="col state" style={`color:${stateColor(d.state)};`}>
            {d.state}
          </span>
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
.col { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.state { font-weight: 600; }
</style>
