import { writable } from 'svelte/store';
import type { Controller, FolderNode } from '@/types';
import { openDirectory } from '@/utils/FSA';

export const controllers = writable<Controller[]>([]);

export const controllerStore = {
	addController(controller: Controller) {
		controllers.update((prev) => [...prev, controller]);
	},
	removeController(serialNumber: string) {
		controllers.update((prev) => prev.filter((c) => c.serialNumber !== serialNumber));
	},
	clearAll() {
		controllers.set([]);
	}
};

export async function loadControllersFromBackup() {
	const backup = await openDirectory();
	if (!backup || backup.type !== 'folder' || !backup.children) return;

	const list: Controller[] = backup.children
		.filter((n) => n.type === 'folder')
		.map((c) => ({
			...c,
			serialNumber: crypto.randomUUID(),
			ipAddress: 'local',
			state: 'idle',
			workspaces: (c.children ?? []).filter((n): n is FolderNode => n.type === 'folder')
		}));

	controllers.set(list);
}
