#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <json/json.h>

class treediff {
public:
    // 생성자
    treediff() = delete;
    treediff(const treediff&) = delete;
    treediff& operator=(const treediff&) = delete;

    // 비교 결과 구조체
    struct Change { char kind; std::string path; }; // 'A','D','M'
    struct Result {
        std::filesystem::path leftRoot;
        std::filesystem::path rightRoot;
        std::vector<Change> changes;
        size_t added{0}, removed{0}, modified{0};
        size_t total() const { return added + removed + modified; }
    };

    // 1) 파일 해시 계산
    static std::string Sha256OfString(const std::string& x);
    static std::string Sha256OfFile(const std::filesystem::path& p);

    // 2) 폴더 간 파일(leaf) 비교
    static Result CompareDirs(const std::filesystem::path& left, const std::filesystem::path& right);

    // 3) 비교 결과 → JSON
    static Json::Value BuildJsonValue(const Result& r);

    // 4) 전체 파이프라인(1 → 2 → 3) 실행
    static Json::Value Run(const std::filesystem::path& left, const std::filesystem::path& right);

    // 추가 유틸 (JSON을 CLI에 출력)
    static void PrintJsonToCli(const Json::Value& json, bool pretty);

    // 추가 유틸 (인자 파싱 → Run 호출 → 출력/종료코드 처리)
    static int RunCli(int argc, char** argv);

    // 추가 유틸 (폴더 해시 값)
    static std::string HashSubtree(const std::filesystem::path& dir);

private:
    // 내부 유틸 (파일 무시 설정)
    static bool IsSymlinkEntry(const std::filesystem::directory_entry& e);
    static bool IsIgnoredName(const std::string& name);
    // 내부 유틸 (리프 노드 해시 맵)
    static std::unordered_map<std::string, std::string>
    BuildLeafHashMap(const std::filesystem::path& root);
    static std::string ToHex(const unsigned char* h, size_t n);
    // 내부 유틸 (SHA256)
    struct SHA256 {
        uint32_t s[8]; uint64_t bitlen; uint8_t buf[64]; size_t blen;
        static inline uint32_t R(uint32_t x, uint32_t n){ return (x>>n)|(x<<(32U-n)); }
        static inline uint32_t ch(uint32_t x,uint32_t y,uint32_t z){ return (x&y)^(~x&z); }
        static inline uint32_t maj(uint32_t x,uint32_t y,uint32_t z){ return (x&y)^(x&z)^(y&z); }
        static inline uint32_t e0(uint32_t x){ return R(x,2)^R(x,13)^R(x,22); }
        static inline uint32_t e1(uint32_t x){ return R(x,6)^R(x,11)^R(x,25); }
        static inline uint32_t s0(uint32_t x){ return R(x,7)^R(x,18)^(x>>3); }
        static inline uint32_t s1(uint32_t x){ return R(x,17)^R(x,19)^(x>>10); }
        static constexpr uint32_t K[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        void init();
        void transform(const uint8_t *d);
        void update(const void* in, size_t len);
        void final(uint8_t out[32]);
    };
};
