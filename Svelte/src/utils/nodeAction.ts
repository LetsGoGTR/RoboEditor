import type { FolderNode } from '@/types';

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

// 확장자 별 언어 감지
export function detectLanguage(name: string): string {
	const ext = name.split('.').pop()?.toLowerCase() ?? '';
	switch (ext) {
		case 'yaml':
		case 'yml':
			return 'yaml';
		case 'json':
			return 'json';
		case 'py':
		case 'pts':
		case 'srl':
			return 'python';
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

function extractNameFromPath(path: string) {
	if (!path || path === '/') return '/';
	return path.split('/').filter(Boolean).pop() ?? path;
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
/**
 * path 앞쪽의 슬래시를 정리해
 * 항상 "슬래시 1개 + 나머지 경로" 형태로 만들어 줍니다.
 */
export function normalizeSlash(path: string): string {
	if (!path) return '/';

	// 앞부분의 연속된 슬래시 제거
	const trimmed = path.replace(/^\/+/, '');

	// 결과가 비면 "/"만 반환, 아니면 앞에 슬래시 하나만 붙여서 반환
	return trimmed ? `/${trimmed}` : '/';
}
