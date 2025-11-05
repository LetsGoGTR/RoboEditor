// Components Items
// Tabs
export type FixedTabId = 'left' | 'right';

export interface FixedTab {
	id: FixedTabId;
	label: string;
	disabled?: boolean;
}

// Business Items
// File System Types
type NodeType = 'folder' | 'file';

interface BaseNode {
	id: string;
	name: string;
	type: NodeType;
	parentId?: string | null; // root of the file/folder
	path?: string; // "src/routes/main.svelte"
}

export interface FolderNode extends BaseNode {
	type: 'folder';
	children: TreeNode[];
}

export interface FileNode extends BaseNode {
	type: 'file';
	contentRef?: string;
	size?: number;
	lastModified?: string; // ISO timestamp
}

export type TreeNode = FolderNode | FileNode;

// Controller Types
export interface Controller {
  serialNumber: string;
  state: 'idle' | 'active' | 'error' | 'disconnected';
  ipAddress: string;
}
