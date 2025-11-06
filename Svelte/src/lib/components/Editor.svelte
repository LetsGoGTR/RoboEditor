<script lang="ts">
  import { onMount, tick } from 'svelte';

  let { readonly = false } = $props<{ readonly?: boolean }>();

  let code = $state(`console.log("Hello, RoboEditor!");`);
  let lines: string[] = $state([]);
  let textarea: HTMLTextAreaElement;
  let display: HTMLDivElement;
  let lineNumbers: HTMLDivElement;

  async function handleInput(event: Event) {
    if (readonly) return;
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
    if (!readonly) textarea?.focus();
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
    <!-- ✅ 변경된 부분: 한 줄씩 pre 요소로 출력 -->
    {#each lines as line, i}
      <pre class="code-line" data-line={i + 1}>
        {line || '\u200B'}
      </pre>
    {/each}
  </div>

  <textarea
    bind:this={textarea}
    bind:value={code}
    oninput={handleInput}
    onscroll={syncScroll}
    spellcheck="false"
    class="hidden-input"
    readonly={readonly}
    tabindex={readonly ? -1 : 0}
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
  padding: 0.4rem 0.4rem 0.4rem 0;
  border-right: 1px solid #ccc;
  color: #888;
  user-select: none;
  background: #f3f3f3;
  font-family: inherit;
  overflow: hidden;
}

.line-numbers > div {
  height: 1.4em;            /* ✅ 코드 라인과 동일한 높이 */
  line-height: 1.4em;
  padding-right: 0.4rem;    /* 약간의 간격 */
}

/* 코드 표시 영역 */
.code-display {
  flex: 1;
  padding: 0.4rem;
  overflow: auto;
  white-space: pre;
  font-family: inherit;
  font-size: inherit;
  cursor: text;
}

/* ✅ 한 줄씩 렌더링된 코드 */
.code-line {
  margin: 0;
  padding: 0 0.6rem;
  height: 1.4em;
  line-height: 1.4em;
}
.code-line:hover {
  background: rgba(0, 0, 0, 0.04);
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
