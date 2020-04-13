/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "volsnap/platform.hpp"

#include <string_view>

#include "d8u/util.hpp"
#include "d8u/transform.hpp"

#include "tdb/fs.hpp"

namespace filetrace
{
	using namespace d8u::util;
	using namespace d8u::transform;

	void volume(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS = 16, size_t BUFFER = 16 * 1024 * 1024)
	{
		using namespace d8u::util;

		typename volsnap::Platform::VOLUME sn;

		sn.Volume<tdb::filesystem::MinimalIndex32>(incremental,volume, snapshot, nullptr, nullptr, nullptr, nullptr, nullptr, THREADS,BUFFER,false);
	}

	void volume_sha256(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS = 16, size_t BUFFER = 16 * 1024 * 1024)
	{
		using namespace d8u::util;

		typename volsnap::Platform::VOLUME sn;

		std::vector< HashState > groups;

		sn.Volume<tdb::filesystem::MinimalIndex32>(incremental, volume, snapshot, [&](auto group, auto s, auto o, auto t, auto th)
		{
			groups[group] = HashState();
		}, [&](auto group, auto block)
		{
			groups[group].Update(block);
		}, [&](auto group, auto & output)
		{
			auto result = groups[group].Finish();

			output.insert(output.end(), (uint8_t*)&result, ((uint8_t*)&result) + sizeof(result));
		}, nullptr,
		[&](auto _groups)
		{
			groups.resize(_groups);
		}, THREADS, BUFFER,false);
	}
}