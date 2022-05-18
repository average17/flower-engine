#pragma once

namespace flower{
	
	struct TickData
	{
		// Application minimized?
		bool  bIsMinimized = false;

		// Application lose focus?
		bool  bLoseFocus = false;

		// Application tick time.
		float deltaTime = 0.0f;
		float smoothDeltaTime = 0.0f;
	};

}