## 📂 File System Access (FSA) API 사용 안내
### 개요

본 프로젝트는 브라우저 환경에서 로컬 디렉토리 및 파일을 직접 접근하기 위해
File System Access API를 사용합니다.
이를 통해 사용자는 별도의 업로드/다운로드 과정 없이
로컬 폴더를 바로 선택하고, 내부 파일을 읽거나 편집할 수 있습니다.

``` typescript
const dirHandle = await window.showDirectoryPicker();
for await (const [name, handle] of dirHandle.entries()) {
  if (handle.kind === 'file') {
    const file = await handle.getFile();
    console.log(name, file.size);
  }
}
```
📘 참고 :
- W3C File System Access API Draft (2024)
- Google Developers – File System Access Guide

### 지원 환경
플랫폼	브라우저	지원 여부	비고
| 플랫폼    | 브라우저    | 지원 여부  | 비고    |
| --------------------------- | -------------------- | ----------------------------------- | ----------- |
| **Windows / macOS / Linux** | Chrome ≥ 86          | ✅ 완전 지원                             | HTTPS 환경 필수 |
|                             | Edge ≥ 88 (Chromium) | ✅ 완전 지원                             |             |
|                             | Brave / Opera        | ✅ 지원                                | Chromium 기반 |
| **Firefox (모든 OS)**         | ⚠️ 제한적               | 실험적 flag(`dom.fsAccess.enabled`) 필요 |             |
| **Safari (macOS / iOS)**    | ❌ 미지원                | WebKit에 미구현                         |             |
| **Electron / QtWebEngine**  | ✅ 가능                 | Chromium 내장 시 사용 가능                 |             |

- **주의** : 이 API는 브라우저별 지원 편차가 큽니다.
Chrome 또는 Edge에서의 사용을 권장합니다.

### 보안 및 실행 조건

HTTPS 또는 localhost 환경에서만 동작합니다.

내부망(사내망)에서도 self-signed 인증서 기반의 HTTPS 서버에서 실행되어야 합니다.

브라우저 보안 정책상 사용자 조작(버튼 클릭 등) 이후에만 디렉토리 선택이 가능합니다.

- 개발 환경 예시 (Vite)
``` bash
npm run dev -- --https
```

### Fallback 지원

비지원 환경(Firefox, Safari 등)을 위해
```<input type="file" webkitdirectory>``` 기반 대체 접근을 제공합니다.

```html
<input type="file" webkitdirectory multiple onChange="handleFiles(event)" />
```

이 fallback은 동일한 디렉토리 구조를 읽어들이며,
File System Access API와 유사한 형태로 FileList를 반환합니다.

### Linux 환경 가이드

- Ubuntu, Fedora, Arch 등 주요 배포판에서 Chromium 기반 브라우저 사용 시 정상 작동
- showDirectoryPicker() 호출 시 native file chooser가 실행됨
- Firefox 환경에서는 자동 fallback 처리됨
- Dockerized 환경에서도 X11 forwarding 또는 Wayland session 내 실행 시 GUI 접근 가능

### 관련 코드 참조
| 파일                                   | 설명                                          |
| ------------------------------------ | ------------------------------------------- |
| `src/stores/fileTree.ts`             | FSA API를 통해 읽은 디렉토리 트리를 Store로 저장           |
| `src/utils/fs.ts`                    | `showDirectoryPicker()` 안전 호출 및 fallback 처리 |
| `src/routes/controller/+page.svelte` | Controller별 workspace 로딩 시 파일 접근 기능 사용      |

### 주의 및 권장사항

브라우저 권한 요청은 매번 발생할 수 있습니다.
(특히 persistent access handle은 아직 완전 표준화되지 않음)

로컬 데이터 저장을 위해 IndexedDB / Dexie.js와 병행 사용할 수 있습니다.

기업망 보안정책(예: CSP, sandbox 제한)에 따라 API가 차단될 수 있으므로
배포 환경에서 반드시 사전 테스트가 필요합니다.

### License & References
- Based on WICG File System Access API Specification (2024 Draft)
- Implementation inspired by GoogleChromeLabs/browser-fs-access
- Tested on Chromium 129 (Ubuntu 24.04 LTS, Windows 11)

