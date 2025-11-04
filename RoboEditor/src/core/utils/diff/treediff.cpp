#include "treediff.h"
#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

using std::string;

// 내부 유틸 (해시 함수)
void treediff::SHA256::init(){
    s[0]=0x6a09e667; s[1]=0xbb67ae85; s[2]=0x3c6ef372; s[3]=0xa54ff53a;
    s[4]=0x510e527f; s[5]=0x9b05688c; s[6]=0x1f83d9ab; s[7]=0x5be0cd19;
    bitlen=0; blen=0;
}
void treediff::SHA256::transform(const uint8_t *d){
    uint32_t m[64];
    for(int i=0;i<16;i++)
        m[i]=(uint32_t(d[i*4])<<24)|(uint32_t(d[i*4+1])<<16)|(uint32_t(d[i*4+2])<<8)|uint32_t(d[i*4+3]);
    for(int i=16;i<64;i++) m[i]=s1(m[i-2])+m[i-7]+s0(m[i-15])+m[i-16];
    uint32_t a=s[0],b=s[1],c=s[2],d0=s[3],e=s[4],f=s[5],g=s[6],h=s[7];
    for(int i=0;i<64;i++){
        uint32_t t1=h+e1(e)+ch(e,f,g)+K[i]+m[i];
        uint32_t t2=e0(a)+maj(a,b,c);
        h=g; g=f; f=e; e=d0+t1; d0=c; c=b; b=a; a=t1+t2;
    }
    s[0]+=a; s[1]+=b; s[2]+=c; s[3]+=d0; s[4]+=e; s[5]+=f; s[6]+=g; s[7]+=h;
}
void treediff::SHA256::update(const void* in, size_t len){
    const uint8_t* p = static_cast<const uint8_t*>(in);
    for(size_t i=0;i<len;i++){
        buf[blen++] = p[i];
        if (blen==64){ transform(buf); bitlen+=512; blen=0; }
    }
}
void treediff::SHA256::final(uint8_t out[32]){
    bitlen += blen*8ULL;
    buf[blen++]=0x80;
    if (blen>56){ while(blen<64) buf[blen++]=0; transform(buf); blen=0; }
    while(blen<56) buf[blen++]=0;
    for(int i=7;i>=0;i--) buf[blen++]=static_cast<uint8_t>((bitlen>>(i*8))&0xFFU);
    transform(buf);
    for(int i=0;i<8;i++){
        out[i*4+0]=static_cast<uint8_t>((s[i]>>24)&0xFFU);
        out[i*4+1]=static_cast<uint8_t>((s[i]>>16)&0xFFU);
        out[i*4+2]=static_cast<uint8_t>((s[i]>>8 )&0xFFU);
        out[i*4+3]=static_cast<uint8_t>((s[i]    )&0xFFU);
    }
}
std::string treediff::ToHex(const unsigned char* h, size_t n){
    static const char* ds="0123456789abcdef";
    string s; s.resize(n*2);
    for(size_t i=0;i<n;i++){ s[i*2]=ds[h[i]>>4]; s[i*2+1]=ds[h[i]&0xF]; }
    return s;
}

// 1) 파일 해시 계산
std::string treediff::Sha256OfString(const std::string& x){
    SHA256 s; s.init(); s.update(x.data(), x.size());
    uint8_t h[32]; s.final(h); return ToHex(h, 32);
}
std::string treediff::Sha256OfFile(const std::filesystem::path& p){
    std::ifstream in(p, std::ios::binary);
    if(!in) throw std::runtime_error("open fail: " + p.string());
    SHA256 s; s.init();
    std::vector<char> buf(1<<20); // 1MiB 버퍼(위 코드 설정 유지)
    while(in){
        in.read(buf.data(), buf.size());
        std::streamsize n = in.gcount();
        if (n>0) s.update(buf.data(), static_cast<size_t>(n));
    }
    uint8_t h[32]; s.final(h); return ToHex(h, 32);
}

// 내부 유틸 (파일 무시 설정)
bool treediff::IsSymlinkEntry(const std::filesystem::directory_entry& e){
    std::error_code ec; auto st = e.symlink_status(ec); if(ec) return false;
    return std::filesystem::is_symlink(st);
}
bool treediff::IsIgnoredName(const std::string& name){
    return (name == ".git" || name == ".DS_Store");
}
// 내부 유틸 (리프 노드 해시 맵)
std::unordered_map<std::string, std::string>
treediff::BuildLeafHashMap(const std::filesystem::path& root){
    std::unordered_map<std::string, std::string> m;
    if (!std::filesystem::exists(root)) return m;
    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        const auto& de = *it;
        if (IsSymlinkEntry(de)) { it.disable_recursion_pending(); continue; }
        auto name = de.path().filename().string();
        if (IsIgnoredName(name)) {
            if (de.is_directory(ec)) it.disable_recursion_pending();
            continue;
        }
        if (!de.is_regular_file(ec)) continue;

        auto rel = std::filesystem::relative(de.path(), root, ec).generic_string(); // POSIX 스타일
        try {
            m.emplace(std::move(rel), Sha256OfFile(de.path()));
        } catch (...) {
            // unreadable -> skip
        }
    }
    return m;
}

