#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

static inline std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

static inline std::string NormalizePath(std::string p) {
    // zamień backslash na slash
    for (char &c : p) if (c == '\\') c = '/';

    // usuń podwójne (i więcej) slashe: "a//b///c" -> "a/b/c"
    std::string out;
    out.reserve(p.size());
    bool prevSlash = false;
    for (char c : p) {
        if (c == '/') {
            if (!prevSlash) out.push_back(c);
            prevSlash = true;
        } else {
            out.push_back(c);
            prevSlash = false;
        }
    }
    return out;
}

static std::string JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;

    std::string A = a;
    std::string B = b;

    // normalizuj slash
    for (char &c : A) if (c=='\\') c='/';
    for (char &c : B) if (c=='\\') c='/';

    // usuń końcowe slashe z A
    while (!A.empty() && (A.back()=='/' )) A.pop_back();
    // usuń początkowe slashe z B
    while (!B.empty() && (B.front()=='/' )) B.erase(B.begin());

    return A + "/" + B;
}

struct Key {
    int v=-1, t=-1, n=-1;
    bool operator==(const Key& o) const { return v==o.v && t==o.t && n==o.n; }
};

struct KeyHash {
    size_t operator()(const Key& k) const noexcept {
        size_t h1 = std::hash<int>{}(k.v);
        size_t h2 = std::hash<int>{}(k.t);
        size_t h3 = std::hash<int>{}(k.n);
        return h1 ^ (h2 * 1315423911u) ^ (h3 * 2654435761u);
    }
};

// token "v/t/n" lub "v//n" lub "v/t"
static Key ParseFaceVertex(const std::string& tok) {
    Key k;
    int parts[3] = {0,0,0};
    int pi = 0;
    std::string tmp;

    for (size_t i=0; i<=tok.size(); i++) {
        char c = (i<tok.size()) ? tok[i] : '/';
        if (c=='/') {
            parts[pi] = tmp.empty() ? 0 : std::stoi(tmp);
            tmp.clear();
            pi++;
            if (pi > 2) break;
        } else {
            tmp.push_back(c);
        }
    }

    k.v = (parts[0] != 0) ? (parts[0] - 1) : -1;
    k.t = (parts[1] != 0) ? (parts[1] - 1) : -1;
    k.n = (parts[2] != 0) ? (parts[2] - 1) : -1;
    return k;
}

// MTL: weź z map_Kd ostatni "sensowny" fragment (obsługa -s, -bm itd.)
static std::string ExtractTexturePathFromMapLine(std::istringstream& iss) {
    std::string rest;
    std::getline(iss, rest);
    rest = Trim(rest);
    rest = NormalizePath(rest);

    // Jeśli są opcje "-s 5 5 5", "-o ...", itp.
    // Prosta strategia: wybierz ostatni token po spacji (może zawierać spacje w nazwie pliku)
    // => lepiej: jechać od końca i brać wszystko po ostatnim wystąpieniu " -"
    // Tutaj: bierzemy "najbardziej prawdopodobną ścieżkę" od ostatniego tokenu,
    // ale uwzględniamy, że nazwy plików u Ciebie mają spacje.
    // Więc: jeśli w rest jest "tEXTURE/", to bierz substring od ostatniego wystąpienia "tEXTURE/"
    const std::string marker = "tEXTURE/";
    size_t pos = rest.rfind(marker);
    if (pos != std::string::npos) {
        return Trim(rest.substr(pos));
    }

    // fallback: ostatni token
    size_t sp = rest.find_last_of(' ');
    if (sp == std::string::npos) return rest;
    return Trim(rest.substr(sp + 1));
}

static std::unordered_map<std::string, Material> LoadMTL(const std::string& mtlPath, const std::string& baseDir) {
    std::ifstream f(mtlPath);
    if (!f) throw std::runtime_error("Nie moge otworzyc MTL: " + mtlPath);

    std::unordered_map<std::string, Material> mats;
    Material* cur = nullptr;

    std::string line;
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "newmtl") {
            std::string name; iss >> name;
            mats[name] = Material{};
            mats[name].name = name;
            cur = &mats[name];
        } else if (!cur) {
            continue;
        } else if (tag == "Kd") {
            iss >> cur->Kd.r >> cur->Kd.g >> cur->Kd.b;
        } else if (tag == "Ks") {
            iss >> cur->Ks.r >> cur->Ks.g >> cur->Ks.b;
        } else if (tag == "Ns") {
            iss >> cur->Ns;
        } else if (tag == "map_Kd") {
            std::string rel = ExtractTexturePathFromMapLine(iss);
            rel = NormalizePath(rel);
            cur->mapKd = JoinPath(baseDir, rel);
        }
        // map_Bump ignorujemy (nie wymagane)
    }

    return mats;
}

