// #include <algorithm>
// #include <cctype>
// #include <fstream>
// #include <iomanip>
// #include <iostream>
// #include <sstream>
// #include <stdexcept>
// #include <string>
// #include <unordered_map>
// #include <utility>
// #include <vector>

// #include <chrono>

// using namespace std;

// // 공통 유틸
// //  - 개행 제거
// //  - 출력 글자수 제한 (개발단계)

// static inline string rstrip_cr(string s)
// {
//     if (!s.empty() && s.back() == '\r')
//         s.pop_back();
//     return s;
// }

// static inline string ellipsis(const string &s, size_t maxw)
// {
//     if (s.size() <= maxw)
//         return s;
//     if (maxw <= 1)
//         return "…";
//     return s.substr(0, maxw - 1) + "…";
// }

// // 비교 결과를 저장할 구조체
// //  - base, compare : line text
// //  - Kind : 비교 타입

// struct Diff
// {
//     string base, compare;
//     int baseLine = -1;
//     int compareLine = -1;
//     enum Kind
//     {
//         SAME,
//         ADD,
//         DEL,
//         MOD
//     } kind;
// };

// //  DiffEngine
// //
// // - 파일 로딩
// // - 정규화/인터닝(문장 정수 변환)
// // - 접두/접미 트림(앞뒤 같은 문자 제외)
// // - 패이션스 앵커 + 히스토그램 앵커(같은 패턴을 확인 후 기준 설정)
// // - Divide&Conquer Myers + 소구간 LCS 폴백
// // - DEL/ADD → MOD 접기
// class DiffEngine
// {
// public:
//     vector<Diff> run(const string &basePath, const string &comparePath, bool ignoreSpace)
//     {
//         reset();
//         ignoreSpace_ = ignoreSpace;

//         // 0) 원본 라인 읽기
//         ifstream fa(basePath), fb(comparePath);
//         if (!fa)
//             throw runtime_error("Cannot open " + basePath);
//         if (!fb)
//             throw runtime_error("Cannot open " + comparePath);

//         {
//             string line;
//             while (getline(fa, line))
//                 baseRaw_.push_back(rstrip_cr(line));
//             while (getline(fb, line))
//                 compareRaw_.push_back(rstrip_cr(line));
//         }
//         baseSize_ = (int)baseRaw_.size();
//         compareSize_ = (int)compareRaw_.size();

//         // 1) 정규화 + 라인 해싱(인터닝)
//         intern_.reserve((size_t)(baseSize_ + compareSize_) * 2);
//         Ai_.assign(baseSize_, 0);
//         Bi_.assign(compareSize_, 0);

//         for (int i = 0; i < baseSize_; ++i)
//         {
//             string t = normalizeLine_(baseRaw_[i]);
//             Ai_[i] = internId_(t);
//         }
//         for (int j = 0; j < compareSize_; ++j)
//         {
//             string t = normalizeLine_(compareRaw_[j]);
//             Bi_[j] = internId_(t);
//         }

//         // 2) 접두/접미 트림
//         int a0 = 0, b0 = 0;
//         while (a0 < baseSize_ && b0 < compareSize_ && Ai_[a0] == Bi_[b0])
//         {
//             ++a0;
//             ++b0;
//         }
//         int a1 = baseSize_, b1 = compareSize_;
//         while (a1 > a0 && b1 > b0 && Ai_[a1 - 1] == Bi_[b1 - 1])
//         {
//             --a1;
//             --b1;
//         }
//         if (a0 == baseSize_ && b0 == compareSize_)
//         {
//             vector<Diff> diffs;
//             diffs.reserve(baseSize_);
//             for (int i = 0; i < baseSize_; ++i)
//                 diffs.push_back({baseRaw_[i], compareRaw_[i], Diff::SAME});
//             return diffs;
//         }

//         // SAME/ADD/DEL를 담아둘 임시 연산 목록
//         // (공통 접두 + 접미) + add가능 + del가능
//         ops_.clear();
//         ops_.reserve((a0 + (baseSize_ - a1)) + (a1 - a0) + (b1 - b0));

