#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <tuple>
#include <json/json.h>
#include "DiffPython.h"

// ─────────────────────────────────────────────────────────────
// 1) DiffEngine : 생성자 테스트
// ─────────────────────────────────────────────────────────────

static std::string joinLines(const std::vector<std::string>& v) {
    // 마지막 줄에 개행 없어도 DiffPython::normalizeAll 이 처리하므로 그냥 '\n'로만 연결
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        s += v[i];
        if (i + 1 < v.size()) s += "\n";
    }
    return s;
}

// ─────────────────────────────────────────────────────────────
// 1) DiffEngine : Normalize 테스트
// ─────────────────────────────────────────────────────────────

// 앞 뒤 공백 제거
TEST(NormalizeTest, TC001_RemoveLeadingAndTrailingWhitespace) {
    std::string pad = "\v\t \f";
    std::vector<std::string> A = { pad + "Token" + pad };
    std::vector<std::string> B = { "Token" };

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);
    auto aNorm = DiffPython::normalizeAll(contentA);
    auto bNorm = DiffPython::normalizeAll(contentB);
    auto diffs = DiffPython::compute(aNorm, bNorm, contentA, contentB);

    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
}

// 서로 다른 공백들이 줄 사이에 여럿 있어도 하나로 축약되어야 함
TEST(NormalizeTest, TC002_CollapseInternalWhitespaceToSingleSpace) {
    std::vector<std::string> A = { std::string("Hi") + "\t \f\v" + "There" };
    std::vector<std::string> B = { "Hi There" };

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);
    auto aNorm = DiffPython::normalizeAll(contentA);
    auto bNorm = DiffPython::normalizeAll(contentB);
    auto diffs = DiffPython::compute(aNorm, bNorm, contentA, contentB);

    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
}

// 6종 공백만 포함된 라인은 ""와 동일
TEST(NormalizeTest, TC003_ConvertWhitespaceOnlyToEmptyLine) {
    std::vector<std::string> A = { " \t\f\v " };
    std::vector<std::string> B = { "" };

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);
    auto aNorm = DiffPython::normalizeAll(contentA);
    auto bNorm = DiffPython::normalizeAll(contentB);
    auto diffs = DiffPython::compute(aNorm, bNorm, contentA, contentB);

    int nonSame = 0; for (auto &d: diffs) if (d.kind != Diff::SAME) nonSame++;
    EXPECT_EQ(nonSame, 0);
}

// 매우 긴 라인 + 혼합 공백 정규화 후 SAME (성능/내성 스모크)
TEST(NormalizeTest, TC004_LongLineMixedWhitespace_ToSame) {
    std::string token = "word";
    std::string Aline, Bline;

    // A: 토큰 사이에 다양한 공백 문자를 섞어 넣음
    for (int i=0; i<2000; ++i) {
        Aline += token;
        if (i < 1999) {
            if (i % 3 == 0) Aline += "\t";
            else if (i % 3 == 1) Aline += " \v";
            else Aline += "\f";
        }
    }
    // B: 동일 토큰들을 단일 스페이스로만 연결
    for (int i=0; i<2000; ++i) {
        Bline += token;
        if (i < 1999) Bline += " ";
    }

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(Aline),
        DiffPython::normalizeAll(Bline),
        Aline, 
        Bline
    );

    ASSERT_EQ(diffs.size(), 1u);
    EXPECT_EQ(diffs[0].kind, Diff::SAME);
}

// 정규화 테스트 문자열 처리
TEST(NormalizeTest, TC005_TokenSplit_NotIn) {
    std::string A = "if x not in s:\n    pass\n";
    std::string B = "if x notin s:\n    pass\n";
    auto aNorm = DiffPython::normalizeAll(A);
    auto bNorm = DiffPython::normalizeAll(B);
    auto diffs = DiffPython::compute(aNorm, bNorm, A, B);

    EXPECT_EQ(diffs[0].kind, Diff::MOD);
    EXPECT_EQ(diffs[0].baseLine,    0);
    EXPECT_EQ(diffs[0].compareLine, 0);

    EXPECT_EQ(diffs[1].kind, Diff::SAME);
    EXPECT_EQ(diffs[1].baseLine,    1);
    EXPECT_EQ(diffs[1].compareLine, 1);
}