LoadedModel LoadOBJ_WithMTL(const std::string& objPath, const std::string& baseDir) {
    std::ifstream f(objPath);
    if (!f) throw std::runtime_error("Nie moge otworzyc OBJ: " + objPath);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    LoadedModel model;
    std::unordered_map<Key, uint32_t, KeyHash> remap;

    std::string activeMtl = "";
    bool submeshOpen = false;

    auto startSubmeshIfNeeded = [&]() {
        if (submeshOpen) return;
        SubMesh sm;
        sm.materialName = activeMtl;
        sm.indexOffset = (uint32_t)model.indices.size();
        sm.indexCount = 0;
        model.submeshes.push_back(sm);
        submeshOpen = true;
    };

    auto closeSubmeshIfOpen = [&]() {
        if (!submeshOpen) return;
        model.submeshes.back().indexCount =
                (uint32_t)model.indices.size() - model.submeshes.back().indexOffset;
        submeshOpen = false;
    };

    std::string line;
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            glm::vec3 p; iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vt") {
            glm::vec2 t; iss >> t.x >> t.y;
            uvs.push_back(t);
        } else if (tag == "vn") {
            glm::vec3 n; iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "mtllib") {
            // nazwa pliku może mieć spacje, więc bierzemy resztę linii
            std::string rest; std::getline(iss, rest);
            rest = Trim(rest);
            std::string mtlPath = baseDir + "/" + NormalizePath(rest);
            model.materials = LoadMTL(mtlPath, baseDir);
        } else if (tag == "usemtl") {
            std::string name; std::getline(iss, name);
            name = Trim(name);
            closeSubmeshIfOpen();
            activeMtl = name;
            startSubmeshIfNeeded();
        } else if (tag == "f") {
            startSubmeshIfNeeded();

            std::vector<Key> face;
            std::string tok;
            while (iss >> tok) face.push_back(ParseFaceVertex(tok));
            if (face.size() < 3) continue;

            // triangulacja fan: (0, i, i+1)
            for (size_t i = 1; i + 1 < face.size(); i++) {
                Key tri[3] = { face[0], face[i], face[i+1] };

                for (int k = 0; k < 3; k++) {
                    Key key = tri[k];

                    auto it = remap.find(key);
                    if (it != remap.end()) {
                        model.indices.push_back(it->second);
                        continue;
                    }

                    if (key.v < 0 || key.v >= (int)positions.size())
                        throw std::runtime_error("Blad indeksu v w OBJ.");

                    Vertex vtx{};
                    vtx.pos = positions[key.v];

                    vtx.uv = glm::vec2(0,0);
                    if (key.t >= 0 && key.t < (int)uvs.size()) vtx.uv = uvs[key.t];

                    vtx.nrm = glm::vec3(0,1,0);
                    if (key.n >= 0 && key.n < (int)normals.size()) vtx.nrm = normals[key.n];

                    uint32_t newIndex = (uint32_t)model.vertices.size();
                    model.vertices.push_back(vtx);
                    remap[key] = newIndex;
                    model.indices.push_back(newIndex);
                }
            }
        }
    }

    closeSubmeshIfOpen();

    // Jeśli nie było usemtl ani mtllib – nadal jest OK, będzie 1 submesh z materialName=""
    if (model.submeshes.empty() && !model.indices.empty()) {
        SubMesh sm;
        sm.materialName = "";
        sm.indexOffset = 0;
        sm.indexCount = (uint32_t)model.indices.size();
        model.submeshes.push_back(sm);
    }

    std::cout << "OBJ loaded: vertices=" << model.vertices.size()
              << " indices=" << model.indices.size()
              << " submeshes=" << model.submeshes.size()
              << " materials=" << model.materials.size() << "\n";

    return model;
}
