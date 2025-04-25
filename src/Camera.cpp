#include "EWEngine/Graphics/Camera.h"

#include "EWEngine/Graphics/DescriptorHandler.h"

#include <LAB/Camera.h>

namespace EWE {
	EWECamera::EWECamera() {
		view.At(3, 3) = 1.f;
	}


	void EWECamera::SetBuffers() {
		DescriptorHandler::SetCameraBuffers(uniformBuffers);
	}

	void EWECamera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
		//projection = lab::mat4{ 1.0f };
		projection = lab::OrthographicMatrix(bottom, top, left, right, near, far);
	}

	void EWECamera::SetPerspectiveProjection(float fovy, float aspect, float near, float far) {
		//inverting aspect, to make it height / width instead of width / height
		projection = lab::ProjectionMatrix<lab::Perspective::Vulkan>(fovy, aspect, near, far);
		//projection = lab::CreateProjectionMatrix(fovy * 1.1f, aspect * 1.1f, near * 1.1f, far * 1.1f);
		//projection = glm::perspective(fovy, aspect, near, far);
		conservativeProjection = lab::ProjectionMatrix<lab::Perspective::Vulkan>(fovy * 1.1f, aspect * 1.1f, near * 1.1f, far * 1.1f);
	}

	void EWECamera::BindBothUBOs() {
		uniformBuffers->at(0)->WriteToBuffer(&ubo, sizeof(GlobalUbo));
		uniformBuffers->at(0)->Flush();
		uniformBuffers->at(1)->WriteToBuffer(&ubo, sizeof(GlobalUbo));
		uniformBuffers->at(1)->Flush();
	}
	void EWECamera::BindUBO() {
		//printf("camera set ubo \n");
		//printf("offset of camera pos : %zu\n", offsetof(GlobalUbo, GlobalUbo::cameraPos));
		uniformBuffers->at(VK::Object->frameIndex)->WriteToBuffer(&ubo, sizeof(GlobalUbo));
		uniformBuffers->at(VK::Object->frameIndex)->Flush();
	}
	void EWECamera::PrintCameraPos() {
		printf("camera pos : %.3f:%.3f:%.3f\n", ubo.cameraPos.x, ubo.cameraPos.y, ubo.cameraPos.z);
	}
	void EWECamera::UpdateViewData(lab::vec3 const& position, lab::vec3 const& target, lab::vec3 const& cameraUp) {
		//probably store a position, target, and camera up variable in this class, then hand out a pointer to those variables
		//being lazy rn
		this->position = position;
		this->target = target;
		this->cameraUp = cameraUp;
		dataHasBeenUpdated = MAX_FRAMES_IN_FLIGHT;
	};


	std::array<lab::vec4, 6> EWECamera::GetFrustumPlanes() {

		enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, NEAR = 4, FAR = 5 };
		std::array<lab::vec4, 6> planes;
		planes[LEFT].x = ubo.projView.columns[0].w + ubo.projView.columns[0].x;
		planes[LEFT].y = ubo.projView.columns[1].w + ubo.projView.columns[1].x;
		planes[LEFT].z = ubo.projView.columns[2].w + ubo.projView.columns[2].x;
		planes[LEFT].w = ubo.projView.columns[3].w + ubo.projView.columns[3].x;

		planes[RIGHT].x = ubo.projView.columns[0].w - ubo.projView.columns[0].x;
		planes[RIGHT].y = ubo.projView.columns[1].w - ubo.projView.columns[1].x;
		planes[RIGHT].z = ubo.projView.columns[2].w - ubo.projView.columns[2].x;
		planes[RIGHT].w = ubo.projView.columns[3].w - ubo.projView.columns[3].x;

		planes[TOP].x = ubo.projView.columns[0].w - ubo.projView.columns[0].y;
		planes[TOP].y = ubo.projView.columns[1].w - ubo.projView.columns[1].y;
		planes[TOP].z = ubo.projView.columns[2].w - ubo.projView.columns[2].y;
		planes[TOP].w = ubo.projView.columns[3].w - ubo.projView.columns[3].y;

		planes[BOTTOM].x = ubo.projView.columns[0].w + ubo.projView.columns[0].y;
		planes[BOTTOM].y = ubo.projView.columns[1].w + ubo.projView.columns[1].y;
		planes[BOTTOM].z = ubo.projView.columns[2].w + ubo.projView.columns[2].y;
		planes[BOTTOM].w = ubo.projView.columns[3].w + ubo.projView.columns[3].y;

		planes[NEAR].x = ubo.projView.columns[0].w + ubo.projView.columns[0].z;
		planes[NEAR].y = ubo.projView.columns[1].w + ubo.projView.columns[1].z;
		planes[NEAR].z = ubo.projView.columns[2].w + ubo.projView.columns[2].z;
		planes[NEAR].w = ubo.projView.columns[3].w + ubo.projView.columns[3].z;

		planes[FAR].x = ubo.projView.columns[0].w - ubo.projView.columns[0].z;
		planes[FAR].y = ubo.projView.columns[1].w - ubo.projView.columns[1].z;
		planes[FAR].z = ubo.projView.columns[2].w - ubo.projView.columns[2].z;
		planes[FAR].w = ubo.projView.columns[3].w - ubo.projView.columns[3].z;

		return planes;
	}
		
	std::array<lab::vec4, 6> EWECamera::GetConservativeFrustumPlanes(const lab::vec3 position, const lab::vec3 rotation) {

		const float c3 = lab::Cos(rotation.z);
		const float s3 = lab::Sin(rotation.z);
		const float c2 = lab::Cos(rotation.x);
		const float s2 = lab::Sin(rotation.x);
		const float c1 = lab::Cos(rotation.y);
		const float s1 = lab::Sin(rotation.y);
		const lab::vec3 forwardDir = lab::Normalized(lab::vec3{ s1, -s2, c1 });
		const lab::vec3 conversativePosition = position + forwardDir * 5.f;
		const lab::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
		const lab::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
		const lab::vec3 w{ (c2 * s1), (-s2), (c1 * c2)};
		//view = lab::mat4{ 1.f };

		lab::mat4 conservativeView;

		conservativeView.columns[0][3] = 0.f;
		conservativeView.columns[1][3] = 0.f;
		conservativeView.columns[2][3] = 0.f;

		conservativeView.columns[0][0] = u.x;
		conservativeView.columns[1][0] = u.y;
		conservativeView.columns[2][0] = u.z;
		conservativeView.columns[0][1] = v.x;
		conservativeView.columns[1][1] = v.y;
		conservativeView.columns[2][1] = v.z;
		conservativeView.columns[0][2] = w.x;
		conservativeView.columns[1][2] = w.y;
		conservativeView.columns[2][2] = w.z;
		conservativeView.columns[3][0] = -lab::Dot(u, conversativePosition);
		conservativeView.columns[3][1] = -lab::Dot(v, conversativePosition);
		conservativeView.columns[3][2] = -lab::Dot(w, conversativePosition);
		conservativeView.columns[3][3] = 1.f;

		const lab::mat4 fakeProjView = conservativeProjection * conservativeView;

		enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, NEAR = 4, FAR = 5 };
		std::array<lab::vec4, 6> planes;
		planes[LEFT].x = fakeProjView.columns[0].w + fakeProjView.columns[0].x;
		planes[LEFT].y = fakeProjView.columns[1].w + fakeProjView.columns[1].x;
		planes[LEFT].z = fakeProjView.columns[2].w + fakeProjView.columns[2].x;
		planes[LEFT].w = fakeProjView.columns[3].w + fakeProjView.columns[3].x;

		planes[RIGHT].x = fakeProjView.columns[0].w - fakeProjView.columns[0].x;
		planes[RIGHT].y = fakeProjView.columns[1].w - fakeProjView.columns[1].x;
		planes[RIGHT].z = fakeProjView.columns[2].w - fakeProjView.columns[2].x;
		planes[RIGHT].w = fakeProjView.columns[3].w - fakeProjView.columns[3].x;

		planes[TOP].x = fakeProjView.columns[0].w - fakeProjView.columns[0].y;
		planes[TOP].y = fakeProjView.columns[1].w - fakeProjView.columns[1].y;
		planes[TOP].z = fakeProjView.columns[2].w - fakeProjView.columns[2].y;
		planes[TOP].w = fakeProjView.columns[3].w - fakeProjView.columns[3].y;

		planes[BOTTOM].x = fakeProjView.columns[0].w + fakeProjView.columns[0].y;
		planes[BOTTOM].y = fakeProjView.columns[1].w + fakeProjView.columns[1].y;
		planes[BOTTOM].z = fakeProjView.columns[2].w + fakeProjView.columns[2].y;
		planes[BOTTOM].w = fakeProjView.columns[3].w + fakeProjView.columns[3].y;

		planes[NEAR].x = fakeProjView.columns[0].w + fakeProjView.columns[0].z;
		planes[NEAR].y = fakeProjView.columns[1].w + fakeProjView.columns[1].z;
		planes[NEAR].z = fakeProjView.columns[2].w + fakeProjView.columns[2].z;
		planes[NEAR].w = fakeProjView.columns[3].w + fakeProjView.columns[3].z;

		planes[FAR].x = fakeProjView.columns[0].w - fakeProjView.columns[0].z;
		planes[FAR].y = fakeProjView.columns[1].w - fakeProjView.columns[1].z;
		planes[FAR].z = fakeProjView.columns[2].w - fakeProjView.columns[2].z;
		planes[FAR].w = fakeProjView.columns[3].w - fakeProjView.columns[3].z;

		return planes;
	}
}