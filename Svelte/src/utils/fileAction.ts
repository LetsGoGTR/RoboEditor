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

const DEV_BASE_ROUTE = '/tmp/drogon-app/storage/';
const REL_BASE_ROUTE = 'C:/backup/';

/**
 * 절대 경로에서 base prefix 제거 → 순수 상대 경로만 반환
 */
export function stripBaseRoute(fullPath: string): string {
	if (!fullPath) return fullPath;

	if (fullPath.startsWith(DEV_BASE_ROUTE)) {
		return fullPath.substring(DEV_BASE_ROUTE.length);
	}
	if (fullPath.startsWith(REL_BASE_ROUTE)) {
		return fullPath.substring(REL_BASE_ROUTE.length);
	}
	return fullPath; // prefix가 없으면 그대로
}

/**
 * 재귀 폴더 트리 로딩 (backend 응답: directories[], files[])
 */
export async function fetchFolderTreeRecursively(
  subPath: string,
  fetchFn: (path: string) => Promise<any>
): Promise<FolderNode> {
  const raw = await fetchFn(subPath);

  const dirs = raw.data?.directories ?? [];
  const files = raw.data?.files ?? [];

  // 🔥 base 제거된 path 사용
  const cleanSubPath = stripBaseRoute(subPath);

  const folderNode: FolderNode = {
    id: crypto.randomUUID(),
    name: extractNameFromPath(cleanSubPath),
    type: 'directory',
    path: cleanSubPath,
    children: []
  };

  /** 파일 추가 */
  for (const f of files) {
    const cleanFilePath = stripBaseRoute(f.path);

    folderNode.children.push({
      id: crypto.randomUUID(),
      name: f.name,
      type: 'file',
      path: cleanFilePath,
      size: f.size ?? 0,
      lastModified: f.lastModified ?? 0
    });
  }

  /** 디렉토리 재귀 */
  for (const d of dirs) {
    // drogon이 절대경로 반환하므로 반드시 base 제거
    const nextFull = subPath + `/${d.name}`;
    const cleanNext = stripBaseRoute(nextFull);

    const child = await fetchFolderTreeRecursively(cleanNext, fetchFn);
    folderNode.children.push(child);
  }

  return folderNode;
}

function extractNameFromPath(path: string) {
	if (!path || path === '/') return '/';
	return path.split('/').filter(Boolean).pop() ?? path;
}
