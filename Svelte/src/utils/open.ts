// // src/utils/openDirectory.ts
// import { writable, get } from 'svelte/store';
// import { _listWorkspaces } from '@/apis/workspace';

// // 다이얼로그가 사용할 상태 (workspaces + resolve 콜백만)
// type DialogState =
//   | {
//       workspaces: any[];                        // res.data.workspaces
//       resolve: (result: any | null) => void;    // { metadata, tree } 또는 null
//     }
//   | null;

// export const openDirectoryDialog = writable<DialogState>(null);

// /**
//  * openDirectory()
//  *  - 호출하면: 워크스페이스 목록을 불러와서 다이얼로그에 넘기고
//  *  - 다이얼로그에서 선택 + 확인할 때까지 기다렸다가
//  *  - { metadata, tree } 또는 null을 반환
//  */
// export async function openDirectory(): Promise<any | null> {
//   const res: any = await _listWorkspaces();

//   if (!res.success) {
//     throw new Error('워크스페이스 목록 조회 실패');
//   }

//   const workspaces = res.data.workspaces;
//   if (!Array.isArray(workspaces) || workspaces.length === 0) {
//     throw new Error('워크스페이스가 없습니다.');
//   }

//   // 다이얼로그 띄우고, 유저 선택이 끝날 때까지 기다림
//   return new Promise((resolve) => {
//     openDirectoryDialog.set({
//       workspaces,
//       resolve
//     });
//   });
// }

// /** 다이얼로그에서 최종 결과를 반환(확인/취소 공용) */
// export function finishOpenDirectory(result: any | null) {
//   const state = get(openDirectoryDialog);
//   if (!state) return;

//   state.resolve(result);      // openDirectory()의 Promise 깨우기
//   openDirectoryDialog.set(null); // 다이얼로그 상태 초기화
// }
