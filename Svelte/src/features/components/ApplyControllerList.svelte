<script lang="ts">
  import ControllerItem from '@components/ControllerItem.svelte';
  import type { Controller } from '@/types';

  let { controllers, onSelectChange }: {
    controllers: Controller[];
    onSelectChange?: (ids: string[]) => void;
  } = $props();

  let selectedIds = $state(new Set<string>());

  function isSelectable(state: string): boolean {
    return state === 'idle' || state === 'error';
  }

  function toggleSelection(id: string) {
    if (selectedIds.has(id)) selectedIds.delete(id);
    else selectedIds.add(id);
    onSelectChange?.(Array.from(selectedIds));
  }

  function isSelected(id: string) {
    return selectedIds.has(id);
  }
</script>

<div class="controller-list">
  {#each controllers as c}
    <div class="controller-row">
      <input
        type="checkbox"
        checked={selectedIds.has(c.serialNumber)}
        disabled={!isSelectable(c.state)}
        onchange={() => toggleSelection(c.serialNumber)}
      />
      <ControllerItem
        controller={c}
        selected={isSelected(c.serialNumber)}
      />
    </div>
  {/each}
</div>

<style>
.controller-list {
  display: flex;
  flex-direction: column;
  gap: 0.4rem;
  width: 100%;
}

.controller-row {
  display: flex;
  align-items: flex-start;
  gap: 0.6rem;
}

.controller-row input[disabled] {
  opacity: 0.5;
  cursor: not-allowed;
}

/* 선택 가능한 항목만 hover 반응 */
.controller-row:not(:has(input[disabled])):hover {
  background: #f7f7f7;
  border-radius: 0.4rem;
  transition: background 0.2s ease;
}
</style>
