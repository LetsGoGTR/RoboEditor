const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const FT_API_BASE = `${API_BASE}/api/v1/ft`;

export const _ftApply = async (formData: FormData) => {
	const res = await fetch(`${FT_API_BASE}/apply`, {
		method: 'POST',
		body: formData
	});
	return res.json();
};

export const _ftBackup = async (payload: any) => {
	const res = await fetch(`${FT_API_BASE}/backup`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _ftChangePassword = async (payload: any) => {
	const res = await fetch(`${FT_API_BASE}/password`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};