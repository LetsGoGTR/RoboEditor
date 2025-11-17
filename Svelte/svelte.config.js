import adapter from '@sveltejs/adapter-static';
import { vitePreprocess } from '@sveltejs/vite-plugin-svelte';

/** @type {import('@sveltejs/kit').Config} */
const config = {
	preprocess: vitePreprocess(),

	kit: {
		adapter: adapter({
			pages: 'build', // 빌드 결과 폴더
			assets: 'build',
			fallback: 'index.html' // SPA 라우팅용 (모든 경로에서 index.html 서빙)
		}),
		alias: {
			'@components/*': 'src/lib/components/*',
			'@layouts/*': 'src/lib/layouts/*',
			'@apis/*': 'src/apis/*',
			'@features/*': 'src/features/components/*',
			'@handlers/*': 'src/features/handlers/*',
			'@routes/*': 'src/routes/*',
			'@styles/*': 'src/styles/*',
			'@utils/*': 'src/utils/*',
			'@/*': 'src/*'
		}
	}
};

export default config;
