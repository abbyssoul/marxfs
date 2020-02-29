/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#pragma once
#ifndef MARXXFS_MODEL_HPP
#define MARXXFS_MODEL_HPP

#include "errorDomain.hpp"

#include <kasofs/kasofs.hpp>

#include <archive.h>
#include <archive_entry.h>

#include <unordered_map>
#include <filesystem>  // C++17 filesystem path

namespace marxfs {

struct ArchiveFS final : public kasofs::Filesystem {
	using ArchiveHandle = std::unique_ptr<archive, int(*)(archive*)>;
	using FileHandle = std::unique_ptr<FILE, int(*)(FILE*)>;

	kasofs::FilePermissions defaultFilePermissions(NodeType type) const noexcept override;

	kasofs::Result<kasofs::INode> createNode(NodeType type, kasofs::User owner, kasofs::FilePermissions perms) override;
	kasofs::Result<void> destroyNode(kasofs::INode& node) override;

	kasofs::Result<OpenFID> open(kasofs::INode& node, kasofs::Permissions op) override;
	kasofs::Result<size_type>
	read(OpenFID fid, kasofs::INode& node, size_type offset, Solace::MutableMemoryView dest) override;

	kasofs::Result<size_type>
	write(OpenFID fid, kasofs::INode& node, size_type offset, Solace::MemoryView src) override;

	kasofs::Result<size_type>
	seek(OpenFID fid, kasofs::INode& node, size_type offset, SeekDirection direction) override;

	kasofs::Result<void> close(OpenFID fid, kasofs::INode& node) override;

	static kasofs::VfsNodeType const kNodeType;

	static bool isArchiveNode(kasofs::INode const& node) noexcept {
		return (kNodeType == node.nodeTypeId);
	}

	kasofs::Result<void> bind(kasofs::INode& node,
							  std::filesystem::path path,
							  std::filesystem::path entryName,
							  archive_entry* entry);

protected:
	using DataId = kasofs::INode::VfsData;

	DataId	nextId() noexcept;
	OpenFID nextOpenId() noexcept { return _openIdBase++; }

private:
	DataId		_idBase{0};
	OpenFID		_openIdBase{0};

	struct ArchiveEntry {
		FileHandle		file;
		ArchiveHandle	archive;
		la_int64_t		readOffset{0};

		ArchiveEntry(FileHandle fileHandle, ArchiveHandle archiveHandle) noexcept
			: file{std::move(fileHandle)}
			, archive{std::move(archiveHandle)}
		{}
	};

	struct BoundEntry {
		std::filesystem::path	filename;
		std::filesystem::path	entryName;

		BoundEntry(std::filesystem::path name, std::filesystem::path entry) noexcept
			: filename{std::move(name)}
			, entryName{std::move(entry)}
		{}
	};

	std::unordered_map<DataId, BoundEntry>			_nameBind;
	std::unordered_map<OpenFID, ArchiveEntry>		_openArchives;
};


kasofs::Result<void>
mapToFs(kasofs::Vfs& vfs,
		kasofs::User user,
		kasofs::INode::Id dirId,
		kasofs::VfsId jsonFsId,
		std::filesystem::path const& file);

}  // namespace marxfs
#endif  // MARXFS_MODEL_HPP
