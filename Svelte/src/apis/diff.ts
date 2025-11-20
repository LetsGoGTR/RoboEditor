<<<<<<< HEAD
const DIFF_API_PATH = 'proxy/api/v1/diff';
=======
const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const DIFF_API_PATH = `${API_BASE}/api/v1/diff`;
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34

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
