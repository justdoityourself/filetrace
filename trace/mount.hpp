/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string_view>
#include <string>

#include "tdb/fs.hpp"
#include "tdb/interface.hpp"

namespace filetrace
{
	template < typename DB = tdb::filesystem::MinimalIndex32 > class Mount
	{
		DB db;
	public:

		Mount() {}

		template < typename T > Mount(T& stream)
		{
			Open(stream);
		}

		template < typename T > Mount(T&& stream)
		{
			Open(stream);
		}

		template < typename T >  void Open(T& stream)
		{
			db.Open(stream);
		}

		template < typename T >  void Open(T&& stream)
		{
			db.Open(stream);
		}

		template < typename F > void Enumerate( F&& f )
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();
			
			for (size_t i = 0; tbl.size(); i++)
				f(tbl[i]);
		}

		template < typename F > void EnumerateFiles(F&& f)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; tbl.size(); i++)
			{
				auto& row = tbl[i];

				if (row.Type() == tdb::filesystem::File)
					f(row);
			}
		}

		template < typename F > void EnumerateFolders(F&& f)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; tbl.size(); i++)
			{
				auto& row = tbl[i];
				
				if (row.Type() == tdb::filesystem::Folder)
					f(row);
			}
		}

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

		template < typename R > std::string Path(const R& row)
		{
			std::string result;

			auto _p = row.Parents();
			auto _n = row.Names();

			result = _n[0];

			while (_p.size() && _p[0] != 5)
			{
				auto & r = db.Table< tdb::filesystem::Tables::Files >()[_p[0]];
				_p = r.Parents();
				_n = r.Names();
				result = std::string(_n[0]) + "\\" + result;
			}

			return "\\" + result;
		}
	};
}