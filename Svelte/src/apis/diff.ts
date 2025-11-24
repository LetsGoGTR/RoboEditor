const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const DIFF_API_PATH = `${API_BASE}/api/v1/diff`;

export const _diffFiles = async (filePathA: string, filePathB: string) => {
	const res = await fetch(`${DIFF_API_PATH}/files`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ filePathA, filePathB })
	});
	return res.json();
};

export const _diffWorkspaces = async (dirPathA: string, dirPathB: string) => {
	console.log({ dirPathA, dirPathB });
	const res = await fetch(`${DIFF_API_PATH}/workspaces`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ dirPathA, dirPathB })
	});
	return res.json();
};