// 주석 테스트(주석 뒤 문자 + 주석 전 정규화 데이터)
TEST(NormalizeTest, TC006_CommentIgnore_SameLine) {
    std::string A = "if #x not in s:\n    pass\n";
    std::string B = "if# x notin s:\n    pass\n";
    auto aNorm = DiffPython::normalizeAll(A);
    auto bNorm = DiffPython::normalizeAll(B);
    auto diffs = DiffPython::compute(aNorm, bNorm, A, B);

    int nonSame = 0;
    for (auto &d: diffs) if (d.kind != Diff::SAME) ++nonSame;
    EXPECT_EQ(nonSame, 0);
}

// 주석 테스트(다음줄 간섭여부)
TEST(NormalizeTest, TC007_CommentIsolation_NextLine) {
    std::string A = "if #x not in s:\n wr   pass\n";
    std::string B = "if# x notin s:\n    pass\n";
    auto aNorm = DiffPython::normalizeAll(A);
    auto bNorm = DiffPython::normalizeAll(B);
    auto diffs = DiffPython::compute(aNorm, bNorm, A, B);

    int nonSame = 0;
    for (auto &d: diffs) if (d.kind != Diff::SAME) ++nonSame;
    EXPECT_EQ(nonSame, 1);
}

// ─────────────────────────────────────────────────────────────
// 2) DiffEngine : compute 테스트 (LCS/변경 유형)
// ─────────────────────────────────────────────────────────────

// 모든 라인이 동일하면 SAME
TEST(DiffEngineTest, TC101_AllLinesIdentical_AllSAME) {
    std::vector<std::string> A = {"a","b","c"};
    std::vector<std::string> B = {"a","b","c"};

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

    ASSERT_EQ(diffs.size(), 3u);
    for (auto &d : diffs) EXPECT_EQ(d.kind, Diff::SAME);
}

// 일부 라인 수정만 있을 때 MOD 카운트 확인
TEST(DiffEngineTest, TC102_Detect_ModifiedLines_Only) {
    std::vector<std::string> A = {"alpha","beta","gamma"};
    std::vector<std::string> B = {"alpha","BETA","delta"};
    
    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

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

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

    EXPECT_TRUE(diffs.empty());
}

// 한쪽이 전부 ADD만 있는 경우
TEST(DiffEngineTest, TC104_AllLinesAdded_ReturnsAddOnly) {
    std::vector<std::string> A = {};
    std::vector<std::string> B = {"a","b","c"};
    
    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

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
    
    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

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

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

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

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

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
    
    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

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

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto ab = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );
    auto ba = DiffPython::compute(
        DiffPython::normalizeAll(contentB),
        DiffPython::normalizeAll(contentA),
        contentB, 
        contentA
    );

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

// ─────────────────────────────────────────────────────────────
// 3) DiffEngine : buildChangesJson 테스트
// ─────────────────────────────────────────────────────────────

// 변경 통계와 changes 배열의 일치 여부
TEST(JsonBuildTest, TC201_JSONStructure_ExactMatch) {
    std::vector<std::string> A = {"alpha","beta","gamma"};
    std::vector<std::string> B = {"alpha","BETA","delta"};
    
    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

    Json::Value actualChanges;
    DiffStats stats{};
    DiffPython::buildChangesJson(diffs, actualChanges, stats);

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
    
    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

    Json::Value changes; DiffStats stats{};
    DiffPython::buildChangesJson(diffs, changes, stats);

    ASSERT_EQ(changes.size(), 1u);
    const auto& it = changes[0];
    EXPECT_EQ(it["type"].asString(), "modified");
    EXPECT_EQ(it["baseLineNumber"].asInt(), 3);
    EXPECT_EQ(it["compareLineNumber"].asInt(), 3);
}


