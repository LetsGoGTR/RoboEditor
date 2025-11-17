const DIFF_API_PATH = '/api/v1/diff';

export const _diffFiles = async (filePathA: string, filePathB: string) => {
	const res = await fetch(`${DIFF_API_PATH}/files`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ filePathA, filePathB })
	});
	return res.json();
};

export const _diffWorkspaces = async (dirPathA: string, dirPathB: string) => {
	const res = await fetch(`${DIFF_API_PATH}/workspaces`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ dirPathA, dirPathB })
	});
	return res.json();
};
