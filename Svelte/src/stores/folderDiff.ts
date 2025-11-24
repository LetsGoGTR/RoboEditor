import { writable } from 'svelte/store';
import type { DiffFilter, FileDiffItem, FileDiffResult, ApiDiffResponse } from '@/types';

export interface FolderDiffState {
	isOpen: boolean;

	leftPath: string;
	rightPath: string;

	diffs: FileDiffItem[];
	statistics: FileDiffResult['statistics'] | null;

	filter: DiffFilter;
}

const initialState: FolderDiffState = {
	isOpen: false,

	leftPath: '',
	rightPath: '',

	diffs: [],
	statistics: null,

	filter: 'all'
};

function createFolderDiffStore() {
	const { subscribe, update, set } = writable<FolderDiffState>(initialState);

	return {
		subscribe,

		/**
		 * 폴더 Diff 초기화
		 * @param leftPath  왼쪽 폴더 경로
		 * @param rightPath 오른쪽 폴더 경로
		 */
		openFolderDiff(leftPath: string, rightPath: string) {
			update((s) => ({
				...s,
				isOpen: true,

				leftPath,
				rightPath,

				diffs: [],
				statistics: null,
				filter: 'all'
			}));
		},

		/**
		 * ⭐ API Raw Response를 그대로 반영 ⭐
		 * 폴더 diff 구조 또한 파일 diff와 동일한 구조를 사용한다고 가정
		 */
		setApiDiffResult(api: ApiDiffResponse) {
			update((s) => {
				console.log('[folderDiffStore] api =', api);

				// 실패 시 초기 상태 유지 형태로 반환
				if (!api.success || !api.data) {
					return {
						...s,
						diffs: [],
						statistics: {
							added: 0,
							modified: 0,
							removed: 0,
							totalChanges: 0
						}
					};
				}

				const { changes, statistics } = api.data;

				return {
					...s,

					// diff 결과는 API 원본 그대로 저장
					diffs: changes ?? [],

					statistics: {
						added: statistics.added,
						modified: statistics.modified,
						removed: statistics.deleted,
						totalChanges: statistics.totalChanges
					}
				};
			});
		},

		setFilter(filter: DiffFilter) {
			update((s) => ({ ...s, filter }));
		},

		reset() {
			set(initialState);
		}
	};
}

export const folderDiffStore = createFolderDiffStore();
