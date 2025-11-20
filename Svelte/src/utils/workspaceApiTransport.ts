// src/utils/workspaceApiTransport.ts
import type { FolderNode, Workspace, WorkspaceMeta } from '@/types';

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

/**
 * GET /api/v1/workspace/{id} 응답을 Workspace로 매핑하는 헬퍼 (선택)
 */
export function toWorkspaceFromApi(res: any): Workspace {
  const meta = res.data.metadata as WorkspaceMeta;
  const rawTree = res.data.tree;
  const rootFolder = toFolderNodeFromDrogon(rawTree);

  const workspace: Workspace = {
    ...rootFolder,
    workspaceMeta: meta
  };

  return workspace;
}
