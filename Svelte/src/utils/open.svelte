<script lang="ts">
  import { _getDevice } from '@/apis/controller';
  import { openDirectoryDialog, finishOpenDirectory } from '@/utils/open';
  

  // store 값 구독
  $: dialog = $openDirectoryDialog; // { workspaces, resolve } | null

  let selectedId: string | null = null;
  let selectedData: any | null = null; // { metadata, tree }
  let loadingWorkspace = false;

  async function handleSelect(ws: any) {
    selectedId = ws.id;
    selectedData = null;
    loadingWorkspace = true;

    try {
      const res: any = await _getDevice(ws.id);

      if (!res.success) {
        throw new Error('워크스페이스 조회 실패');
      }

      // C++ 쪽에서 result.data["metadata"], result.data["tree"]
      selectedData = res.data; // 👉 { metadata, tree }
    } catch (err) {
      console.error(err);
      selectedData = null;
    } finally {
      loadingWorkspace = false;
    }
  }

  function handleConfirm() {
    // 선택한 게 없으면 null 반환
    finishOpenDirectory(selectedData ?? null);
  }

  function handleCancel() {
    finishOpenDirectory(null);
  }
</script>

{#if dialog}
  <div class="backdrop">
    <div class="dialog">
      <h2>워크스페이스 선택</h2>

      <div class="body">
        <!-- 왼쪽: 워크스페이스 목록 -->
        <div class="workspace-list">
          {#each dialog.workspaces as w}
            <button
              type="button"
              class:selected={selectedId === w.id}
              on:click={() => handleSelect(w)}
            >
              {w.name} ({w.id})
            </button>
          {/each}
        </div>

        <!-- 오른쪽: 선택된 워크스페이스 트리 미리보기 -->
        <div class="workspace-tree">
          {#if loadingWorkspace}
            <p>트리 로딩 중...</p>
          {:else if selectedData}
            <!-- 여기에 실제 트리 컴포넌트 연결하면 됨 -->
            <pre>{JSON.stringify(selectedData.tree, null, 2)}</pre>
          {:else}
            <p>워크스페이스를 선택하면 하위 트리가 표시됩니다.</p>
          {/if}
        </div>
      </div>

      <div class="footer">
        <button type="button" on:click={handleCancel}>
          취소
        </button>
        <button
          type="button"
          on:click={handleConfirm}
          disabled={!selectedData}
        >
          확인
        </button>
      </div>
    </div>
  </div>
{/if}
