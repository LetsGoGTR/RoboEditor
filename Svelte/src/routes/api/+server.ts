// src/routes/api/+server.ts
import type { RequestHandler } from './$types';
import { json } from '@sveltejs/kit';
import { validateApply } from './_validate';
import { uploadFileViaSftp } from './_sftp';

export const POST: RequestHandler = async ({ request }) => {
  try {
    const body = await request.json();
    const dto = validateApply(body);

    await Promise.all(
      dto.targets.map((t) =>
        uploadFileViaSftp(
          {
            host: t.host,
            port: t.port,
            username: t.username,
            password: t.password,
          },
          dto.localPath,
          dto.remotePath
        )
      )
    );

    return json({ ok: true, message: '적용 성공' }, { status: 200 });
  } catch (err: any) {
    console.error('API /api 에러:', err);
    return json(
      {
        ok: false,
        message: err?.message ?? '서버 내부 오류가 발생했습니다.'
      },
      { status: 500 }
    );
  }
};
