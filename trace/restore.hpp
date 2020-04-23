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
		//TODO flatten common code.

		using namespace d8u::util;
		using namespace d8u::transform;

		template <typename S, typename D> Direct file(std::string_view dest, std::string_view path, const DefaultHash& volume_key, S& store, const D& domain, bool validate_blocks = false, bool hash_file = false, size_t BLOCK = 1024 * 1024, size_t THRESHOLD = 128 * 1024 * 1024, size_t P = 1)
		{
			Statistics s;

			auto folder_record = dircopy::restore::block(s, volume_key, store, domain, validate_blocks);

			auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_file);


			filetrace::Mount< tdb::filesystem::MinimalIndex32M > table(database);

			if (validate_blocks && !table.Validate())
				throw std::runtime_error("Database Corruption");

			auto handle = table.FindPath(path);

			auto restore_file = [&](size_t size, std::string _dest, gsl::span<DefaultHash> keys)
			{
				try
				{
					if (!size)
					{
						d8u::util::empty_file(_dest);
						return;
					}

					if (size >= THRESHOLD)
					{
						if (keys.size() != 2)
							throw std::runtime_error("Threshold mismatch");

						dircopy::restore::file2(s, _dest, *keys.data(), store, domain, validate_blocks, hash_file, P);
					}
					else
						dircopy::restore::_file2(s, _dest, keys, store, domain, validate_blocks, hash_file, P);
				}
				catch (...)
				{
					std::cout << "Failed to restore " << _dest << std::endl;
				}
			};

			restore_file(handle.Filesize(), dest, handle.DescriptorT<DefaultHash>());

			return s.direct;
		}

		template <typename S, typename D> Direct folder(std::string_view dest, std::string_view path, const DefaultHash& volume_key, S& store, const D& domain, bool validate_blocks = false, bool hash_file = false, size_t BLOCK = 1024 * 1024, size_t THRESHOLD = 128 * 1024 * 1024, size_t P = 1, size_t F = 1)
		{
			Statistics s;

			auto folder_record = dircopy::restore::block(s, volume_key, store, domain, validate_blocks);

			auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_file);


			filetrace::Mount< tdb::filesystem::MinimalIndex32M > table(database);

			if (validate_blocks && !table.Validate())
				throw std::runtime_error("Database Corruption");

			auto parent = table.FindPathIndex(path);

			auto restore_file = [&](size_t size, std::string _dest, gsl::span<DefaultHash> keys)
			{
				try
				{
					if (!size)
					{
						d8u::util::empty_file(_dest);
						return;
					}

					if (size >= THRESHOLD)
					{
						if (keys.size() != 2)
							throw std::runtime_error("Threshold mismatch");

						dircopy::restore::file2(s, _dest, *keys.data(), store, domain, validate_blocks, hash_file, P);
					}
					else
						dircopy::restore::_file2(s, _dest, keys, store, domain, validate_blocks, hash_file, P);
				}
				catch (...)
				{
					std::cout << "Failed to restore " << _dest << std::endl;
				}
			};

			if (F == 1)
				table.EnumerateChildren(parent,[&](auto _path, auto _file, auto& handle)
				{
					restore_file(handle.Filesize(), std::string(dest) + std::string(_path) + std::string(_file), handle.DescriptorT<DefaultHash>());
				});
			else
			{
				table.EnumerateChildren(parent,[&](auto _path, auto _file, auto& handle)
				{
					fast_wait(s.atomic.files, F);

					s.atomic.files++;

					std::thread(restore_file, handle.Filesize(), std::string(dest) + std::string(_path) + std::string(_file), handle.DescriptorT<DefaultHash>()).detach();
				});

				fast_wait(s.atomic.files);
			}

			return s.direct;
		}

		template <typename S, typename D> Direct volume(std::string_view dest, const DefaultHash& volume_key, S& store, const D& domain, bool validate_blocks = false, bool hash_file = false, size_t BLOCK = 1024 * 1024, size_t THRESHOLD = 128 * 1024 * 1024, size_t P = 1, size_t F = 1)
		{
			Statistics s;

			auto folder_record = dircopy::restore::block(s, volume_key, store, domain, validate_blocks);

			auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_file);


			filetrace::Mount< tdb::filesystem::MinimalIndex32M > table(database);

			if(validate_blocks && !table.Validate())
				throw std::runtime_error("Database Corruption");

			auto restore_file = [&](size_t size, std::string _dest, gsl::span<DefaultHash> keys)
			{
				try
				{
					if (!size)
					{
						d8u::util::empty_file(_dest);
						return;
					}

					if (size >= THRESHOLD)
					{
						if (keys.size() != 2)
							throw std::runtime_error("Threshold mismatch");

						dircopy::restore::file2(s, _dest, *keys.data(), store, domain, validate_blocks, hash_file, P);
					}
					else
						dircopy::restore::_file2(s, _dest, keys, store, domain, validate_blocks, hash_file, P);
				}
				catch (...)
				{
					std::cout << "Failed to restore " << _dest << std::endl;
				}
			};

			if (F == 1)
				table.EnumerateFilesystem([&](auto _path, auto _file, auto& handle)
				{
					restore_file(handle.Filesize(), std::string(dest) + std::string(_path) + std::string(_file), handle.DescriptorT<DefaultHash>());
				});
			else
			{
				table.EnumerateFilesystem([&](auto _path, auto _file, auto& handle)
				{
					fast_wait(s.atomic.files, F);

					s.atomic.files++;

					std::thread(restore_file, handle.Filesize(), std::string(dest) + std::string(_path) + std::string(_file), handle.DescriptorT<DefaultHash>()).detach();
				});

				fast_wait(s.atomic.files);
			}

			return s.direct;
		}
	}
}

