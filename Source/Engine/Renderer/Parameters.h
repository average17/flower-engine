#pragma once
#include "RendererCommon.h"

namespace Flower
{
	constexpr uint32_t GMaxCascadePerDirectionalLight = 8;
	constexpr uint32_t GMinCascadePerDirectionalLightDimXY = 512;
	constexpr uint32_t GMaxCascadePerDirectionalLightDimXY = 4096;

	// All units in kilometers
	struct EarthAtmosphere
	{
		glm::vec3  absorptionExtinction;
		float multipleScatteringFactor;  // x4

		glm::vec3 rayleighScattering;
		float miePhaseFunctionG; // x4

		glm::vec3 mieScattering;
		float bottomRadius; // x4

		glm::vec3 mieExtinction;
		float topRadius; // x4

		glm::vec3 mieAbsorption;
		uint32_t viewRayMarchMinSPP; // x4

		glm::vec3 groundAlbedo;
		uint32_t viewRayMarchMaxSPP; // x4

		float rayleighDensity[12]; //
		float mieDensity[12]; //
		float absorptionDensity[12]; //

		float cloudAreaStartHeight; // km
		float cloudAreaThickness;
		float atmospherePreExposure;
		float pad1;

		void reset();
	};

	struct GPUDirectionalLightInfo
	{
		glm::vec3  color;
		float intensity;    // x4............

		glm::vec3 direction;
		float shadowFilterSize; // shadow filter size for pcf. used for sdsm.

		uint32_t cascadeCount;
		uint32_t perCascadeXYDim;
		float splitLambda;
		float shadowBiasConst; // x4.....

		float shadowBiasSlope; // shadow bias slope.
		float cascadeBorderAdopt;
		float cascadeEdgeLerpThreshold;
		float maxDrawDistance;

		float maxFilterSize;
		float pad0;
		float pad1;
		float pad2;
	};

	struct TonemapperParam
	{
		float tonemapper_P = 500.0f;  // Max brightness.
		float tonemapper_a = 1.0f;  // contrast
		float tonemapper_m = 0.22f; // linear section start
		float tonemapper_l = 0.4f;  // linear section length

		float tonemapper_c = 1.33f; // black
		float tonemapper_b = 0.0f;  // pedestal
		float tonemmaper_s = 500.0f; // scale 
		float pad1;


		uint32_t shoulder;
		uint32_t con;
		uint32_t soft;
		uint32_t con2;

		uint32_t clip;
		uint32_t scaleOnly;
		uint32_t displayMode;
		uint32_t bHdr10;

		glm::mat4 inputToOutputMatrix;
		glm::ivec4 ctl[24 * 4];
	};

	// See FrameData struct in Common.glsl
	struct GPUFrameData
	{
		alignas(16) glm::vec4 appTime;
		// .x is app runtime
		// .y is sin(.x)
		// .z is cos(.x)
		// .w is pad

		alignas(16) glm::uvec4 frameIndex;
		// .x is frame count
		// .y is frame count % 8
		// .z is frame count % 16
		// .w is frame count % 32

		uint32_t jitterPeriod; // jitter period for jitter data.
		float basicTextureLODBias; // Lod basic texture bias when render mesh.
		uint32_t staticMeshCount;
		uint32_t bSdsmDraw;

		float pad0;
		uint32_t  globalIBLEnable;
		float globalIBLIntensity;
		float pad2;

		alignas(16) glm::vec4 jitterData;
		// .xy is current frame jitter data.
		// .zw is prev frame jitter data.

		// Light infos which cast shadow.
		uint32_t directionalLightCount;
		uint32_t pointLightCount;
		uint32_t spotLightCount;
		uint32_t rectLightCount; // x4

		GPUDirectionalLightInfo directionalLight;

		EarthAtmosphere earthAtmosphere;

		TonemapperParam toneMapper;
	};

	// See ViewData struct in Common.glsl
	struct GPUViewData
	{
		alignas(16) glm::vec4 camWorldPos;

		// .x fovy, .y aspectRatio, .z nearZ, .w farZ
		alignas(16) glm::vec4 camInfo;
		alignas(16) glm::vec4 camInfoPrev; // prev-frame's cam info.

