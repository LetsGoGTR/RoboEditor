import type { TreeNode } from './types';

// 파일만 있는 Data
export const dummyFilesOnly: TreeNode[] = [
	{
		id: 'A1d3ZxP7LkQ9TfG6YhR2',
		name: 'main.ts',
		type: 'file',
		parentId: null,
		path: 'main.ts',
		content: "console.log('Hello, world!');",
		size: 42,
		lastModified: '2025-11-04T10:00:00Z'
	},
	{
		id: 'B7g2RjT1QvM5LpN3XeK8',
		name: 'utils.ts',
		type: 'file',
		parentId: null,
		path: 'utils.ts',
		content: 'export function add(a:number,b:number){return a+b;}',
		size: 64,
		lastModified: '2025-11-04T10:05:00Z'
	},
	{
		id: 'C6y9VbH2NpJ4MwE7LrF5',
		name: 'config.json',
		type: 'file',
		parentId: null,
		path: 'config.json',
		content: '{ "version": "1.0.0" }',
		size: 28,
		lastModified: '2025-11-04T10:07:00Z'
	}
];

// 단일 폴더 + 내부 파일 3개 Data
export const dummySingleFolder: TreeNode = {
	id: 'P9x4TrE2JqA7VwF1KzL5',
	name: 'src',
	type: 'folder',
	parentId: null,
	path: 'src',
	children: [
		{
			id: 'D8r6GhS3YpQ9NfC2XvT4',
			name: 'index.ts',
			type: 'file',
			parentId: 'P9x4TrE2JqA7VwF1KzL5',
			path: 'src/index.ts',
			content: "import { init } from './app'; init();",
			size: 55,
			lastModified: '2025-11-04T10:10:00Z'
		},
		{
			id: 'E5m3ZjL8RtK7YwD1PaV6',
			name: 'app.ts',
			type: 'file',
			parentId: 'P9x4TrE2JqA7VwF1KzL5',
			path: 'src/app.ts',
			content: "export function init(){ console.log('app start'); }",
			size: 62,
			lastModified: '2025-11-04T10:12:00Z'
		},
		{
			id: 'F4q2UxW7SnM9HbG3CvR8',
			name: 'types.d.ts',
			type: 'file',
			parentId: 'P9x4TrE2JqA7VwF1KzL5',
			path: 'src/types.d.ts',
			content: "export type AppMode = 'dev' | 'prod';",
			size: 40,
			lastModified: '2025-11-04T10:15:00Z'
		}
	]
};

// 4중 Directory 구조 (tree) Data
export const dummyDeepTree: TreeNode = {
	id: 'R1t7LpD5QzV9KaS3MfW8',
	name: 'root',
	type: 'folder',
	parentId: null,
	path: 'root',
	children: [
		{
			id: 'S8a2JcN6XqT4WbE9RfP1',
			name: 'src',
			type: 'folder',
			parentId: 'R1t7LpD5QzV9KaS3MfW8',
			path: 'root/src',
			children: [
				{
					id: 'T4y3XzV9KpB7FgQ1MdL8',
					name: 'index.ts',
					type: 'file',
					parentId: 'S8a2JcN6XqT4WbE9RfP1',
					path: 'root/src/index.ts',
					content: "export * from './features';",
					size: 36,
					lastModified: '2025-11-04T10:18:00Z'
				},
				{
					id: 'U9v1RfE4NwJ7CxL3KbT2',
					name: 'setup.ts',
					type: 'file',
					parentId: 'S8a2JcN6XqT4WbE9RfP1',
					path: 'root/src/setup.ts',
					content: "console.log('setup complete');",
					size: 33,
					lastModified: '2025-11-04T10:19:00Z'
				},
				{
					id: 'V5s8MbA3ZhD2PqY7FrN1',
					name: 'features',
					type: 'folder',
					parentId: 'S8a2JcN6XqT4WbE9RfP1',
					path: 'root/src/features',
					children: [
						{
							id: 'W3c6YxE8RaN4KtP2VbQ7',
							name: 'user',
							type: 'folder',
							parentId: 'V5s8MbA3ZhD2PqY7FrN1',
							path: 'root/src/features/user',
							children: [
								{
									id: 'X2a9LgF7JpD5QsT1HbM8',
									name: 'profile',
									type: 'folder',
									parentId: 'W3c6YxE8RaN4KtP2VbQ7',
									path: 'root/src/features/user/profile',
									children: [
										{
											id: 'Y1t4VrN8LkM2QpZ6SdJ3',
											name: 'view.ts',
											type: 'file',
											parentId: 'X2a9LgF7JpD5QsT1HbM8',
											path: 'root/src/features/user/profile/view.ts',
											content: "export const render = () => 'Profile view';",
											size: 48,
											lastModified: '2025-11-04T10:20:00Z'
										},
										{
											id: 'Z9h7BgE1FpT5RkC2XaL4',
											name: 'edit.ts',
											type: 'file',
											parentId: 'X2a9LgF7JpD5QsT1HbM8',
											path: 'root/src/features/user/profile/edit.ts',
											content: "export const edit = () => 'Edit form';",
											size: 41,
											lastModified: '2025-11-04T10:21:00Z'
										}
									]
								},
								{
									id: 'A8r3JhT6WpC1VfN9QxL2',
									name: 'auth.ts',
									type: 'file',
									parentId: 'W3c6YxE8RaN4KtP2VbQ7',
									path: 'root/src/features/user/auth.ts',
									content: 'export const login = () => true;',
									size: 33,
									lastModified: '2025-11-04T10:22:00Z'
								},
								{
									id: 'B4d7YnK1LsW9PvT3FjE6',
									name: 'logout.ts',
									type: 'file',
									parentId: 'W3c6YxE8RaN4KtP2VbQ7',
									path: 'root/src/features/user/logout.ts',
									content: 'export const logout = () => false;',
									size: 34,
									lastModified: '2025-11-04T10:23:00Z'
								}
							]
						},
						{
							id: 'C5x9RbP3FkT8MwY2NqH7',
							name: 'dashboard',
							type: 'folder',
							parentId: 'V5s8MbA3ZhD2PqY7FrN1',
							path: 'root/src/features/dashboard',
							children: [
								{
									id: 'D2l8TpH4JsV6XqC9RfM1',
									name: 'main.ts',
									type: 'file',
									parentId: 'C5x9RbP3FkT8MwY2NqH7',
									path: 'root/src/features/dashboard/main.ts',
									content: 'export const dashboard = () => {};',
									size: 40,
									lastModified: '2025-11-04T10:24:00Z'
								},
								{
									id: 'E7a3GfV1LqD9ZpS4BnC6',
									name: 'chart.ts',
									type: 'file',
									parentId: 'C5x9RbP3FkT8MwY2NqH7',
									path: 'root/src/features/dashboard/chart.ts',
									content: "export const chart = () => 'chart render';",
									size: 45,
									lastModified: '2025-11-04T10:25:00Z'
								}
							]
						}
					]
				}
			]
		}
	]
};
