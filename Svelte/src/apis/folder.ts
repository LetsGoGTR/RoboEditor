<<<<<<< HEAD
const FOLDER_API_PATH = '/proxy/api/v1/folder';
=======
const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const FOLDER_API_PATH = `${API_BASE}/api/v1/folder`;
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34

export const _getFolder = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`);
	return res.json();
};

export const _createFolder = async (path: string) => {
<<<<<<< HEAD
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`, {
		method: 'POST'
=======
	const res = await fetch(FOLDER_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	});
	return res.json();
};

export const _updateFolder = async (path: string, newPath: string) => {
<<<<<<< HEAD
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ newPath })
=======
	const res = await fetch(FOLDER_API_PATH, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path, newPath })
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	});
	return res.json();
};

export const _deleteFolder = async (path: string) => {
<<<<<<< HEAD
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`, {
		method: 'DELETE'
=======
	const res = await fetch(FOLDER_API_PATH, {
		method: 'DELETE',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	});
	return res.json();
};
