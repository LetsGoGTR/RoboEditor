#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <json/json.h>
#include "utils/diff/line_diff_v4.h"

// ─────────────────────────────────────────────────────────────
// 1) DiffEngine : 생성자 테스트
// ─────────────────────────────────────────────────────────────



// ─────────────────────────────────────────────────────────────
// 1) DiffEngine : Normalize 테스트
// ─────────────────────────────────────────────────────────────

// 앞 뒤 공백 제거
TEST(NormalizeTest, TC001_RemoveLeadingAndTrailingWhitespace) {
    std::string pad = "\v\t \n\r\f";
    std::vector<std::string> A = { pad + "Token" + pad };
    std::vector<std::string> B = { "Token" };

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);
    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
}

// 서로 다른 공백들이 줄 사이에 여럿 있어도 하나로 축약되어야 함
TEST(NormalizeTest, TC002_CollapseInternalWhitespaceToSingleSpace) {
    std::vector<std::string> A = { std::string("Hi") + "\t \n\r\f\v" + "There" };
    std::vector<std::string> B = { "Hi There" };

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);
    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
}

// 6종 공백만 포함된 라인은 ""와 동일
TEST(NormalizeTest, TC003_ConvertWhitespaceOnlyToEmptyLine) {
    std::vector<std::string> A = { " \t\n\r\f\v " };
    std::vector<std::string> B = { "" };

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);
    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
}

// ─────────────────────────────────────────────────────────────
// 2) DiffEngine : compute 테스트 (LCS/변경 유형)
// ─────────────────────────────────────────────────────────────

// 모든 라인이 동일하면 SAME
TEST(DiffEngineTest, TC101_AllLinesIdentical_AllSAME) {
    std::vector<std::string> A = {"a","b","c"};
    std::vector<std::string> B = {"a","b","c"};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);
    ASSERT_EQ(diffs.size(), 3u);
    for (auto &d : diffs) EXPECT_EQ(d.kind, Diff::SAME);
}

// 일부 라인 수정만 있을 때 MOD 카운트 확인
TEST(DiffEngineTest, TC102_Detect_ModifiedLines_Only) {
    std::vector<std::string> A = {"alpha","beta","gamma"};
    std::vector<std::string> B = {"alpha","BETA","delta"};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    int add=0, del=0, mod=0, same=0;
    for (auto &d: diffs) {
        if (d.kind==Diff::ADD) add++;
        else if (d.kind==Diff::DEL) del++;
        else if (d.kind==Diff::MOD) mod++;
        else same++;
    }
    EXPECT_EQ(add, 0);
    EXPECT_EQ(del, 0);
    EXPECT_EQ(mod, 2);
    EXPECT_EQ(same, 1);
}

// 양쪽 모두 비어 있으면 결과도 비어야 함
TEST(DiffEngineTest, TC103_EmptyInputs_ReturnsEmptyDiff) {
    std::vector<std::string> A, B;
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);
    EXPECT_TRUE(diffs.empty());
}

// 한쪽이 전부 ADD만 있는 경우
TEST(DiffEngineTest, TC104_AllLinesAdded_ReturnsAddOnly) {
    std::vector<std::string> A = {};
    std::vector<std::string> B = {"a","b","c"};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    int add=0, del=0, mod=0, same=0;
    for (auto &d: diffs) {
        if (d.kind==Diff::ADD) add++;
        else if (d.kind==Diff::DEL) del++;
        else if (d.kind==Diff::MOD) mod++;
        else same++;
    }
    EXPECT_EQ(add, 3);
    EXPECT_EQ(del, 0);
    EXPECT_EQ(mod, 0);
    EXPECT_EQ(same, 0);
}

// 한쪽이 전부 DEL만 있는 경우
TEST(DiffEngineTest, TC105_AllLinesDeleted_ReturnsDelOnly) {
    std::vector<std::string> A = {"a","b","c"};
    std::vector<std::string> B = {};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    int add=0, del=0, mod=0, same=0;
    for (auto &d: diffs) {
        if (d.kind==Diff::ADD) add++;
        else if (d.kind==Diff::DEL) del++;
        else if (d.kind==Diff::MOD) mod++;
        else same++;
    }
    EXPECT_EQ(add, 0);
    EXPECT_EQ(del, 3);
    EXPECT_EQ(mod, 0);
    EXPECT_EQ(same, 0);
}