//         // 접두 SAME
//         for (int i = 0; i < a0; ++i)
//             ops_.push_back({Op::SAME, i, i});

//         // 3) 패이션스 앵커(양쪽에서 유니크 라인 기반 → LIS)
//         vector<pair<int, int>> anchors = buildAnchors_(a0, a1, b0, b1);
//         constexpr size_t kMinAnchors = 2; // 임계값은 취향/데이터에 맞게 조정
//         auto hist = buildAnchorsHistogram_(a0, a1, b0, b1);
//         if (anchors.size() < kMinAnchors && hist.size() > anchors.size())
//         {
//             anchors = std::move(hist);
//         }

//         // 4) Divide&Conquer Myers (앵커로 구간 분할)
//         int pa = a0, pb = b0;
//         for (auto &anc : anchors)
//         {
//             int qa = anc.first;  // in [a0,a1)
//             int qb = anc.second; // in [b0,b1)
//             if (qa < pa || qb < pb || qa < a0 || qa >= a1 || qb < b0 || qb >= b1)
//                 continue;
//             if (pa < qa || pb < qb)
//             {
//                 dcMyers_(pa, qa, pb, qb); // 좌측 구간
//             }
//             ops_.push_back({Op::SAME, qa, qb}); // 앵커 자체는 SAME
//             pa = qa + 1;
//             pb = qb + 1;
//         }
//         // 마지막 구간
//         if (pa < a1 || pb < b1)
//             dcMyers_(pa, a1, pb, b1);

//         // 접미 SAME
//         for (int i = a1, j = b1; i < baseSize_ && j < compareSize_; ++i, ++j)
//             ops_.push_back({Op::SAME, i, j});

//         // 5) Op → Diff + DEL/ADD → MOD 접기
//         return foldOpsToDiffs_();
//     }

// private:
//     // 내부 상태
//     struct Op
//     {
//         enum K
//         {
//             SAME,
//             ADD,
//             DEL
//         };
//         K k;
//         int ai, bj;
//     };

    
//     vector<string> baseRaw_, compareRaw_;
//     vector<int> Ai_, Bi_;
//     vector<Op> ops_;
//     unordered_map<string, int> intern_;
//     bool ignoreSpace_ = false;
//     int baseSize_ = 0, compareSize_ = 0;

//     void reset()
//     {
//         ignoreSpace_ = false;
//         baseRaw_.clear();
//         compareRaw_.clear();
//         baseSize_ = compareSize_ = 0;
//         intern_.clear();
//         Ai_.clear();
//         Bi_.clear();
//         ops_.clear();
//     }

//     // 공백 무시 정규화 ignoreSpace 입력 시
//     string normalizeLine_(const string &s) const
//     {
//         if (!ignoreSpace_)
//             return s;
//         bool allSpace = true;
//         for (unsigned char c : s)
//         {
//             if (!isspace(c))
//             {
//                 allSpace = false;
//                 break;
//             }
//         }
//         if (allSpace)
//             return string();

//         string t;
//         t.reserve(s.size());
//         bool inSpace = false;
//         size_t i = 0, j = s.size();
//         while (i < j && isspace((unsigned char)s[i]))
//             i++;
//         while (j > i && isspace((unsigned char)s[j - 1]))
//             j--;
//         for (; i < j; ++i)
//         {
//             unsigned char c = (unsigned char)s[i];
//             if (isspace(c))
//             {
//                 if (!inSpace)
//                 {
//                     t.push_back(' ');
//                     inSpace = true;
//                 }
//             }
//             else
//             {
//                 t.push_back((char)c);
//                 inSpace = false;
//             }
//         }
//         return t;
//     }

//     // 문자열 인터닝 → 정수 id
//     int internId_(const string &s)
//     {
//         int next_id = (int)intern_.size() + 1;
//         auto [it, inserted] = intern_.try_emplace(s, next_id);
//         return it->second;
//     }

