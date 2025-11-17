import { defineConfig } from 'vitest/config';
import { sveltekit } from '@sveltejs/kit/vite';
import path from 'path';

export default defineConfig({
	plugins: [
		sveltekit(),
	],
	server: {
		proxy: {
			'/proxy': {
				target: 'http://localhost:8889',
				changeOrigin: true,
				rewrite: (path) => path.replace(/^\/proxy/, '')
			}
		}
	},
	ssr: {
		noExternal: ['monaco-editor'] // SSR 번들에서 monaco 제외
	},
	optimizeDeps: {
		include: ['monaco-editor']
	},
	test: {
		expect: { requireAssertions: true },
		projects: [
			{
				extends: './vite.config.ts',
				test: {
					name: 'client',
					environment: 'browser',
					browser: {
						enabled: true,
						provider: 'playwright',
						instances: [{ browser: 'chromium' }]
					},
					include: ['src/**/*.svelte.{test,spec}.{js,ts}'],
					exclude: ['src/lib/server/**'],
					setupFiles: ['./vitest-setup-client.ts']
				}
			},
			{
				extends: './vite.config.ts',
				test: {
					name: 'server',
					environment: 'node',
					include: ['src/**/*.{test,spec}.{js,ts}'],
					exclude: ['src/**/*.svelte.{test,spec}.{js,ts}']
				}
			}
		]
	},
	resolve: {
		alias: {
			'@': path.resolve(__dirname, './src'),
			'@components': path.resolve(__dirname, './src/lib/components'),
			'@layouts': path.resolve(__dirname, './src/lib/layouts'),
			'@features': path.resolve(__dirname, './src/features/components'),
			'@handlers': path.resolve(__dirname, './src/features/handlers'),
			'@apis': path.resolve(__dirname, '/src/apis'),
			'@routes': path.resolve(__dirname, './src/routes'),
			'@utils': path.resolve(__dirname, './src/utils'),
			'@styles': path.resolve(__dirname, './src/styles')
		}
	}
});
