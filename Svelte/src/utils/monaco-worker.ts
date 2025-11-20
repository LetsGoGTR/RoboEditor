import EditorWorker from 'monaco-editor/esm/vs/editor/editor.worker?worker';
import JsonWorker from 'monaco-editor/esm/vs/language/json/json.worker?worker';
import CssWorker from 'monaco-editor/esm/vs/language/css/css.worker?worker';
import HtmlWorker from 'monaco-editor/esm/vs/language/html/html.worker?worker';
import TsWorker from 'monaco-editor/esm/vs/language/typescript/ts.worker?worker';

export function setupMonacoWorkers() {
	(self as any).MonacoEnvironment = {
		getWorker(_: string, label: string) {
			switch (label) {
				case 'json':
					return new JsonWorker();
				case 'css':
					return new CssWorker();
				case 'html':
					return new HtmlWorker();
				case 'typescript':
				case 'javascript':
					return new TsWorker();
				default:
					return new EditorWorker();
			}
		}
	};
}