// 복합 패턴이 올바르게 분류되는지 확인: same → add → same → del → same → mod → same
TEST(DiffEngineTest, TC106_ComplexPattern_CorrectlyClassified) {
    std::vector<std::string> A = {
        "HEAD", "K1", "K2", "DEL_ONLY", "K3", "MOD_OLD", "K4", "TAIL1", "TAIL2"
    };
    std::vector<std::string> B = {
        "HEAD", "K1", "ADD_ONLY", "K2", "K3", "MOD_NEW", "K4", "TAIL1", "TAIL2"
    };

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    int add=0, del=0, mod=0, same=0;
    for (auto &d: diffs) {
        if (d.kind==Diff::ADD) add++;
        else if (d.kind==Diff::DEL) del++;
        else if (d.kind==Diff::MOD) mod++;
        else same++;
    }
    EXPECT_EQ(add, 1);
    EXPECT_EQ(del, 1);
    EXPECT_EQ(mod, 1);
    EXPECT_GT(same, 0);
}


// 긴 동일 블록 중 가운데 한 줄만 변경되었을 때 MOD가 1개만 발생해야 함
TEST(DiffEngineTest, TC107_SingleChangeInLargeBlock_OneModDetected) {
    std::vector<std::string> A, B;
    for (int i=0;i<100;i++) {
        A.push_back("line-" + std::to_string(i));
        B.push_back("line-" + std::to_string(i));
    }
    A[50] = "line-CHANGED";
    B[50] = "line-changed";

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    int mod=0, same=0;
    for (auto &d: diffs) {
        if (d.kind==Diff::MOD) mod++;
        else if (d.kind==Diff::SAME) same++;
    }
    EXPECT_EQ(mod, 1);
    EXPECT_EQ(same, 99);
}

// 중복 라인 해소: 가운데만 삭제되는지
TEST(DiffEngineTest, TC108_DuplicateLines_DeleteMiddleOnly) {
    std::vector<std::string> A = {"x","a","x"};
    std::vector<std::string> B = {"x","x"};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    int add=0, del=0, mod=0, same=0;
    for (auto &d: diffs) {
        if (d.kind==Diff::ADD) add++;
        else if (d.kind==Diff::DEL) del++;
        else if (d.kind==Diff::MOD) mod++;
        else same++;
    }
    EXPECT_EQ(add, 0);
    EXPECT_EQ(del, 1);
    EXPECT_EQ(mod, 0);
    EXPECT_EQ(same, 2);
}

// 대칭성: A→B의 DEL 수 == B→A의 ADD 수, A→B의 ADD 수 == B→A의 DEL 수
TEST(DiffEngineTest, TC109_AddDelSymmetry_Holds) {
    std::vector<std::string> A = {"k1","x","k2","z"};
    std::vector<std::string> B = {"k1","k2","y","z"};

    LcsDiffEngine eng;
    auto ab = eng.compute(A, B);
    auto ba = eng.compute(B, A);

    auto countKinds = [](const std::vector<Diff>& diffs){
        int add=0, del=0, mod=0, same=0;
        for (auto &d: diffs) {
            if (d.kind==Diff::ADD) add++;
            else if (d.kind==Diff::DEL) del++;
            else if (d.kind==Diff::MOD) mod++;
            else same++;
        }
        return std::tuple<int,int,int,int>(add,del,mod,same);
    };

    auto [addAB, delAB, modAB, sameAB] = countKinds(ab);
    auto [addBA, delBA, modBA, sameBA] = countKinds(ba);

    EXPECT_EQ(delAB, addBA);
    EXPECT_EQ(addAB, delBA);
}

// 매우 긴 라인 + 혼합 공백 정규화 후 SAME (성능/내성 스모크)
TEST(NormalizeTest, TC110_LongLineMixedWhitespace_NormalizesToSame) {
    std::string token = "word";
    std::string Aline, Bline;

    // A: 토큰 사이에 다양한 공백 문자를 섞어 넣음
    for (int i=0; i<2000; ++i) {
        Aline += token;
        if (i < 1999) {
            if (i % 3 == 0) Aline += "\t";
            else if (i % 3 == 1) Aline += " \v";
            else Aline += "\n\r\f";
        }
    }
    // B: 동일 토큰들을 단일 스페이스로만 연결
    for (int i=0; i<2000; ++i) {
        Bline += token;
        if (i < 1999) Bline += " ";
    }

    std::vector<std::string> A = {Aline};
    std::vector<std::string> B = {Bline};

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);
    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].kind, Diff::SAME);
}

// ─────────────────────────────────────────────────────────────
// 3) DiffEngine : build_changes_json 테스트
// ─────────────────────────────────────────────────────────────

