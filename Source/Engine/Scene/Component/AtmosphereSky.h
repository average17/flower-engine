#pragma once
#include "../Component.h"
#include "../../Core/Core.h"
#include "../../Renderer/DeferredRenderer/SceneStaticUniform.h"

namespace flower
{
	// same with gpu pushconst.
	struct AtmosphereParameters
	{
		glm::vec3 groundAlbedo = glm::vec3(0.3f);
		float mieAbsorptionBase = 4.4f;

		glm::vec3 rayleighScatteringBase = glm::vec3(5.802, 13.558, 33.1);
		float rayleighAbsorptionBase = 0.0f;

		glm::vec3 ozoneAbsorptionBase = glm::vec3(0.650, 1.881, .085);
		float mieScatteringBase = 3.996f;
	
		// Units are in megameters.
		float groundRadiusMM = 6.360f;
		float atmosphereRadiusMM = 6.460f;
		float offsetHeight = 0.0002f;
		float atmosphereExposure = 10.0f;

		float rayleighAltitude = 8.0f;
		float mieAltitude = 1.2f;
		float ozoneAltitude = 25.0f;
		float ozoneThickness = 30.0f;
	};

	class AtmosphereSky : public Component
	{
		friend class Scene;
	public:
		AtmosphereSky() {}
		COMPONENT_NAME_OVERRIDE(AtmosphereSky);

		virtual ~AtmosphereSky() = default;

	public: // parameters.
		AtmosphereParameters params;
	};
}