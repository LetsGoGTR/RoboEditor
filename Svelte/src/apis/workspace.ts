<<<<<<< HEAD
export const _listWorkspaces = async () => {
	const res = await fetch(`/proxy/api/v1/workspace`);
	return res.json();
};

export const _createWorkspace = async (payload: any) => {
	const res = await fetch(`/proxy/api/v1/workspace`, {
=======
const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const WORKSPACE_API_PATH = `${API_BASE}/api/v1/repo`;

//전체 가져오는 용도?
export const _listWorkspaces = async () => {
	const res = await fetch(WORKSPACE_API_PATH);
	return res.json();
};

export const _createWorkspace = async (deviceId: string, payload: any) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${deviceId}`, {
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

<<<<<<< HEAD
export const _getWorkspace = async (id: string) => {
	const res = await fetch(`/proxy/api/v1/workspace/${id}`);
	return res.json();
};

export const _updateWorkspace = async (id: string, payload: any) => {
	const res = await fetch(`/proxy/api/v1/workspace/${id}`, {
=======
export const _getWorkspace = async (deviceId: string, workspaceId: string) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${deviceId}/${workspaceId}`);
	return res.json();
};

export const _updateWorkspace = async (deviceId: string, workspaceId: string, payload: any) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${deviceId}/${workspaceId}`, {
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

<<<<<<< HEAD
export const _deleteWorkspace = async (id: string) => {
	const res = await fetch(`/proxy/api/v1/workspace/${id}`, {
		method: 'DELETE'
	});
	return res.json();
};
=======
export const _deleteWorkspace = async (deviceId: string, workspaceId: string) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${deviceId}/${workspaceId}`, {
		method: 'DELETE'
	});
	return res.json();
};
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
