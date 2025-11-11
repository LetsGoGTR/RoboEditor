import { browser } from '$app/environment';
import type { FolderNode, TreeNode } from '@/types';
import { fileTree } from '@/stores/fileTree';
import { get } from 'svelte/store';

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
export async function readDirectory(handle: FileSystemDirectoryHandle): Promise<FolderNode> {
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

// Delete Entry (file or folder)
export async function deleteEntry(
	parentHandle: FileSystemDirectoryHandle,
	name: string,
	recursive = false
): Promise<boolean> {
	if (!browser) return false;

	try {
		await parentHandle.removeEntry(name, { recursive });
		console.info(`🗑️ '${name}' 삭제 성공`);
		return true;
	} catch (err) {
		console.error(`❌ '${name}' 삭제 실패:`, err);
		return false;
	}
}

/**
 * 📁 사용자가 폴더를 직접 선택한 뒤, 해당 위치에 새 폴더를 생성
 * - showDirectoryPicker()로 경로 선택
 * - 선택된 디렉토리 안에 지정된 이름으로 폴더 생성
 */
export async function createFolderWithDialog(): Promise<FileSystemDirectoryHandle | null> {
	try {
		// 1️⃣ 폴더 선택
		const parentHandle = await window.showDirectoryPicker({
			id: 'roboeditor-folder-select',
			mode: 'readwrite',
			startIn: 'documents'
		});

		// 2️⃣ 새 폴더 이름 입력받기
		const name = prompt('새 폴더 이름을 입력하세요:', 'new_folder');
		if (!name) return null;

		// 3️⃣ 동일 이름 검사
		for await (const [entryName, entry] of parentHandle.entries()) {
			if (entryName === name && entry.kind === 'directory') {
				console.warn(`⚠️ 이미 동일한 이름의 폴더가 존재합니다: ${name}`);
				return entry as FileSystemDirectoryHandle;
			}
		}

		// 4️⃣ 폴더 생성
		const newHandle = await parentHandle.getDirectoryHandle(name, { create: true });
		console.info(`📁 새 폴더 생성됨: ${parentHandle.name}/${name}`);
		return newHandle;
	} catch (err) {
		console.error('❌ 폴더 생성 실패:', err);
		return null;
	}
}

/**
 * 🔄 현재 workspace(루트)는 유지하면서 트리 내용을 다시 읽기
 * - 루트 handle을 재사용
 * - 내부 구조를 최신 상태로 갱신
 */
export async function refreshWorkspaceTree(): Promise<void> {
	const root = get(fileTree) as FolderNode | null;

	if (!root || !root.handle) {
		console.warn('⚠️ 현재 열린 workspace 폴더가 없습니다.');
		return;
	}

	try {
		const updated = await readDirectory(root.handle);
		const newRoot: FolderNode = {
			...root,
			children: updated.children // ✅ 내부 항목만 교체
		};
		fileTree.set(newRoot);
		console.info('✅ workspace 갱신 완료');
	} catch (err) {
		console.error('❌ workspace 갱신 실패:', err);
	}
}

/**
 * 📄 단일 파일 선택 (File System Access API)
 * - 비교 대상 파일 선택용
 * - YAML, JSON, TXT 등 텍스트 파일에 적합
 */
export async function openFile(): Promise<FileSystemFileHandle | null> {
	if (!('showOpenFilePicker' in window)) {
		alert('이 브라우저는 File System Access API를 지원하지 않습니다.');
		return null;
	}

	try {
		const [handle] = await window.showOpenFilePicker({
			id: 'roboeditor-file-select',
			multiple: false,
			startIn: 'documents',
			types: [
				{
					description: 'Text and Config Files',
					accept: {
						'text/*': ['.yaml', '.yml', '.json', '.txt', '.xml', '.cfg']
					}
				}
			]
		});
		return handle;
	} catch (err) {
		console.warn('❗ 파일 선택 취소 또는 접근 거부:', err);
		return null;
	}
}
