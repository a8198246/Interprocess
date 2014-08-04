#ifndef HEADER_VTT_INTERPROCESS_DETAILS_PRECOMPILED
#define HEADER_VTT_INTERPROCESS_DETAILS_PRECOMPILED

#pragma once

#include "Main.hpp"
#include "Main Windows SDK Control.hpp"
#include "Main Boost Control.hpp"

#include <intrin.h>

#include <sal.h>

#include <array>
#include <string>
#include <vector>
#include <utility>
#include <cassert>
#include <memory.h>

#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/thread.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

#endif