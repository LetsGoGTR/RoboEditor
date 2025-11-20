<script lang="ts">
  import type { Controller } from "@/types";

  let { controller, selected = false }: {
    controller: Controller;
    selected?: boolean;
  } = $props();
</script>

<div class="controller-item {selected ? 'selected' : ''}">
  <div class="header">
    <span class="serial">{controller.controllerMeta.name} ({controller.controllerMeta.serialNumber})</span>
    <span class="state {controller.controllerMeta.state.toLowerCase()}">{controller.controllerMeta.state}</span>
  </div>
  <div class="info">
    <div><strong>IP:</strong> {controller.controllerMeta.api}</div>
  </div>
</div>

<style>
.controller-item {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
  padding: 0.6rem 0.8rem;
  border: 1px solid #dcdcdc;
  border-radius: 0.3rem;
  background: #fff;
  transition: background 0.2s ease, border-color 0.2s ease;
  width: 100%;
  min-width: 320px;
}

/* 선택 시 단순한 테두리 강조 */
.controller-item.selected {
  border-color: #0078d7;
  background: #f0f7ff;
}

.controller-item:hover {
  background: #f9f9f9;
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.serial {
  font-weight: 600;
  color: #222;
  font-size: 0.9rem;
}

.state {
  font-size: 0.74rem;
  padding: 0.15rem 0.5rem;
  border-radius: 0.35rem;
  color: #fff;
  font-weight: 500;
  text-transform: capitalize;
}

.state.active { background: #607d8b; }
.state.idle { background: #43a047; }
.state.error { background: #e53935; }
.state.disconnected { background: #9e9e9e; }

.info {
  font-size: 0.77rem;
  color: #555;
  line-height: 1.3;
}
</style>
