/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "tdb/fs.hpp"

#include "d8u/util.hpp"
#include "d8u/transform.hpp"

#include "dircopy/restore.hpp"

#include "mount.hpp"

namespace filetrace
{
	namespace restore
	{
		using namespace d8u::util;
		using namespace d8u::transform;

		template <typename S, typename D> Direct file(std::string_view dest, std::string_view file, S& store, const D& domain, bool validate_blocks = false, bool hash_file = false, size_t P = 1)
		{
			Statistics s;

			//file2(s, dest, file_key, store, domain, validate_blocks, hash_file, P);

			return s.direct;
		}

		template <typename S, typename D> Direct folder(std::string_view dest, std::string_view path, const DefaultHash& volume_key, S& store, const D& domain, bool validate_blocks = false, bool hash_file = false, size_t BLOCK = 1024 * 1024, size_t THRESHOLD = 128 * 1024 * 1024, size_t P = 1, size_t F = 1)
		{
			Statistics s;

			auto folder_record = dircopy::restore::block(s, volume_key, store, domain, validate_blocks);

			auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_file);


			filetrace::Mount< tdb::filesystem::MinimalIndex32M > trace(database);

			trace.Enumerate([&](auto& row)
			{

			});

			return s.direct;
		}

		template <typename S, typename D> Direct volume(std::string_view dest, const DefaultHash& volume_key, S& store, const D& domain, bool validate_blocks = false, bool hash_file = false, size_t BLOCK = 1024 * 1024, size_t THRESHOLD = 128 * 1024 * 1024, size_t P = 1, size_t F = 1)
		{
			Statistics s;

			auto folder_record = dircopy::restore::block(s, volume_key, store, domain, validate_blocks);

			auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_file);


			filetrace::Mount< tdb::filesystem::MinimalIndex32M > trace(database);

			trace.EnumerateFiles([&](auto& file)
			{

			});

			return s.direct;
		}
	}
}

