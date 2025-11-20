import { writable } from 'svelte/store';
import type { DiffFilter, FileDiffItem, FileDiffResult, ApiDiffResponse } from '@/types';

export interface FileDiffState {
	isOpen: boolean;

	leftFileName: string;
	rightFileName: string;

	leftFilePath: string;
	rightFilePath: string;

	leftContent: string;
	rightContent: string;

	diffs: FileDiffItem[];
	statistics: FileDiffResult['statistics'] | null;

	filter: DiffFilter;
}
const initialState: FileDiffState = {
	isOpen: false,

	leftFileName: '',
	rightFileName: '',

	leftFilePath: '',
	rightFilePath: '',

	leftContent: '',
	rightContent: '',

	diffs: [],
	statistics: null,

	filter: 'all'
};

function createFileDiffStore() {
	const { subscribe, update, set } = writable<FileDiffState>(initialState);

	return {
		subscribe,

		openFileDiff(leftName: string, rightName: string, leftPath: string, rightPath: string) {
			update((s) => ({
				...s,
				isOpen: true,

				leftFileName: leftName,
				rightFileName: rightName,

				leftFilePath: leftPath,
				rightFilePath: rightPath,

				leftContent: '',
				rightContent: '',

				diffs: [],
				statistics: null,
				filter: 'all'
			}));
		},

		applyFileDiffContent(left: string, right: string) {
			update((s) => ({
				...s,
				leftContent: left,
				rightContent: right
			}));
		},

		/** ⭐ 서버 raw response를 받아 UI-friendly 구조로 변환 ⭐ */
		/** API Raw Response를 그대로 저장하는 모드 */
		setApiDiffResult(api: ApiDiffResponse) {
			update((s) => {
				console.log('[fileDiffStore] api =', api);

				// 실패 시 초기화 형태로 반환
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

				// ⭐ API 응답 구조를 그대로 반영 ⭐
				const { file1, file2, changes, statistics } = api.data;

				return {
					...s,

					// 파일 이름은 API 기준으로 갱신
					leftFileName: file1,
					rightFileName: file2,

					// diff 결과도 API 원본 그대로 저장
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

export const fileDiffStore = createFileDiffStore();
