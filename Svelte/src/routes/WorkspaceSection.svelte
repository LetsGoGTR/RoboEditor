<script lang="ts">
	import TabLayout from '@layouts/TabLayout.svelte';
  	import DirectoryTree from '@components/DirectoryTree.svelte';
	import { dummyDeepTree } from '@/testData';
</script>

<TabLayout
	leftTab={{ id: 'left', label: 'Workspace' }}
	rightTab={{ id: 'right', label: 'Controller' }}
>
	<div slot="left">
    <DirectoryTree root={dummyDeepTree} />
  </div>

	<div slot="right">
		<p>고급 설정 탭의 내용입니다.</p>
	</div>
</TabLayout>
