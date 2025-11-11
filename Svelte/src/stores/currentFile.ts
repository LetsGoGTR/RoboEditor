import { writable } from 'svelte/store';
import type { FileNode } from '@/types';

/** 단일 파일 편집 상태 */
export interface FileView {
	file: FileNode | null;
	content: string | null;
}

/** Compare 모드 지원을 포함한 전체 상태 */
export interface CurrentFileState {
	active: FileView;              // 현재 활성 파일 (기본 editor)
	right?: FileView | null;       // 비교용 파일 (Diff editor의 우측)
	isDiffMode: boolean;           // Diff Editor 활성 여부
	group: FileView[];             // 열린 탭 그룹 (EditorPage)
	activeIndex: number | null;    // 현재 활성 탭 index
}

function createCurrentFileStore() {
	const { subscribe, set, update } = writable<CurrentFileState>({
		active: { file: null, content: null },
		right: null,
		isDiffMode: false,
		group: [],
		activeIndex: null
	});

	return {
		subscribe,

		/** ✅ 단일 파일 열기 (EditorPage) */
		open(file: FileNode) {
			update((s) => {
				const existingIndex = s.group.findIndex((g) => g.file?.id === file.id);
				if (existingIndex !== -1) {
					return {
						...s,
						activeIndex: existingIndex,
						active: s.group[existingIndex],
						isDiffMode: false
					};
				}

				const newView: FileView = { file, content: null };
				return {
					...s,
					group: [...s.group, newView],
					active: newView,
					activeIndex: s.group.length,
					isDiffMode: false
				};
			});
		},

		/** ✅ 파일 내용 갱신 */
		setContent(content: string) {
			update((s) => {
				if (s.activeIndex === null) return s;
				const updated = [...s.group];
				const current = updated[s.activeIndex];
				if (current) current.content = content;
				return { ...s, group: updated, active: current };
			});
		},

		/** ✅ 탭 전환 */
		switchTab(index: number) {
			update((s) => {
				if (index < 0 || index >= s.group.length) return s;
				return { ...s, activeIndex: index, active: s.group[index], isDiffMode: false };
			});
		},

		/** ✅ 탭 닫기 */
		closeTab(index: number) {
			update((s) => {
				if (index < 0 || index >= s.group.length) return s;
				const newGroup = s.group.filter((_, i) => i !== index);
				const newActiveIndex = Math.min(index, newGroup.length - 1);
				return {
					...s,
					group: newGroup,
					activeIndex: newGroup.length ? newActiveIndex : null,
					active: newGroup[newActiveIndex] ?? { file: null, content: null }
				};
			});
		},

		/** ✅ Diff 비교 모드 진입 */
		openDiff(left: FileNode, right: FileNode) {
			set({
				active: { file: left, content: null },
				right: { file: right, content: null },
				isDiffMode: true,
				group: [],
				activeIndex: null
			});
		},

		/** ✅ Compare 모드 종료 */
		exitDiff() {
			update((s) => ({
				...s,
				right: null,
				isDiffMode: false
			}));
		},

		/** ✅ 전체 닫기 */
		reset() {
			set({
				active: { file: null, content: null },
				right: null,
				isDiffMode: false,
				group: [],
				activeIndex: null
			});
		}
	};
}

export const currentFile = createCurrentFileStore();
