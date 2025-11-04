<script lang="ts">
  let container: HTMLDivElement | null = null;

  export let initialLeftRatio: number = 25;
  export let minLeftRatio: number = 10;
  export let maxLeftRatio: number = 50;

  let leftRatio = initialLeftRatio;
  let isResizing = false;
  let startX = 0;
  let startLeftRatio = leftRatio;

  const clamp = (v: number, min: number, max: number) =>
    Math.min(max, Math.max(min, v));

  const handlePointerDown = (e: PointerEvent) => {
    if (!container) return;
    isResizing = true;
    startX = e.clientX;
    startLeftRatio = leftRatio;
    e.preventDefault();

    window.addEventListener('pointermove', handlePointerMove);
    window.addEventListener('pointerup', handlePointerUp);
  };

  const handlePointerMove = (e: PointerEvent) => {
    if (!isResizing || !container) return;
    const deltaX = e.clientX - startX;
    const { width } = container.getBoundingClientRect();
    const deltaRatio = (deltaX / width) * 100;
    leftRatio = clamp(startLeftRatio + deltaRatio, minLeftRatio, maxLeftRatio);
  };

  const handlePointerUp = () => {
    if (!isResizing) return;
    isResizing = false;
    window.removeEventListener('pointermove', handlePointerMove);
    window.removeEventListener('pointerup', handlePointerUp);
  };
</script>

<div bind:this={container} class="split-container">
  <!-- 좌측 영역 -->
  <div class="pane" style={`flex-basis: ${leftRatio}%; flex-shrink: 0;`}>
    <slot name="left" />
  </div>

  <!-- 구분선 -->
  <div class="divider" on:pointerdown={handlePointerDown}></div>

  <!-- 우측 영역 -->
  <div class="pane" style="flex: 1 1 auto;">
    <slot name="right" />
  </div>
</div>

<style>
  :global(html, body) {
    margin: 0;
    height: 100%;
    overflow: hidden; /* 스크롤 완전 제거 */
  }

  .split-container {
    display: flex;
    width: 100vw;
    height: 100vh; /* 뷰포트 높이에 완전히 맞춤 */
    overflow: hidden;
  }

  .pane {
    height: 100%;
    overflow: auto;
  }

  .divider {
    width: 6px;
    cursor: col-resize;
    user-select: none;
    touch-action: none;
    background-color: #e0e0e0;
    transition: background-color 0.2s ease;
  }

  .divider:hover {
    background-color: #ccc;
  }
</style>