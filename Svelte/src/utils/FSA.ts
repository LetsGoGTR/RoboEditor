import { browser } from '$app/environment';
import type { FolderNode, TreeNode } from '@/types';

// Type for Browser only
interface Window {
	showDirectoryPicker?: () => Promise<FileSystemDirectoryHandle>;
}

// Open Directory (FSA + Fallback)
export async function openDirectory(): Promise<TreeNode | null> {
	if (!browser) return null;

	const win = window as Window;

	if (typeof win.showDirectoryPicker === 'function') {
		try {
			const handle = await win.showDirectoryPicker();
			return await readDirectory(handle);
		} catch (err) {
			console.warn('❗ 디렉토리 선택 취소됨 또는 접근 거부:', err);
			return null;
		}
	}

	return await openDirectoryFallback();
}

// Read Directory
async function readDirectory(handle: FileSystemDirectoryHandle): Promise<FolderNode> {
	const children: TreeNode[] = [];

	for await (const [name, entry] of handle.entries()) {
		if (entry.kind === 'file') {
			const file = await entry.getFile();
			children.push({
				id: crypto.randomUUID(),
				name,
				type: 'file',
				path: `${handle.name}/${name}`,
				size: file.size,
				lastModified: file.lastModified,
				handle: entry,
				kind: 'file'
			});
		} else if (entry.kind === 'directory') {
			children.push(await readDirectory(entry));
		}
	}

	return {
		id: crypto.randomUUID(),
		name: handle.name,
		type: 'folder',
		path: handle.name,
		children,
		handle,
		kind: 'directory'
	};
}

// Fallback (webkitdirectory)
function openDirectoryFallback(): Promise<TreeNode | null> {
	if (!browser) return Promise.resolve(null);

	return new Promise((resolve) => {
		const input = document.createElement('input') as HTMLInputElement;
		input.type = 'file';
		input.webkitdirectory = true;
		input.multiple = true;
		input.style.display = 'none';

		input.addEventListener('change', async (e) => {
			const target = e.target as HTMLInputElement;
			const files = target.files;
			if (!files) {
				resolve(null);
				return;
			}

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

			resolve(root);
		});

		document.body.appendChild(input);
		input.click();
	});
}

// node type check
function isFolderNode(node: TreeNode): node is FolderNode {
	return node.type === 'folder';
}

// recursive insert node
async function insertFileNode(root: TreeNode, pathParts: string[], file: File) {
	const [head, ...rest] = pathParts;

	// safey narrowing type
	if (!isFolderNode(root)) return;

	root.children = root.children || [];

	if (rest.length === 0) {
		root.children.push({
			id: crypto.randomUUID(),
			name: head,
			type: 'file',
			path: file.webkitRelativePath,
			size: file.size,
			lastModified: file.lastModified
		});
	} else {
		let folder = root.children.find((n): n is FolderNode => n.name === head && n.type === 'folder');
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

// Read text from a file
export async function readTextFile(handle: FileSystemFileHandle): Promise<string | null> {
	try {
		const file = await handle.getFile();
		return await file.text(); // YAML, XML, SRL 등 모두 텍스트 처리 가능
	} catch (err) {
		console.error('❌ 파일 읽기 실패:', err);
		return null;
	}
}

// Write text from a file
export async function writeTextFile(
	handle: FileSystemFileHandle,
	content: string
): Promise<boolean> {
	try {
		const writable = await handle.createWritable();
		await writable.write(content);
		await writable.close();
		return true;
	} catch (err) {
		console.error('❌ 파일 쓰기 실패:', err);
		return false;
	}
}
