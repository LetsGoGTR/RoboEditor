import { fetchFolderTreeRecursively } from '@utils/fileAction';
import type { PageLoad } from './$types';
import { fileTree } from '@/stores/fileTree';
import { _getFolder } from '@apis/folder';
export const ssr = false;

export const load = (async () => {
	const tree = await fetchFolderTreeRecursively('', _getFolder);
	console.log(tree);

	// 필요하다면 즉시 store 업데이트
	fileTree.set(tree);

	return {
		fileTree: tree // 페이지 props로도 전달 가능
	};
}) satisfies PageLoad;
