export const _getFile = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`/proxy/api/v1/file?${params}`);
	return res.json();
};

export const _createFile = async (path: string, content: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`/proxy/api/v1/file?${params}`, {
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
	const res = await fetch(`/proxy/api/v1/file?${params}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _deleteFile = async (path: string) => {
	const params = new URLSearchParams({ path });
	const res = await fetch(`/proxy/api/v1/file?${params}`, {
		method: 'DELETE'
	});
	return res.json();
};
