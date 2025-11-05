import { initialTree } from '@/data';
import type { TreeNode } from '@/types';
import { writable } from 'svelte/store';

export const fileTree = writable<TreeNode>(initialTree);