//     // 2-1) 간단 토큰화: [a-zA-Z0-9_] 연속을 토큰으로, 그 외는 분리자
//     static inline vector<string> tokenizeWords_(const string &s)
//     {
//         vector<string> out;
//         string cur;
//         cur.reserve(16);
//         auto flush = [&]()
//         {
//             if (!cur.empty())
//             {
//                 out.push_back(cur);
//                 cur.clear();
//             }
//         };
//         for (unsigned char ch : s)
//         {
//             if (std::isalnum(ch) || ch == '_')
//                 cur.push_back(char(ch));
//             else
//                 flush();
//         }
//         flush();
//         return out;
//     }

//     // 2-2) 히스토그램 기반 앵커 빌더
//     vector<pair<int, int>> buildAnchorsHistogram_(int a0, int a1, int b0, int b1) const
//     {
//         // ① 양쪽 구간의 토큰 빈도(합산) 계산
//         unordered_map<string, int> freq;
//         freq.reserve(size_t((a1 - a0) + (b1 - b0)) * 4);

//         auto countSide = [&](int l0, int l1, const vector<string> &raw)
//         {
//             for (int i = l0; i < l1; ++i)
//             {
//                 for (const auto &t : tokenizeWords_(raw[i]))
//                 {
//                     if (t.size() <= 1)
//                         continue;
//                     ++freq[t];
//                 }
//             }
//         };
//         countSide(a0, a1, baseRaw_);
//         countSide(b0, b1, compareRaw_);

//         // ② 라인 점수: 희소 토큰일수록 가중치 ↑ (Σ 1/freq(token))
//         auto lineScore = [&](const vector<string> &raw, int idx) -> double
//         {
//             double s = 0.0;
//             for (const auto &t : tokenizeWords_(raw[idx]))
//             {
//                 if (t.size() <= 1)
//                     continue;
//                 auto it = freq.find(t);
//                 if (it == freq.end())
//                     continue;
//                 int f = it->second;
//                 if (f > 0)
//                     s += 1.0 / double(f);
//             }
//             return s;
//         };

//         struct Cand
//         {
//             int pos;
//             double score;
//         };

//         vector<Cand> candA;
//         candA.reserve(size_t(a1 - a0));
//         vector<Cand> candB;
//         candB.reserve(size_t(b1 - b0));

//         for (int i = a0; i < a1; ++i)
//         {
//             double sc = lineScore(baseRaw_, i);
//             if (sc > 0.0)
//                 candA.push_back({i, sc});
//         }
//         for (int j = b0; j < b1; ++j)
//         {
//             double sc = lineScore(compareRaw_, j);
//             if (sc > 0.0)
//                 candB.push_back({j, sc});
//         }
//         if (candA.empty() || candB.empty())
//             return {};

//         // ③ 상위 퍼센타일만 유지(노이즈 컷)
//         auto keepTopPercent = [&](vector<Cand> &v, double topPct)
//         {
//             if (v.empty())
//                 return;
//             size_t k = std::max<size_t>(1, static_cast<size_t>(v.size() * topPct));
//             std::nth_element(v.begin(), v.begin() + k - 1, v.end(),
//                              [](const Cand &x, const Cand &y)
//                              { return x.score > y.score; });
//             v.resize(k);
//             std::sort(v.begin(), v.end(), [](auto &x, auto &y)
//                       { return x.pos < y.pos; });
//         };
//         constexpr double kTopPct = 0.25; // 25%만 유지 (0.1~0.3 사이 튜닝 추천)
//         keepTopPercent(candA, kTopPct);
//         keepTopPercent(candB, kTopPct);

//         // ④ “정규화된 라인”(Ai_/Bi_) 버킷으로 묶고, 가까운 쌍을 그리디 매칭
//         //    ── 변경 포인트: unordered_map → vector<vector<int>> (성능/안정성 ↑)
//         int K = (int)intern_.size() + 2;
//         vector<vector<int>> bucketA(K), bucketB(K);
//         bucketA.reserve(candA.size());
//         bucketB.reserve(candB.size());

//         for (auto &c : candA)
//             bucketA[Ai_[c.pos]].push_back(c.pos);
//         for (auto &c : candB)
//             bucketB[Bi_[c.pos]].push_back(c.pos);

