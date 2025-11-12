// src/routes/api/_validate.ts
export interface TargetDto {
  host: string;
  port: number;
  username: string;        // SFTP username
  password: string;  // SFTP password
}

export interface ApplyRequestDto {
  targets: TargetDto[];
  localPath: string;
  remotePath: string;
}

export function validateApply(body: any): ApplyRequestDto {
  if (!body || typeof body !== 'object') {
    throw new Error('요청 본문이 잘못되었습니다.');
  }

  const { targets, localPath, remotePath } = body;

  if (!Array.isArray(targets) || targets.length === 0) {
    throw new Error('제어기를 하나 이상 선택하세요.');
  }

  if (!localPath || typeof localPath !== 'string') {
    throw new Error('localPath가 비어 있습니다.');
  }

  if (!remotePath || typeof remotePath !== 'string') {
    throw new Error('remotePath가 비어 있습니다.');
  }

  const normalizedTargets: TargetDto[] = targets.map((t: any, idx: number) => {
    if (!t || typeof t !== 'object') {
      throw new Error(`targets[${idx}]가 잘못되었습니다.`);
    }
    const { host, port, username, password } = t;

    if (!host || typeof host !== 'string') {
      throw new Error(`targets[${idx}].host가 비어 있거나 잘못되었습니다.`);
    }

    const portNum = Number(port);
    if (!Number.isFinite(portNum) || portNum <= 0) {
      throw new Error(`targets[${idx}].port가 비어 있거나 잘못되었습니다.`);
    }

    if (!username || typeof username !== 'string') {
      throw new Error(`targets[${idx}].username이 비어 있거나 잘못되었습니다.`);
    }

    if (!password || typeof password !== 'string') {
      throw new Error(`targets[${idx}].password가 비어 있거나 잘못되었습니다.`);
    }

    return { host, port: portNum, username, password };
  });
  
  return {
    targets: normalizedTargets,
    localPath,
    remotePath
  };
}
