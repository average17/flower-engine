#include "UUID.h"
#include "Core.h"
#define UUID_SYSTEM_GENERATOR
#include "uuid/uuid.h"

namespace flower
{
	std::string getUUID()
	{
		uuids::uuid const id = uuids::uuid_system_generator{}();
		CHECK(!id.is_nil());
		CHECK(id.version() == uuids::uuid_version::random_number_based);
		CHECK(id.variant() == uuids::uuid_variant::rfc);

		return uuids::to_string(id);
	}
}