// 변경 통계와 changes 배열의 일치 여부
TEST(JsonBuildTest, TC201_JSONStructure_ExactMatch) {
    std::vector<std::string> A = {"alpha","beta","gamma"};
    std::vector<std::string> B = {"alpha","BETA","delta"};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    Json::Value actualChanges;
    DiffStats stats{};
    LcsDiffEngine::build_changes_json(diffs, actualChanges, stats);

    Json::Value expected(Json::arrayValue);

    Json::Value mod1(Json::objectValue);
    mod1["type"] = "modified";
    mod1["baseValue"] = "beta";
    mod1["baseLineNumber"] = 2;
    mod1["baseLineCount"] = 1;
    mod1["compareValue"] = "BETA";
    mod1["compareLineNumber"] = 2;
    mod1["compareLineCount"] = 1;
    mod1["path"] = Json::nullValue;

    Json::Value mod2(Json::objectValue);
    mod2["type"] = "modified";
    mod2["baseValue"] = "gamma";
    mod2["baseLineNumber"] = 3;
    mod2["baseLineCount"] = 1;
    mod2["compareValue"] = "delta";
    mod2["compareLineNumber"] = 3;
    mod2["compareLineCount"] = 1;
    mod2["path"] = Json::nullValue;

    expected.append(mod1);
    expected.append(mod2);

    // 완전 구조 비교
    EXPECT_EQ(actualChanges, expected)
        << "Actual JSON:\n" << actualChanges.toStyledString()
        << "\nExpected:\n" << expected.toStyledString();
}

//빈 줄 사이에 변경이 낀 경우 라인 번호
TEST(JsonBuildTest, TC202_EmptyLinesAroundChange_LineNumbersCorrect) {
    std::vector<std::string> A = {"keep","","old",""};
    std::vector<std::string> B = {"keep","","NEW",""};
    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    Json::Value changes; DiffStats stats{};
    LcsDiffEngine::build_changes_json(diffs, changes, stats);
    ASSERT_EQ(changes.size(), 1u);
    const auto& it = changes[0];
    EXPECT_EQ(it["type"].asString(), "modified");
    EXPECT_EQ(it["baseLineNumber"].asInt(), 3);
    EXPECT_EQ(it["compareLineNumber"].asInt(), 3);
}

// 선두/후미 빈 줄 변화: 라인 번호/카운트 매핑
TEST(JsonBuildTest, TC203_LeadingTrailingEmptyLines_CountedAsdeleted) {
    std::vector<std::string> A = {"","alpha","","beta",""};
    std::vector<std::string> B = {"alpha","beta"};

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    Json::Value changes; DiffStats stats{};
    LcsDiffEngine::build_changes_json(diffs, changes, stats);

    EXPECT_EQ(stats.deleted, 3u);
    EXPECT_EQ(stats.added, 0u);
    EXPECT_EQ(stats.modified, 0u);

    size_t deletedCount = 0;
    for (const auto& it : changes) {
        if (it["type"].asString() == "deleted") {
            deletedCount++;
            EXPECT_TRUE(it["compareLineNumber"].isNull());
            EXPECT_TRUE(it["compareValue"].isNull());
            EXPECT_EQ(it["baseLineCount"].asInt(), 1);
            EXPECT_TRUE(it["baseLineNumber"].asInt() >= 1);
        }
    }
    EXPECT_EQ(deletedCount, 3u);
}

// build_changes_json 불변식: path는 null, stats == changes 합계
TEST(JsonBuildTest, TC204_PathIsNull_StatsMatch) {
    std::vector<std::string> A = {"H","X","Y","T"};
    std::vector<std::string> B = {"H","x","T","Z"};

    LcsDiffEngine eng;
    auto diffs = eng.compute(A, B);

    Json::Value changes; DiffStats stats{};
    LcsDiffEngine::build_changes_json(diffs, changes, stats);

    for (const auto& it : changes) {
        EXPECT_TRUE(it["path"].isNull());
    }

    size_t added=0, deleted=0, modified=0;
    for (const auto& it : changes) {
        auto t = it["type"].asString();
        if (t=="added") added++;
        else if (t=="deleted") deleted++;
        else if (t=="modified") modified++;
    }
    EXPECT_EQ(stats.added,   added);
    EXPECT_EQ(stats.deleted, deleted);
    EXPECT_EQ(stats.modified,modified);
}

//jsonString 파라미터 테스트
TEST(JsonStringTest, TC205_JsonString_IndentationChangesOutput) {
    LcsDiffEngine eng(std::vector<std::string>{"x"}, std::vector<std::string>{"y"}, "A","B");
    std::string s0 = eng.jsonString(0);
    std::string s2 = eng.jsonString(2);
    EXPECT_NE(s0, s2); // indent 0과 2 결과는 다르다
    EXPECT_NE(s2.find("\n"), std::string::npos); // indent 2에는 줄바꿈이 포함되어야 한다
}
