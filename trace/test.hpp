/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include "../catch.hpp"
#include "trace.hpp"

#include "volsnap/compat.hpp"

TEST_CASE("Trace", "[filetrace::]")
{
	filetrace::volume<volsnap::win32::Compatability>(false, "C:\\", "test", 1);
}