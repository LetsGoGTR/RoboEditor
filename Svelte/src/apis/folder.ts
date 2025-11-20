const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const FOLDER_API_PATH = `${API_BASE}/api/v1/folder`;

export const _getFolder = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`);
	return res.json();
};

export const _createFolder = async (path: string) => {
	const res = await fetch(FOLDER_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
	});
	return res.json();
};

export const _updateFolder = async (path: string, newPath: string) => {
	const res = await fetch(FOLDER_API_PATH, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path, newPath })
	});
	return res.json();
};

export const _deleteFolder = async (path: string) => {
	const res = await fetch(FOLDER_API_PATH, {
		method: 'DELETE',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
	});
	return res.json();
};
