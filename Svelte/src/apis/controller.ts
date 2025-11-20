const API_BASE = import.meta.env.DEV ? '/proxy' : '';
const DEVICE_API_PATH = `${API_BASE}/api/v1/repo`;

export const _listDevices = async () => {
	const res = await fetch(DEVICE_API_PATH);
	return res.json();
};

export const _createDevice = async (payload: any) => {
	const res = await fetch(DEVICE_API_PATH, {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _getDevice = async (deviceId: string) => {
	const res = await fetch(`${DEVICE_API_PATH}/${deviceId}`);
	return res.json();
};

export const _updateDevice = async (id: string, payload: any) => {
	const res = await fetch(`${DEVICE_API_PATH}/${id}`, {
		method: 'PUT',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify(payload)
	});
	return res.json();
};

export const _deleteDevice = async (id: string) => {
	const res = await fetch(`${DEVICE_API_PATH}/${id}`, {
		method: 'DELETE'
	});
	return res.json();
};