/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "../catch.hpp"

#include "trace.hpp"
#include "mount.hpp"
#include "restore.hpp"

#include "volrng/volume.hpp"
#include "volrng/platform.hpp"

#include "d8u/util.hpp"

#include "volstore/simple.hpp"

using namespace d8u;
using namespace d8u::util;


TEST_CASE("Volume", "[volcopy::backup/restore]")
{
	constexpr auto itr_count = 3;
	constexpr auto folder_size = util::_mb(100);

	volrng::DISK::Dismount("tempdisk\\disk.img");
	volrng::DISK::Dismount("resdisk.img");

	std::filesystem::remove_all("tempdisk");
	std::filesystem::remove_all("testsnap");
	std::filesystem::remove_all("teststore");	
	std::filesystem::remove_all("resdisk.img");

	std::filesystem::create_directories("tempdisk");
	std::filesystem::create_directories("testsnap");

	volstore::Simple store("teststore");

	{
		volrng::volume::Test<volrng::DISK> handle("tempdisk");

		for (size_t i = 0; i < itr_count; i++)
		{
			handle.Run(folder_size, volrng::MOUNT);

			handle.Mount(volrng::MOUNT);

			Statistics stats;
			auto vkey = filetrace::trace::volume_files(i != 0, stats, string(volrng::MOUNT) + "\\", "testsnap", store, d8u::util::default_domain, nullptr, 32, 16, 16 * 1024 * 1024, 19);

			filetrace::Mount trace(std::string("testsnap") + "\\meta.db");

			handle.Enumerate([&](auto path, auto& hash)
			{
				bool found_file = false;
				bool found_hash = false;
				std::string computed_path;

				trace.SearchNames(std::filesystem::path(path).filename().string(), [&](auto& row)
				{
					found_file = true;

					computed_path = trace.Path(row);

					return false;
				});

				CHECK(computed_path == path);

				trace.SearchHash(*(tdb::Key32*) & hash, [&](auto& row)
				{
					found_hash = true;

					return false;
				});

				if (!(found_file && found_hash))
					std::cout << "Problem With: " << path << std::endl;

				CHECK((found_file && found_hash));
			});


			{
				volrng::DISK res_disk("resdisk.img", util::_gb(100), volrng::MOUNT2);

				filetrace::restore::volume(volrng::MOUNT2, vkey, store, d8u::util::default_domain, true, true, 1024 * 1024, 128 * 1024 * 1024, 8, 8);

				CHECK(handle.Validate(volrng::MOUNT2));
			}

			std::filesystem::remove_all("resdisk.img");
			handle.Dismount();
		}
	}

	std::filesystem::remove_all("tempdisk");
	std::filesystem::remove_all("testsnap");
	std::filesystem::remove_all("teststore");
	std::filesystem::remove_all("resdisk.img");
}


TEST_CASE("mount", "[filetrace::]")
{
	constexpr auto itr_count = 5;
	constexpr auto folder_size = util::_mb(1000);

	volrng::DISK::Dismount("tempdisk\\disk.img");

	std::filesystem::remove_all("tempdisk");
	std::filesystem::remove_all("testsnap");
	std::filesystem::remove_all("teststore");

	std::filesystem::create_directories("tempdisk");
	std::filesystem::create_directories("testsnap");

	{
		volrng::volume::Test<volrng::DISK> handle("tempdisk");

		for (size_t i = 0; i < itr_count; i++)
		{
			handle.Run(folder_size, volrng::MOUNT);

			handle.Mount(volrng::MOUNT);

			filetrace::trace::volume_sha256(i!=0, string(volrng::MOUNT) + "\\", "testsnap", 32);

			filetrace::Mount trace(std::string("testsnap") + "\\meta.db");

			handle.Enumerate([&](auto path, auto& hash)
			{
				bool found_file = false;
				bool found_hash = false;
				std::string computed_path;

				trace.SearchNames(std::filesystem::path(path).filename().string(),[&](auto& row)
				{
					found_file = true;

					computed_path = trace.Path(row);

					return false;
				});

				CHECK(computed_path == path);

				trace.SearchHash(*(tdb::Key32*)&hash, [&](auto& row)
				{
					found_hash = true;

					return false;
				});

				if (!(found_file && found_hash))
					std::cout << "Problem With: " << path << std::endl;

				CHECK((found_file && found_hash));
			});

			handle.Dismount();
		}
	}

	std::filesystem::remove_all("tempdisk");
	std::filesystem::remove_all("testsnap");
}


TEST_CASE("Trace Multithreaded", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::trace::volume(false, "C:\\", "test", 8);

	std::filesystem::remove_all("test");
}

TEST_CASE("Trace File Hashing", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::trace::volume_sha256(false, "C:\\", "test", 32);

	std::filesystem::remove_all("test");
}

TEST_CASE("Trace simple", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::trace::volume(false, "C:\\", "test", 1);

	std::filesystem::remove_all("test");
}