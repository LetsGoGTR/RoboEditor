import { writable } from 'svelte/store';
import type { FolderNode } from '@/types';

export const fileTree = writable<FolderNode | null>(null);

/** 전체 트리 교체 */
export function setFileTree(tree: FolderNode) {
	fileTree.set(tree);
}

/** 전체 초기화 */
export function resetFileTree() {
	fileTree.set(null);
}

/** 특정 경로에 새 FolderNode 삽입 (부분 갱신) */
export function insertFolderNode(parentPath: string, newNode: FolderNode) {
	fileTree.update((root) => {
		if (!root) return root;

		function dfs(node: FolderNode): boolean {
			if (node.path === parentPath) {
				// 하위 children에 삽입
				node.children = [...node.children, newNode];
				return true;
			}
			for (const child of node.children) {
				if (child.type === 'directory') {
					if (dfs(child)) return true;
				}
			}
			return false;
		}

		dfs(root);
		return root;
	});
}

/** 트리 전체 새로고침을 위한 헬퍼 */
export async function refreshFileTree(loader: () => Promise<FolderNode>) {
	const updated = await loader();
	fileTree.set(updated);
}
