<script lang="ts">
	import { currentPage } from "@/stores/currentPage";
	import ApplyPage from "./ApplyPage.svelte";
	import BackupPage from "./BackupPage.svelte";
	import ComparePage from "./ComparePage.svelte";
	import EditorPage from "./EditorPage.svelte";
	import RegisterPage from "./RegisterPage.svelte";
</script>

<main class="workspace">
	{#if $currentPage === 'edit'}
		<EditorPage />
	{:else if $currentPage === 'compare'}
		<ComparePage />
	{:else if $currentPage === 'apply'}
		<ApplyPage />
	{:else if $currentPage === 'backup'}
		<BackupPage />
	{:else if $currentPage === 'register'}
		<RegisterPage />
	{/if}
</main>

<style>
	.workspace {
		flex: 1;
		width: 100%;
		height: 100%;
		display: block;
		overflow: hidden;
	}
</style>
