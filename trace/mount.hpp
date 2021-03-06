/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string_view>
#include <string>

#include "tdb/fs.hpp"
#include "tdb/interface.hpp"

#include "mhttp/ftp.hpp"

#include "d8u/transform.hpp"
#include "d8u/util.hpp"
#include "d8u/string.hpp"

#include "dircopy/restore.hpp"

#include <map>

namespace filetrace
{
	using namespace d8u::util;
	using namespace d8u::transform;

	template < typename DB = tdb::filesystem::MinimalIndex32 > class Mount : public DB
	{
	public:

		bool Validate() { return DB::Validate(); }

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
			DB::Open(stream);
		}

		template < typename T >  void Open(T&& stream)
		{
			DB::Open(stream);
		}

		template < typename F > void Enumerate( F&& f )
		{
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files> ();
			
			for (size_t i = 0; i < tbl.size(); i++)
				f(tbl[i]);
		}

		template < typename F > void EnumerateChildrenRecursive(size_t parent, F&& f, bool dir = false)
		{
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
			{
				auto& row = tbl[i];

				if (row.Time() && ((dir) ? true : row.Type() == tdb::filesystem::File))
				{
					auto path = ChildPath(parent,row);

					if (path.size())
						f(std::string_view(path), row.FirstName(), row);
				}
			}
		}

		template < typename F > void EnumerateChildrenImmediate(size_t parent, F&& f, bool dir = false)
		{
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
			{
				auto& row = tbl[i];

				if (row.Time() && ((dir) ? true : row.Type() == tdb::filesystem::File))
				{
					if (IsChild(parent,row))
						f(row.FirstName(), row);
				}
			}
		}

		template < typename F > void EnumerateFiles(F&& f)
		{
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();

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

			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();

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
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();

			for (size_t i = 0; i < tbl.size(); i++)
			{
				auto& row = tbl[i];
				
				if (row.Type() == tdb::filesystem::Folder)
					f(row);
			}
		}

		template < typename F > void SearchNames(std::string_view text, F&& f)
		{
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();
			tdb::TableHelper<DB,decltype(tbl)> h(*this, tbl);

			h.StringSearch< tdb::filesystem::Values::Name>(text, f);
		}

		template < typename F > void SearchNamesIndex(std::string_view text, F&& f)
		{
			auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();
			tdb::TableHelper<DB, decltype(tbl)> h(*this, tbl);

			h.StringSearchIndex< tdb::filesystem::Values::Name>(text, f);
		}

		template < typename F > void SearchHash(const tdb::Key32 & k, F&& f)
		{
			auto hashes = DB::template Table<tdb::filesystem::Tables::Files>();

			hashes.MultiFindSurrogate<tdb::filesystem::Indexes::Hash>(f,&k);
		}

		auto FindPathIndex(std::string_view __path)
		{
			std::string _path(__path);

			std::replace(_path.begin(), _path.end(), '/', '\\');
			size_t result = -1;

			std::string_view file,path;

			auto l2 = _path.find_last_of('\\');

			if (l2 == -1)
			{
				file = std::string_view(_path);
				path = "\\";
			}
			else
			{
				file = std::string_view(_path).substr(l2 + 1);
				path = std::string_view(_path).substr(0,l2+1);
			}	

			SearchNamesIndex(file, [&](auto dx, auto& row)
			{
				auto parent_path = Path(row, nullptr, true);
				if (parent_path == path)
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
				auto tbl = DB::template Table<tdb::filesystem::Tables::Files>();
				result = &tbl[dx];
			}

			return result;
		}

		template < typename R > bool IsChild(size_t parent, const R& row)
		{
			for (auto pv : row.Parents())
			{
				if (pv == (uint32_t)parent)
					return true;
			}

			return false;
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
					if (pv = (uint32_t)parent)
					{
						is_child = true;
						break;
					}
				}

				auto& r = DB::template Table< tdb::filesystem::Tables::Files >()[_p[0]];
				_p = r.Parents();
				_n = r.Names();

				if (!_n.size())
					return "";

				result = std::string(_n[0]) + "\\" + result;
			}

			if (root && _p.size() && _p[0] == uint32_t(-1))
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
				auto & r = DB::template Table< tdb::filesystem::Tables::Files >()[_p[0]];
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

