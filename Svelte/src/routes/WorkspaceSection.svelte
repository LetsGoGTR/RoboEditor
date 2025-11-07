<script lang="ts">
	import TabLayout from '@layouts/TabLayout.svelte';
	import DirectoryTree from '@components/DirectoryTree.svelte';
	import type { TreeNode } from '@/types';
	import { goto } from '$app/navigation';
	import { fileTree } from '@/stores/fileTree';
	import ControllerDir from './ControllerDir.svelte';
	import { onDestroy } from 'svelte';

	let tree: TreeNode | null = $derived($fileTree);
	let activeId: 'left' | 'right' = $state('left'); // 현재 활성 탭 상태

	function handleSelect(node: TreeNode) {
		if (node.type === 'file') goto(`/${encodeURIComponent(node.id)}`);
	}

	// ✅ fileTree 변경 감지 → Workspace 탭으로 자동 전환
	const unsubscribe = fileTree.subscribe((value) => {
		if (value) activeId = 'right';
	});
	onDestroy(unsubscribe);

	// ✅ 수동 탭 클릭 대응
	function handleTabChange(id: 'left' | 'right') {
		activeId = id;
	}
</script>

<TabLayout
	leftTab={{ id: 'left', label: 'Controller' }}
	rightTab={{ id: 'right', label: 'Workspace' }}
	activeId={activeId}
	onTabChange={handleTabChange}
>
	<div slot="left">
		<ControllerDir />
	</div>

	<div slot="right">
		{#if tree}
			<DirectoryTree root={tree} mode="view" onselect={handleSelect} />
		{:else}
			<p class="notice">아직 열려 있는 디렉토리가 없습니다.</p>
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
