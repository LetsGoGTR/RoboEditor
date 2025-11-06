<script lang="ts">
</script>

<form>
  <h2>제어기 정보 등록</h2>

  <div class="form-row">
    <label for="serial">Serial Number</label>
    <input type="text" id="serial" name="serial" placeholder="e.g. CTR-00123456" />
  </div>

  <div class="form-row">
    <label for="ip">IP</label>
    <input type="text" id="ip" name="ip" placeholder="192.168.0.10" />
  </div>

  <div class="form-row">
    <label for="username">Username</label>
    <input type="text" id="username" name="username" />
  </div>

  <div class="form-row">
    <label for="password">Password</label>
    <input type="password" id="password" name="password" />
  </div>

  <!-- ✅ Port 가로 일렬 정렬 (정확한 baseline 정렬) -->
  <div class="form-row port-row">
    <label for="sftp-port">SFTP Port</label>
    <input type="number" id="sftp-port" name="sftp-port" min="1" max="65535" value="22" />

    <label for="api-port" class="api-label">API Port</label>
    <input type="number" id="api-port" name="api-port" min="1" max="65535" value="80" />
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

  /* ✅ Port 라벨과 입력칸 정렬 보정 */
  .port-row {
    display: grid;
    grid-template-columns: 110px 1fr 90px 1fr;
    align-items: center;
    column-gap: 0.5rem;
  }

  .api-label {
    text-align: right;
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
