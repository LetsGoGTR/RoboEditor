import type { FolderNode, FileNode } from '@/types';
import { _updateFile } from '@apis/file';

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
 * 서버 기반 파일 저장 함수
 * - file.path 기준으로 서버가 파일 생성 or 수정
 */
export async function saveFile(file: FileNode, content: string): Promise<FileNode> {
	if (!file || !file.path) {
		throw new Error('FileNode 또는 파일 경로가 올바르지 않습니다.');
	}

	try {
		// 🔥 서버에 업데이트 요청
		const res = await _updateFile(file.path, { content });

		// 서버 응답으로 최신 메타데이터 갱신
		const updated: FileNode = {
			...file,
			lastModified: Date.now(),
			size: content.length,
			// 서버가 반환한 값 우선 적용
			...res
		};

		console.info(`💾 저장 완료: ${updated.path}`);
		return updated;
	} catch (err) {
		console.error('❌ 파일 저장 중 오류:', err);
		throw err;
	}
}

/**
 * 재귀 폴더 트리 로딩 (backend 응답: directories[], files[])
 */
export async function fetchFolderTreeRecursively(
	subPath: string,
	fetchFn: (path: string) => Promise<any>
): Promise<FolderNode> {
	const raw = await fetchFn(subPath);

	// backend는 raw.data 안에 directories/files 제공
	const dirs = raw.data?.directories ?? [];
	const files = raw.data?.files ?? [];

	const folderNode: FolderNode = {
		id: crypto.randomUUID(),
		name: extractNameFromPath(subPath),
		type: 'directory',
		path: subPath,
		children: []
	};

	/** 파일 추가 */
	for (const f of files) {
		folderNode.children.push({
			id: crypto.randomUUID(),
			name: f.name,
			type: 'file',
			path: f.path,
			size: f.size ?? 0,
			lastModified: f.lastModified ?? 0
		});
	}

	/** 디렉토리 재귀 */
	for (const d of dirs) {
		const newPath = subPath + `/${d.name}`; // drogon이 절대경로 반환

		const child = await fetchFolderTreeRecursively(newPath, fetchFn);
		folderNode.children.push(child);
	}

	return folderNode;
}

function extractNameFromPath(path: string) {
	if (!path || path === '/') return '/';
	return path.split('/').filter(Boolean).pop() ?? path;
}