	template < typename STORE, typename DATA_DOMAIN = decltype(default_domain), typename DB = tdb::filesystem::MinimalIndex32M > class FtpServer
	{
		mhttp::ftp::FtpServer server;

		STORE& store;
		const DATA_DOMAIN& domain;

		struct Context
		{
			Context(std::vector<uint8_t> && db_buffer)
				: db(db_buffer)
			{ }

			DB db;
			std::atomic<size_t> ref = 1;
		};

		std::mutex volume_lock;
		std::map < DefaultHash, Context> volumes;
		bool validate_blocks = false;
		bool hash_files = false;

		Statistics s;

	public:
		FtpServer(STORE & _store, const DATA_DOMAIN& _domain = default_domain, bool _validate_blocks = false, bool _hash_files = false)
			: server (	std::bind(&FtpServer::Enumerate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
						std::bind(&FtpServer::Send, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
						std::bind(&FtpServer::Login, this, std::placeholders::_1),
						std::bind(&FtpServer::Logout, this, std::placeholders::_1))
			, store(_store)
			, domain(_domain)
			, validate_blocks(_validate_blocks)
			, hash_files(_hash_files) { }

		FtpServer(STORE& _store, const std::string_view _ip, const std::string_view _port1, std::string_view _port2, const DATA_DOMAIN& _domain = default_domain, bool _validate_blocks = false, bool _hash_files = false)
			: FtpServer(_store,_domain,_validate_blocks,_hash_files)
		{
			Open(_ip,_port1,_port2);
		}

		void Open(const std::string_view _ip, const std::string_view _port1, std::string_view _port2)
		{
			server.Open(_ip, _port1, _port2);
		}

		auto Stats()
		{
			return s.direct;
		}

	private:

		void Enumerate(mhttp::ftp::FtpConnection& c,std::string_view resource, mhttp::ftp::on_ftp_enum_result cb)
		{
			Mount<DB>* mount = nullptr;

			{
				std::lock_guard<std::mutex> lck(volume_lock);
				auto key = d8u::util::to_bin_t<DefaultHash>(c.user);

				auto v = volumes.find(key);
				if (v != volumes.end())
					mount = (Mount<DB> *)&v->second.db;
			}

			if (!mount)
				return;

			if (resource == std::string_view("/"))
				mount->EnumerateChildrenImmediate(-1, [&](auto _file, auto& handle)
				{
					cb(handle.Type() == tdb::filesystem::Folder,handle.Filesize(),handle.Time(),_file);
				},true);
			else
			{
				//Todo fix second level enumeration.
				auto parent = mount->FindPathIndex(resource);

				if (-1 == parent)
					return;

				mount->EnumerateChildrenImmediate(parent, [&]( auto _file, auto& handle)
				{
					cb(handle.Type() == tdb::filesystem::Folder, handle.Filesize(), handle.Time(), _file);
				}, true);
			}
		}

		bool Login(mhttp::ftp::FtpConnection & c)
		{
			try
			{
				std::lock_guard<std::mutex> lck(volume_lock);
				auto key = d8u::util::to_bin_t<DefaultHash>(c.user);

				auto v = volumes.find(key);
				if (v != volumes.end())
					v->second.ref++;
				else
				{
					auto folder_record = dircopy::restore::block(s, key, store, domain, true);

					auto database = dircopy::restore::file_memory(s, gsl::span<DefaultHash>((DefaultHash*)folder_record.data(), folder_record.size() / sizeof(DefaultHash)), store, domain, validate_blocks, hash_files);

					auto i = volumes.try_emplace(key, std::move(database)).first;

					if (validate_blocks && !i->second.db.Validate())
					{
						volumes.erase(i);
						throw std::runtime_error("Database Corruption");
					}
				}
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

		void Logout(mhttp::ftp::FtpConnection& c)
		{
			if (c.user.size() == 0)
				return;

			std::lock_guard<std::mutex> lck(volume_lock);
			auto key = d8u::util::to_bin_t<DefaultHash>(c.user);

			auto v = volumes.find(key);
			if (v != volumes.end())
			{
				v->second.ref--;
				if(v->second.ref == 0)
					volumes.erase(v);
			}
		}

		void Send(mhttp::ftp::FtpConnection& c,std::string_view resource, mhttp::ftp::on_ftp_io_result cb)
		{
			Mount<DB>* mount = nullptr;

			{
				std::lock_guard<std::mutex> lck(volume_lock);
				auto key = d8u::util::to_bin_t<DefaultHash>(c.user);

				auto v = volumes.find(key);
				if (v != volumes.end())
					mount = (Mount<DB>*) & v->second.db;
			}

			if (!mount)
				return;

			auto file = mount->FindPath(resource);

			if (!file)
				return;

			auto keys = file->DescriptorT<DefaultHash>();
			std::vector<uint8_t> _keys;

			if (!file->Filesize())
				return;

			if (file->Filesize() >= 128*1024*1024/*THRESHOLD*/)
			{
				if (keys.size() != 2)
					throw std::runtime_error("Threshold mismatch");

				_keys = dircopy::restore::file_memory(s,keys, store, domain, validate_blocks, hash_files);
				keys = gsl::span<DefaultHash>((DefaultHash*)_keys.data(), _keys.size() / sizeof(DefaultHash));
			}
			
			for (auto& key : keys)
			{
				if (&key == keys.end() - 1)
					break; //Last hash is the file hash

				auto buffer = dircopy::restore::block(s, key, store, domain, validate_blocks);
				cb(buffer);
			}
		}
	};

}