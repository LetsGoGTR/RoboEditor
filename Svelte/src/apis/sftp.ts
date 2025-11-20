<<<<<<< HEAD
export const _ftApply = async (formData: FormData) => {
	const res = await fetch(`/proxy/api/v1/ft/apply`, {
=======
const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const FT_API_BASE = `${API_BASE}/api/v1/ft`;

export const _ftApply = async (formData: FormData) => {
	const res = await fetch(`${FT_API_BASE}/apply`, {
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
		method: 'POST',
		body: formData
	});
	return res.json();
};

export const _ftBackup = async (payload: any) => {
<<<<<<< HEAD
	const res = await fetch(`/proxy/api/v1/ft/backup`, {
=======
	const res = await fetch(`${FT_API_BASE}/backup`, {
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};
<<<<<<< HEAD
=======

export const _ftChangePassword = async (payload: any) => {
	const res = await fetch(`${FT_API_BASE}/password`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};
>>>>>>> 11ee473db00a5d3b3757571b29c9d95ac4ab1e34
