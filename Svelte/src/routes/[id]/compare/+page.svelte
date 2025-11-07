<script lang="ts">
  import DiffResult from "./DiffResult.svelte";
	import HorizontalSplit from "@layouts/HorizontalSplit.svelte";
	import { dummyDiffs } from "@/testData";
  import { onMount } from 'svelte';
  import * as monaco from 'monaco-editor';

  let leftContainer: HTMLDivElement;
  let rightContainer: HTMLDivElement;

  onMount(() => {
    const leftEditor = monaco.editor.create(leftContainer, {
      value: 'console.log("Hello, Monaco!");',
      language: 'javascript',
      theme: 'vs',
      automaticLayout: true
    });

    const rightEditor = monaco.editor.create(rightContainer, {
      value: 'console.log("Hello, Monaco!");',
      language: 'javascript',
      theme: 'vs',
      automaticLayout: true
    });

    // 메모리 누수 방지를 위한 cleanup
    return () => {leftEditor.dispose();rightEditor.dispose();}
  });
</script>

<HorizontalSplit initialLeftRatio={70}>
  <main slot="left" class="code-main">
    <HorizontalSplit initialLeftRatio={50}>
      <section slot="left">
        <div bind:this={leftContainer} style="width:auto; height: 100%;"></div>
      </section>
      <section slot="right">
        <div bind:this={rightContainer} style="width:auto; height: 100%;"></div>
      </section>
    </HorizontalSplit>
  </main>
  <section slot="right" class="compare-section">
    <DiffResult diffs={dummyDiffs} />
  </section>
</HorizontalSplit>

<style>
main {
  display: flex;
  flex: 1 1 auto;
  height: 100%;
  min-height: 0;
}

section {
  flex: 1 1 auto;
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
}

.code-main {
  flex: 1 1 auto;
  overflow: hidden;
  display: flex;
  height: 100%;
  min-height: 0;
}

.compare-section {
  flex: 1 1 auto;
  overflow: scroll;
  display: flex;
  height: 100%;
  min-height: 0;
}
</style>