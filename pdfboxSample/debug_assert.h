// debug_assert.h
#pragma once

#ifdef _WIN32
#	include <crtdbg.h>
#else
#	include <assert.h>
#	define _ASSERTE assert
#endif
