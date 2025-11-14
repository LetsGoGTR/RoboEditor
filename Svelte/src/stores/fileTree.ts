import { writable } from 'svelte/store';
import type { FolderNode } from '@/types';

export const fileTree = writable<FolderNode | null>(null);

// 선택적으로 reset 기능
export function resetFileTree() {
	fileTree.set(null);
}
