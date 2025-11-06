<script lang="ts">
	import { fileTree } from '@/stores/fileTree';
	import TabLayout from '@layouts/TabLayout.svelte';
	import DirectoryTree from '@components/DirectoryTree.svelte';
	import type { TreeNode } from '@/types';
	import { goto } from '$app/navigation';

	let tree: TreeNode = $derived($fileTree);

  function handleSelect(node: TreeNode) {
    // console.log("clicked:", node.name);
    if (node.type === "file") goto(`/${encodeURIComponent(node.id)}`);
  }
</script>

<TabLayout
	leftTab={{ id: 'left', label: 'Controller' }}
	rightTab={{ id: 'right', label: 'Workspace' }}
>
	<div slot="left">
		<p>고급 설정 탭의 내용입니다.</p>
	</div>

	<div slot="right">
		<DirectoryTree root={tree} mode="view" onselect={handleSelect}/>
	</div>
</TabLayout>