#pragma once

#include "EWEngine/Systems/PipelineSystem.h"

#include <LAB/Transform.h>

#include <random>

//angF = angularFrequency * timestep
//EllRatio = ratio of minor to major axis in ellipse
//rotRatio = ratio of elliptical oscillation to rotation of leaf itself

//credit to Martin Pazout, his masters thesis on Falling Leaves Simulation
//idk how to cite so im just gonna put the school too
//czech technical university in prague


namespace EWE {
	class LeafSystem : private PipelineSystem {
	private:
		const int LEAF_COUNT = 250;
		const float WIND_SPEED{4.f};
	public:

		enum LeafFallMotions {
			LF_Steady,
			LF_Tumbling,
			LF_Fluttering,
			LF_Chaotic, //only diff between chaotic and tumbling is directrion of rotation
			LF_Helix,
			LF_Spiral, //no diff between helix and spiral, just variable values
		};

		struct LeafStruct {
			lab::Transform<float, 3> transform{};
			lab::vec3 origin{ 0.f };
			LeafFallMotions fallMotion{ LF_Steady };
			lab::vec3 averageVelocity{ 0 };
			float angF{ 0.f };
			float ellRatio{ 0.f };
			float rotRatio{ 0.f };
			float ellOsc{ 0.f };
			float time = 0.f;
			float fallAmplitude{ 0.f };
			float swingAmplitude{ 0.f };
			float rotSpeed{ 0.f };
		};


		//helix -> ellRatio~1  rotRatio~1
		//spiral -> elLRatio~0 rotRatio~4
		LeafSystem();
		~LeafSystem() override;

		void LeafPhysicsInitialization();
		void FallCalculation(float timeStep);

		////this should be a graphics queue command buffer
		void InitData();

		void LoadLeafModel();
		void LoadLeafTexture();
		void CreateDescriptor();

		void Render();


	protected:
		void CreatePipeline() final;
		void CreatePipeLayout() final;

	private:
		std::random_device ranDev;
		std::mt19937 randomGen;
		std::uniform_real_distribution<float> ellipseRatioDistribution;
		std::uniform_real_distribution<float> rotRatioDistribution;
		std::uniform_int_distribution<int> motionDistribution;
		std::uniform_real_distribution<float> ellipseOscDistribution;
		std::uniform_real_distribution<float> angularFrequencyDistribution;
		std::uniform_real_distribution<float> initTimeDistribution;
		std::uniform_real_distribution<float> depthVarianceDistribution;
		std::uniform_real_distribution<float> widthVarianceDistribution;
		std::uniform_real_distribution<float> fallSwingVarianceDistribution;
		std::uniform_real_distribution<float> initHeightVarianceDistribution;
		std::uniform_real_distribution<float> varianceDistribution;
		std::uniform_real_distribution<float> rockDist;



		const float gravity = -1.06566f;
		//float FrictionPerp = 5.f;
		//float leafWeight = 1.f; //grams
		//float leafDensity = 0.1f;
		//float leafKA = 4.f;

		std::vector<LeafStruct> leafs{};

		std::array<float*, MAX_FRAMES_IN_FLIGHT> leafBufferData{};
		std::array<EWEBuffer*, MAX_FRAMES_IN_FLIGHT> leafBuffer{};

		EWEModel* leafModel;
		//ImageInfo leafImageInfo;
		ImageID leafImgID{IMAGE_INVALID};
		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> leafDescriptor{};

		VkShaderModule vertexShaderModule{VK_NULL_HANDLE};
		VkShaderModule fragmentShaderModule{VK_NULL_HANDLE};

		EWEDescriptorSetLayout* leafEDSL{};

		/*
		//POSITION:
			yVelocity = 200; //pixels per second
			oscFreq = 1.5; //oscillations per second
			oscDepth = 35; //oscillation depth (pixels)
			drift = 25; // drift (wind?) (pixels per second: - = left, + = right)

			value + [oscDepth * Math.sin(oscFreq * Math.PI * 2 * time) + drift * time,
				yVelocity * time, 0]

		//Z ROTATION :

			seed_random(index, true);
			random(360);

		//Y ROTATION :

			oscFreq = 1.5;
			maxTilt = 15; //degrees

			maxTilt* Math.cos(oscFreq* Math.PI * 2 * time)
		*/
	};
}