//         vector<pair<int, int>> pairs;
//         pairs.reserve(std::min(candA.size(), candB.size()));

//         for (int id = 0; id < K; ++id)
//         {
//             auto &va = bucketA[id]; // A positions
//             auto &vb = bucketB[id]; // B positions
//             if (va.empty() || vb.empty())
//                 continue;
//             std::sort(va.begin(), va.end());
//             std::sort(vb.begin(), vb.end());
//             size_t i = 0, j = 0;
//             while (i < va.size() && j < vb.size())
//             {
//                 int pa = va[i], pb = vb[j];
//                 pairs.emplace_back(pa, pb);
//                 // 대략 위치 유사 쪽으로 전진
//                 if (pa < pb)
//                     ++i;
//                 else
//                     ++j;
//             }
//         }

//         // A-순서 보장 (LIS 전 정렬)
//         std::sort(pairs.begin(), pairs.end(),
//                   [](const auto &a, const auto &b)
//                   { return a.first < b.first; });
//         if (pairs.empty())
//             return {};

//         // ⑤ posB 기준 LIS로 순서 보존되는 앵커만 남김(패이션스)
//         vector<int> tails, tail_idx, prev_idx(pairs.size(), -1);
//         tails.reserve(pairs.size());
//         tail_idx.reserve(pairs.size());
//         for (int i = 0; i < (int)pairs.size(); ++i)
//         {
//             int val = pairs[i].second;
//             auto it = std::lower_bound(tails.begin(), tails.end(), val);
//             int k = int(it - tails.begin());
//             if (it == tails.end())
//             {
//                 tails.push_back(val);
//                 tail_idx.push_back(i);
//             }
//             else
//             {
//                 *it = val;
//                 tail_idx[k] = i;
//             }
//             if (k > 0)
//                 prev_idx[i] = tail_idx[k - 1];
//         }
//         vector<pair<int, int>> anchors;
//         if (!tails.empty())
//         {
//             int p = tail_idx.back();
//             while (p != -1)
//             {
//                 anchors.push_back(pairs[p]);
//                 p = prev_idx[p];
//             }
//             std::reverse(anchors.begin(), anchors.end());
//         }
//         return anchors;
//     }

//     // 앵커 구성
//     vector<pair<int, int>> buildAnchors_(int a0, int a1, int b0, int b1) const
//     {
//         vector<int> freqA(intern_.size() + 2, 0), freqB(intern_.size() + 2, 0);

//         for (int i = a0; i < a1; ++i)
//             ++freqA[Ai_[i]];
//         for (int j = b0; j < b1; ++j)
//             ++freqB[Bi_[j]];

//         // intern_.size() = 라인 고유 ID 최대값 (1..K)
//         // id는 Ai_, Bi_에서 1 이상 값으로 이미 채번돼 있음
//         int K = (int)intern_.size() + 2; // 안전하게 +2 여유

//         // posA, posB를 벡터로 초기화 (-1은 미존재 의미)
//         vector<int> posA(K, -1);
//         vector<int> posB(K, -1);

//         // 1) base(A) 쪽 유니크 라인 위치 기록
//         for (int i = a0; i < a1; ++i)
//         {
//             int id = Ai_[i];
//             if (freqA[id] == 1)
//                 posA[id] = i; // id → 해당 라인의 위치
//         }

//         // 2) compare(B) 쪽 유니크 라인 위치 기록
//         for (int j = b0; j < b1; ++j)
//         {
//             int id = Bi_[j];
//             if (freqB[id] == 1)
//                 posB[id] = j;
//         }

//         // 3) 양쪽 모두 유니크한 라인 쌍 찾기 (A 순서 보존)
//         vector<pair<int, int>> pairs;
//         pairs.reserve(min(a1 - a0, b1 - b0));

//         for (int i = a0; i < a1; ++i)
//         {
//             int id = Ai_[i];
//             if (freqA[id] == 1)
//             {
//                 int j = posB[id];
//                 if (j != -1)
//                     pairs.emplace_back(i, j);
//             }
//         }

