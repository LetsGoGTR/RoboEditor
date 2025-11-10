<script lang="ts">
	import { currentPage } from '@/stores/workspace';
	import EditPage from '@/pages/EditPage.svelte';
	import ComparePage from '@/pages/ComparePage.svelte';
	import ApplyPage from '@/pages/ApplyPage.svelte';
	import BackupPage from '@/pages/BackupPage.svelte';
	import RegisterPage from '@/pages/RegisterPage.svelte';
	import EmptyPage from '@/pages/EmptyPage.svelte';

	let page = $currentPage;
</script>

<main class="workspace">
	{#if page === 'edit'}
		<EditPage />
	{:else if page === 'compare'}
		<ComparePage />
	{:else if page === 'apply'}
		<ApplyPage />
	{:else if page === 'backup'}
		<BackupPage />
	{:else if page === 'register'}
		<RegisterPage />
	{:else}
		<EmptyPage />
	{/if}
</main>

<style>
	.workspace {
		flex: 1;
		height: 100%;
		display: flex;
		overflow: hidden;
		background: var(--background, #1e1e1e);
	}
</style>
