#pragma once

// For now, error if any of these names are already defined. We could override them or hope they
// work like we'd like, but opt for breaking the build should there actually be a conflict
#if defined(PP_CONCAT1)
#	error PP_CONCAT1 already defined
#endif

#if defined(PP_CONCAT)
#	error PP_CONCAT already defined
#endif

#if defined(PP_UNIQUE_VAR)
#	error PP_UNIQUE_VAR already defined
#endif

#define PP_CONCAT1(X, Y) X##Y
#define PP_CONCAT(X, Y) PP_CONCAT1(X, Y)
#define PP_UNIQUE_VAR(name) PP_CONCAT(name, __LINE__)