//         // LIS on posB
//         vector<int> tails, tail_idx, prev_idx(pairs.size(), -1);
//         tails.reserve(pairs.size());
//         tail_idx.reserve(pairs.size());

//         for (int i = 0; i < (int)pairs.size(); ++i)
//         {
//             int val = pairs[i].second;
//             auto it = lower_bound(tails.begin(), tails.end(), val);
//             int k = int(it - tails.begin());
//             if (it == tails.end())
//             {
//                 tails.push_back(val);
//                 tail_idx.push_back(i);
//             }
//             else
//             {
//                 *it = val;
//                 tail_idx[k] = i;
//             }
//             if (k > 0)
//                 prev_idx[i] = tail_idx[k - 1];
//         }

//         vector<pair<int, int>> anchors;
//         if (!tails.empty())
//         {
//             int p = tail_idx.back();
//             while (p != -1)
//             {
//                 anchors.push_back(pairs[p]);
//                 p = prev_idx[p];
//             }
//             reverse(anchors.begin(), anchors.end());
//         }
//         return anchors;
//     }

//     // 작은 구간 LCS 폴백 (ops_에 바로 push)
//     void dpFallback_(int ax, int ay, int bx, int by)
//     {
//         int n = ay - ax, m = by - bx;
//         if (n == 0)
//         {
//             for (int j = 0; j < m; ++j)
//                 ops_.push_back({Op::ADD, -1, bx + j});
//             return;
//         }
//         if (m == 0)
//         {
//             for (int i = 0; i < n; ++i)
//                 ops_.push_back({Op::DEL, ax + i, -1});
//             return;
//         }

//         vector<vector<int>> dp(n + 1, vector<int>(m + 1, 0));
//         for (int i = 1; i <= n; ++i)
//             for (int j = 1; j <= m; ++j)
//                 dp[i][j] =
//                     (Ai_[ax + i - 1] == Bi_[bx + j - 1]) ? dp[i - 1][j - 1] + 1 : max(dp[i - 1][j], dp[i][j - 1]);

//         vector<Op> tmp;
//         tmp.reserve(n + m);
//         int i = n, j = m;
//         while (i > 0 || j > 0)
//         {
//             if (i > 0 && j > 0 && Ai_[ax + i - 1] == Bi_[bx + j - 1])
//             {
//                 tmp.push_back({Op::SAME, ax + i - 1, bx + j - 1});
//                 --i;
//                 --j;
//             }
//             else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j]))
//             {
//                 tmp.push_back({Op::ADD, -1, bx + j - 1});
//                 --j;
//             }
//             else
//             {
//                 tmp.push_back({Op::DEL, ax + i - 1, -1});
//                 --i;
//             }
//         }
//         for (int t = (int)tmp.size() - 1; t >= 0; --t)
//             ops_.push_back(tmp[t]);
//     }

//     // Divide & Conquer Myers (선형 메모리), ops_에 push
//     void dcMyers_(int ax, int ay, int bx, int by)
//     {
//         const int n = ay - ax, m = by - bx;
//         if (n == 0)
//         {
//             for (int j = 0; j < m; ++j)
//                 ops_.push_back({Op::ADD, -1, bx + j});
//             return;
//         }
//         if (m == 0)
//         {
//             for (int i = 0; i < n; ++i)
//                 ops_.push_back({Op::DEL, ax + i, -1});
//             return;
//         }

//         // 소구간 빠른 처리(세이프가드)
//         if (n <= 32 || m <= 32)
//         {
//             dpFallback_(ax, ay, bx, by);
//             return;
//         }

//         const int delta = n - m;
//         const bool odd = (delta & 1);
//         const int Dmax = (n + m + 1) / 2;

//         const int OFF = Dmax + 1;
//         vector<int> Vf(2 * Dmax + 3, -1), Vr(2 * Dmax + 3, -1);
//         Vf[OFF] = 0;
//         Vr[OFF] = 0;

