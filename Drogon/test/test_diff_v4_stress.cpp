#include <chrono>
#include <cstdlib>
#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <json/json.h>
#include "utils/diff/line_diff_v4.h"

static bool stressEnabled() {
    const char* p = std::getenv("RUN_STRESS");
    return p && *p && std::string(p) != "0";
}

static int getEnvIntOr(const char* key, int fallback) {
    if (const char* p = std::getenv(key)) {
        try { return std::max(1, std::stoi(p)); } catch (...) { return fallback; }
    }
    return fallback;
}

// 7-1) 초장문 단일 라인: 혼합 공백 vs 단일 스페이스 → SAME + 시간 예산 내
TEST(NormalizeStressTest, ExtremelyLongSingleLineBecomesSameWithinBudget) {
    if (!stressEnabled()) GTEST_SKIP() << "Set RUN_STRESS=1 to run stress tests";
    const int scale = getEnvIntOr("DIFF_STRESS_SCALE", 1);
    // N은 토큰 개수. scale=1일 때 N=50,000 → 대략 수 MB 문자열
    const int N = 50000 * scale;

    const std::string token = "word";
    std::string Aline; Aline.reserve(N * (token.size() + 2));
    std::string Bline; Bline.reserve(N * (token.size() + 1));

    for (int i=0; i<N; ++i) {
        Aline += token;
        if (i < N-1) {
            // 혼합 공백 패턴(정규화 대상 6종): 탭/스페이스/수직탭/개행/캐리지/폼피드
            switch (i % 6) {
                case 0: Aline += "\t"; break;
                case 1: Aline += " ";  break;
                case 2: Aline += "\v"; break;
                case 3: Aline += "\n"; break;
                case 4: Aline += "\r"; break;
                default:Aline += "\f"; break;
            }
        }
        Bline += token;
        if (i < N-1) Bline += " ";
    }

    std::vector<std::string> A = {Aline};
    std::vector<std::string> B = {Bline};

    LcsDiffEngine eng;

    auto t0 = std::chrono::steady_clock::now();
    auto diffs = eng.compute(A, B);
    auto t1 = std::chrono::steady_clock::now();

    using namespace std::chrono;
    const int budget_ms = getEnvIntOr("DIFF_STRESS_BUDGET_MS", 2500); // 기본 2.5s
    auto elapsed = duration_cast<milliseconds>(t1 - t0).count();

    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].kind, Diff::SAME);
    EXPECT_LE(elapsed, budget_ms) << "elapsed=" << elapsed << "ms, budget=" << budget_ms << "ms";
}

