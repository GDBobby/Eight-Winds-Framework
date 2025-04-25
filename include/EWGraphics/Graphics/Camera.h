#pragma once

#include "EWEngine/Graphics/LightBufferObject.h"
#include "EWEngine/Graphics/Device_Buffer.h"

#include <LAB/Camera.h>

#include <vector>


namespace EWE {
	class EWECamera {
	public:
		EWECamera();

		void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
		void SetPerspectiveProjection(float fovy, float aspect, float near, float far);

		template<typename CoordinateSystem>
		requires(lab::IsCoordinateSystem<CoordinateSystem>::value)
		void ViewDirection(const lab::vec3 position, const lab::vec3 direction, const lab::vec3 up = lab::vec3{ 0.0f,1.0f, 0.0f }) {
			lab::ViewDirection<CoordinateSystem>(view, position, direction);
			ubo.projView = projection * view;
			ubo.cameraPos = position;
		}

		template<typename CoordinateSystem>
		requires(lab::IsCoordinateSystem<CoordinateSystem>::value)
		void ViewRotation(const lab::vec3 position, const lab::vec3 rotation) {
			lab::ViewRotation<CoordinateSystem>(view, position, rotation);
			ubo.projView = projection * view;
			ubo.cameraPos = position;
		}

		template<typename CoordinateSystem>
		requires(lab::IsCoordinateSystem<CoordinateSystem>::value)
		void UpdateCamera() {
			if (dataHasBeenUpdated == 0) {
				return;
			}
			dataHasBeenUpdated--;
			const lab::vec3 forward = lab::Normalized(target - position);
			ViewDirection<CoordinateSystem>(position, forward, cameraUp);
			uniformBuffers->at(VK::Object->frameIndex)->WriteToBuffer(&ubo);
			uniformBuffers->at(VK::Object->frameIndex)->Flush();
		}
		//void SetViewYXZ(lab::vec3 const& position, lab::vec3 const& rotation);

		const lab::mat4& GetProjection() const { return projection; }
		const lab::mat4& GetView() const { return view; }

		void BindBothUBOs();
		void BindUBO();

		void SetBuffers();
		void UpdateViewData(lab::vec3 const& position, lab::vec3 const& target, lab::vec3 const& cameraUp = lab::vec3{ 0.f,1.f,0.f });
		void PrintCameraPos();

		std::array<lab::vec4, 6> GetFrustumPlanes();
		std::array<lab::vec4, 6> GetConservativeFrustumPlanes(const lab::vec3 position, const lab::vec3 rotation);

		lab::mat4 projection{ 0.f };
		lab::mat4 view{ 0.f };
		lab::mat4 conservativeProjection;

		lab::vec3 position;
		lab::vec3 target;
		lab::vec3 cameraUp{ 0.f, 1.f, 0.f };
		GlobalUbo ubo{};
	private:
		std::array<EWEBuffer*, MAX_FRAMES_IN_FLIGHT>* uniformBuffers;

		uint8_t dataHasBeenUpdated = 0;

	};
}