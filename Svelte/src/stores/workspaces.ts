import { writable } from 'svelte/store';
import type { FileNode } from '@/types';

/** 페이지 종류 타입 */
export type WorkspacePage = 'empty' | 'edit' | 'compare' | 'apply' | 'backup' | 'register';

/** 현재 활성화된 페이지 */
export const currentPage = writable<WorkspacePage>('empty');

/** 현재 선택된 파일 정보 */
export const currentFile = writable<FileNode | null>(null);

/** 페이지 이동 함수 */
export function gotoPage(page: WorkspacePage) {
	currentPage.set(page);
}

/** 파일 선택 + 페이지 이동 (예: Editor 열기용) */
export function openFile(file: FileNode) {
	currentFile.set(file);
	currentPage.set('edit');
}

/** 초기화 */
export function resetWorkspace() {
	currentFile.set(null);
	currentPage.set('empty');
}
