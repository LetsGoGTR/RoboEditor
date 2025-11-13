// src/routes/api/_sftp.ts
import SftpClient from 'ssh2-sftp-client';

export interface SftpOptions {
  host: string;
  port: number;
  username: string;
  password: string;
  privateKey?: string;
}

export async function uploadFileViaSftp(
  opts: SftpOptions,
  localPath: string,
  remotePath: string
) {
  const client = new SftpClient();
  try {
    await client.connect({
      host: opts.host,
      port: opts.port,
      username: opts.username,
      password: opts.password,
      privateKey: opts.privateKey,
    });

    await client.fastPut(localPath, remotePath);
  } finally {
    client.end();
  }
}