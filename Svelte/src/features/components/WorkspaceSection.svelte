<script lang="ts">
  import TabLayout from '@layouts/TabLayout.svelte';
  import DirectoryTree from '@components/DirectoryTree.svelte';
  import type { FileNode, TreeNode, Workspace } from '@/types';
  import { fileTree } from '@/stores/fileTree';
  import ControllerDir from './ControllerDir.svelte';
  import { onDestroy } from 'svelte';
  import { currentFile } from '@/stores/currentFile';
  import { gotoPage } from '@/stores/currentPage';
  import { selectedDirectory } from '@/stores/selectedDirectory';
  import { _getWorkspace} from '@/apis/workspace';
  import { toWorkspaceFromApi } from '@/utils/workspaceApiTransport';

  let tree: TreeNode | null = null;
  let activeId: 'left' | 'right' = 'left';

  let controllerDirRef: any;

  // 탭 변경 시
  function handleTabChange(id: 'left' | 'right') {
    activeId = id;

    if (activeId === 'left') {
      controllerDirRef?.loadDevices();
    }
  }

  // 실제 처리 로직 (async)
  async function handleNodeSelectInternal(node: TreeNode) {
    // 1) 워크스페이스 엔트리 노드인지: id가 "workspace:"로 시작하는 디렉토리
    if (node.type === 'directory' && node.id.startsWith('workspace:')) {
      const workspaceId = node.id.slice('workspace:'.length);

      // deviceId는 루트 노드의 path 또는 id에서 가져옴
      if (!tree || tree.type !== 'directory' || !tree.id.startsWith('device:')) {
        console.error('디바이스 정보가 없습니다.');
        return;
      }

      const deviceId = tree.path ?? tree.id.slice('device:'.length);

      try {
        const res: any = await _getWorkspace(deviceId, workspaceId);

        if (!res?.success) {
          console.error('워크스페이스 조회 실패');
          return;
        }

        const workspace: Workspace = toWorkspaceFromApi(res);

        // 🔥 여기서 그냥 "이제부터 이 워크스페이스 트리만 보여준다" 라고 생각하고 덮어쓰기
        fileTree.set(workspace);
        selectedDirectory.set(workspace);
        return;
      } catch (err) {
        console.error('워크스페이스 트리 조회 중 오류', err);
        return;
      }
    }

    // 2) 일반 디렉토리 / 파일 처리
    if (node.type === 'directory') {
      selectedDirectory.set(node);
    } else if (node.type === 'file') {
      currentFile.open(node as FileNode);
      gotoPage('edit');
    }
  }

  // DirectoryTree에 넘길 콜백 (시그니처 맞추기용)
  function handleSelect(node: TreeNode): void {
    void handleNodeSelectInternal(node);
  }

  // fileTree 구독 → 트리/탭 상태 갱신
  const unsubscribe = fileTree.subscribe((value) => {
    tree = value;
    if (value) activeId = 'right';
  });

  onDestroy(unsubscribe);
</script>

<TabLayout
  leftTab={{ id: 'left', label: 'Controller' }}
  rightTab={{ id: 'right', label: 'Workspace' }}
  {activeId}
  onTabChange={handleTabChange}
>
  <div slot="left">
    <ControllerDir bind:this={controllerDirRef} />
  </div>

  <div slot="right">
    {#if tree}
      <DirectoryTree root={tree} mode="view" onselect={handleSelect} />
    {:else}
      <p class="notice">디바이스를 선택하면 워크스페이스 / 디렉토리 트리가 표시됩니다.</p>
    {/if}
  </div>
</TabLayout>

<style>
  .notice {
    font-size: 0.95rem;
    color: #333;
    text-align: center;
    padding: 0.8rem 0;
  }
</style>
