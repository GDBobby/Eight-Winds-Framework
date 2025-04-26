#pragma once
#include "EWGraphics/Model/Model.h"

namespace EWE {
    namespace Basic_Model {
        EWEModel* Quad(lab::vec2 uvScale = lab::vec2{ 1.f } SRC_HEADER_PARAM);
        EWEModel* QuadPNU(lab::vec2 uvScale = lab::vec2{ 1.f } SRC_HEADER_PARAM);
        EWEModel* Simple3DQuad(lab::vec2 uvScale = lab::vec2{ 1.f } SRC_HEADER_PARAM);
        EWEModel* TileQuad3D(lab::vec2 uvScale SRC_HEADER_PARAM);

        EWEModel* Grid2D(lab::vec2 scale = { 1.f,1.f } SRC_HEADER_PARAM);
        EWEModel* Grid3DTrianglePrimitive(const uint32_t patchSize, const lab::vec2 uvScale = { 1.f, 1.f } SRC_HEADER_PARAM);
        EWEModel* Grid3DQuadPrimitive(const uint32_t patchSize, lab::vec2 uvScale = {1.f, 1.f} SRC_HEADER_PARAM);
        EWEModel* Quad2D(lab::vec2 scale = { 1.f,1.f } SRC_HEADER_PARAM);

        EWEModel* NineUIQuad(SRC_HEADER_FIRST_PARAM);

        EWEModel* Circle(uint16_t const points, float radius = 0.5f SRC_HEADER_PARAM);

        EWEModel* SkyBox(float scale SRC_HEADER_PARAM);
        EWEModel* Sphere(uint8_t const subdivisions, float const radius SRC_HEADER_PARAM);
    };
}

