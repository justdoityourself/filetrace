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
			
			for (size_t i = 0; i < tbl.size(); i++)
				f(tbl[i]);
		}

		template < typename R, typename F > void EnumerateChildren(R& row, F&& f)
		{
			//todo
		}

		template < typename F > void EnumerateFiles(F&& f)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
			{
				auto& row = tbl[i];

				if (row.Time() && row.Type() == tdb::filesystem::File)
					f(row);
			}
		}

		template < typename F > void EnumerateFilesystem(F&& f)
		{
			//TODO Hard Links

			auto tbl = db.Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
			{
				auto& row = tbl[i];

				if (row.Time() && row.Type() == tdb::filesystem::File)
				{
					bool root = false;
					auto path = ParentPath(row, &root);

					if (root)
						f(std::string_view(path), row.FirstName(), row);
				}
			}
		}

		template < typename F > void EnumerateFolders(F&& f)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
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

		auto FindPath(std::string_view path)
		{
			tdb::filesystem::Row * result;
			std::string_view file;

			auto l1 = path.find_last_of('/');
			auto l2 = path.find_last_of('\\');

			if (l1 == -1 && l2 == -1)
				file = path;
			else if(l1 != -1 && l2 != -1)
				file=path.substr((l1>l2)?l1:l2);
			else if (l1 != -1)
				file = path.substr(l1);
			else if (l2 != -1)
				file = path.substr(l2);

			SearchNames(file,[&](auto& row)
			{
				if(row.Path() == path)
				{
					result = &row;
					return false;
				}

				return true;
			});

			return result;
		}

		template < typename R > std::string Path(const R& row,bool * root = nullptr,bool parent=false)
		{
			std::string result;

			auto _p = row.Parents();
			auto _n = row.Names();

			if (!_n.size())
				return "";

			if(!parent)
				result = _n[0];

			while (_p.size() && _p[0] != 5) // 5 being volume . in NTFS systems
			{
				auto & r = db.Table< tdb::filesystem::Tables::Files >()[_p[0]];
				_p = r.Parents();
				_n = r.Names();

				if (!_n.size())
					return "";

				result = std::string(_n[0]) + "\\" + result;
			}

			if (root && _p.size() && _p[0] == 5)
				*root = true;

			return "\\" + result;
		}

		template < typename R > std::string ParentPath(const R& row, bool* root = nullptr)
		{
			return Path(row, root, true);
		}
	};
}