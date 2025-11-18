import { fileDiffStore } from '@/stores/fileDiff';
import { _diffFiles } from '@apis/diff';
import type { FileNode } from '@/types';
import { _getFile } from '@apis/file';

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
