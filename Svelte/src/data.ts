import type { TreeNode } from './types';
import { generateId } from './utils/FileNode';

export const initialTree: TreeNode = {
	id: generateId(),
	name: 'workspace',
	type: 'folder',
	parentId: null,
	path: 'workspace',
	children: [
		{
			id: generateId(),
			name: 'untitle.yaml',
			type: 'file',
			parentId: 'initFolder01',
			path: 'workspace/untitle.yaml',
			contentRef: '',
			size: 60,
			lastModified: new Date().toISOString()
		}
	]
};