//         int x_split_f = 0, y_split_f = 0, x_split_r = 0, y_split_r = 0;
//         bool found = false;

//         for (int d = 0; d <= Dmax && !found; ++d)
//         {
//             // forward
//             for (int k = -d; k <= d; k += 2)
//             {
//                 int idx = k + OFF;
//                 int x;
//                 if (k == -d || (k != d && Vf[idx - 1] < Vf[idx + 1]))
//                     x = Vf[idx + 1];
//                 else
//                     x = Vf[idx - 1] + 1;
//                 int y = x - k;
//                 while (x < n && y < m && Ai_[ax + x] == Bi_[bx + y])
//                 {
//                     ++x;
//                     ++y;
//                 }
//                 Vf[idx] = x;

//                 if (odd && (k >= delta - d) && (k <= delta + d))
//                 {
//                     int kr = k - delta;
//                     int idxr = kr + OFF;
//                     if (Vr[idxr] != -1 && Vf[idx] + Vr[idxr] >= n)
//                     {
//                         x_split_f = x;
//                         y_split_f = y;
//                         x_split_r = n - Vr[idxr];
//                         y_split_r = m - (Vr[idxr] - kr);
//                         found = true;
//                         break;
//                     }
//                 }
//             }
//             if (found)
//                 break;

//             // reverse
//             for (int k = -d; k <= d; k += 2)
//             {
//                 int idx = k + OFF;
//                 int x;
//                 if (k == -d || (k != d && Vr[idx + 1] < Vr[idx - 1]))
//                     x = Vr[idx + 1];
//                 else
//                     x = Vr[idx - 1] + 1;
//                 int y = x - k;
//                 while (x < n && y < m && Ai_[ay - 1 - x] == Bi_[by - 1 - y])
//                 {
//                     ++x;
//                     ++y;
//                 }
//                 Vr[idx] = x;

//                 if (!odd)
//                 {
//                     int kf = k + delta;
//                     int idxf = kf + OFF;
//                     if (Vf[idxf] != -1 && Vf[idxf] + Vr[idx] >= n)
//                     {
//                         x_split_f = Vf[idxf];
//                         y_split_f = x_split_f - kf;
//                         x_split_r = n - Vr[idx];
//                         y_split_r = m - (Vr[idx] - k);
//                         found = true;
//                         break;
//                     }
//                 }
//             }
//         }

//         if (!found)
//         {
//             dpFallback_(ax, ay, bx, by);
//             return;
//         }

//         // 중앙 스네이크 길이
//         int snake = 0;
//         while (x_split_f + snake < x_split_r && y_split_f + snake < y_split_r &&
//                Ai_[ax + x_split_f + snake] == Bi_[bx + y_split_f + snake])
//         {
//             ++snake;
//         }

//         // 좌측
//         dcMyers_(ax, ax + x_split_f, bx, bx + y_split_f);
//         // 스네이크(같은 부분)
//         for (int t = 0; t < snake; ++t)
//             ops_.push_back({Op::SAME, ax + x_split_f + t, bx + y_split_f + t});
//         // 우측
//         dcMyers_(ax + x_split_f + snake, ay, bx + y_split_f + snake, by);
//     }

//     // Op 시퀀스를 최종 Diff로 변환 + DEL/ADD → MOD 접기
//     vector<Diff> foldOpsToDiffs_() const
//     {
//         vector<Diff> diffs;
//         diffs.reserve(ops_.size());

