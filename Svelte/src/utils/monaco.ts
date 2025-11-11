import type * as monaco from 'monaco-editor';  // ✅ 타입 전용 import

let monacoInstance: typeof monaco | null = null;  // ✅ 정확한 타입 지정

export async function getMonaco(): Promise<typeof monaco> {
  if (!monacoInstance) {
    console.log('🧩 Loading Monaco Editor...');
    // import() 결과가 typeof monaco와 동일한 구조
    monacoInstance = await import('monaco-editor');
    console.log('✅ Monaco loaded once');
  }
  return monacoInstance;
}
