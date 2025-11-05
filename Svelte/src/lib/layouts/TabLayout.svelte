<script lang="ts">
  import TabItem from '@components/TabItem.svelte';
  import type { FixedTab, FixedTabId } from '@/types';

  export let leftTab: FixedTab = { id: 'left', label: '왼쪽 탭' };
  export let rightTab: FixedTab = { id: 'right', label: '오른쪽 탭' };

  export let activeId: FixedTabId = 'left';
  export let onTabChange: (id: FixedTabId) => void = () => {};

  const selectTab = (id: FixedTabId) => {
    activeId = id;
    onTabChange(id);
  };

  interface $$Slots {
    left: {};
    right: {};
  }
</script>

<nav class="tab-layout" aria-label="2-tab layout">
  <div class="tab-list" role="tablist">
    <TabItem tab={leftTab} isActive={activeId === 'left'} onSelect={selectTab} />
    <TabItem tab={rightTab} isActive={activeId === 'right'} onSelect={selectTab} />
  </div>

  <div class="tab-panels">
    <div
      role="tabpanel"
      aria-labelledby={leftTab.id}
      hidden={activeId !== 'left'}
    >
      <slot name="left" />
    </div>

    <div
      role="tabpanel"
      aria-labelledby={rightTab.id}
      hidden={activeId !== 'right'}
    >
      <slot name="right" />
    </div>
  </div>
</nav>

<style>
  .tab-layout {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .tab-list {
    display: flex;
    border-bottom: 1px solid #e5e7eb;
  }
</style>
