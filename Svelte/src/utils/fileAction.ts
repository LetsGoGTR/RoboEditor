import type { FileNode } from '@/types';
import { writeTextFile } from './FSA';

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
  if (!file) throw new Error("FileNode가 지정되지 않았습니다.");

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
      suggestedName: file.name ?? "untitled.txt",
      types: [
        {
          description: "Text / Config Files",
          accept: {
            "text/plain": [".txt", ".yaml", ".yml", ".json", ".ts", ".js", ".cpp", ".h"]
          }
        }
      ]
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
    if ((err as DOMException).name === "AbortError") {
      console.warn("❗ 파일 저장이 취소되었습니다.");
    } else {
      console.error("❌ 파일 저장 중 오류:", err);
    }
    return file;
  }
}