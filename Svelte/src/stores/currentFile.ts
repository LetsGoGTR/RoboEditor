import { writable } from 'svelte/store';
import type { FileNode } from '@/types';

/* ============================================================
 * 타입 정의
 * ============================================================ */

/** 단일 파일 편집 View */
export interface FileView {
	file: FileNode;
	content: string | null;
}

/** 비교 모드(Diff) 상태 */
export interface DiffView {
	left: FileView;
	right: FileView;
}

/** 전체 Editor 상태 */
export interface CurrentFileState {
	// Normal Editor Mode
	group: FileView[]; // 열린 탭들
	active: FileView | null; // 현재 활성 탭
	activeIndex: number | null; // 활성 탭 index

	// Diff Mode
	diff: DiffView | null;

	// Flags
	isDiffMode: boolean;
}

/* ============================================================
 * 초기 상태
 * ============================================================ */
const initialState: CurrentFileState = {
	group: [],
	active: null,
	activeIndex: null,
	diff: null,
	isDiffMode: false
};

/* ============================================================
 * Store
 * ============================================================ */

function createCurrentFileStore() {
	const { subscribe, set, update } = writable<CurrentFileState>(initialState);

	return {
		subscribe,
		update,

		/* --------------------------------------------------------
		 * 1) 단일 파일 열기
		 * ------------------------------------------------------ */
		open(file: FileNode) {
			update((s) => {
				// Diff 모드 → 종료
				if (s.isDiffMode) s = { ...initialState };

				// 이미 열린 탭인지 확인
				const existingIndex = s.group.findIndex((g) => g.file.id === file.id);

				// 이미 열린 탭이면 해당 탭 활성화
				if (existingIndex !== -1) {
					return {
						...s,
						activeIndex: existingIndex,
						active: s.group[existingIndex],
						isDiffMode: false,
						diff: null
					};
				}

				// 새 탭 생성
				const newView: FileView = { file, content: null };
				const newGroup = [...s.group, newView];

				return {
					...s,
					group: newGroup,
					active: newView,
					activeIndex: newGroup.length - 1,
					isDiffMode: false,
					diff: null
				};
			});
		},

		/* --------------------------------------------------------
		 * 2) 파일 내용 갱신
		 * ------------------------------------------------------ */
		setContent(content: string) {
			update((s) => {
				if (s.isDiffMode || s.activeIndex === null || s.active === null) return s;

				const updatedGroup = s.group.map((view, idx) =>
					idx === s.activeIndex ? { ...view, content } : view
				);

				return {
					...s,
					group: updatedGroup,
					active: updatedGroup[s.activeIndex]
				};
			});
		},

		/* --------------------------------------------------------
		 * 3) 탭 전환
		 * ------------------------------------------------------ */
		switchTab(index: number) {
			update((s) => {
				if (s.isDiffMode) return s; // diff 상태에서는 탭 전환 불가
				if (index < 0 || index >= s.group.length) return s;

				return {
					...s,
					activeIndex: index,
					active: s.group[index]
				};
			});
		},

		/* --------------------------------------------------------
		 * 4) 수정 내용 임시(store) 저장 (실제 저장 X)
		 * ------------------------------------------------------ */
		setContentManual(content: string) {
			update((s) => {
				if (s.activeIndex === null || !s.active) return s;

				const updated = s.group.map((v, i) =>
					i === s.activeIndex ? { ...v, content, dirty: true } : v
				);

				return {
					...s,
					group: updated,
					active: updated[s.activeIndex]
				};
			});
		},

		/* --------------------------------------------------------
		 * 5) 탭 닫기
		 * ------------------------------------------------------ */
		closeTab(index: number) {
			update((s) => {
				if (s.isDiffMode) return s; // diff 상태에서는 탭 수정 불가
				if (index < 0 || index >= s.group.length) return s;

				const newGroup = s.group.filter((_, i) => i !== index);

				// activeIndex 계산
				const newActiveIndex = newGroup.length === 0 ? null : Math.min(index, newGroup.length - 1);

				return {
					...s,
					group: newGroup,
					activeIndex: newActiveIndex,
					active: newActiveIndex !== null ? newGroup[newActiveIndex] : null
				};
			});
		},

		/* --------------------------------------------------------
		 * 6) 파일 삭제에 의한 파일 닫기
		 * ------------------------------------------------------ */
		closeByFilePath(targetPath: string) {
			update((s) => {
				if (s.isDiffMode) return s;

				const idx = s.group.findIndex((g) => g.file.path === targetPath);
				if (idx === -1) return s;

				const newGroup = s.group.filter((_, i) => i !== idx);
				const newActiveIndex = newGroup.length === 0 ? null : Math.min(idx, newGroup.length - 1);

				return {
					...s,
					group: newGroup,
					activeIndex: newActiveIndex,
					active: newActiveIndex !== null ? newGroup[newActiveIndex] : null
				};
			});
		},

		/* --------------------------------------------------------
		 * 7) 폴더 삭제에 의한 파일 닫기
		 * ------------------------------------------------------ */
		closeByFolderPath(folderPath: string) {
			update((s) => {
				if (s.isDiffMode) return s;

				// folderPath로 시작하는 모든 파일 제거
				const newGroup = s.group.filter((view) => !view.file.path?.startsWith(folderPath));

				const newActiveIndex =
					newGroup.length === 0 ? null : Math.min(s.activeIndex ?? 0, newGroup.length - 1);

				return {
					...s,
					group: newGroup,
					activeIndex: newActiveIndex,
					active: newActiveIndex !== null ? newGroup[newActiveIndex] : null
				};
			});
		},

		/* --------------------------------------------------------
		 * 8) Diff 비교 모드 진입
		 * ------------------------------------------------------ */
		openDiff(left: FileNode, right: FileNode) {
			const leftView: FileView = { file: left, content: null };
			const rightView: FileView = { file: right, content: null };

			set({
				group: [],
				active: null,
				activeIndex: null,
				isDiffMode: true,
				diff: { left: leftView, right: rightView }
			});
		},

		/* --------------------------------------------------------
		 * 9) Diff 모드 종료 → normal 모드로 복귀
		 * ------------------------------------------------------ */
		exitDiff() {
			set(initialState);
		},

		/* --------------------------------------------------------
		 * 10) 전체 초기화
		 * ------------------------------------------------------ */
		reset() {
			set(initialState);
		}
	};
}

export const currentFile = createCurrentFileStore();
