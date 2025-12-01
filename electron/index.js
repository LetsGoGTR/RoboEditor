// index.js
const { app, BrowserWindow, dialog } = require('electron');
const path = require('path');
const { spawn } = require('child_process');

// Drogon이 리슨할 포트 (config.json의 listeners와 맞추기)
const BACKEND_PORT = 5555;

process.on('uncaughtException', (err) => {
  console.error('Uncaught in main:', err);
  try {
    dialog.showErrorBox('메인 프로세스 오류', String(err));
  } catch (_) {
    // dialog 쓸 수 없는 시점이면 그냥 무시
  }
});

let mainWindow = null;
let backendProc = null;

// Drogon.exe 경로 계산
function getBackendPath() {
  if (app.isPackaged) {
    return path.join(process.resourcesPath, 'back', process.platform === 'win32' ? 'Drogon.exe' : 'Drogon');
  } else {
    return path.join(__dirname, 'back', process.platform === 'win32' ? 'Drogon.exe' : 'Drogon');
  }
}

function startBackend() {
  const backendPath = getBackendPath();
  const backendDir = path.dirname(backendPath);
  const dataRoot = app.getPath('userData');
	
  console.log('Starting backend:', backendPath);
  console.log('DATA_ROOT:', dataRoot);

  backendProc = spawn(backendPath, [], {
    cwd: backendDir,      // back 폴더 기준으로 실행
    detached: false,
    stdio: app.isPackaged ? 'ignore' : 'inherit',  // 배포에서는 로그 안 봄
    windowsHide: true,
    env: {
      ...process.env,
      ROBOEDITOR_DATA_DIR: dataRoot
    }
  });

  backendProc.on('error', (err) => {
    console.error('Failed to start backend:', err);
    dialog.showErrorBox('백엔드 실행 오류', String(err));
  });

  backendProc.on('exit', (code, signal) => {
    console.log('Backend exited:', code, signal);
  });
}

function getIconPath() {
  return path.join(process.resourcesPath, 'app', 'roboeditor-icon.ico');
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    icon: getIconPath(),
    webPreferences: {
      preload: path.join(__dirname, 'preload.js')
    }
  });

  const url = `http://127.0.0.1:${BACKEND_PORT}/`;
  console.log('Loading URL:', url);
  mainWindow.loadURL(url);

  // 실패했을 때 무슨 오류인지 눈에 보이게
  mainWindow.webContents.on(
    'did-fail-load',
    (event, errorCode, errorDesc, validatedURL) => {
      console.error('Load failed:', errorCode, errorDesc, validatedURL);
      dialog.showErrorBox(
        '프론트 로드 오류',
        `URL: ${validatedURL}\n${errorCode}: ${errorDesc}`
      );
    }
  );

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  // 1) 백엔드 먼저 띄우고
  startBackend();
  // 2) 창 생성
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('before-quit', () => {
  if (backendProc && !backendProc.killed) {
    backendProc.kill();
  }
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});
