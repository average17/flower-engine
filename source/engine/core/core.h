#pragma once
#pragma warning (disable: 4996)
#pragma warning (disable: 4834)
#pragma warning (disable: 4005)

#include "CVar.h"
#include "Log.h"
#include "UtilsPath.h"

#if defined(_DEBUG) || defined(DEBUG)
#define FLOWER_DEBUG
#endif

// always enable log to help developer and user found hidden error and bug.
#define ENABLE_LOG

#ifdef ENABLE_LOG
	#define LOG_TRACE(...) ::flower::Logger::getInstance()->getLoggerUtil()->trace(__VA_ARGS__)
	#define LOG_INFO(...)  ::flower::Logger::getInstance()->getLoggerUtil()->info(__VA_ARGS__)
	#define LOG_WARN(...)  ::flower::Logger::getInstance()->getLoggerUtil()->warn(__VA_ARGS__)
	#define LOG_ERROR(...) ::flower::Logger::getInstance()->getLoggerUtil()->error(__VA_ARGS__)
	#define LOG_FATAL(...) ::flower::Logger::getInstance()->getLoggerUtil()->critical(__VA_ARGS__);throw std::runtime_error("Utils Fatal!")

	#define LOG_IO_TRACE(...) ::flower::Logger::getInstance()->getLoggerIo()->trace(__VA_ARGS__)
	#define LOG_IO_INFO(...)  ::flower::Logger::getInstance()->getLoggerIo()->info(__VA_ARGS__)
	#define LOG_IO_WARN(...)  ::flower::Logger::getInstance()->getLoggerIo()->warn(__VA_ARGS__)
	#define LOG_IO_ERROR(...) ::flower::Logger::getInstance()->getLoggerIo()->error(__VA_ARGS__)
	#define LOG_IO_FATAL(...) ::flower::Logger::getInstance()->getLoggerIo()->critical(__VA_ARGS__);throw std::runtime_error("IO Fatal!")

	#define LOG_GRAPHICS_TRACE(...) ::flower::Logger::getInstance()->getLoggerGraphics()->trace(__VA_ARGS__)
	#define LOG_GRAPHICS_INFO(...)  ::flower::Logger::getInstance()->getLoggerGraphics()->info(__VA_ARGS__)
	#define LOG_GRAPHICS_WARN(...)  ::flower::Logger::getInstance()->getLoggerGraphics()->warn(__VA_ARGS__)
	#define LOG_GRAPHICS_ERROR(...) ::flower::Logger::getInstance()->getLoggerGraphics()->error(__VA_ARGS__)
	#define LOG_GRAPHICS_FATAL(...) ::flower::Logger::getInstance()->getLoggerGraphics()->critical(__VA_ARGS__);throw std::runtime_error("Graphics Fatal!")
#else
	#define LOG_TRACE(...)   
	#define LOG_INFO(...)    
	#define LOG_WARN(...)   
	#define LOG_ERROR(...)    
	#define LOG_FATAL(...) throw std::runtime_error("UtilsFatal!")

	#define LOG_IO_TRACE(...) 
	#define LOG_IO_INFO(...)
	#define LOG_IO_WARN(...)    
	#define LOG_IO_ERROR(...)   
	#define LOG_IO_FATAL(...) throw std::runtime_error("IOFatal!")

	#define LOG_GRAPHICS_TRACE(...) 
	#define LOG_GRAPHICS_INFO(...)
	#define LOG_GRAPHICS_WARN(...)  
	#define LOG_GRAPHICS_ERROR(...)   
	#define LOG_GRAPHICS_FATAL(...) throw std::runtime_error("GraphicsFatal!")
#endif


#define ASSERT(x, ...) { if(!(x)) { LOG_FATAL("Assert failed: {0}.", __VA_ARGS__); __debugbreak(); } }
#define CHECK(x) { if(!(x)) { LOG_FATAL("Check error."); __debugbreak(); } }

#include <vector>
#include <mutex>

// glm force radians.
#define GLM_FORCE_RADIANS

// glm vulkan depth force 0 to 1.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// glm enable experimental.
#define GLM_ENABLE_EXPERIMENTAL

// glm force default aligned gentypes.
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

// common glm headers.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

template<typename T>
const char* getTypeName()
{
	return typeid(T).name();
}

template<typename T>
const char* getTypeName(T)
{
	return typeid(T).name();
}

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/cereal.hpp> 
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/base_class.hpp>

namespace glm
{
	template<class Archive> void serialize(Archive& archive, glm::vec2& v)  { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::vec3& v)  { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::vec4& v)  { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::ivec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::ivec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::ivec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::uvec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::uvec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::uvec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::dvec2& v) { archive(v.x, v.y); }
	template<class Archive> void serialize(Archive& archive, glm::dvec3& v) { archive(v.x, v.y, v.z); }
	template<class Archive> void serialize(Archive& archive, glm::dvec4& v) { archive(v.x, v.y, v.z, v.w); }
	template<class Archive> void serialize(Archive& archive, glm::mat2& m)  { archive(m[0], m[1]); }
	template<class Archive> void serialize(Archive& archive, glm::dmat2& m) { archive(m[0], m[1]); }
	template<class Archive> void serialize(Archive& archive, glm::mat3& m)  { archive(m[0], m[1], m[2]); }
	template<class Archive> void serialize(Archive& archive, glm::mat4& m)  { archive(m[0], m[1], m[2], m[3]); }
	template<class Archive> void serialize(Archive& archive, glm::dmat4& m) { archive(m[0], m[1], m[2], m[3]); }
	template<class Archive> void serialize(Archive& archive, glm::quat& q)  { archive(q.x, q.y, q.z, q.w); }
	template<class Archive> void serialize(Archive& archive, glm::dquat& q) { archive(q.x, q.y, q.z, q.w); }
}

constexpr size_t CACHELINE_SIZE = 64;

inline uint32_t getNextPOT(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline float Halton(uint64_t index, uint64_t base)
{
	float f = 1; float r = 0;
	while (index > 0)
	{
		f = f / static_cast<float>(base);
		r = r + f * (index % base);
		index = index / base;
	}
	return r;
}

inline glm::vec2 Halton2D(uint64_t index, uint64_t baseA, uint64_t baseB)
{
	return glm::vec2(Halton(index, baseA), Halton(index, baseB));
}