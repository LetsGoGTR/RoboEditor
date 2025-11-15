<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  export let selected: string[] = [];
  export let folderName = '';

  let dlg!: HTMLDialogElement;
  let input!: HTMLInputElement;
  let step: 'confirm' | 'password' = 'confirm';
  const dispatch = createEventDispatcher();

  export function open() {
    step = 'confirm';
    dlg?.showModal();
  }

  function close() {
    dlg?.close();
  }

  function goNext() {
    step = 'password';
    setTimeout(() => input?.focus(), 0);
  }

  function apply() {
    const password = input?.value ?? '';
    dispatch('apply', { password });
    input.value = '';
    close();
  }
</script>

<dialog bind:this={dlg}>
  {#if step === 'confirm'}
    <h3>적용 확인</h3>
    <p>
      선택된 제어기: <strong>{selected.join(', ') || '없음'}</strong><br />
      선택된 폴더: <strong>{folderName || '없음'}</strong>
    </p>

    <div class="dialog-buttons">
      <button type="button" on:click={close}>취소</button>
      <button type="button" on:click={goNext}>다음</button>
    </div>

  {:else if step === 'password'}
    <h3>비밀번호 확인</h3>
    <div class="field">
      <label for="pw">비밀번호</label>
      <input id="pw" type="password" bind:this={input}
             placeholder="비밀번호를 입력하세요" required autocomplete="current-password" />
    </div>

    <div class="dialog-buttons">
      <button type="button" on:click={close}>취소</button>
      <button type="button" on:click={apply}>적용</button>
    </div>
  {/if}
</dialog>

<style>
dialog{
  position:fixed;
  top:50%;
  left:50%;
  transform:translate(-50%,-50%);
  margin:0;
  border:none;
  border-radius:.75rem;
  background:#fff;
  box-shadow:0 8px 30px rgba(0,0,0,.25);
  padding:1.5rem 2rem;
  max-width:420px;
  width:90%;
}
dialog::backdrop{
  background:rgba(0,0,0,.4);
}
.field{
  display:flex;
  flex-direction:column;
  gap:.4rem;
  margin:1rem 0 .5rem;
}
.field input{
  padding:.6rem .7rem;
  border:1px solid #ddd;
  border-radius:.4rem;
  font-size:.95rem;
}
.dialog-buttons{
  margin-top:1rem;
  display:flex;
  gap:.5rem;
  justify-content:flex-end;
}
.dialog-buttons button{
  padding:.5rem 1rem;
  border:none;
  border-radius:.4rem;
  cursor:pointer;
  font-weight:500;
  font-size:.9rem;
}
.dialog-buttons button:first-child{
  background:#ccc;
  color:#333;
}
.dialog-buttons button:last-child{
  background:#0078d7;
  color:#fff;
}
.dialog-buttons button:last-child:hover{
  background:#005fa3;
}
</style>
