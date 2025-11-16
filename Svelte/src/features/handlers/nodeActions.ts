import { currentFile } from '@/stores/currentFile';
import { gotoPage } from '@/stores/currentPage';
import { insertFileNode, removeFileNode, removeFolderNode } from '@/stores/fileTree';
import type { FileNode, FolderNode } from '@/types';
import { _createFile, _deleteFile, _getFile, _updateFile } from '@apis/file';
import { _deleteFolder } from '@apis/folder';
import type * as monaco from 'monaco-editor';
import { get } from 'svelte/store';

/** ==============================
 *   1. File
 *  ==============================  */

/** 새 파일 생성 */
export async function handleCreateFile(payload: { name: string; folder: FolderNode }) {
	const { name, folder } = payload;

	const newPath = `${folder.path}/${name}`;

	const resp = await _createFile(newPath, ''); // 초기 content는 빈 문자열
	if (!resp?.success) {
		return alert('파일 생성 중 오류가 발생했습니다.');
	}

	// 신규 파일 노드 생성
	const newFile: FileNode = {
		id: crypto.randomUUID(),
		name,
		type: 'file',
		path: newPath,
		size: 0,
		lastModified: Date.now()
	};

	// 3) fileTree 트리에 삽입
	if (folder.path) insertFileNode(folder.path, newFile);

	// Editor 탭 열기
	currentFile.open(newFile);
	gotoPage('edit');
}

/**
 * 파일 저장 핸들러 (Monaco editor.getValue() 전달)
 */
export async function handleSaveFile(editor: monaco.editor.IStandaloneCodeEditor | null) {
	if (!editor) return;

	const active = get(currentFile).active;
	if (!active) return;

	const file = active.file;
	const content = editor.getValue();
	const path = file.path;

	try {
		/* ----------------------------------------------------------
		 * 1) 신규 파일: path 없음 → create
		 * -------------------------------------------------------- */
		if (!path) {
			console.warn('[save] 신규 파일로 생성:', file.name);

			const resp = await _createFile(file.name, content);
			const createdPath = resp?.path ?? file.name;

			// store 반영
			currentFile.update((s) => {
				if (!s.active) return s;

				s.active.file.path = createdPath;

				const idx = s.group.findIndex((g) => g.file.id === s.active!.file.id);
				if (idx !== -1) s.group[idx].file.path = createdPath;

				return s;
			});

			currentFile.setContent(content);
			return;
		}

		/* ----------------------------------------------------------
		 * 2) 기존 파일: 실제 서버 존재 여부 확인
		 * -------------------------------------------------------- */
		let exists = true;
		const check = await _getFile(path).catch(() => (exists = false));

		if (!exists || !check?.data) {
			console.warn('[save] 파일이 서버에 없어 신규 생성으로 대체:', path);

			const resp = await _createFile(path, content);
			const createdPath = resp?.path ?? path;

			currentFile.update((s) => {
				if (!s.active) return s;
				s.active.file.path = createdPath;

				const idx = s.group.findIndex((g) => g.file.id === s.active!.file.id);
				if (idx !== -1) s.group[idx].file.path = createdPath;
				return s;
			});

			currentFile.setContent(content);
			return;
		}

		/* ----------------------------------------------------------
		 * 3) 기존 파일 존재함 → update
		 * -------------------------------------------------------- */
		await _updateFile(path, { content });

		currentFile.setContent(content);
	} catch (err) {
		console.error('[save] 파일 저장 중 오류:', err);
	}
}

/**
 * 파일 삭제 핸들러
 * @param file 삭제할 FileNode
 */
export async function handleDeleteFile(file: FileNode) {
	if (!file?.path) {
		alert('삭제할 파일의 경로가 올바르지 않습니다.');
		return;
	}

	const confirmDelete = confirm(`파일 '${file.name}' 을(를) 삭제하시겠습니까?`);
	if (!confirmDelete) return;

	try {
		// 1) 서버에 파일 삭제 요청
		const resp = await _deleteFile(file.path);

		if (!resp?.success) {
			alert('파일 삭제 중 오류가 발생했습니다.');
			return;
		}

		// 2) fileTree store에서 제거
		removeFileNode(file.path);

		// 3) 열린 탭 제거
		currentFile.closeByFilePath(file.path);
	} catch (err) {
		console.error('[delete] 파일 삭제 오류:', err);
		alert('파일 삭제 중 문제가 발생했습니다.');
	}
}

/** ==============================
 *   2. Folder
 *  ==============================  */

// 폴더 삭제 handler
export async function handleDeleteFolder(folder: FolderNode) {
	if (!folder?.path) {
		alert('삭제할 폴더 정보가 올바르지 않습니다.');
		return;
	}

	const confirmDelete = confirm(`폴더 '${folder.name}' 및 하위 내용들을 삭제하시겠습니까?`);
	if (!confirmDelete) return;

	try {
		// 1) 서버 삭제 요청 (재귀)
		const resp = await _deleteFolder(folder.path);
		if (!resp?.success) {
			alert('폴더 삭제 중 오류가 발생했습니다.');
			return;
		}

		// 2) 트리에서 제거
		removeFolderNode(folder.path);

		// 3) 폴더 내부 모든 탭 제거
		currentFile.closeByFolderPath(folder.path);
	} catch (err) {
		console.error('[delete-folder] 폴더 삭제 오류:', err);
		alert('폴더 삭제 중 문제가 발생했습니다.');
	}
}
