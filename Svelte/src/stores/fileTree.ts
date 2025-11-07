import { writable } from 'svelte/store';
import type { TreeNode } from '@/types';
import { openDirectory } from '@/utils/FSA';

export const fileTree = writable<TreeNode | null>(null);

export async function loadDirectoryToStore() {
	const result = await openDirectory();
	if (result) fileTree.set(result);
}
