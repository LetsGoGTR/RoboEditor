<script lang="ts">
	import { fileTree } from "@/stores/fileTree";
	import type { FolderNode, TreeNode } from "@/types";
	import DirectoryTree from "@components/DirectoryTree.svelte";

  let selectedFolder = $state<FolderNode | null>(null);

  function handleSelect(node: TreeNode) {
    if (node.type === "directory") selectedFolder = node;
  }
</script>

<form>
  <h2>제어기 정보 등록</h2>

  <div class="form-row">
    <label for="ip">IP</label>
    <input type="text" id="ip" name="ip" placeholder="ex. 192.168.0.10" />
  </div>

  <div class="form-row">
    <label for="sftp-port">SFTP Port</label>
    <input type="number" id="sftp-port" name="sftp-port" min="1" max="65535" value="8889" />
  </div>

  <div class="form-row">
    <label for="serial">Serial Number</label>
    <input type="text" id="serial" name="serial" />
  </div>

  <div class="form-row">
    <label for="username">Username</label>
    <input type="text" id="username" name="username" />
  </div>

  <div class="form-row">
    <label for="password">Password</label>
    <input type="password" id="password" name="password" />
  </div>
    <div class="form-row">
      <label for="folder-path">폴더 경로</label>
      <input
        id="folder-path"
        type="text"
        class="path-input"
        readonly
        value={selectedFolder ? selectedFolder.path : ""}
      />
      
    </div>
    <div class="folder-section">
      {#if $fileTree}
        <DirectoryTree root={$fileTree} mode="view" onselect={handleSelect} />
      {/if}
    </div>
    

  <div class="button-row">
    <button type="submit">OK</button>
    <button type="button">Cancel</button>
  </div>
</form>

<style>
  form {
    display: flex;
    flex-direction: column;
    font-family: system-ui, sans-serif;
    background: #fff;
    padding: 2.5rem;
    max-width: 500px;
    margin: 0 auto;
    
  }

  h2 {
    text-align: center;
    font-size: 1.2rem;
    margin: 0 0 1.5rem 0;
  }

  .form-row {
    display: flex;
    align-items: center;
    margin-bottom: 1rem;
    gap: 0.5rem;
  }

  .folder-section {
    border: solid 1px #ccc;
    border-radius: 0.3rem;

    min-height: 200px;
    max-height: 200px;
    overflow-y: auto;
    padding: 0.5rem;
    background: #fafafa;
  }

  label {
    flex: 0 0 110px; /* 라벨 고정 폭 */
    font-size: 0.9rem;
    white-space: nowrap;
    text-align: right;
  }

  input[type="text"],
  input[type="number"],
  input[type="password"] {
    flex: 1;
    padding: 0.4rem 0.5rem;
    border: 1px solid #ccc;
    border-radius: 0.3rem;
    font-size: 0.9rem;
  }

  .button-row {
    display: flex;
    justify-content: flex-end;
    gap: 0.75rem;
    margin-top: 1.5rem;
  }

  button {
    padding: 0.5rem 1rem;
    border: 1px solid #ccc;
    border-radius: 0.3rem;
    background-color: #f3f3f3;
    cursor: pointer;
    transition: background 0.2s;
  }

  button:hover {
    background-color: #e5e5e5;
  }

  button[type="submit"] {
    background-color: #0078d4;
    color: white;
    border: none;
  }

  button[type="submit"]:hover {
    background-color: #005ea6;
  }
</style>
