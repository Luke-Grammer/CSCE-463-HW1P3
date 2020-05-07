// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#include <windows.h>
#include <inttypes.h>
#include <shlwapi.h>

#include <iostream>
#include <string>
#include <exception>
#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <queue>

#include "Properties.h"
#include "StatsManager.h"
#include "Constants.h"
#include "HTMLParserBase.h"
#include "FileHandler.h"
#include "ParsedURL.h"
#include "WebCrawler.h"

#endif //PCH_H
