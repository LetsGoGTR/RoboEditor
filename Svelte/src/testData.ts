import type { Controller, TreeNode } from './types'; //DiffItem, 

// 파일만 있는 Data
export const dummyFilesOnly: TreeNode = {
	id: 'A1d3ZxP7LkQ9TfG6YhR2',
	name: 'main.ts',
	type: 'file',
	parentId: null,
	path: 'main.ts',
	size: 42,
	lastModified: '2025-11-04T10:00:00Z'
};

// 단일 폴더 + 내부 파일 3개 Data
export const dummySingleFolder: TreeNode = {
	id: 'P9x4TrE2JqA7VwF1KzL5',
	name: 'src',
	type: 'directory',
	parentId: null,
	path: 'src',
	children: [
		{
			id: 'D8r6GhS3YpQ9NfC2XvT4',
			name: 'index.ts',
			type: 'file',
			parentId: 'P9x4TrE2JqA7VwF1KzL5',
			path: 'src/index.ts',
			size: 55,
			lastModified: '2025-11-04T10:10:00Z'
		},
		{
			id: 'E5m3ZjL8RtK7YwD1PaV6',
			name: 'app.ts',
			type: 'file',
			parentId: 'P9x4TrE2JqA7VwF1KzL5',
			path: 'src/app.ts',
			size: 62,
			lastModified: '2025-11-04T10:12:00Z'
		},
		{
			id: 'F4q2UxW7SnM9HbG3CvR8',
			name: 'types.d.ts',
			type: 'file',
			parentId: 'P9x4TrE2JqA7VwF1KzL5',
			path: 'src/types.d.ts',
			size: 40,
			lastModified: '2025-11-04T10:15:00Z'
		}
	]
};

// 4중 Directory 구조 (tree) Data
export const dummyDeepTree: TreeNode = {
	id: 'R1t7LpD5QzV9KaS3MfW8',
	name: 'root',
	type: 'directory',
	parentId: null,
	path: 'root',
	children: [
		{
			id: 'S8a2JcN6XqT4WbE9RfP1',
			name: 'src',
			type: 'directory',
			parentId: 'R1t7LpD5QzV9KaS3MfW8',
			path: 'root/src',
			children: [
				{
					id: 'T4y3XzV9KpB7FgQ1MdL8',
					name: 'index.ts',
					type: 'file',
					parentId: 'S8a2JcN6XqT4WbE9RfP1',
					path: 'root/src/index.ts',
					size: 36,
					lastModified: '2025-11-04T10:18:00Z'
				},
				{
					id: 'U9v1RfE4NwJ7CxL3KbT2',
					name: 'setup.ts',
					type: 'file',
					parentId: 'S8a2JcN6XqT4WbE9RfP1',
					path: 'root/src/setup.ts',
					size: 33,
					lastModified: '2025-11-04T10:19:00Z'
				},
				{
					id: 'V5s8MbA3ZhD2PqY7FrN1',
					name: 'features',
					type: 'directory',
					parentId: 'S8a2JcN6XqT4WbE9RfP1',
					path: 'root/src/features',
					children: [
						{
							id: 'W3c6YxE8RaN4KtP2VbQ7',
							name: 'user',
							type: 'directory',
							parentId: 'V5s8MbA3ZhD2PqY7FrN1',
							path: 'root/src/features/user',
							children: [
								{
									id: 'X2a9LgF7JpD5QsT1HbM8',
									name: 'profile',
									type: 'directory',
									parentId: 'W3c6YxE8RaN4KtP2VbQ7',
									path: 'root/src/features/user/profile',
									children: [
										{
											id: 'Y1t4VrN8LkM2QpZ6SdJ3',
											name: 'view.ts',
											type: 'file',
											parentId: 'X2a9LgF7JpD5QsT1HbM8',
											path: 'root/src/features/user/profile/view.ts',
											size: 48,
											lastModified: '2025-11-04T10:20:00Z'
										},
										{
											id: 'Z9h7BgE1FpT5RkC2XaL4',
											name: 'edit.ts',
											type: 'file',
											parentId: 'X2a9LgF7JpD5QsT1HbM8',
											path: 'root/src/features/user/profile/edit.ts',
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
									size: 33,
									lastModified: '2025-11-04T10:22:00Z'
								},
								{
									id: 'B4d7YnK1LsW9PvT3FjE6',
									name: 'logout.ts',
									type: 'file',
									parentId: 'W3c6YxE8RaN4KtP2VbQ7',
									path: 'root/src/features/user/logout.ts',
									size: 34,
									lastModified: '2025-11-04T10:23:00Z'
								}
							]
						},
						{
							id: 'C5x9RbP3FkT8MwY2NqH7',
							name: 'dashboard',
							type: 'directory',
							parentId: 'V5s8MbA3ZhD2PqY7FrN1',
							path: 'root/src/features/dashboard',
							children: [
								{
									id: 'D2l8TpH4JsV6XqC9RfM1',
									name: 'main.ts',
									type: 'file',
									parentId: 'C5x9RbP3FkT8MwY2NqH7',
									path: 'root/src/features/dashboard/main.ts',
									size: 40,
									lastModified: '2025-11-04T10:24:00Z'
								},
								{
									id: 'E7a3GfV1LqD9ZpS4BnC6',
									name: 'chart.ts',
									type: 'file',
									parentId: 'C5x9RbP3FkT8MwY2NqH7',
									path: 'root/src/features/dashboard/chart.ts',
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

// Controller 정보 Data
export const dummyController: Controller[] = [
  {
    controllerMeta: {
      name: '컨트롤러 1',
      serialNumber: 'SN-A9K3L1X7',
      state: 'active',
      ipAddress: '192.168.0.21'
      // description, sftpPort 필요하면 추가
    },
    workspaces: [] // 아직 더미 없으면 빈 배열
  },
  {
    controllerMeta: {
      name: '컨트롤러 2',
      serialNumber: 'SN-Q4R7C2P8',
      state: 'idle',
      ipAddress: '10.0.0.56'
    },
    workspaces: []
  },
  {
    controllerMeta: {
      name: '컨트롤러 3',
      serialNumber: 'SN-M8T5Z9B3',
      state: 'error',
      ipAddress: '172.16.1.102'
    },
    workspaces: []
  },
  {
    controllerMeta: {
      name: '컨트롤러 4',
      serialNumber: 'SN-V2N6H4W5',
      state: 'disconnected',
      ipAddress: '192.168.1.77'
    },
    workspaces: []
  }
];

// // 비교 결과 Data
// export const dummyDiffs: DiffItem[] = [
// 	{ line: 1, path: 'action', leftValue: '{"action":0}', rightValue: '', state: 'REMOVED' },
// 	{ line: 2, path: 'cd_jog', leftValue: '{"cd_jog":0}', rightValue: '', state: 'REMOVED' },
// 	{ line: 4, path: 'min_req_version', leftValue: '2400', rightValue: '1500', state: 'CHANGED' },
// 	{ line: 7, path: 'path_info', leftValue: '', rightValue: '{}', state: 'ADDED' }
// ];
