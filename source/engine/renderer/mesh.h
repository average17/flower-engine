#pragma once
#include <cstdint>
#include "../RHI/Common.h"
#include "../RHI/Buffer.h"
#include "../Core/UUID.h"

namespace flower
{
	// standard index type.
	using VertexIndexType = uint32_t;

	// keep same with RHI/Common.h EVertexAttribute
	struct StandardVertex
	{
		glm::vec3 pos     = glm::vec3(0.0f);
		glm::vec2 uv0     = glm::vec2(0.0f);
		glm::vec3 normal  = glm::vec3(0.0f);
		glm::vec4 tangent = glm::vec4(0.0f);
	};
	
	// standard mesh attributes.
	inline std::vector<EVertexAttribute> getStandardMeshAttributes()
	{
		return std::vector<EVertexAttribute>{
			EVertexAttribute::pos,
			EVertexAttribute::uv0,
			EVertexAttribute::normal,
			EVertexAttribute::tangent
		};
	}

	// standard mesh attributes vertex count.
	inline constexpr uint32_t getStandardMeshAttributesVertexCount()
	{
		return 3 + 2 + 3 + 4;
	}

	// render bounds of submesh.
	struct RenderBounds
	{
		glm::vec3 origin;
		glm::vec3 extents;

		float radius;

		static void toExtents(
			const RenderBounds& in, 
			float& zmin, 
			float& zmax, 
			float& ymin, 
			float& ymax, 
			float& xmin, 
			float& xmax, 
			float scale = 1.5f
		);
		
		static RenderBounds combine(
			const RenderBounds& b0, 
			const RenderBounds& b1
		);
	};

	struct SubMesh
	{
		RenderBounds renderBounds = {};

		uint32_t indexStartPosition = 0;
		uint32_t indexCount = 0;

		UUID material = UNVALID_UUID;
	};

	struct Mesh
	{
		std::vector<SubMesh> subMeshes = {};
		std::vector<EVertexAttribute> layout = {};

		uint32_t vertexStartPosition = 0;
		uint32_t vertexCount = 0;

		uint32_t indexStartPosition = 0;
		uint32_t indexCount = 0;

		bool bReady;
		Mesh* fallBackMesh = nullptr;

		Mesh* getValidRenderMesh();
	};
}