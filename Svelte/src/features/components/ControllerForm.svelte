<script lang="ts">
  import { fileTree } from "@/stores/fileTree";
  import type { FolderNode, TreeNode } from "@/types";
  import DirectoryTree from "@components/DirectoryTree.svelte";
  import { _createDevice } from "@/apis/controller";

  let selectedFolder = $state<FolderNode | null>(null);

  // 폼 필드 상태
  let ip           = $state("");
  let sftpPort     = $state(8889);
  let serialNumber = $state("");
  let username     = $state("");
  let password     = $state("");
  let name         = $state("");        // 디바이스 이름 (별도로 받고 싶으면)
  let description  = $state("");        // 설명 (선택)

  function handleCancel() {
    // 모든 입력값 초기화
    name = "";
    serialNumber = "";
    description = "";
    ip = "";
    sftpPort = 8889;
    username = "";
    password = "";
    selectedFolder = null;
  }

  function handleSelect(node: TreeNode) {
    if (node.type === "directory") selectedFolder = node as FolderNode;
  }

  async function handleOk() {
    // 필수 값 체크 (serialNumber, name)
    if (!serialNumber || !name) {
      alert("Serial Number와 이름은 필수입니다.");
      return;
    }

    const payload = {
      serialNumber,                      // 필수
      name,                              // 필수
      description: description || (selectedFolder?.path ?? ""), // 선택
      api: ip,                           // 필요하면 `http://${ip}` 이런 식으로 바꿔도 됨
      sftpHost: ip,
      sftpPort: Number(sftpPort),
      sftpUser: username,
      sftpPassword: password
    };

    try {
      const res = await _createDevice(payload);
      console.log("device created:", res);
      handleCancel();
      // TODO: 성공 후 다이얼로그 닫기/리셋 등 처리
    } catch (err) {
      console.error("device create failed:", err);
    }
  }
</script>

<form>
  <h2>제어기 정보 등록</h2>

  <div class="form-row">
    <label for="serial">Serial Number</label>
    <input
      type="text"
      id="serial"
      name="serial"
      bind:value={serialNumber}
    />
  </div>

  <div class="form-row">
    <label for="name">Name</label>
    <input
      type="text"
      id="name"
      name="name"
      bind:value={name}
      placeholder="디바이스 이름"
    />
  </div>

  <div class="form-row">
    <label for="description">Description</label>
    <input
      type="text"
      id="description"
      name="description"
      bind:value={description}
      placeholder="설명 (선택)"
    />
  </div>

  <div class="form-row">
    <label for="ip">IP</label>
    <input
      type="text"
      id="ip"
      name="ip"
      placeholder="ex. 192.168.0.10"
      bind:value={ip}
    />
  </div>

  <div class="form-row">
    <label for="sftp-port">SFTP Port</label>
    <input
      type="number"
      id="sftp-port"
      name="sftp-port"
      min="1"
      max="65535"
      bind:value={sftpPort}
    />
  </div>

  <div class="form-row">
    <label for="username">Username</label>
    <input
      type="text"
      id="username"
      name="username"
      bind:value={username}
    />
  </div>

  <div class="form-row">
    <label for="password">Password</label>
    <input
      type="password"
      id="password"
      name="password"
      bind:value={password}
    />
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
    <button type="button" onclick={handleOk}>OK</button>
    <button type="button" onclick={handleCancel}>Cancel</button>
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
    flex: 0 0 110px;
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

  button[type="button"]:first-child {
    background-color: #0078d4;
    color: white;
    border: none;
  }

  button[type="button"]:first-child:hover {
    background-color: #005ea6;
  }
</style>
