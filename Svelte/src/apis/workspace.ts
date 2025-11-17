const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const WORKSPACE_API_PATH = `${API_BASE}/api/v1/workspace`;

export const _listWorkspaces = async () => {
	const res = await fetch(WORKSPACE_API_PATH);
	return res.json();
};

export const _createWorkspace = async (payload: any) => {
	const res = await fetch(WORKSPACE_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _getWorkspace = async (id: string) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${id}`);
	return res.json();
};

export const _updateWorkspace = async (id: string, payload: any) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${id}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _deleteWorkspace = async (id: string) => {
	const res = await fetch(`${WORKSPACE_API_PATH}/${id}`, {
		method: 'DELETE'
	});
	return res.json();
};