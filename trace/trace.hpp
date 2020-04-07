/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "volsnap/compat.hpp"

#include <string_view>

#include "d8u/util.hpp"
#include "d8u/transform.hpp"

namespace filetrace
{
	using namespace d8u::util;
	using namespace d8u::transform;

	template <typename COMPAT> void volume(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS)
	{
		using namespace d8u::util;

		typename COMPAT::STREAM sn;

		sn.Volume(incremental,volume, snapshot, nullptr, nullptr, nullptr, nullptr, THREADS);
	}
}