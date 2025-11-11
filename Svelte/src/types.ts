// Components Items
// Fixed Tabs
export type FixedTabId = 'left' | 'right';

export interface FixedTab {
	id: FixedTabId;
	label: string;
	disabled?: boolean;
}

// addable Tabs
export interface TabItem {
	id: string;
	name: string;
	path?: string | null;
}

// Business Items
// File System Types for FSA API + Fallback
export type NodeType = 'folder' | 'file';

interface BaseNode {
	id: string;
	name: string;
	type: NodeType;
	path: string | null; // "src/routes/main.svelte", 새 파일인 경우 null
	parentId?: string | null; // 상위 폴더 참조
	checked?: boolean; // 선택용 (UI 상태)
}

export interface FolderNode extends BaseNode {
	type: 'folder';
	children: TreeNode[];
	handle?: FileSystemDirectoryHandle; // ✅ FSA 전용 핸들 (디렉토리)
	kind?: 'directory'; // FSA 호환 속성 (선택)
}

export interface FileNode extends BaseNode {
	type: 'file';
	size?: number;
	lastModified?: number; // epoch ms (File.lastModified)
	contentRef?: string; // IndexedDB나 Blob URL 참조 시 사용
	handle?: FileSystemFileHandle; // ✅ FSA 전용 핸들 (파일)
	file?: File; // fallback 모드에서 직접 참조
	kind?: 'file'; // FSA 호환 속성 (선택)
}

export type TreeNode = FolderNode | FileNode;

// Controller Types
export interface Controller extends FolderNode {
	serialNumber: string;
	state: 'idle' | 'active' | 'error' | 'disconnected';
	ipAddress: string;
	sftpPort: number;
	// apiPort: number;
	workspaces: FolderNode[]; // 1-depth 폴더 목록
}

export interface BackupRoot extends FolderNode {
	controllers: Controller[];
}

// Differences Checks Types
export type DiffState = 'ADDED' | 'REMOVED' | 'CHANGED';
export type DiffFilter = 'ALL' | DiffState;

export interface DiffItem {
	line: number;
	path: string;
	leftValue: string;
	rightValue: string;
	state: DiffState;
}
