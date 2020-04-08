/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "volsnap/compat.hpp"

#include <string_view>

#include "d8u/util.hpp"
#include "d8u/transform.hpp"

#include "tdb/fs.hpp"

namespace filetrace
{
	using namespace d8u::util;
	using namespace d8u::transform;

	void volume(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS)
	{
		using namespace d8u::util;

		typename volsnap::Platform::STREAM sn;

		sn.Volume<tdb::filesystem::HalfIndex32>(incremental,volume, snapshot, nullptr, nullptr, nullptr, nullptr, THREADS);
	}

	void volume_sha256(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS)
	{
		using namespace d8u::util;

		typename volsnap::Platform::STREAM sn;

		sn.Volume<tdb::filesystem::HalfIndex32>(incremental, volume, snapshot, nullptr, [&](auto group, auto block)
		{

		}, [&](auto group, auto & output)
		{

		}, nullptr, THREADS);
	}
}