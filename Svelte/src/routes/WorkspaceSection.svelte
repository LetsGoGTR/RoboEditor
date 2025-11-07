<script lang="ts">
	import TabLayout from '@layouts/TabLayout.svelte';
	import DirectoryTree from '@components/DirectoryTree.svelte';
	import { browser } from '$app/environment';
	import type { TreeNode } from '@/types';
	import { goto } from '$app/navigation';
	import { fileTree, openDirectory } from '@/stores/fileTree';

	let tree: TreeNode | null = $derived($fileTree);

  function handleSelect(node: TreeNode) {
    // console.log("clicked:", node.name);
    if (node.type === "file") goto(`/${encodeURIComponent(node.id)}`);
  }

	async function handleClick() {
		if (!browser) return;
		await openDirectory();
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
		<!-- 디렉토리 표시 영역 -->
		{#if tree}
			<DirectoryTree root={tree} mode="view" onselect={handleSelect} />
		{:else}
			<p>📂 아직 열려 있는 디렉토리가 없습니다.</p>
		{/if}

		<!-- 디렉토리 열기 버튼 -->
		<button onclick={handleClick} class="open-dir-btn">
			📁 Open Directory
		</button>
	</div>
</TabLayout>