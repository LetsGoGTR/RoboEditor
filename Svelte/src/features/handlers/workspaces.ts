// handlers/workspace.ts
import type { Workspace } from '@/types';
import {
	_createWorkspace,
	_getWorkspace,
	_updateWorkspace,
	_deleteWorkspace
} from '@apis/workspace';

import { _getDevice } from '@apis/controller';
import {
	buildWorkspaceEntry,
	buildWorkspaceFromApi,
	type ApiWorkspaceMetadata
} from '@/utils/workspaceApiTransport';

async function fetchDeviceWorkspaces(deviceId: string): Promise<Workspace[]> {
	const res: any = await _getDevice(deviceId);

	if (!res?.success) {
		throw new Error('디바이스 정보 재조회 실패');
	}

	const wsList: ApiWorkspaceMetadata[] = res.workspaces?.workspaces ?? [];
	return wsList.map((meta) => buildWorkspaceEntry(meta));
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

		const workspaces = await fetchDeviceWorkspaces(deviceId);

		// 생성 후 상세 페이지 이동
		// gotoPage('workspace-detail', { edit });

		return {
			workspace: res.data,
			workspaces
		};
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

		const workspace: Workspace = buildWorkspaceFromApi(res);

		return workspace;
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

		const workspaces = await fetchDeviceWorkspaces(deviceId);
		return {
			workspace: res.data,
			workspaces
		};
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

		const workspaces = await fetchDeviceWorkspaces(deviceId);
		return {
			workspaces
		};
	} catch (err) {
		console.error('[workspace:delete] 오류:', err);
		alert('워크스페이스 삭제 중 문제가 발생했습니다.');
	}
}
