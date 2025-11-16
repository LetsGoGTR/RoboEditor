import { currentFile } from '@/stores/currentFile';
import { _diffFiles, _diffWorkspaces } from '@apis/diff';
import type { FileNode } from '@/types';

/**
 * 파일 Diff Handler
 * @param leftFile 왼쪽 diff 파일
 * @param rightFile 오른쪽 diff 파일
 */
export async function handleDiffFiles(leftFile: FileNode, rightFile: FileNode) {
	if (!leftFile?.path || !rightFile?.path) {
		alert('비교할 파일 경로가 올바르지 않습니다.');
		return;
	}

	try {
		// 1) Diff API 호출
		const resp = await _diffFiles(leftFile.path, rightFile.path);

		if (!resp?.success) {
			alert('파일 비교 중 오류가 발생했습니다.');
			return;
		}

		// 2) Diff 모드 진입 (기본 구조)
		currentFile.openDiff(leftFile, rightFile);

		// 3) diff content store 반영
		currentFile.update((s) => {
			if (!s.diff) return s;

			return {
				...s,
				diff: {
					left: { file: leftFile, content: resp.leftContent },
					right: { file: rightFile, content: resp.rightContent }
				}
			};
		});
	} catch (err) {
		console.error('[diff-files] 비교 중 오류:', err);
		alert('파일 비교 중 문제가 발생했습니다.');
	}
}

/**
 * 워크스페이스(폴더) Diff Handler
 * @param dirA 비교 기준이 되는 좌측 폴더 경로
 * @param dirB 비교 대상이 되는 우측 폴더 경로
 */
export async function handleDiffWorkspaces(dirA: string, dirB: string) {
	if (!dirA || !dirB) {
		alert('비교할 폴더 경로가 올바르지 않습니다.');
		return;
	}

	try {
		const resp = await _diffWorkspaces(dirA, dirB);

		if (!resp?.success) {
			alert('폴더 비교 중 오류가 발생했습니다.');
			return;
		}
	} catch (err) {
		console.error('[diff-workspaces] 비교 오류:', err);
		alert('폴더 비교 중 문제가 발생했습니다.');
	}
}
