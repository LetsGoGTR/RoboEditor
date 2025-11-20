import { writable } from 'svelte/store';
import type { TreeNode } from '@/types';

export const selectedDirectory = writable<TreeNode | null>(null);
