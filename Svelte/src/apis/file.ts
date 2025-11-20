<<<<<<< HEAD
const FILE_API_PATH = '/proxy/api/v1/file';

export const _getFile = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`);
=======
const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const FILE_API_PATH = `${API_BASE}/api/v1/file`;

export const _getFile = async (path: string) => {
	const res = await fetch(FILE_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
	});
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	return res.json();
};

export const _createFile = async (path: string, content: string) => {
<<<<<<< HEAD
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ content })
=======
	const res = await fetch(FILE_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path, content })
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	});
	return res.json();
};

export const _updateFile = async (
	path: string,
<<<<<<< HEAD
	payload: { content?: string; newPath?: string }
) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
=======
	payload: { content: string; newPath?: string }
) => {
	const res = await fetch(FILE_API_PATH, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path, ...payload })
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	});
	return res.json();
};

export const _deleteFile = async (path: string) => {
<<<<<<< HEAD
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`, {
		method: 'DELETE'
=======
	const res = await fetch(FILE_API_PATH, {
		method: 'DELETE',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ path })
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
	});
	return res.json();
};
