const FILE_API_PATH = '/proxy/api/v1/file';

export const _getFile = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`);
	return res.json();
};

export const _createFile = async (path: string, content: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ content })
	});
	return res.json();
};

export const _updateFile = async (
	path: string,
	payload: { content?: string; newPath?: string }
) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _deleteFile = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FILE_API_PATH}?${params}`, {
		method: 'DELETE'
	});
	return res.json();
};
