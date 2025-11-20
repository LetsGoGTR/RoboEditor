# RoboEditor

## 프로젝트 개요

- **주제**: Svelte 기반 로컬 로봇 설정 파일 편집·비교·관리 프로그램
- **설명**: 브라우저(Chromium 계열) 기반 환경에서 로컬 디렉토리를 선택해 파일을 탐색, 비교(diff), 편집하고 컨트롤러별 워크스페이스를 관리하는 Svelte App입니다. Monaco Editor를 이용한 코드 편집, 파일/폴더 트리, 파일 비교 기능 등이 포함되어 있습니다.

## 상세

- **주요 기능**: 제어기로부터 작업 폴더 백업 기능, 작업 폴더 편집기 기능, 파일 및 폴더 별 비교 기능, 제어기로 작업 폴더 적용 기능
- **기반 기능**: 파일 탐색(`FileExplorer`), 파일 편집(Monaco Editor), 파일 저장, 컨트롤러별 워크스페이스 관리

## 사용 방법

- 로컬 개발(빠른 시작):
- **로컬 개발 (빠른 시작)** — 주요 명령을 복사해 붙여 넣어 사용하세요:

```bash
# 의존성 설치
npm install

# 개발 서버 (기본)
npm run dev

# HTTPS로 실행이 필요한 경우
npm run dev -- --https

# 프로덕션 빌드
npm run build

# 빌드 결과 프리뷰
npm run preview

# 단위 테스트 실행
npm run test
```

- 브라우저 권장: Chromium 기반(Chrome, Edge) — HTTPS 또는 `localhost` 환경 권장

## 사용한 라이브러리(주요)

| 라이브러리                      | 용도                         |      버전 |
| ------------------------------- | ---------------------------- | --------: |
| `svelte`                        | UI 프레임워크                | `^5.41.0` |
| `@sveltejs/kit`                 | SvelteKit 앱 런타임 / 라우팅 | `^2.47.1` |
| `vite`                          | 번들러 / 개발 서버           | `^7.1.10` |
| `monaco-editor`                 | 코드 편집기(에디터)          | `^0.53.0` |
| `vite-plugin-monaco-editor-esm` | Monaco 번들링 최적화         |  `^2.0.2` |
| `@testing-library/svelte`       | 컴포넌트 테스트 유틸리티     |  `^5.2.8` |
| `playwright`                    | E2E / 브라우저 테스트        | `^1.56.1` |
| `typescript`                    | 정적 타입 검사 및 빌드       |  `^5.9.3` |
| `prettier`                      | 코드 포맷터                  |  `^3.6.2` |
| `eslint`                        | 린팅 도구                    | `^9.38.0` |

## 폴더 구조 (주요 파일/폴더)

- `src/` : 애플리케이션 소스
  **`src/` 폴더 구조 (주요 항목)**

| 경로            | 설명                                     |
| --------------- | ---------------------------------------- |
| `src/`          | 애플리케이션 소스 루트                   |
| `src/apis/`     | 파일, 폴더, 워크스페이스 등의 API 레이어 |
| `src/features/` | 페이지별 기능 컴포넌트 및 대화상자       |
| `src/lib/`      | 재사용 가능한 UI 컴포넌트 및 레이아웃    |
| `src/routes/`   | SvelteKit 라우트 및 페이지               |
| `src/stores/`   | 애플리케이션 상태 관리(Store)            |
| `src/utils/`    | 유틸리티 및 Helper                       |

## 기타 유의사항

- `monaco-editor` 관련 설정은 `vite-plugin-monaco-editor-esm`을 사용해 번들링 최적화를 적용했습니다.
- 코드 포맷은 `prettier`와 `prettier-plugin-svelte`를 따릅니다. `npm run format`로 자동 포맷하세요.
