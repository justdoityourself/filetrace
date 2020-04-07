/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "../catch.hpp"
#include "trace.hpp"

#include "volsnap/compat.hpp"

TEST_CASE("Trace", "[filetrace::]")
{
	std::filesystem::remove_all("test");

	filetrace::volume<volsnap::win32::Compatability>(false, "C:\\", "test", 16);

	std::filesystem::remove_all("test");
}