<script lang="ts">
	import TabLayout from '@layouts/TabLayout.svelte';
	import DirectoryTree from '@components/DirectoryTree.svelte';
	import type { FileNode, TreeNode } from '@/types';
	import { fileTree } from '@/stores/fileTree';
	import ControllerDir from './ControllerDir.svelte';
	import { onDestroy } from 'svelte';
	import { currentFile } from '@/stores/currentFile';
	import { gotoPage } from '@/stores/currentPage';
	import { selectedDirectory } from '@/stores/selectedDirectory';
	import { get } from 'svelte/store';

	let tree = $derived(get(fileTree));

	let activeId: 'left' | 'right' = $state('left');

	// 파일 클릭 시 store 저장 및 routing
	function handleSelect(node: TreeNode) {
		if (node.type === "directory") {
    	selectedDirectory.set(node);
		}	else if (node.type === 'file') {
			currentFile.open(node as FileNode);
			gotoPage('edit');
		}
	}

	// workspace 감지
	const unsubscribe = fileTree.subscribe((value) => {
		tree = value;
		if (value) activeId = 'right';
	});

	// 수동 클릭 대응
	function handleTabChange(id: 'left' | 'right') {
		activeId = id;
	}

	onDestroy(unsubscribe);
</script>

<TabLayout
	leftTab={{ id: 'left', label: 'Controller' }}
	rightTab={{ id: 'right', label: 'Workspace' }}
	{activeId}
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
