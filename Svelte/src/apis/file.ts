const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const FILE_API_PATH = `${API_BASE}/api/v1/file`;

export const _getFile = async (path: string) => {
	const res = await fetch(FILE_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
	});
	return res.json();
};

export const _createFile = async (path: string, content: string) => {
	const res = await fetch(FILE_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path, content })
	});
	return res.json();
};

export const _updateFile = async (
	path: string,
	payload: { content: string; newPath?: string }
) => {
	const res = await fetch(FILE_API_PATH, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path, ...payload })
	});
	return res.json();
};

export const _deleteFile = async (path: string) => {
	const res = await fetch(FILE_API_PATH, {
		method: 'DELETE',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
	});
	return res.json();
};
