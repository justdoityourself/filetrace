/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "tdb/fs.hpp"

#include "d8u/util.hpp"
#include "d8u/transform.hpp"

#include "dircopy/validate.hpp"

#include "mount.hpp"

namespace filetrace
{
	namespace validate
	{
		using namespace d8u::util;
		using namespace d8u::transform;

		template <typename S, typename D, typename V=decltype(dircopy::validate::deep_block<S,D>)> bool volume(const DefaultHash& volume_key, S& store, const D& domain, bool validate_blocks = true, bool hash_file = true, size_t BLOCK = 1024 * 1024, size_t THRESHOLD = 128 * 1024 * 1024, size_t P = 1, size_t F = 1, V v = dircopy::validate::deep_block<S, D>)
		{
			Statistics s;
			bool res = true;

			auto folder_record = dircopy::restore::block(s, volume_key, store, domain, validate_blocks);

			auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_file);


			filetrace::Mount< tdb::filesystem::MinimalIndex32M > table(database);

			if (validate_blocks && !table.Validate())
				throw std::runtime_error("Database Corruption");

			auto validate_file = [&](size_t size,auto keys)
			{
				dec_scope lock(s.atomic.files);

				try
				{
					if (!size)
						return res;

					if (size >= THRESHOLD)
					{
						if (keys.size() != 2)
							throw std::runtime_error("Threshold mismatch");

						if (!dircopy::validate::core_file(s, *keys.data(), store, domain, v, P))
							return res = false;
					}
					else
					{
						for (auto& k : keys)
						{
							if (&k == keys.end() - 1)
								break; //Last hash is the file hash

							if (!v(s, k, store, domain))
								return res = false;
						}
					}
				}
				catch (...)
				{
					res = false;
				}

				s.atomic.read += size;

				return res;
			};

			if (F == 1)
				table.EnumerateFilesystem([&](auto _path, auto _file, auto& handle)
				{
					validate_file(handle.Filesize(), handle.DescriptorT<DefaultHash>());
				});
			else
			{
				table.EnumerateFilesystem([&](auto _path, auto _file, auto& handle)
				{
					fast_wait(s.atomic.files, F);

					s.atomic.files++;

					std::thread(validate_file, handle.Filesize(), handle.DescriptorT<DefaultHash>()).detach();
				});

				fast_wait(s.atomic.files);
			}

			return res;
		}
	}
}

