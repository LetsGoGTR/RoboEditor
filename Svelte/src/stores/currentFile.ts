import { writable } from 'svelte/store';
import type { FileNode } from '@/types';

export interface CurrentFileState {
	file: FileNode | null;
	content: string | null;
}

function createCurrentFileStore() {
	const { subscribe, set, update } = writable<CurrentFileState>({
		file: null,
		content: null
	});

	return {
		subscribe,

		/** 파일 열기 */
		open(file: FileNode) {
			set({ file, content: null });
		},

		/** 파일 내용 갱신 */
		setContent(content: string) {
			update((state) => ({
				...state,
				content
			}));
		},

		/** 파일 닫기 */
		close() {
			set({ file: null, content: null });
		}
	};
}

export const currentFile = createCurrentFileStore();
