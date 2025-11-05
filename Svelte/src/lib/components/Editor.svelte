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

  <!-- 작고 숨겨진 입력 textarea -->
  <textarea
    bind:this={textarea}
    bind:value={code}
    oninput={handleInput}
    onscroll={syncScroll}
    spellcheck="false"
    class="hidden-input"
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

/* 코드 표시 영역 */
.code-display {
  flex: 1;
  padding: 0.5rem;
  overflow: auto;
  white-space: pre;
  font-family: inherit;
  font-size: inherit;
  cursor: text;
}
.code-display pre {
  margin: 0;
}

/* 숨겨진 입력 필드 */
.hidden-input {
  position: absolute;
  width: 1px;
  height: 1px;
  left: -9999px;
  top: 0;
  opacity: 0;
  border: none;
  outline: none;
  resize: none;
  background: transparent;
  color: transparent;
  caret-color: black;
  font: inherit;
  line-height: inherit;
}
</style>
