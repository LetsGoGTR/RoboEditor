import { writable } from 'svelte/store';
import type { DiffFilter, FileDiffItem, FileDiffResult } from '@/types';

/* ===========================
 * Store State
 * =========================== */
interface FileDiffState {
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
/* ===========================
 * 초기값
 * =========================== */
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

		setApiDiffResult(result: FileDiffResult) {
			update((s) => ({
				...s,
				diffs: result.diff,
				statistics: result.statistics
			}));
		},

		/** 필터 변경 */
		setFilter(filter: DiffFilter) {
			update((s) => ({ ...s, filter }));
		},

		reset() {
			set(initialState);
		}
	};
}

export const fileDiffStore = createFileDiffStore();
