// src/lib/stores/fileTree.ts
import { writable } from 'svelte/store';
import type { TreeNode } from '@/types';
import { browser } from '$app/environment';

export const fileTree = writable<TreeNode | null>(null);

/** 디렉토리 열기 (FSA + Fallback) */
export async function openDirectory() {
	if (!browser) return; // SSR 방어

	// FSA API 사용 가능?
	if ('showDirectoryPicker' in window) {
		try {
			const handle = await (window as any).showDirectoryPicker();
			const root = await readDirectory(handle);
			fileTree.set(root);
			return;
		} catch (err) {
			console.warn('❗ 디렉토리 선택 취소됨 또는 접근 거부:', err);
		}
	}

	// Fallback 방식
	await openDirectoryFallback();
}

/** 디렉토리 읽기 */
async function readDirectory(dirHandle: FileSystemDirectoryHandle): Promise<TreeNode> {
	const children: TreeNode[] = [];

	for await (const [name, handle] of dirHandle.entries()) {
		if (handle.kind === 'file') {
			const file = await handle.getFile();
			children.push({
				id: crypto.randomUUID(),
				name,
				type: 'file',
				path: `${dirHandle.name}/${name}`,
				size: file.size,
				lastModified: file.lastModified
			});
		} else if (handle.kind === 'directory') {
			children.push(await readDirectory(handle));
		}
	}

	return {
		id: crypto.randomUUID(),
		name: dirHandle.name,
		type: 'folder',
		path: dirHandle.name,
		children
	};
}

/** Fallback 방식 (webkitdirectory) */
function openDirectoryFallback(): Promise<void> {
	if (!browser) return Promise.resolve();

	return new Promise((resolve) => {
		const input = document.createElement('input');
		input.type = 'file';
		(input as any).webkitdirectory = true;
		input.multiple = true;
		input.style.display = 'none';

		input.addEventListener('change', async (e) => {
			const files = (e.target as HTMLInputElement).files;
			if (!files) return;

			const root: TreeNode = {
				id: crypto.randomUUID(),
				name: 'selected_folder',
				type: 'folder',
				path: 'selected_folder',
				children: []
			};

			for (const file of Array.from(files)) {
				const segments = file.webkitRelativePath.split('/');
				await insertFileNode(root, segments, file);
			}

			fileTree.set(root);
			resolve();
		});

		document.body.appendChild(input);
		input.click();
	});
}

/** 재귀 삽입 */
async function insertFileNode(root: TreeNode, pathParts: string[], file: File) {
	const [head, ...rest] = pathParts;

	if (rest.length === 0) {
		root.children = root.children || [];
		root.children.push({
			id: crypto.randomUUID(),
			name: head,
			type: 'file',
			path: file.webkitRelativePath,
			size: file.size,
			lastModified: file.lastModified
		});
	} else {
		root.children = root.children || [];
		let folder = root.children.find((n) => n.name === head && n.type === 'folder');
		if (!folder) {
			folder = {
				id: crypto.randomUUID(),
				name: head,
				type: 'folder',
				path: head,
				children: []
			};
			root.children.push(folder);
		}
		await insertFileNode(folder, rest, file);
	}
}
