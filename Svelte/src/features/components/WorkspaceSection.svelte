<script lang="ts">
  import TabLayout from '@layouts/TabLayout.svelte';
  import DirectoryTree from '@components/DirectoryTree.svelte';
  import type { FileNode, TreeNode, FolderNode } from '@/types';
  import { fileTree } from '@/stores/fileTree';
  import ControllerDir from './ControllerDir.svelte';
  import { onDestroy } from 'svelte';
  import { currentFile } from '@/stores/currentFile';
  import { gotoPage } from '@/stores/currentPage';
  import { selectedDirectory } from '@/stores/selectedDirectory';

  let tree: FolderNode | null = null;
  let activeId: 'left' | 'right' = 'left';

  let controllerDirRef: any;

  function handleTabChange(id: 'left' | 'right') {
    activeId = id;

    if (activeId === 'left') {
      controllerDirRef?.loadDevices();
    }
  }

  async function handleNodeSelectInternal(node: TreeNode) {

    if (node.type === 'directory') {
      selectedDirectory.set(node);
    } else if (node.type === 'file') {
      currentFile.open(node as FileNode);
      gotoPage('edit');
    }
  }

  function handleSelect(node: TreeNode): void {
    void handleNodeSelectInternal(node);
  }

  // fileTree 구독 → 워크스페이스 트리 세팅 / 탭 전환
  const unsubscribe = fileTree.subscribe((value) => {
    tree = value;
    if (value) {
      activeId = 'right';
    }
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
      <p class="notice">왼쪽에서 워크스페이스를 선택하면 디렉토리 트리가 표시됩니다.</p>
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
