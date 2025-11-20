import * as monaco from "monaco-editor/esm/vs/editor/editor.api"; // ✅ 타입 전용 import

let monacoInstance: typeof monaco | null = null; // ✅ 정확한 타입 지정

export async function getMonaco(): Promise<typeof monaco> {
	if (!monacoInstance) {
		console.log('🧩 Loading Monaco Editor...');
		// import() 결과가 typeof monaco와 동일한 구조
		monacoInstance = await import('monaco-editor');
		console.log('✅ Monaco loaded once');
	}
	return monacoInstance;
}

/** API → Monaco friendly path */
export function normalizeApiPath(apiPath: string): string {
	if (!apiPath) throw new Error('Empty API path');

	// ?path= 처리
	const idx = apiPath.indexOf('?path=');
	if (idx !== -1) {
		apiPath = apiPath.slice(idx + 6);
	}

	// Windows → POSIX
	apiPath = apiPath.replace(/\\/g, '/');

	// drogon base route 제거 (tmp, storage 등)
	apiPath = apiPath.replace(/^([A-Za-z]:)?\/?tmp\/drogon-app\/storage\//, '');

	// 중복 슬래시 제거 → "/hello"
	apiPath = apiPath.replace(/^\/+/, '/');

	// 상대경로 보정
	if (!apiPath.startsWith('/')) apiPath = '/' + apiPath;

	return apiPath;
}

/** 안전한 URI 생성 */
export function toMonacoUri(apiPath: string): monaco.Uri {
	return monaco.Uri.file(normalizeApiPath(apiPath));
}

/** 모델 중복 방지 + 기존 모델 재사용 */
export function getOrCreateModel(uri: monaco.Uri, content: string): monaco.editor.ITextModel {
	const existing = monaco.editor.getModel(uri);
	if (existing) {
		existing.setValue(content ?? '');
		return existing;
	}
	return monaco.editor.createModel(content ?? '', undefined, uri);
}