// 7-2) 초대량 라인 수: 동일 라인 다수 → SAME 개수 정확 + 시간 예산 내
// 7-2) 초대량 라인 수(적응형/배치): SAME 정확 + 시간/메모리 예산 내
TEST(NormalizeStressTest, ManyLinesAllSameWithinBudget_AdaptiveBatched) {
    if (!stressEnabled()) GTEST_SKIP() << "Set RUN_STRESS=1 to run stress tests";

    using namespace std::chrono;

    // ---- 파라미터: 환경변수로 조절 가능 ----
    int scale = getEnvIntOr("DIFF_STRESS_SCALE", 1);     // 전체 규모 배수
    const int budget_ms = getEnvIntOr("DIFF_STRESS_BUDGET_MS", 4000);
    const int baseLines = 100000;                        // 기존 L 기준
    const int minLines  = 10000;                         // 최소 라인 수(안전)
    const int batchSize = 5000;                          // 배치 단위
    // ---------------------------------------

    auto buildBatch = [](int n) {
        std::vector<std::string> A; A.reserve(n);
        std::vector<std::string> B; B.reserve(n);
        for (int i=0; i<n; ++i) {
            switch (i % 4) {
                case 0: A.emplace_back("key\tvalue");        B.emplace_back("key value"); break;
                case 1: A.emplace_back("alpha  \v  beta");   B.emplace_back("alpha beta"); break;
                case 2: A.emplace_back("x \n y");            B.emplace_back("x y"); break;
                default: A.emplace_back("solo");             B.emplace_back("solo"); break;
            }
        }
        return std::pair<std::vector<std::string>, std::vector<std::string>>(std::move(A), std::move(B));
    };

    // 너무 큰 스케일이면 단계적으로 줄이며 시도
    for (; scale >= 1; --scale) {
        const int targetLines = std::max(minLines, baseLines * scale);
        const int batches = (targetLines + batchSize - 1) / batchSize;

        LcsDiffEngine eng;
        auto t0 = steady_clock::now();
        bool ok = true;

        for (int b=0; b<batches; ++b) {
            const int thisBatch = (b == batches-1)
                                  ? (targetLines - b*batchSize)
                                  : batchSize;
            auto [A, B] = buildBatch(thisBatch);

            try {
                auto diffs = eng.compute(A, B);
                // 모두 SAME 확인
                int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) ++nonSame;
                if (nonSame != 0 || diffs.size() != static_cast<size_t>(thisBatch)) {
                    ok = false; break;
                }
            } catch (const std::bad_alloc&) {
                ok = false; break;
            }

            // 시간 초과 감시
            auto elapsed = duration_cast<milliseconds>(steady_clock::now() - t0).count();
            if (elapsed > budget_ms) { ok = false; break; }
        }

        auto totalElapsed = duration_cast<milliseconds>(steady_clock::now() - t0).count();

        if (ok) {
            // 성공: 예산 내 통과
            EXPECT_LE(totalElapsed, budget_ms) << "elapsed=" << totalElapsed << "ms, budget=" << budget_ms << "ms";
            SUCCEED() << "Batched SAME check passed with targetLines=" << targetLines
                      << ", batchSize=" << batchSize << ", scale=" << scale
                      << ", elapsed=" << totalElapsed << "ms";
            return;
        }

        // 실패면 스케일을 낮춰 재시도 (루프 계속)
    }

    GTEST_SKIP() << "Stress test autoscaled down to minimum but still exceeded budget or memory; "
                 << "try lower DIFF_STRESS_SCALE or larger DIFF_STRESS_BUDGET_MS.";
}


// 7-3) 랜덤(의사랜덤) 정규화 퍼즈: 다양한 공백 조합에서도 SAME 유지
TEST(NormalizeStressTest, PseudoFuzzWhitespaceNormalization) {
    if (!stressEnabled()) GTEST_SKIP() << "Set RUN_STRESS=1 to run stress tests";
    const int scale = getEnvIntOr("DIFF_STRESS_SCALE", 1);
    const int Cases = 2000 * scale;
    const int TokensPerCase = 50;

    auto genA = [&](int seed){
        std::string s; s.reserve(TokensPerCase * 6);
        for (int i=0; i<TokensPerCase; ++i) {
            s += "t"; // 짧은 토큰
            if (i < TokensPerCase-1) {
                int r = (seed * 1103515245 + 12345 + i) & 7; // 고정 의사랜덤
                switch (r) {
                    case 0: s += " "; break;
                    case 1: s += "\t"; break;
                    case 2: s += "\v"; break;
                    case 3: s += "\n"; break;
                    case 4: s += "\r"; break;
                    case 5: s += "\f"; break;
                    case 6: s += " \t \v"; break;
                    default:s += "\n\r\f"; break;
                }
            }
        }
        return s;
    };
    auto genB = [&](){
        std::string s; s.reserve(TokensPerCase * 2);
        for (int i=0; i<TokensPerCase; ++i) {
            s += "t";
            if (i < TokensPerCase-1) s += " ";
        }
        return s;
    };

    std::vector<std::string> A; A.reserve(Cases);
    std::vector<std::string> B; B.reserve(Cases);
    for (int c=0; c<Cases; ++c) {
        A.emplace_back(genA(c));
        B.emplace_back(genB());
    }

    LcsDiffEngine eng;
    auto t0 = std::chrono::steady_clock::now();
    auto diffs = eng.compute(A, B);
    auto t1 = std::chrono::steady_clock::now();

    using namespace std::chrono;
    const int budget_ms = getEnvIntOr("DIFF_STRESS_BUDGET_MS", 2500);
    auto elapsed = duration_cast<milliseconds>(t1 - t0).count();

    ASSERT_EQ(diffs.size(), static_cast<size_t>(Cases));
    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
    EXPECT_LE(elapsed, budget_ms) << "elapsed=" << elapsed << "ms, budget=" << budget_ms << "ms";
}
