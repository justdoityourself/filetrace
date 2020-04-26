/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string_view>
#include <string>

#include "tdb/fs.hpp"
#include "tdb/interface.hpp"

#include "mhttp/ftp.hpp"

#include "d8u/transform.hpp"
#include "d8u/util.hpp"

namespace filetrace
{
	using namespace d8u::util;

	template < typename DB = tdb::filesystem::MinimalIndex32 > class Mount
	{
		DB db;
	public:

		bool Validate() { return db.Validate(); }

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

		template < typename R, typename F > void EnumerateChildren(size_t parent, F&& f, bool dir = false)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
			{
				auto& row = tbl[i];

				if (row.Time() && ((dir) ? true : row.Type() == tdb::filesystem::File))
				{
					bool root = false;
					auto path = ChildPath(parent,row, &root);

					if (root)
						f(std::string_view(path), row.FirstName(), row);
				}
			}
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

		template < typename F > void SearchNamesIndex(std::string_view text, F&& f)
		{
			auto tbl = db.Table<tdb::filesystem::Tables::Files>();
			tdb::TableHelper<decltype(db), decltype(tbl)> h(db, tbl);

			h.StringSearchIndex< tdb::filesystem::Values::Name>(text, f);
		}

		template < typename F > void SearchHash(const tdb::Key32 & k, F&& f)
		{
			auto hashes = db.Table<tdb::filesystem::Tables::Files>();

			hashes.MultiFindSurrogate<tdb::filesystem::Indexes::Hash>(f,&k);
		}

		auto FindPathIndex(std::string_view path)
		{
			size_t result = -1;
			std::string_view file;

			auto l1 = path.find_last_of('/');
			auto l2 = path.find_last_of('\\');

			if (l1 == -1 && l2 == -1)
				file = path;
			else if (l1 != -1 && l2 != -1)
				file = path.substr((l1 > l2) ? l1 : l2);
			else if (l1 != -1)
				file = path.substr(l1);
			else if (l2 != -1)
				file = path.substr(l2);

			SearchNamesIndex(file, [&](auto dx, auto& row)
			{
				if (row.Path() == path)
				{
					result = dx;
					return false;
				}

				return true;
			});

			return result;
		}

		auto FindPath(std::string_view path)
		{
			tdb::filesystem::Row* result = nullptr;
			
			auto dx = FindPathIndex(path);

			if (dx != -1)
			{
				auto tbl = db.Table<tdb::filesystem::Tables::Files>();
				result = &tbl[dx];
			}

			return result;
		}

		template < typename R > std::string ChildPath(size_t parent, const R& row, bool* root = nullptr, bool parent_path = false)
		{
			bool is_child = false;

			std::string result;

			auto _p = row.Parents();
			auto _n = row.Names();

			if (!_n.size())
				return "";

			if (!parent_path)
				result = _n[0];

			while (!is_child && _p.size() && _p[0] != uint32_t(-1)) // -1 being volume root
			{
				for (auto pv : _p)
				{
					if (parent == (size_t)pv)
					{
						is_child = true;
						break;
					}
				}

				auto& r = db.Table< tdb::filesystem::Tables::Files >()[_p[0]];
				_p = r.Parents();
				_n = r.Names();

				if (!_n.size())
					return "";

				result = std::string(_n[0]) + "\\" + result;
			}

			if (root && _p.size() && _p[0] == 5)
				*root = true;

			return is_child ? "\\" + result : "";
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

			while (_p.size() && _p[0] != uint32_t(-1)) // -1 being volume root
			{
				auto & r = db.Table< tdb::filesystem::Tables::Files >()[_p[0]];
				_p = r.Parents();
				_n = r.Names();

				if (!_n.size())
					return "";

				result = std::string(_n[0]) + "\\" + result;
			}

			if (root && _p.size() && _p[0] == uint32_t(-1))
				*root = true;

			return "\\" + result;
		}

		template < typename R > std::string ParentPath(const R& row, bool* root = nullptr)
		{
			return Path(row, root, true);
		}
	};

	template < typename STORE, typename DATA_DOMAIN = decltype(default_domain) > class FtpServer
	{
		mhttp::FtpServer server;

		STORE& store;
		const DATA_DOMAIN& domain;
	public:
		FtpServer(STORE & _store, const DATA_DOMAIN& _domain = default_domain)
			: server (	std::bind(&FtpServer::Enumerate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
						std::bind(&FtpServer::Send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
						std::bind(&FtpServer::Login, this, std::placeholders::_1),
						std::bind(&FtpServer::Logout, this, std::placeholders::_1))
			, store(_store)
			, domain(_domain) { }

		template < typename T > FtpServer(STORE& _store, const std::string_view _ip, const std::string_view _port1, std::string_view _port2, const DATA_DOMAIN& _domain = default_domain)
			: FtpServer(_store,_domain)
		{
			Open(_ip,_port1,_port2);
		}

		template < typename T >  void Open(const std::string_view _ip, const std::string_view _port1, std::string_view _port2)
		{
			server.Open(_ip, _port1, _port2);
		}

	private:

		void Enumerate(mhttp::FtpConnection& c,std::string_view resource, mhttp::on_ftp_enum_result cb)
		{
			/*if (resource == std::string_view("/"))
				mount.EnumerateChildren(-1, [&](auto _path, auto _file, auto& handle)
				{
					cb(handle.Type() == tdb::filesystem::Folder,handle.Filesize(),handle.Time(),handle.FirstName());
				},true);
			else
			{
				auto parent = mount.FindPathIndex(resource);

				mount.EnumerateChildren(parent, [&](auto _path, auto _file, auto& handle)
				{
					cb(handle.Type() == tdb::filesystem::Folder, handle.Filesize(), handle.Time(), handle.FirstName());
				}, true);
			}*/
		}

		bool Login(mhttp::FtpConnection & c)
		{
			return true;
		}

		void Logout(mhttp::FtpConnection& c)
		{

		}

		void Send(mhttp::FtpConnection& c,std::string_view resource, mhttp::on_ftp_io_result cb)
		{

		}
	};

}