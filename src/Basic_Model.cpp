#include "EWEngine/Graphics/Model/Basic_Model.h"

namespace EWE {
    namespace Basic_Model {

        EWEModel* Circle(uint16_t const points, float radius SRC_PARAM) {
            //utilizing a triangle fan
#if EWE_DEBUG
            if (points < 5) {
                std::cout << "yo wyd? making a circle with too few points : " << points << std::endl;
            }
#endif

            const lab::vec3 normal = { 0.f, -1.f, 0.f };
            std::vector<VertexNT> vertices{};
            vertices.push_back({ { 0.0f,0.0f,0.0f }, normal, { 0.5f,0.5f } });

            const float angle = lab::GetPI(2.f) / points;

            for (uint16_t i = 0; i < points; i++) {
                const float theSin = lab::Sin(angle * static_cast<float>(i));
                const float theCos = lab::Cos(angle * static_cast<float>(i));
                //std::cout << "theSin:theCos ~ " << theSin << " : " << theCos << std::endl; //shit is tiling when i want it to stretch
                vertices.push_back({ {radius * theSin, 0.f, radius * theCos}, normal, {(theSin + 1.f) / 2.f, (theCos + 1.f) / 2.f} });
            }
            std::vector<uint32_t> indices{};

            for (uint16_t i = 2; i < points; i++) {
                indices.push_back(0);
                indices.push_back(i - 1);
                indices.push_back(i);
            }
            indices.push_back(0);
            indices.push_back(points - 1);
            indices.push_back(1);
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices} SRC_PASS);
        }

        EWEModel* Quad(lab::vec2 uvScale SRC_PARAM) {
            std::vector<Vertex> vertices{
                {{0.5f,0.0f, -0.5f}, {0.f,1.f,0.f}, {uvScale.x,uvScale.y}, {1.f, 0.f, 0.f}},
                {{-0.5f,0.0f, -0.5f}, {0.f,1.f,0.f}, {0.0f,uvScale.y}, {1.f, 0.f, 0.f}},
                {{-0.5f,0.0f, 0.5f}, {0.f,1.f,0.f}, {0.0f,0.f}, {1.f, 0.f, 0.f}},
                {{0.5f,0.0f, 0.5f}, {0.f,1.f,0.f}, {uvScale.x,0.f}, {1.f, 0.f, 0.f}},
            };
            std::vector<uint32_t> indices{ 0, 1, 2, 2,3,0 };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices} SRC_PASS);
        }
        EWEModel* QuadPNU(lab::vec2 uvScale SRC_PARAM) {
            std::vector<VertexNT> vertices{
                {{0.5f,0.0f, -0.5f}, {0.f,1.f,0.f}, {uvScale.x,uvScale.y}},
                {{-0.5f,0.0f, -0.5f}, {0.f,1.f,0.f}, {0.0f,uvScale.y}},
                {{-0.5f,0.0f, 0.5f}, {0.f,1.f,0.f}, {0.0f,0.f}},
                {{0.5f,0.0f, 0.5f}, {0.f,1.f,0.f}, {uvScale.x,0.f}},
            };
            std::vector<uint32_t> indices{ 0, 1, 2, 2,3,0 };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices} SRC_PASS);
        }
        EWEModel* Simple3DQuad(lab::vec2 uvScale SRC_PARAM) {
            std::vector<EffectVertex> vertices{
                {{0.5f,0.0f, -0.5f}, {uvScale.x,uvScale.y}},
                {{-0.5f,0.0f, -0.5f}, {0.0f,uvScale.y}},
                {{-0.5f,0.0f, 0.5f}, {0.0f,0.f}},
                {{0.5f,0.0f, 0.5f}, {uvScale.x,0.f}},
            };
            std::vector<uint32_t> indices{ 0, 1, 2, 2, 3, 0 };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices } SRC_PASS);
        }

        EWEModel* TileQuad3D(lab::vec2 uvScale SRC_PARAM) {
            std::vector<TileVertex> vertices{
                {{uvScale.x,uvScale.y}},
                {{0.0f,uvScale.y}},
                {{0.0f,0.f}},
                {{0.0f,0.f}},
                {{uvScale.x,0.f}},
                {{uvScale.x,uvScale.y}},
            };
            //std::vector<uint32_t> indices{};// 0, 1, 2, 2, 3, 0 };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0])} SRC_PASS);
        }

        EWEModel* Grid2D(lab::vec2 scale SRC_PARAM) {
            const float leftX = -1.f * scale.x;
            const float rightX = 1.f * scale.x;
            const float topY = -1.f * scale.y;
            const float botY = 1.f * scale.y;

            const std::vector<VertexGrid2D> vertices{
                {leftX, topY},
                {leftX, botY},
                {rightX, topY},
                {rightX, topY},
                {leftX, botY},
                {rightX, botY}
            };
            //std::vector<uint32_t> indices{ 0, 1, 2, 2, 3, 0 };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0])} SRC_PASS);
        }
        EWEModel* Grid3DTrianglePrimitive(const uint32_t patchSize, const lab::vec2 uvScale SRC_PARAM) {
            const uint32_t vertexCount = patchSize * patchSize;
            std::vector<VertexNT> vertices(vertexCount);

            const float wx = 2.0f;
            const float wy = 2.0f;
            const uint32_t w = patchSize - 1;
            const float wf = static_cast<float>(w);

            // Generate vertices
            for (uint32_t x = 0; x < patchSize; x++) {
                for (uint32_t y = 0; y < patchSize; y++) {
                    auto& vertex = vertices[x + y * patchSize];
                    vertex.position.x = x * wx + wx / 2.0f - static_cast<float>(patchSize) * wx / 2.0f;
                    vertex.position.y = 0.0f;
                    vertex.position.z = y * wy + wy / 2.0f - static_cast<float>(patchSize) * wy / 2.0f;
                    vertex.uv = lab::vec2(static_cast<float>(x) / wf, static_cast<float>(y) / wf) * uvScale;

                    // Placeholder normal
                    vertex.normal = lab::vec3(0.f, -1.f, 0.f);
                }
            }

            // Generate indices for triangles (2 per quad)
            const uint32_t indexCount = w * w * 6;
            std::vector<uint32_t> indices(indexCount);

            for (uint32_t x = 0; x < w; x++) {
                for (uint32_t y = 0; y < w; y++) {
                    uint32_t topLeft = x + y * patchSize;
                    uint32_t topRight = topLeft + 1;
                    uint32_t bottomLeft = topLeft + patchSize;
                    uint32_t bottomRight = bottomLeft + 1;

                    uint32_t index = (x + y * w) * 6;

                    // Triangle 1
                    indices[index + 0] = topLeft;
                    indices[index + 1] = bottomLeft;
                    indices[index + 2] = bottomRight;

                    // Triangle 2
                    indices[index + 3] = topLeft;
                    indices[index + 4] = bottomRight;
                    indices[index + 5] = topRight;
                }
            }

            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices } SRC_PASS);
        }

        EWEModel* Grid3DQuadPrimitive(const uint32_t patchSize, const lab::vec2 uvScale SRC_PARAM){
            
            const uint32_t vertexCount = patchSize * patchSize;
            std::vector<VertexNT> vertices(vertexCount);
    
            const float wx = 2.0f;
            const float wy = 2.0f;
            const uint32_t w = (patchSize - 1);
            const float wf = static_cast<float>(w);

            for(uint32_t x = 0; x < patchSize; x++){
                for(uint32_t y = 0; y < patchSize; y++){
                    auto& vertex = vertices[x + y * patchSize];
                    vertex.position.x = x * wx + wx / 2.0f - static_cast<float>(patchSize) * wx / 2.0f;
                    vertex.position.y = 0.0f;
                    vertex.position.z = y * wy + wy / 2.0f - static_cast<float>(patchSize) * wy / 2.0f;
                    vertex.uv = lab::vec2(static_cast<float>(x) / wf, static_cast<float>(y) / wf) * uvScale;

                    //temporary
                    vertex.normal = lab::vec3(0.f, -1.f, 0.f);
                }
            }

            const uint32_t indexCount = w * w * 4;
            std::vector<uint32_t> indices(indexCount);
            for (auto x = 0; x < w; x++) {
                for (auto y = 0; y < w; y++) {
                    uint32_t index = (x + y * w) * 4;
                    indices[index] = (x + y * patchSize);
                    indices[index + 1] = indices[index] + patchSize;
                    indices[index + 2] = indices[index + 1] + 1;
                    indices[index + 3] = indices[index] + 1;
                }
            }

            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices } SRC_PASS);
        }

        EWEModel* Quad2D(lab::vec2 scale SRC_PARAM) {
            std::vector<VertexUI> vertices{
                {{-0.5f, -0.5f}, {0.f, 0.f}},
                {{0.5f, -0.5f}, {scale.x, 0.f}},
                {{0.5f, 0.5f}, {scale.x, scale.y}},
                {{-0.5f, 0.5f}, {0.f, scale.y}}
            };
            std::vector<uint32_t> indices{ 0, 1, 2, 2, 3, 0 };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices} SRC_PASS);
        }
        EWEModel* NineUIQuad(SRC_FIRST_PARAM) {
            std::vector<VertexUI> vertices{
                {{-0.5f, -0.5f}, {0.f, 0.f}}, //top left corner
                {{-.5f, -.5f}, {.0625f, .0625f}}, //inner top left corner

                {{0.5f, -0.5f}, {1.f, 0.f}}, //bottom left
                {{0.5f, -.5f}, {1.f - .0625f, .0625f}}, //inner bottom left

                {{0.5f, 0.5f}, {1.f, 1.f}}, //bottom right
                {{.5f, .5f}, {1.f - .0625f,1.f - .0625f}}, //inner bottom right

                {{-0.5f, 0.5f}, {0.f, 1.f}}, //top right
                {{-.5f, .5f}, {.0625f, 1.f - .0625f}}, //inner top right
            };
            std::vector<uint32_t> indices{ 
                1,0,6,  1,6,7,  1,7,3,  1,3,2,  1,2,0,  5,4,2,  5,2,3,  5,3,7,  5,7,6,  5,6,4 
            };
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices} SRC_PASS);
        }
        EWEModel* SkyBox(float scale SRC_PARAM) {
            //hopefully never have to look at this again

            std::vector<SkyVertex> vertices = {
                {{ -1.0f, -1.0f,  1.0f}}, //0
                {{ -1.0f,  1.0f,  1.0f}}, //1
                {{  1.0f,  1.0f,  1.0f}}, //2
                {{  1.0f, -1.0f,  1.0f}}, //3
                {{ -1.0f,  1.0f, -1.0f}}, //4
                {{ -1.0f, -1.0f, -1.0f}}, //5
                {{  1.0f,  1.0f, -1.0f}}, //6
                {{  1.0f, -1.0f, -1.0f}}, //7
            };

            std::vector<uint32_t> indices = {
                0, 1, 2,
                2, 3, 0,
                4, 1, 0,
                0, 5, 4,
                2, 6, 7,
                7, 3, 2,
                4, 5, 7,
                7, 6, 4,
                0, 3, 7,
                7, 5, 0,
                1, 4, 2,
                2, 4, 6
            };

            for (int i = 0; i < vertices.size(); i++) {
                vertices[i].position *= scale;
            }

            //printf("vertex size ? : %d \n", vertices.size());
            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices} SRC_PASS);
        }

        VertexNT MidPoint(VertexNT const& vertex1, VertexNT const& vertex2) {
            return VertexNT{ {(vertex1.position.x + vertex2.position.x) / 2.f, (vertex1.position.y + vertex2.position.y) / 2.f, (vertex1.position.z + vertex2.position.z) / 2.f}, };
        }

        uint32_t IndexMidPoint(std::unordered_map<uint64_t, uint32_t>& midPoints, std::vector<VertexNT>& vertices, uint32_t index1, uint32_t index2) {
            const uint64_t key = ((long long)std::min(index1, index2) << 32) | std::max(index1, index2);

            {
                auto findRet = midPoints.find(key);
                if (findRet != midPoints.end()) {
                    return findRet->second;
                }
            }
            vertices.push_back(MidPoint(vertices[index1], vertices[index2]));
            uint32_t index = vertices.size() - 1;
            midPoints.try_emplace(key, index);
            return index;
        }

        EWEModel* Sphere(uint8_t const subdivisions, float const radius SRC_PARAM) {
            const float t = 0.851f * radius;
            const float s = 0.526f * radius;

            std::vector<VertexNT> vertices = {
                {lab::vec3{-s, t, 0.f}}, {lab::vec3{s, t, 0.f}}, {lab::vec3{-s, -t, 0.f}}, {lab::vec3{s, -t, 0.f}},
                {lab::vec3{0.f, -s, t}}, {lab::vec3{0.f, s, t}}, {lab::vec3{0.f, -s, -t}}, {lab::vec3{0.f, s, -t}},
                {lab::vec3{t, 0.f, -s}}, {lab::vec3{t, 0.f, s}}, {lab::vec3{-t, 0.f, -s}}, {lab::vec3{-t, 0.f, s}}
            };
            std::vector<uint32_t> indices = {
                0,11,5,  0,5,1,   0,1,7,    0,7,10,  0,10,11,
                1,5,9,   5,11,4,  11,10,2,  10,7,6,  7,1,8,
                3,9,4,   3,4,2,   3,2,6,    3,6,8,   3,8,9,
                4,9,5,   2,4,11,  6,2,10,   8,6,7,   9,8,1
            };
            std::unordered_map<uint64_t, uint32_t> midpoints{};
            for (uint8_t i = 0; i < subdivisions; ++i) {
                std::vector<uint32_t> newIndices{};
                for (size_t j = 0; j < indices.size(); j += 3) {
                    uint32_t a = IndexMidPoint(midpoints, vertices, indices[j], indices[j + 1]);
                    uint32_t b = IndexMidPoint(midpoints, vertices, indices[j + 1], indices[j + 2]);
                    uint32_t c = IndexMidPoint(midpoints, vertices, indices[j + 2], indices[j]);
                    newIndices.insert(newIndices.end(), { 
                        indices[j], a, c, 
                        a, indices[j + 1], b, 
                        c, b, indices[j + 2], 
                        a, b, c 
                    });
                }
                indices = std::move(newIndices);
            }

            for (auto& vert : vertices) {
                vert.position.Normalize();
                vert.normal = vert.position;
                vert.uv.x = 0.5f + std::atan2(vert.position.z, vert.position.x) / lab::GetPI(2.f);
                vert.uv.y = 0.5f - std::asin(vert.position.y) / lab::PI<float>;
            }

            printf("sphere index to vert ratio : %.2f\n", static_cast<float>(indices.size()) / static_cast<float>(vertices.size()));

            //im having an issue with the shading, this reverses the indices (it wasnt the issue)
            //for (uint64_t i = 0; i < indices.size(); i += 3) {
            //    std::swap(indices[i], indices[i + 2]);
            //}

            return Construct<EWEModel>({ vertices.data(), vertices.size(), sizeof(vertices[0]), indices } SRC_PASS);
        }
        /*
        EWEModel* generateSimpleZedQuad(lab::vec2 uvScale = lab::vec2{ 1.f }) {
            std::vector<EffectVertex> vertices{
                {{0.5f,0.0f, -0.5f}, {uvScale.x,uvScale.y}},
                {{-0.5f,0.0f, -0.5f}, {0.0f,uvScale.y}},
                {{-0.5f,0.0f, 0.5f}, {0.0f,0.f}},
                {{0.5f,0.0f, 0.5f}, {uvScale.x,0.f}},
            };
            std::vector<uint32_t> indices{ 0, 1, 2, 2, 3, 0 };
            return std::make_unique<EWEModel>(device, vertices, indices);
        }
        */
    }//namespace Basic_Model
} //namespace EWE