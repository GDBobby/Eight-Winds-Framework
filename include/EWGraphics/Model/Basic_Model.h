#pragma once
#include "EWGraphics/Model/Model.h"

namespace EWE {
    namespace Basic_Model {
        EWEModel* Quad(lab::vec2 uvScale = lab::vec2{ 1.f });
        EWEModel* QuadPNU(lab::vec2 uvScale = lab::vec2{ 1.f });
        EWEModel* Simple3DQuad(lab::vec2 uvScale = lab::vec2{ 1.f });
        EWEModel* TileQuad3D(lab::vec2 uvScale);

        EWEModel* Grid2D(lab::vec2 scale = { 1.f,1.f });
        EWEModel* Grid3DTrianglePrimitive(const uint32_t patchSize, const lab::vec2 uvScale = { 1.f, 1.f });
        EWEModel* Grid3DQuadPrimitive(const uint32_t patchSize, lab::vec2 uvScale = {1.f, 1.f});
        EWEModel* Quad2D(lab::vec2 scale = { 1.f,1.f });

        EWEModel* NineUIQuad();

        EWEModel* Circle(uint16_t const points, float radius = 0.5f);

        EWEModel* SkyBox(float scale);
        EWEModel* Sphere(uint8_t const subdivisions, float const radius);
    };
}

