/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "../catch.hpp"
#include "trace.hpp"

#include "volsnap/compat.hpp"

TEST_CASE("Trace Multithreaded", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::volume<volsnap::win32::Compatability>(false, "C:\\", "test", 8);

	std::filesystem::remove_all("test");
}

TEST_CASE("Trace simple", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::volume<volsnap::win32::Compatability>(false, "C:\\", "test", 1);

	std::filesystem::remove_all("test");
}