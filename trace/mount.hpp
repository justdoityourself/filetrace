/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string_view>

#include "tdb/fs.hpp"
#include "tdb/interface.hpp"

namespace filetrace
{
	class Mount
	{
		tdb::filesystem::HalfIndex32 db;
	public:
		Mount(std::string_view root)
			: db(string(root) + "\\meta.db")
		{ }

		template < typename F > void SearchNames(std::string_view text, F&& f)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();
			tdb::TableHelper<decltype(db),decltype(tbl)> h(db, tbl);

			h.StringSearch< tdb::filesystem::Values::Name>(text, f);
		}

		template < typename F > void SearchHash(const tdb::Key32 & k, F&& f)
		{
			auto hashes = db.Table<tdb::filesystem::Tables::Files>();

			hashes.MultiFindSurrogate<tdb::filesystem::Indexes::Hash>(f,&k);
		}
	};
}