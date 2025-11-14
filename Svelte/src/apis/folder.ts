const FOLDER_API_PATH = '/proxy/api/v1/folder';

export const _getFolder = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`);
	return res.json();
};

export const _createFolder = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`, {
		method: 'POST'
	});
	return res.json();
};

export const _updateFolder = async (path: string, newPath: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ newPath })
	});
	return res.json();
};

export const _deleteFolder = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`${FOLDER_API_PATH}?${params}`, {
		method: 'DELETE'
	});
	return res.json();
};
