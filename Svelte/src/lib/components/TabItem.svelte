<script lang="ts">
  import type { FixedTab } from '@/types';

  export let tab: FixedTab;
  export let isActive = false;
  export let onSelect: (id: FixedTab['id']) => void;
</script>

<button
  type="button"
  class="tab-item"
  role="tab"
  aria-selected={isActive}
  on:click={() => !tab.disabled && onSelect(tab.id)}
  disabled={tab.disabled}
>
  {tab.label}
</button>

<style>
  .tab-item {
    padding: 0.5rem 1rem;
    border: none;
    border-bottom: 2px solid transparent;
    background: transparent;
    cursor: pointer;
    font: inherit;
  }

  .tab-item[aria-selected="true"] {
    border-bottom-color: #2563eb;
    font-weight: 600;
  }

  .tab-item:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }
</style>