		alignas(16) glm::mat4 camView;
		alignas(16) glm::mat4 camProj;
		alignas(16) glm::mat4 camViewProj;

		alignas(16) glm::mat4 camInvertView;
		alignas(16) glm::mat4 camInvertProj;
		alignas(16) glm::mat4 camInvertViewProj;

		// Matrix always no jitter effects.
		alignas(16) glm::mat4 camProjNoJitter;
		alignas(16) glm::mat4 camViewProjNoJitter;

		// Camera invert matrixs no jitter effects.
		alignas(16) glm::mat4 camInvertProjNoJitter;
		alignas(16) glm::mat4 camInvertViewProjNoJitter;

		// Prev camera infos.
		alignas(16) glm::mat4 camViewProjPrev;
		alignas(16) glm::mat4 camViewProjPrevNoJitter;

		alignas(16) glm::vec4 frustumPlanes[6];

		float cameraAtmosphereOffsetHeight;
		float cameraAtmosphereMoveScale;
		float exposure;
		float ev100;

		float evCompensation;
		float pad0;
		float pad1;
		float pad2;
	};

	class GPUImageAsset;
	struct GPUStaticMeshStandardPBRMaterial;
	struct CPUStaticMeshStandardPBRMaterial
	{
		// Material keep image shared reference avoid release.
		std::shared_ptr<GPUImageAsset> baseColor;
		std::shared_ptr<GPUImageAsset> normal;
		std::shared_ptr<GPUImageAsset> specular;
		std::shared_ptr<GPUImageAsset> occlusion;
		std::shared_ptr<GPUImageAsset> emissive;

		float cutoff = 0.5f;
		float faceCut = 0.0f; 
			// > 1.0f is backface cut, < -1.0f is frontface cut, [-1.0f, 1.0f] is no face cut.

		bool buildWithMaterialUUID(UUID materialId);

		// Return: all material asset load already?
		GPUStaticMeshStandardPBRMaterial buildGPU();
	};
	
	struct GPUStaticMeshStandardPBRMaterial
	{
		uint32_t baseColorId;
		uint32_t baseColorSampler;
		uint32_t normalTexId;
		uint32_t normalSampler; // x4

		uint32_t specTexId;
		uint32_t specSampler;
		uint32_t occlusionTexId;
		uint32_t occlusionSampler; // x4

		uint32_t emissiveTexId;
		uint32_t emissiveSampler;

		float cutoff = 0.5f; // base color alpha cut off.
		float faceCut = 0.0f; // > 1.0f is backface cut, < -1.0f is frontface cut, [-1.0f, 1.0f] is no face cut.

		// x4

		static GPUStaticMeshStandardPBRMaterial buildDeafult();
	};

	// See PerObjectData in StaticMeshCommon.glsl
	struct GPUPerObjectData
	{
		alignas(16) glm::mat4 modelMatrix;
		alignas(16) glm::mat4 modelMatrixPrev;

		uint32_t verticesArrayId;
		uint32_t indicesArrayId;

		// Index start offset position.
		uint32_t indexStartPosition;

		// Current object index count.
		uint32_t indexCount; // x4

		// .xyz is localspace center pos
		// .w   sphere radius
		alignas(16) glm::vec4 sphereBounds;

		// .xyz extent XYZ
		glm::vec3 extents;
		uint32_t bObjectMove;

		// Material.
		GPUStaticMeshStandardPBRMaterial material;
	};

	// See DrawIndirectCommand in StaticMeshCommon.glsl
	struct GPUDrawIndirectCommand
	{
		// Same with VkDrawIndirectCommand.
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstVertex;
		uint32_t firstInstance;

		// Object id for GPUPerObjectData array indexing.
		uint32_t objectId;
	};

	struct GPUDrawIndirectCount
	{
		uint32_t count;
	};

	struct GPUCascadeInfo
	{
		glm::mat4 viewProj;
		glm::vec4 frustumPlanes[6];
		glm::vec4 cascadeScale;
	};

	struct GPUDispatchIndirectCommand
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;
		uint32_t pad;
	};
}