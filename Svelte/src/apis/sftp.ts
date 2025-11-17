export const _ftApply = async (formData: FormData) => {
	const res = await fetch(`/api/v1/ft/apply`, {
		method: 'POST',
		body: formData
	});
	return res.json();
};

export const _ftBackup = async (payload: any) => {
	const res = await fetch(`/api/v1/ft/backup`, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};
