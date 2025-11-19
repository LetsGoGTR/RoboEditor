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

/* ============================================================
 * Business Items
 * ============================================================ */

/* ============================================================
 * 1. 기본 타입 정의
 * ============================================================ */

export type NodeType = 'directory' | 'file';

/** 모든 노드가 공통으로 가지는 속성 */
export interface BaseNode {
	id: string;
	name: string;
	type: NodeType;
	path: string | null; // 절대 경로
	parentId?: string | null; // 상위 폴더 id
	checked?: boolean; // UI 선택 용도
}

/* ============================================================
 * 2. 파일(Files)
 * ============================================================ */

export interface FileNode extends BaseNode {
	type: 'file';
	size?: number; // byte
	lastModified?: string; // epoch ms
}

export interface FileDetail extends FileNode {
	content: string;
}

/* ============================================================
 * 3. 폴더(Folders)
 * ============================================================ */

export interface FolderNode extends BaseNode {
	type: 'directory';
	children: TreeNode[]; // 재귀 구조
}

export type TreeNode = FolderNode | FileNode;

/* ============================================================
 * 4. 워크스페이스(Workspace) — Drogon Workspace API 기반
 * ============================================================ */

export interface WorkspaceMeta {
	id: string;
	name: string;
	target?: string | null;
	description?: string | null;
	createdAt: string;
	updatedAt: string;
}

export interface Workspace extends FolderNode {
	workspaceMeta: WorkspaceMeta;
}

/* ============================================================
 * 5. 컨트롤러(Controller) — Device API + 트리 구조 통합
 * ============================================================ */

export type ControllerState = 'idle' | 'active' | 'error' | 'disconnected';

export interface ControllerMeta {
	
	serialNumber: string;
	name: string;
	description?: string | null;

	api: string;
	sftpHost: string;
	sftpPort: number;
	sftpUser?: string;
	sftpPassword?: string;

	createdAt?: string;
  	updatedAt?: string;
	state: ControllerState;
}

/**
 * Controller는 폴더처럼 트리 구조를 가지며(FolderNode),
 * 그 안에 controllerMeta와 workspaces가 추가됩니다.
 */
export interface Controller {
	controllerMeta: ControllerMeta;
	workspaces: Workspace[];
}

/* ============================================================
 * 6. 백업 루트(BackupRoot)
 * ============================================================ */

export interface BackupRoot {
	controllers: Controller[];
}

/* ============================================================
 * 7. Diff 타입 — Monaco Diff 및 Drogon Diff API 기반
 * ============================================================ */
export type DiffState = 'added' | 'removed' | 'modified';
export type DiffFilter = 'all' | DiffState;

export interface FileDiffItem {
	oldLineNumber: number;
	oldLineCount: number;
	oldValue: string | number | null;

	newLineNumber: number;
	newLineCount: number;
	newValue: string | number | null;

	path: string;
	type: DiffState;
}

export interface FileDiffResult {
	success: boolean;
	diff: FileDiffItem[];

	file1: string;
	file2: string;

	statistics: {
		added: number;
		modified: number;
		removed: number;
		totalChanges: number;
	};
}

export interface ApiDiffResponse {
	success: boolean;

	data?: {
		file1: string;
		file2: string;

		changes: FileDiffItem[];

		statistics: {
			added: number;
			deleted: number;
			modified: number;
			totalChanges: number;
		};
	};

	message?: string; // 실패 시 존재
}
