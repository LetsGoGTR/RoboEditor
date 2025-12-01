// src/features/handlers/controller.ts
import type { Controller, ControllerMeta } from '@/types';
import { _listDevices } from '@/apis/controller';

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

/** 디바이스 목록 조회 공용 함수 */
export async function fetchControllers(): Promise<Controller[]> {
  const res: any = await _listDevices();

  if (!res?.success) {
    throw new Error('디바이스 목록 조회 실패');
  }

  return (res.data?.devices ?? []).map((raw: any) => wrapDeviceAsController(raw));
}
