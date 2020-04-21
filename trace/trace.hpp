/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "volsnap/platform.hpp"

#include <string>
#include <string_view>

#include "d8u/util.hpp"
#include "d8u/transform.hpp"

#include "dircopy/defs.hpp"
#include "dircopy/backup.hpp"

#include "tdb/fs.hpp"

namespace filetrace
{
	using namespace d8u::util;
	using namespace d8u::transform;

	std::string volume(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS = 16, size_t BUFFER = 16 * 1024 * 1024)
	{
		using namespace d8u::util;

		typename volsnap::Platform::VOLUME sn;

		return sn.Volume<tdb::filesystem::MinimalIndex32>(incremental,volume, snapshot, nullptr, nullptr, nullptr, nullptr, nullptr, THREADS,BUFFER,false);
	}

	template <typename STORE, typename D> DefaultHash volume_store(bool incremental, std::string_view _volume, std::string_view snapshot, STORE& store, const D& domain = default_domain, size_t THREADS = 16, size_t BUFFER = 16 * 1024 * 1024,int c = 5)
	{
		Statistics stats;

		return dircopy::backup::submit_file2(stats, volume(incremental,_volume,snapshot,THREADS,BUFFER), store, domain, 1024 * 1024, THREADS, c);
	}

	std::string volume_sha256(bool incremental, std::string_view volume, std::string_view snapshot, size_t THREADS = 16, size_t BUFFER = 16 * 1024 * 1024)
	{
		using namespace d8u::util;

		typename volsnap::Platform::VOLUME sn;

		std::vector< HashState > groups;

		return sn.Volume<tdb::filesystem::MinimalIndex32>(incremental, volume, snapshot, [&](auto group, auto s, auto o, auto t, auto th)
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

	template <typename STORE, typename D> DefaultHash volume_sha256_store(bool incremental, std::string_view _volume, std::string_view snapshot, STORE& store, const D& domain = default_domain, size_t THREADS = 16, size_t BUFFER = 16 * 1024 * 1024, int c = 5)
	{
		Statistics stats;

		return dircopy::backup::submit_file2(stats, volume_sha256(incremental, _volume, snapshot, THREADS, BUFFER), store, domain, 1024 * 1024, THREADS, c);
	}

	template <typename STORE, typename D, typename CB = decltype(nullptr)> DefaultHash volume_files(bool incremental, Statistics& stats, std::string_view volume, std::string_view snapshot, STORE& store, const D& domain = default_domain, CB on_file = nullptr, size_t FILES = 16, size_t THREADS = 32, size_t BUFFER = 16 * 1024 * 1024, int compression = 5)
	{
		using namespace d8u::util;

		typename volsnap::Platform::VOLUME sn;

		struct thread_context
		{
			thread_context() :target(0) {}

			thread_context(size_t t)
				: target(t)
			{
				keys.resize(target + 1);
			}

			thread_context(const thread_context& r)
			{
				*this = r;
			}

			void operator =(const thread_context& r)
			{
				ready = r.ready.load();
				current = r.current;
				target = r.target;
				keys = r.keys;
				file_hash = r.file_hash;
			}

			std::atomic<size_t> ready = 0;
			size_t current = 0;
			size_t target;
			std::vector<DefaultHash> keys;
			HashState file_hash;
		};

		std::vector< thread_context > groups;

		auto meta = sn.Volume<tdb::filesystem::MinimalIndex32>(incremental, volume, snapshot, [&](auto group, auto size, auto name, auto runs, auto index)
		{
			if constexpr (!std::is_null_pointer< decltype(on_file) >())
				on_file(name);

			groups[group] = thread_context(size / (1024 * 1024) + ((size % (1024 * 1024)) ? 1 : 0));
		}, [&](auto group, auto block)
		{
			groups[group].file_hash.Update(block);

			dircopy::backup::async_block(THREADS, groups[group].keys[groups[group].current++], stats, std::move(block), store, domain, compression, &groups[group].ready);
		}, [&](auto group, auto& output)
		{
			groups[group].keys.back() = groups[group].file_hash.Finish();

			fast_until(groups[group].ready, groups[group].target);

			if (groups[group].target >= 128)
			{
				auto buffer = d8u::t_buffer_copy<uint8_t>(groups[group].keys);
				auto group_key = dircopy::backup::block(stats, buffer, store, domain, compression);

				output.insert(output.end(), (uint8_t*)group_key.data(), ((uint8_t*)group_key.data()) + sizeof(DefaultHash));
				output.insert(output.end(), (uint8_t*)groups[group].keys.back().data(), ((uint8_t*)groups[group].keys.back().data()) + sizeof(DefaultHash));
			}
			else
				output.insert(output.end(), (uint8_t*)groups[group].keys.data(), ((uint8_t*)groups[group].keys.data()) + sizeof(DefaultHash) * groups[group].keys.size());

		}, nullptr,
			[&](auto _groups)
		{
			groups.resize(_groups);
		}, FILES, BUFFER, false);

		return dircopy::backup::submit_file2(stats, meta, store, domain, 1024 * 1024, THREADS, compression);
	}

}