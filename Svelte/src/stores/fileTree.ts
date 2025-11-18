import { writable } from 'svelte/store';
import type { FileNode, FolderNode } from '@/types';

export const fileTree = writable<FolderNode | null>(null);

/** 전체 트리 교체 */
export function setFileTree(tree: FolderNode) {
	fileTree.set(tree);
}

/** 전체 초기화 */
export function resetFileTree() {
	fileTree.set(null);
}

function cloneFolder(node: FolderNode): FolderNode {
	return {
		...node,
		children: node.children.map((c) => (c.type === 'directory' ? cloneFolder(c) : { ...c }))
	};
}

/* 특정 경로에 File 삽입 */
export function insertFileNode(parentPath: string, newFile: FileNode) {
	fileTree.update((root) => {
		if (!root) return root;

		const cloned = cloneFolder(root);

		function dfs(node: FolderNode): FolderNode {
			if (node.path === parentPath) {
				return {
					...node,
					children: [...node.children, newFile]
				};
			}

			return {
				...node,
				children: node.children.map((c) => (c.type === 'directory' ? dfs(c) : c))
			};
		}

		return dfs(cloned);
	});
}

/** 특정 경로에 새 FolderNode 삽입 (부분 갱신) */
export function insertFolderNode(parentPath: string, newNode: FolderNode) {
	fileTree.update((root) => {
		if (!root) return root;

		const cloned = cloneFolder(root);

		function dfs(node: FolderNode): FolderNode {
			if (node.path === parentPath) {
				return {
					...node,
					children: [...node.children, newNode]
				};
			}

			return {
				...node,
				children: node.children.map((c) => (c.type === 'directory' ? dfs(c) : c))
			};
		}

		return dfs(cloned);
	});
}

/** 트리에서 파일 노드를 제거 */
export function removeFileNode(targetPath: string) {
	fileTree.update((root) => {
		if (!root) return root;

		const cloned = cloneFolder(root);

		function dfs(node: FolderNode): FolderNode {
			return {
				...node,
				children: node.children
					.map((c) => (c.type === 'directory' ? dfs(c) : c))
					.filter((c) => (c.type === 'file' ? c.path !== targetPath : true))
			};
		}

		return dfs(cloned);
	});
}

/** 폴더 및 하위 전체 제거 */
export function removeFolderNode(targetPath: string) {
	fileTree.update((root) => {
		if (!root) return root;

		const cloned = cloneFolder(root);

		function dfs(node: FolderNode): FolderNode {
			return {
				...node,
				children: node.children
					.filter((c) => (c.type === 'directory' ? c.path !== targetPath : true))
					.map((c) => (c.type === 'directory' ? dfs(c) : c))
			};
		}

		return dfs(cloned);
	});
}

/** 트리 전체 새로고침을 위한 헬퍼 */
export async function refreshFileTree(loader: () => Promise<FolderNode>) {
	const updated = await loader();
	fileTree.set(structuredClone(updated));
}
