<script lang="ts">
  import { onMount, tick } from 'svelte';

  let code = $state(`console.log("Hello, RoboEditor!");`);
  let lines: string[] = $state([]);
  let textarea: HTMLTextAreaElement;
  let display: HTMLDivElement;
  let lineNumbers: HTMLDivElement;

  async function handleInput(event: Event) {
    const target = event.target as HTMLTextAreaElement;
    code = target.value;
    await tick();
    lines = code.split('\n');
  }

  function syncScroll() {
    const scroll = textarea.scrollTop;
    display.scrollTop = scroll;
    lineNumbers.scrollTop = scroll;
  }

  function focusTextarea() {
    textarea?.focus();
  }

  onMount(() => {
    lines = code.split('\n');
  });
</script>

<div class="editor-container">
  <div class="line-numbers" bind:this={lineNumbers}>
    {#each lines as _, i}
      <div>{i + 1}</div>
    {/each}
  </div>

  <div
    class="code-display"
    bind:this={display}
    role="textbox"
    tabindex="0"
    aria-label="Code editor"
    onclick={focusTextarea}
    onkeydown={(e) => {
      if (e.key === 'Enter' || e.key === ' ') focusTextarea();
    }}
  >
    <pre><code>{code || '\n'}</code></pre>
  </div>

  <textarea
    bind:this={textarea}
    bind:value={code}
    oninput={handleInput}
    onscroll={syncScroll}
    spellcheck="false"
    class="code-input"
  ></textarea>
</div>

<style>
.editor-container {
  position: relative;
  display: flex;
  height: 100%;
  min-height: 0;
  font-size: 15px;
  font-family: 'JetBrains Mono', 'Consolas', monospace;
  line-height: 1.4;
  background: #fafafa;
  color: #111;
  overflow: hidden;
}

/* 줄 번호 */
.line-numbers {
  width: 3rem;
  text-align: right;
  padding: 0.5rem 0.5rem 0.5rem 0;
  border-right: 1px solid #ccc;
  color: #888;
  user-select: none;
  background: #f3f3f3;
  font-family: inherit;
  overflow: hidden;
}

/* 코드 영역 */
.code-display {
  flex: 1;
  padding: 0.5rem;
  overflow: hidden; /* 직접 스크롤하지 않음 */
  white-space: pre;
  font-family: inherit;
  font-size: inherit;
  cursor: text;
}
.code-display pre {
  margin: 0;
}

/* 입력 textarea (투명 오버레이) */
.code-input {
  position: absolute;
  top: 0;
  left: 3rem;
  right: 0;
  bottom: 0;
  padding: 0.5rem;
  border: none;
  outline: none;
  resize: none;
  background: transparent;
  color: transparent;
  caret-color: black;
  font: inherit;
  line-height: inherit;
  overflow: auto;
  white-space: pre;
}

.code-input::selection {
  background: rgba(80, 140, 255, 0.35);
}
</style>
