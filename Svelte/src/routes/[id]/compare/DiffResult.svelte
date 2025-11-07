<script lang="ts">
  import type { DiffItem, DiffFilter, DiffState } from "@/types";

  // props
  let { diffs }: { diffs: DiffItem[] } = $props();

  // states
  let filter: DiffFilter = $state("ALL");

  // derived
  const filteredDiffs = $derived(
    filter === "ALL" ? diffs : diffs.filter(d => d.state === filter)
  );

  // helpers
  const stateColor = (s: DiffState) => {
    switch (s) {
      case "ADDED": return "#28a745";
      case "REMOVED": return "#d73a49";
      case "CHANGED": return "#f0ad4e";
      default: return "#888";
    }
  };

  // Actions
  function handleCompareFiles() {
    console.log("Compare Files clicked");
  }
  function handleCompareFolders() {
    console.log("Compare Folders clicked");
  }
</script>

<section class="diff-section">
  <!-- 상단 툴바 -->
  <div class="toolbar" role="toolbar" aria-label="Diff operations">
    <div class="left-buttons">
      <button class="btn primary" onclick={handleCompareFiles}>Compare Files</button>
      <button class="btn primary" onclick={handleCompareFolders}>Compare Folders</button>
    </div>

    <div class="filter-group">
      {#each ["ALL", "ADDED", "REMOVED", "CHANGED"] as f}
        <button
          class="btn filter {filter === f ? 'active' : ''}"
          onclick={() => (filter = f as DiffFilter)}
        >
          {f === "ALL" ? "All" : f.charAt(0) + f.slice(1).toLowerCase()}
        </button>
      {/each}
    </div>
  </div>

  <!-- Diff 리스트 -->
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
          <span class="col left" title={d.leftValue}>{d.leftValue}</span>
          <span class="col right" title={d.rightValue}>{d.rightValue}</span>
          <span class="col state" style={`color:${stateColor(d.state)};`}>{d.state}</span>
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
  min-width: 500px;
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

.left-buttons,
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
.btn.primary { background: #5b73e8; color: #fff; border-color: #5b73e8; }
.btn.primary:hover { background: #4a61d1; }

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
