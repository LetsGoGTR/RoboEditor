// src/utils/workspaceApiTransport.ts
import type { FolderNode, Workspace, WorkspaceMeta, ControllerMeta } from '@/types';

/**
 * Drogon의 getDirectoryTree 응답을 FolderNode로 변환
 */
export function toFolderNodeFromDrogon(raw: any): FolderNode {
  const node: FolderNode = {
    id: crypto.randomUUID(),
    name: raw.name,
    type: 'directory',
    path: raw.path ?? null,
    children: []
  };

  if (Array.isArray(raw.files)) {
    for (const f of raw.files) {
      node.children.push({
        id: crypto.randomUUID(),
        name: f.name,
        type: 'file',
        path: f.path ?? null,
        size: f.size ?? 0,
        lastModified: String(f.lastModified ?? '')
      });
    }
  }

  if (Array.isArray(raw.directories)) {
    for (const d of raw.directories) {
      node.children.push(toFolderNodeFromDrogon(d));
    }
  }

  return node;
}

/** API 응답 metadata(raw) → ApiWorkspaceMetadata */
export interface ApiWorkspaceMetadata {
  uuid: string;
  name: string;
  target?: string | null;
  description?: string | null;
  createdAt: string;
  updatedAt: string;
}

export function mapApiWorkspaceMetadata(raw: any): ApiWorkspaceMetadata {
  return {
    uuid: raw.uuid,
    name: raw.name,
    target: raw.target ?? null,
    description: raw.description ?? null,
    createdAt: raw.createdAt,
    updatedAt: raw.updatedAt
  };
}

/** ApiWorkspaceMetadata → WorkspaceMeta (name 제거) */
export function toWorkspaceMeta(apiMeta: ApiWorkspaceMetadata): WorkspaceMeta {
  return {
    uuid: apiMeta.uuid,
    target: apiMeta.target,
    description: apiMeta.description,
    createdAt: apiMeta.createdAt,
    updatedAt: apiMeta.updatedAt
  };
}

/** 컨트롤러 아래 엔트리용 Workspace 노드 */
export function buildWorkspaceEntry(apiMeta: ApiWorkspaceMetadata): Workspace {
  const meta = toWorkspaceMeta(apiMeta);

  return {
    id: `workspace:${meta.uuid}`,
    name: apiMeta.name,
    type: 'directory',
    path: null,
    children: [],
    workspaceMeta: meta
  };
}

/** GET /workspace/{id} 응답 전체 → Workspace(트리 포함) */
export function buildWorkspaceFromApi(res: any): Workspace {
  const apiMeta = mapApiWorkspaceMetadata(res.data.metadata);
  const meta = toWorkspaceMeta(apiMeta);

  const rawTree = res.data.tree;
  const rootFolder = toFolderNodeFromDrogon(rawTree);

  return {
    ...rootFolder,
    name: apiMeta.name,
    workspaceMeta: meta
  };
}

/** 컨트롤러 루트 FolderNode 생성 */
export function buildControllerRoot(
  meta: ControllerMeta,
  workspaces: Workspace[]
): FolderNode {
  return {
    id: `device:${meta.serialNumber}`,
    name: meta.name ?? meta.serialNumber,
    type: 'directory',
    path: meta.serialNumber,
    children: workspaces
  };
}