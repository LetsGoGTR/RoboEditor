import type { FileNode, TreeNode } from '@/types';
import { readDirectory, writeTextFile } from './FSA';
import { fileTree } from '@/stores/fileTree';

export function createNewFileNode(
	name = 'untitled.yaml',
	parentId: string | null = null
): FileNode {
	const id = crypto.randomUUID();
	const now = Date.now();

	return {
		id,
		name,
		type: 'file',
		path: null,
		parentId,
		size: 0,
		lastModified: now,
		checked: false,
		handle: undefined,
		file: undefined,
		kind: 'file'
	};
}

// 확장자 별 언어 감지
export function detectLanguage(name: string): string {
	const ext = name.split('.').pop()?.toLowerCase() ?? '';
	switch (ext) {
		case 'yaml':
		case 'yml':
			return 'yaml';
		case 'json':
			return 'json';
		case 'ts':
			return 'typescript';
		case 'js':
			return 'javascript';
		case 'cpp':
		case 'h':
			return 'cpp';
		default:
			return 'plaintext';
	}
}

/**
 * 파일 저장 함수 (FSA + 새 파일 모두 지원)
 * - handle이 있으면 FSA로 저장
 * - handle이 없으면 showSaveFilePicker()로 새 파일 생성
 * @returns 저장 후 갱신된 FileNode
 */
export async function saveFile(file: FileNode, content: string): Promise<FileNode> {
	if (!file) throw new Error('FileNode가 지정되지 않았습니다.');

	// 1️⃣ 기존 로컬 파일 (FSA handle 존재)
	if (file.handle) {
		await writeTextFile(file.handle, content);
		file.lastModified = Date.now();
		console.info(`💾 저장 완료: ${file.name}`);
		return file;
	}

	// 2️⃣ 새 파일 (handle 없음 → 새로 생성)
	try {
		const handle = await window.showSaveFilePicker({
			suggestedName: file.name ?? 'untitled.txt'
		});

		await writeTextFile(handle, content);

		const updated: FileNode = {
			...file,
			handle,
			path: handle.name,
			lastModified: Date.now()
		};

		console.info(`💾 새 파일 저장 완료: ${updated.path}`);
		return updated;
	} catch (err) {
		if ((err as DOMException).name === 'AbortError') {
			console.warn('❗ 파일 저장이 취소되었습니다.');
		} else {
			console.error('❌ 파일 저장 중 오류:', err);
		}
		return file;
	}
}

export async function saveFileAndRefresh(file: FileNode, content: string) {
  const updated = await saveFile(file, content);

  // ✅ 저장된 폴더 기준으로 1회만 리프레시
  if (updated.handle) {
    const parentDir = await updated.handle.getParent?.(); // 직접 관리 필요
    if (parentDir) {
      const newTree = await readDirectory(parentDir);
      fileTree.set(newTree);
    }
  }

  return updated;
}

/**
 * 파일 또는 폴더 삭제
 * @param node 삭제 대상 (FileNode 또는 FolderNode)
 * @param parentHandle 상위 디렉토리 핸들
 */
export async function deleteNode(node: TreeNode, parentHandle?: FileSystemDirectoryHandle) {
  try {
    // --- 1️⃣ 실제 FSA 접근 가능할 때
    if (node.handle && parentHandle) {
      await parentHandle.removeEntry(node.name, { recursive: node.type === "folder" });
      console.info(`🗑️ '${node.name}' 삭제됨 (로컬 파일 시스템)`);
    } else {
      console.warn("⚠️ FSA 핸들이 없어 store 트리에서만 삭제됩니다.");
    }

    // --- 2️⃣ 현재 열려 있는 파일이 삭제된 경우 닫기
    const current = $currentFile.file;
    if (current && current.id === node.id) {
      currentFile.close();
    }

    // --- 3️⃣ 트리 재스캔으로 반영
    if (parentHandle) {
      const updatedTree = await readDirectory(parentHandle);
      fileTree.set(updatedTree);
    }

    alert(`'${node.name}' 파일이 삭제되었습니다.`);
  } catch (err) {
    console.error("❌ 파일 삭제 실패:", err);
    alert("파일 삭제 중 오류가 발생했습니다.");
  }
}