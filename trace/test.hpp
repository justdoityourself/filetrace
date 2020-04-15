/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "../catch.hpp"
#include "trace.hpp"
#include "mount.hpp"

#include "volrng/volume.hpp"
#include "volrng/platform.hpp"

#include "d8u/util.hpp"

using namespace d8u;


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

			filetrace::volume_sha256(i!=0, string(volrng::MOUNT) + "\\", "testsnap", 32);

			filetrace::Mount trace("testsnap");

			handle.Enumerate([&](auto path, auto& hash)
			{
				bool found_file = false;
				bool found_hash = false;

				trace.SearchNames(std::filesystem::path(path).filename().string(),[&](auto& row)
				{
					found_file = true;

					return false;
				});
				trace.SearchHash(*(tdb::Key32*)&hash, [&](auto& row)
				{
					found_hash = true;

					return false;
				});

				if (!(found_file && found_hash))
					std::cout << path << std::endl;

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

	filetrace::volume(false, "C:\\", "test", 8);

	std::filesystem::remove_all("test");
}

TEST_CASE("Trace File Hashing", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::volume_sha256(false, "C:\\", "test", 32);

	std::filesystem::remove_all("test");
}

TEST_CASE("Trace simple", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::volume(false, "C:\\", "test", 1);

	std::filesystem::remove_all("test");
}