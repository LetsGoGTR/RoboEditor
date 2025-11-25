// src/features/handlers/controller.ts
import type { Controller, ControllerMeta } from '@/types';

/** API에서 받은 raw 디바이스 → Controller 도메인 모델 */
export function wrapDeviceAsController(raw: any): Controller {
  const meta: ControllerMeta = {
    serialNumber: raw.serialNumber,
    name: raw.name,
    description: raw.description ?? null,
    host: raw.host,
    scheme: raw.scheme ?? null,
    apiPort: raw.apiPort,
    sftpPort: raw.sftpPort,
    sftpUser: raw.sftpUser,
    sftpPassword: raw.sftpPassword,
    createdAt: raw.createdAt,
    updatedAt: raw.updatedAt,
    state: raw.state ?? 'idle'
  };

  return {
    controllerMeta: meta,
    workspaces: []
  };
}
