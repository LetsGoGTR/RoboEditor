import { defineConfig } from 'vitest/config';
import { sveltekit } from '@sveltejs/kit/vite';
import monacoEditorPlugin from 'vite-plugin-monaco-editor-esm';
import path from 'path';

export default defineConfig({
	plugins: [
		sveltekit(),
		monacoEditorPlugin({
			languageWorkers: ['editorWorkerService', 'typescript', 'json'],
			publicPath: 'monaco'
		})
	],
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
			'@features': path.resolve(__dirname, './src/features'),
			'@routes': path.resolve(__dirname, './src/routes'),
			'@utils': path.resolve(__dirname, './src/utils'),
			'@styles': path.resolve(__dirname, './src/styles')
		}
	}
});
