// handlers/workspace.ts
import { gotoPage } from '@/stores/currentPage';
import { workspaceStore } from '@/stores/workspace';
import { fileTree } from '@/stores/fileTree';
import type { WorkspaceNode } from '@/types';

import {
	_listWorkspaces,
	_createWorkspace,
	_getWorkspace,
	_updateWorkspace,
	_deleteWorkspace
} from '@apis/workspace';

/* ============================================================
 *  Workspace: 전체 목록 가져오기
 * ============================================================ */
export async function handleListWorkspaces() {
	try {
		const res = await _listWorkspaces();
		if (!res?.success) return alert('워크스페이스 목록을 불러오지 못했습니다.');

		workspaceStore.setList(res.data);
		return res.data;
	} catch (err) {
		console.error('[workspace:list] 오류:', err);
		alert('워크스페이스 목록 조회 중 오류가 발생했습니다.');
	}
}

/* ============================================================
 *  Workspace 생성
 * ============================================================ */
export async function handleCreateWorkspace(deviceId: string, payload: Record<string, any>) {
	try {
		const res = await _createWorkspace(deviceId, payload);
		if (!res?.success) {
			return alert('워크스페이스 생성에 실패했습니다.');
		}

		const ws: WorkspaceNode = res.data;
		workspaceStore.add(ws);

		// 생성 후 상세 페이지 이동
		gotoPage('workspace-detail', { workspaceId: ws.id });

		return ws;
	} catch (err) {
		console.error('[workspace:create] 오류:', err);
		alert('워크스페이스 생성 중 문제가 발생했습니다.');
	}
}

/* ============================================================
 *  Workspace 상세 정보 불러오기
 * ============================================================ */
export async function handleGetWorkspace(deviceId: string, workspaceId: string) {
	try {
		const res = await _getWorkspace(deviceId, workspaceId);

		if (!res?.success) {
			alert('워크스페이스 정보를 불러올 수 없습니다.');
			return null;
		}

		workspaceStore.setCurrent(res.data);

		// workspace 내부 폴더 트리 초기화
		if (res.data?.rootPath) {
			fileTree.loadWorkspaceTree(res.data.rootPath);
		}

		return res.data;
	} catch (err) {
		console.error('[workspace:get] 오류:', err);
		alert('워크스페이스 상세조회 중 문제가 발생했습니다.');
	}
}

/* ============================================================
 *  Workspace 업데이트
 * ============================================================ */
export async function handleUpdateWorkspace(
	deviceId: string,
	workspaceId: string,
	payload: Record<string, any>
) {
	try {
		const res = await _updateWorkspace(deviceId, workspaceId, payload);

		if (!res?.success) {
			alert('워크스페이스 수정에 실패했습니다.');
		 return;
		}

		workspaceStore.update(res.data);
		return res.data;
	} catch (err) {
		console.error('[workspace:update] 오류:', err);
		alert('워크스페이스 수정 중 문제가 발생했습니다.');
	}
}

/* ============================================================
 *  Workspace 삭제
 * ============================================================ */
export async function handleDeleteWorkspace(deviceId: string, workspaceId: string) {
	const ok = confirm('해당 워크스페이스를 삭제하시겠습니까?');
	if (!ok) return;

	try {
		const res = await _deleteWorkspace(deviceId, workspaceId);

		if (!res?.success) {
			return alert('워크스페이스 삭제에 실패했습니다.');
		}

		workspaceStore.remove(workspaceId);

		// workspace 화면이면 목록으로 이동
		gotoPage('workspace-list');
	} catch (err) {
		console.error('[workspace:delete] 오류:', err);
		alert('워크스페이스 삭제 중 문제가 발생했습니다.');
	}
}
