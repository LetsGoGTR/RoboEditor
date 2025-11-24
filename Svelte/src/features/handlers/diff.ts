import { fileDiffStore } from '@/stores/fileDiff';
import { _diffFiles, _diffWorkspaces } from '@apis/diff';
import type { FileNode, FolderNode } from '@/types';
import { _getFile } from '@apis/file';
import { folderDiffStore } from '@/stores/folderDiff';
import { normalizeSlash } from '@utils/nodeAction';

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
		// 🔥 파일명 + 경로 모두 전달
		fileDiffStore.openFileDiff(leftFile.name, rightFile.name, leftFile.path, rightFile.path);

		const [leftRes, rightRes] = await Promise.all([
			_getFile(leftFile.path),
			_getFile(rightFile.path)
		]);
		console.log(leftRes, rightRes);

		const leftContent = leftRes?.data?.content ?? '';
		const rightContent = rightRes?.data?.content ?? '';

		fileDiffStore.applyFileDiffContent(leftContent, rightContent);

		const apiRes = await _diffFiles(leftFile.path, rightFile.path);

		if (!apiRes?.success) {
			console.warn('[diff-files] diff API 실패:', apiRes);
			return;
		}
		console.log(apiRes);

		fileDiffStore.setApiDiffResult(apiRes);
	} catch (err) {
		console.error('[diff-files] 오류 발생:', err);
		alert('파일 비교 중 오류가 발생했습니다.');
	}
}

/**
 * 작업 폴더 Diff Handler
 * @param leftPath  왼쪽 폴더 경로
 * @param rightPath 오른쪽 폴더 경로
 */
export async function handleDiffWorkspaces(leftFolder: FolderNode, rightFolder: FolderNode) {
	if (!leftFolder || !rightFolder) {
		alert('비교할 폴더 경로가 올바르지 않습니다.');
		return;
	}

	if (!leftFolder.path || !rightFolder.path) {
		alert('폴더 경로 해석에 오류가 발생하였습니다.');
		return;
	}

	try {
		const pathA = normalizeSlash(leftFolder.path);
		const pathB = normalizeSlash(rightFolder.path);

		// 🔥 diff 상태 초기화 (폴더 경로 정보 전달)
		folderDiffStore.openFolderDiff(pathA, pathB);

		// 🔥 Diff API 호출
		const apiRes = await _diffWorkspaces(pathA, pathB);

		if (!apiRes?.success) {
			console.warn('[diff-workspaces] diff API 실패:', apiRes);
			return;
		}

		console.log(apiRes);

		// 🔥 API diff 결과 저장
		fileDiffStore.setApiDiffResult(apiRes);
	} catch (err) {
		console.error('[diff-workspaces] 오류 발생:', err);
		alert('폴더 비교 중 오류가 발생했습니다.');
	}
}
