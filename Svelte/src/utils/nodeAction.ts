import type { FolderNode } from '@/types';
import { get } from 'svelte/store';
import { workspaceContext } from '@/stores/selectedDirectory';

let BASE_DIRS: string[] = [];

/**
 * 나중에 백에서 base_dir를 내려주면 이 함수로 등록해서 사용
 */
export function configureBaseDirs(dirs: string[]) {
	BASE_DIRS = dirs
		.filter(Boolean)
		.map((d) =>
			d
				.replace(/\\/g, '/')
				.replace(/\/+$/, '') // 뒤쪽 슬래시 정리
		);
}

configureBaseDirs([
	'/tmp/drogon-app/storage',
	'C:/backup'
]);

export function stripBaseRoute(fullPath: string): string {
	if (!fullPath) return fullPath;

	const normalized = fullPath.replace(/\\/g, '/');

	for (const base of BASE_DIRS) {
		if (!base) continue;
		if (normalized === base) return '';
		if (normalized.startsWith(base + '/')) {
			return normalized.slice(base.length + 1);
		}
	}

	// prefix가 없으면 일단 슬래시만 정리해서 반환
	return normalized.replace(/^\/+/, '');
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

	const folderNode: FolderNode = {
		id: crypto.randomUUID(),
		name: extractNameFromPath(subPath),
		type: 'directory',
		path: subPath,
		children: []
	};

	/** 파일 추가 */
	for (const f of files) {

		const rawPath: string | undefined = f.path;
		const cleanPath =
			rawPath != null && rawPath !== ''
				? stripBaseRoute(rawPath)
				: subPath
				? `${subPath}/${f.name}`
				: f.name;

		folderNode.children.push({
			id: crypto.randomUUID(),
			name: f.name,
			type: 'file',
			path: cleanPath,
			size: f.size ?? 0,
			lastModified: f.lastModified ?? 0
		});
	}

	/** 디렉토리 재귀 */
	for (const d of dirs) {
		// drogon이 절대경로 반환하므로 반드시 base 제거
		const nextSubPath = subPath ? `${subPath}/${d.name}` : d.name;

		const child = await fetchFolderTreeRecursively(nextSubPath, fetchFn);
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

// 워크스페이스 내부 기준 상대 경로로만 보여주기
export function toWorkspaceDisplayPath(path: string | null | undefined): string {
  if (!path) return '/';

  const rel = stripBaseRoute(path);
  const parts = rel.split('/').filter(Boolean);

  // [deviceId, workspaceUuid] 만 있으면 워크스페이스 루트
  if (parts.length === 0) {
    return '/';
  }

  const ctx = get(workspaceContext);

  const deviceIdSeg = parts[0];
  const workspaceIdSeg = parts.length > 1 ? parts[1] : undefined;
  const inner = parts.slice(2); // 워크스페이스 내부 경로

  const segments: string[] = [];

  if (ctx) {
    // 컨텍스트가 있으면 → “이름으로 치환”
    segments.push(ctx.deviceName);
    segments.push(ctx.workspaceName);
  } else {
    // 혹시 컨텍스트가 없으면 → 기존 id/uuid 그대로라도 보여주기
    segments.push(deviceIdSeg);
    if (workspaceIdSeg) segments.push(workspaceIdSeg);
  }

  segments.push(...inner);

  return '/' + segments.join('/');
}