// 선두/후미 빈 줄 변화: 라인 번호/카운트 매핑
TEST(JsonBuildTest, TC203_LeadingTrailingEmptyLines_Counted) {
    std::vector<std::string> A = {"","alpha","","beta",""};
    std::vector<std::string> B = {"alpha","alpha",""};

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

    Json::Value changes; DiffStats stats{};
    DiffPython::buildChangesJson(diffs, changes, stats);

    EXPECT_EQ(stats.added, 0u);
    EXPECT_EQ(stats.deleted, 0u);
    EXPECT_EQ(stats.modified, 1u);

    ASSERT_EQ(changes.size(), 1u);
    const auto& it = changes[0];
    EXPECT_EQ(it["type"].asString(), "modified");
    EXPECT_EQ(it["baseLineNumber"].asInt(), 4);
    EXPECT_EQ(it["compareLineNumber"].asInt(), 2);

    EXPECT_EQ(it["baseLineCount"].asInt(), 1);
    EXPECT_EQ(it["compareLineCount"].asInt(), 1);

    EXPECT_EQ(it["baseValue"].asString(), "beta");
    EXPECT_EQ(it["compareValue"].asString(), "alpha");
}

// build_changes_json 불변식: path는 null, stats == changes 합계
TEST(JsonBuildTest, TC204_PathIsNull_StatsMatch) {
    std::vector<std::string> A = {"H","X","Y","T"};
    std::vector<std::string> B = {"H","x","T","Z"};

    std::string contentA = joinLines(A);
    std::string contentB = joinLines(B);

    auto diffs = DiffPython::compute(
        DiffPython::normalizeAll(contentA),
        DiffPython::normalizeAll(contentB),
        contentA, 
        contentB
    );

    Json::Value changes; DiffStats stats{};
    DiffPython::buildChangesJson(diffs, changes, stats);

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

// 빈 라인, 주석, 트리플 따옴표 테스트
TEST(JsonBuildTest, TC205_Integrate_MultiBlock) {
    std::string A = R"CODE(
x = 6
s = [4, 5, 8, 9]

if x not in s:
    print("ok")

j = 6     # test
j = x + j

test = "t  e  s  t"

"""
wrwqtw
erwqte
teeq
"""

j = 0

""" wrwqtw
erwqte
teeq """
)CODE";
    std::string B = R"CODE(
x = 6
s = [4, 5, 8, 9]

if x not in s:
    print("ok")

j = 6     # test j = x
j = x + j + j
test = "t e s t"

"""
wrwqtwwgegegw
erwqtewfwfwgw
teeqqqrqqqq
"""

j = 0

''' wrwqtw
erwtegwgwggwwwgww
teeq '''
)CODE";
    auto aNorm = DiffPython::normalizeAll(A);
    auto bNorm = DiffPython::normalizeAll(B);
    auto diffs = DiffPython::compute(aNorm, bNorm, A, B);

    int add=0, del=0, mod=0, same=0;
    for (const auto& d: diffs) {
        if      (d.kind == Diff::ADD) ++add;
        else if (d.kind == Diff::DEL) ++del;
        else if (d.kind == Diff::MOD) ++mod;
    }
    EXPECT_EQ(add, 0);
    EXPECT_EQ(del, 0);
    EXPECT_EQ(mod, 2);

    Json::Value changes; DiffStats stats{};
    DiffPython::buildChangesJson(diffs, changes, stats);

    EXPECT_EQ(stats.added, add);
    EXPECT_EQ(stats.deleted, del);
    EXPECT_EQ(stats.modified, mod);

    ASSERT_EQ(changes.size(), 2u);

    auto assertModJson = [](const Json::Value& it,
                            int expBaseLine1, const std::string& expBaseValue,
                            int expCompLine1, const std::string& expCompValue) {
        EXPECT_EQ(it["type"].asString(), "modified");
        EXPECT_EQ(it["baseLineNumber"].asInt(),    expBaseLine1);
        EXPECT_EQ(it["compareLineNumber"].asInt(), expCompLine1);
        EXPECT_EQ(it["baseLineCount"].asInt(),     1);
        EXPECT_EQ(it["compareLineCount"].asInt(),  1);
        EXPECT_EQ(it["baseValue"].asString(),      expBaseValue);
        EXPECT_EQ(it["compareValue"].asString(),   expCompValue);
    };

    assertModJson(changes[0], 9, "j = x + j", 9, "j = x + j + j");
    assertModJson(changes[1], 11, "test = \"t  e  s  t\"", 10, "test = \"t e s t\"");
    // std::cout << "── A normalized result ──\n";
    // for (size_t i = 0; i < aNorm.size(); ++i) {
    //     std::cout << std::setw(2) << i + 1 << " | " << aNorm[i].body << "\n";
    // }
}