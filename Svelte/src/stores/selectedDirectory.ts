import { writable } from 'svelte/store';
import type { TreeNode } from '@/types';

export interface WorkspaceContext {
    deviceId: string;
    deviceName: string;
    workspaceUuid: string;
    workspaceName: string;
}

export const workspaceContext = writable<WorkspaceContext | null>(null);

export const selectedDirectory = writable<TreeNode | null>(null);