// 2) 폴더 간 파일(leaf) 비교
treediff::Result treediff::CompareDirs(const std::filesystem::path& left, const std::filesystem::path& right){
    Result r; r.leftRoot = left; r.rightRoot = right;
    auto L = BuildLeafHashMap(left);
    auto R = BuildLeafHashMap(right);

    std::set<std::string> all;
    for (auto& kv : L) all.insert(kv.first);
    for (auto& kv : R) all.insert(kv.first);

    r.changes.reserve(all.size());
    for (const auto& p : all){
        bool hasL = L.find(p) != L.end();
        bool hasR = R.find(p) != R.end();
        if (!hasL && hasR) { r.changes.push_back({'A', p}); ++r.added;    continue; }
        if (hasL && !hasR) { r.changes.push_back({'D', p}); ++r.removed;  continue; }
        if (L[p] != R[p])  { r.changes.push_back({'M', p}); ++r.modified; }
    }
    return r;
}

// 3) 비교 결과 → JSON
Json::Value treediff::BuildJsonValue(const Result& r){
    Json::Value root(Json::objectValue);

    root["base"]["name"]    = std::filesystem::absolute(r.leftRoot).filename().generic_string();
    root["compare"]["name"] = std::filesystem::absolute(r.rightRoot).filename().generic_string();

    Json::Value stats(Json::objectValue);
    stats["fileType"]     = "folder";
    stats["added"]        = static_cast<Json::UInt64>(r.added);
    stats["removed"]      = static_cast<Json::UInt64>(r.removed);
    stats["modified"]     = static_cast<Json::UInt64>(r.modified);
    stats["totalChanges"] = static_cast<Json::UInt64>(r.total());
    root["statistics"] = std::move(stats);

    Json::Value arr(Json::arrayValue);

    for (const auto& c : r.changes){
        Json::Value item(Json::objectValue);
        item["type"] = (c.kind=='A') ? "added" : (c.kind=='D' ? "removed" : "modified");
        item["path"] = c.path;
        arr.append(std::move(item));
    }
    root["changes"] = std::move(arr);
    return root;
}

// 4) 전체 파이프라인(1 → 2 → 3) 실행
Json::Value treediff::Run(const std::filesystem::path& left, const std::filesystem::path& right){
    Result r = CompareDirs(left, right);
    return BuildJsonValue(r);
}

// 추가 유틸 (JSON을 CLI에 출력)
void treediff::PrintJsonToCli(const Json::Value& json, bool pretty){
    Json::StreamWriterBuilder w;
    if (pretty) {
        w["indentation"] = "  ";
    } else {
        w["indentation"] = "";
        w["enableYAMLCompatibility"] = false;
    }
    w["emitUTF8"] = true;
    std::cout << Json::writeString(w, json);
}
// 추가 유틸 (인자 파싱 → Run 호출 → 출력/종료코드 처리)
int treediff::RunCli(int argc, char** argv){
    std::ios::sync_with_stdio(false);
    try{
        if (argc < 3){
            std::cerr << "Usage:\n  " << argv[0] << " <left_dir> <right_dir> [--pretty]\n";
            return 1;
        }
        bool pretty = (argc >= 4 && std::string(argv[3]) == "--pretty");

        std::filesystem::path left  = std::filesystem::path(argv[1]);
        std::filesystem::path right = std::filesystem::path(argv[2]);
        if (!std::filesystem::exists(left)  || !std::filesystem::is_directory(left))  { std::cerr << "Left not a directory: "  << left  << "\n"; return 2; }
        if (!std::filesystem::exists(right) || !std::filesystem::is_directory(right)) { std::cerr << "Right not a directory: " << right << "\n"; return 2; }

        // 파이프라인 실행
        Json::Value j = Run(left, right);
        PrintJsonToCli(j, pretty);

        // 종료코드 결정: 변경 없으면 0, 있으면 3
        const auto& changes = j["changes"];
        if (changes.isArray() && changes.empty()) return 0;
        return 3;
    } catch (const std::exception& e){
        std::cerr << "Error: " << e.what() << "\n"; return 99;
    }
}
// 추가 유틸 (폴더 해시 값)
std::string treediff::HashSubtree(const std::filesystem::path& dir){
    struct EntryHash { char type; std::string name; std::string hash; };
    std::vector<EntryHash> items;
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, std::filesystem::directory_options::skip_permission_denied, ec), end;
    for (; !ec && it != end; it.increment(ec)) {
        const auto& de = *it;
        std::string name = de.path().filename().string();

        if (IsSymlinkEntry(de) || IsIgnoredName(name)) continue;

        if (de.is_regular_file(ec)) {
            std::string h = Sha256OfFile(de.path());
            items.emplace_back(EntryHash{'B', std::move(name), std::move(h)});
        } else if (de.is_directory(ec)) {
            std::string h = HashSubtree(de.path());
            items.emplace_back(EntryHash{'T', std::move(name), std::move(h)});
        } else {
            // special files ignored
        }
    }
    std::sort(items.begin(), items.end(), [](const EntryHash& a, const EntryHash& b){
        if (a.type != b.type) return a.type < b.type;
        return a.name < b.name;
    });

    std::string canon;
    canon.reserve(6 + items.size() * 128); // 여유 버퍼(위 가정 유지)
    canon += "TREE\n";
    for (const auto& e : items){
        canon += e.type; 
        canon += ' ';
        canon += std::to_string(e.name.size());
        canon += ' ';
        canon += e.name; 
        canon += '\n';
        canon += e.hash; 
        canon += '\n';
    }
    return Sha256OfString(canon);
}