//         for (size_t i = 0; i < ops_.size();)
//         {
//             if (ops_[i].k == Op::SAME)
//             {
//                 diffs.push_back({baseRaw_[ops_[i].ai], compareRaw_[ops_[i].bj], ops_[i].ai, ops_[i].bj, Diff::SAME});
//                 ++i;
//                 continue;
//             }
//             size_t j = i;
//             vector<size_t> delIdx, addIdx;
//             while (j < ops_.size() && ops_[j].k != Op::SAME)
//             {
//                 if (ops_[j].k == Op::DEL)
//                     delIdx.push_back(j);
//                 else
//                     addIdx.push_back(j);
//                 ++j;
//             }
//             size_t pair = min(delIdx.size(), addIdx.size());
//             for (size_t t = 0; t < pair; ++t)
//             {
//                 auto &d1 = ops_[delIdx[t]], &d2 = ops_[addIdx[t]];
//                 diffs.push_back({baseRaw_[d1.ai], compareRaw_[d2.bj], d1.ai, d2.bj, Diff::MOD});
//             }
//             for (size_t t = pair; t < delIdx.size(); ++t)
//             {
//                 auto &d1 = ops_[delIdx[t]];
//                 diffs.push_back({baseRaw_[d1.ai], "", d1.ai, -1, Diff::DEL});
//             }
//             for (size_t t = pair; t < addIdx.size(); ++t)
//             {
//                 auto &d2 = ops_[addIdx[t]];
//                 diffs.push_back({"", compareRaw_[d2.bj], -1, d2.bj, Diff::ADD});
//             }
//             i = j;
//         }
//         return diffs;
//     }
// };

// // ──────────────────────────────
// // 출력 로직 (기존 그대로)
// // ──────────────────────────────
// static inline std::string __jsonEscape(const std::string &s)
// {
//     std::string out;
//     out.reserve(s.size() + 8);
//     for (unsigned char c : s)
//     {
//         switch (c)
//         {
//         case '\"':
//             out += "\\\"";
//             break;
//         case '\\':
//             out += "\\\\";
//             break;
//         case '\b':
//             out += "\\b";
//             break;
//         case '\f':
//             out += "\\f";
//             break;
//         case '\n':
//             out += "\\n";
//             break;
//         case '\r':
//             out += "\\r";
//             break;
//         case '\t':
//             out += "\\t";
//             break;
//         default:
//             if (c < 0x20)
//             {
//                 char buf[7];
//                 snprintf(buf, sizeof(buf), "\\u%04x", c);
//                 out += buf;
//             }
//             else
//                 out.push_back((char)c);
//         }
//     }
//     return out;
// }
// static inline std::string __filenameOnly(const std::string &path)
// {
//     size_t p = path.find_last_of("/\\");
//     return (p == std::string::npos) ? path : path.substr(p + 1);
// }
// static inline std::string __fileTypeFromExt(const std::string &path)
// {
//     std::string name = __filenameOnly(path);
//     size_t dot = name.find_last_of('.');
//     if (dot == std::string::npos || dot + 1 >= name.size())
//         return "";
//     std::string ext = name.substr(dot + 1);
//     for (auto &c : ext)
//         c = (char)std::tolower((unsigned char)c);
//     if (ext == "yml")
//         return "yaml";
//     return ext;
// }
// static inline const char *__kindToTypeStr(Diff::Kind k)
// {
//     switch (k)
//     {
//     case Diff::ADD:
//         return "added";
//     case Diff::DEL:
//         return "removed";
//     case Diff::MOD:
//         return "modified";
//     case Diff::SAME:
//         return "same";
//     }
//     return "unknown";
// }

// // --- JSON printer ---
// // 규칙:
// //  - Line 존재 여부는 d.baseLine / d.compareLine >= 0 로 판단
// //  - Value는 "존재하면 문자열(빈 문자열 포함)", 없으면 null
// //  - LineCount는 존재하면 1, 없으면 0
// void printDiff(const std::vector<Diff> &diffs,
//                const std::string &basePath,
//                const std::string &comparePath,
//                bool /*ignoreSpace*/)
// {
//     // 통계
//     size_t addCnt = 0, delCnt = 0, modCnt = 0;
//     for (const auto &d : diffs)
//     {
//         if (d.kind == Diff::ADD)
//             ++addCnt;
//         else if (d.kind == Diff::DEL)
//             ++delCnt;
//         else if (d.kind == Diff::MOD)
//             ++modCnt;
//     }
//     size_t total = addCnt + delCnt + modCnt;

//     // 파일 타입
//     std::string ftype = __fileTypeFromExt(comparePath);
//     if (ftype.empty())
//         ftype = __fileTypeFromExt(basePath);

