<script lang="ts">
  import Editor from "@components/Editor.svelte";
  import DiffResult from "./DiffResult.svelte";
	import HorizontalSplit from "@layouts/HorizontalSplit.svelte";
	import { dummyDiffs } from "@/testData";
</script>


<HorizontalSplit initialLeftRatio={70}>
  <main slot="left" class="code-main">
    <HorizontalSplit initialLeftRatio={50}>
      <section slot="left">
        <Editor />
      </section>
      <section slot="right">
        <Editor />
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
  overflow: hidden;
  display: flex;
  height: 100%;
  min-height: 0;
}
</style>