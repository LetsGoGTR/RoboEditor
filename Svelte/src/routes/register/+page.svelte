<script lang="ts">
    import type { PageProps } from './$types';
	import ControllerForm from './ControllerForm.svelte';

    let { data }: PageProps = $props();
</script>

<ControllerForm />