//     auto printLineNum = [](int zeroBased)
//     {
//         if (zeroBased >= 0)
//             std::cout << (zeroBased + 1);
//         else
//             std::cout << "null";
//     };
//     auto printMaybeString = [](int lineIdx, const std::string &text)
//     {
//         if (lineIdx >= 0)
//             std::cout << "\"" << __jsonEscape(text) << "\""; // 빈 문자열도 ""로 출력
//         else
//             std::cout << "null";
//     };

//     // 출력 시작
//     std::cout << "{\n";
//     std::cout << "  \"base\": { \"name\": \"" << __jsonEscape(__filenameOnly(basePath)) << "\" },\n";
//     std::cout << "  \"compare\": { \"name\": \"" << __jsonEscape(__filenameOnly(comparePath)) << "\" },\n";

//     std::cout << "  \"statistics\": {\n";
//     std::cout << "    \"fileType\": " << (ftype.empty() ? "null" : ("\"" + __jsonEscape(ftype) + "\"")) << ",\n";
//     std::cout << "    \"added\": " << addCnt << ",\n";
//     std::cout << "    \"removed\": " << delCnt << ",\n";
//     std::cout << "    \"modified\": " << modCnt << ",\n";
//     std::cout << "    \"totalChanges\": " << total << "\n";
//     std::cout << "  },\n";

//     std::cout << "  \"changes\": [\n";
//     bool first = true;
//     for (const auto &d : diffs)
//     {
//         if (d.kind == Diff::SAME)
//             continue; // SAME은 changes에 안 넣음

//         if (!first)
//             std::cout << ",\n";
//         first = false;

//         const int baseExists = (d.baseLine >= 0) ? 1 : 0;
//         const int compExists = (d.compareLine >= 0) ? 1 : 0;

//         std::cout << "    {\n";
//         std::cout << "      \"type\": \"" << __kindToTypeStr(d.kind) << "\",\n";
//         std::cout << "      \"path\": null,\n";

//         std::cout << "      \"baseValue\": ";
//         printMaybeString(d.baseLine, d.base);
//         std::cout << ",\n";

//         std::cout << "      \"compareValue\": ";
//         printMaybeString(d.compareLine, d.compare);
//         std::cout << ",\n";

//         std::cout << "      \"baseLineNumber\": ";
//         printLineNum(d.baseLine);
//         std::cout << ",\n";

//         std::cout << "      \"baseLineCount\": " << baseExists << ",\n";

//         std::cout << "      \"compareLineNumber\": ";
//         printLineNum(d.compareLine);
//         std::cout << ",\n";

//         std::cout << "      \"compareLineCount\": " << compExists << "\n";
//         std::cout << "    }";
//     }
//     std::cout << "\n  ]\n";
//     std::cout << "}\n";
// }

// // ──────────────────────────────
// // main: 비교 호출 + 출력
// // ──────────────────────────────
// int main(int argc, char *argv[])
// {
//     auto start = std::chrono::high_resolution_clock::now();
//     ios::sync_with_stdio(false);
//     cin.tie(nullptr);

//     if (argc < 3)
//     {
//         cerr << "Usage: " << argv[0] << " <fileA> <fileB> [--ignore-space]\n";
//         return 1;
//     }

//     string basePath = argv[1];
//     string comparePath = argv[2];
//     bool ignoreSpace = (argc >= 4 && string(argv[3]) == "--ignore-space");

//     try
//     {
//         DiffEngine engine;
//         auto diffs = engine.run(basePath, comparePath, ignoreSpace);
//         printDiff(diffs, basePath, comparePath, ignoreSpace);

//         bool hasDiff = any_of(diffs.begin(), diffs.end(), [](const Diff &d)
//                               { return d.kind != Diff::SAME; });
//         auto end = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//         std::cout << "Execution time: " << duration.count() / 1000.0 << " milliseconds" << std::endl;
//         return hasDiff ? 10 : 0;
//     }
//     catch (const exception &e)
//     {
//         cerr << "Error: " << e.what() << "\n";
//         return 2;
//     }
// }
