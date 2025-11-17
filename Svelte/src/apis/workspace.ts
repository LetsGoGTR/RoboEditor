export const _listWorkspaces = async () => {
	const res = await fetch(`/api/v1/workspace`);
	return res.json();
};

export const _createWorkspace = async (payload: any) => {
	const res = await fetch(`/api/v1/workspace`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _getWorkspace = async (id: string) => {
	const res = await fetch(`/api/v1/workspace/${id}`);
	return res.json();
};

export const _updateWorkspace = async (id: string, payload: any) => {
	const res = await fetch(`/api/v1/workspace/${id}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _deleteWorkspace = async (id: string) => {
	const res = await fetch(`/api/v1/workspace/${id}`, {
		method: 'DELETE'
	});
	return res.json();
};
