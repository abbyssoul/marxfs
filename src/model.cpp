/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/


#include "model.hpp"


#include <filesystem>  // std::path
#include <iostream>

using namespace Solace;
using namespace kasofs;
using namespace marxfs;


kasofs::VfsNodeType const ArchiveFS::kNodeType{983614};

using EntryHandle = std::unique_ptr<archive_entry, void(*)(archive_entry*)>;


auto make_reader() {
	return ArchiveFS::ArchiveHandle{archive_read_new(), &archive_read_free};
}

auto make_archiveEntry() {
	return EntryHandle{archive_entry_new(), &archive_entry_free};
}


uint32 nodeEpochTime() noexcept {
	return time(nullptr);
}


kasofs::Result<kasofs::INode::Id>
createPath(kasofs::User user, kasofs::Vfs& vfs, kasofs::INode::Id rootId, std::filesystem::path const& walkPath) {
	bool isKnowPath = true;
	for (auto const& pathSegment : walkPath) {
		if (pathSegment.empty())
			break;

		// Walk a single segment from the last node
		if (isKnowPath) {
			auto pathView = StringView{pathSegment.c_str()};
			StringView pathWalk[] = {pathView};

			auto validWalk = vfs.walk(user, rootId, pathWalk, [&rootId](kasofs::Entry entry, kasofs::INode& /*node*/) {
				rootId = entry.nodeId;
			});

			isKnowPath = validWalk.isOk();
			if (isKnowPath) {
				continue;
			}
		}

		// In case segment does not yet exist:
		auto maybeNewDirId = vfs.createDirectory(rootId, pathSegment.c_str(), user, 0777);
		if (!maybeNewDirId) {
			return maybeNewDirId.moveError();
		}

		rootId = *maybeNewDirId;
	}

	return Ok(rootId);
}


kasofs::Result<void>
marxfs::mapToFs(kasofs::Vfs& vfs, kasofs::User user, kasofs::INode::Id dirId, kasofs::VfsId fsId,
				std::filesystem::path const& filePath) {
	auto maybeArchive = make_reader();
	archive *const inArchive = maybeArchive.get();
	if (inArchive == nullptr) {
		return makeError(GenericError::IO, "archive_read_new");
	}

	archive_read_support_filter_all(inArchive);
	archive_read_support_format_all(inArchive);
	int const r = archive_read_open_filename(inArchive, filePath.c_str(), 10240);
	if (r != ARCHIVE_OK) {
		return makeErrno(inArchive);
	}

	// Get archiveFs driver:
	auto maybeDriver = vfs.findFs(fsId);
	if (!maybeDriver) {
		return makeError(GenericError::IO, "ArchiveFS driver not found");
	}

	auto* const archiveFS = static_cast<ArchiveFS*>(*maybeDriver);

	auto entry = make_archiveEntry();
	while (archive_read_next_header2(inArchive, entry.get()) == ARCHIVE_OK) {
		const char* pathname = archive_entry_pathname(entry.get());
		printf("%s\n", pathname);

		auto entryPath = std::filesystem::path{pathname};
		auto const namesFile = entryPath.has_filename();
		auto walkPath = namesFile
				? entryPath.parent_path()
				: entryPath;

		auto pathCreated = createPath(user, vfs, dirId, walkPath);
		if (!pathCreated) {
			return pathCreated.moveError();
		}

		if (namesFile) {
			auto filename = entryPath.filename();
			auto name = StringView{filename.c_str()};
			auto fileCreated = 	vfs.mknode(*pathCreated, name, fsId, ArchiveFS::kNodeType, user, 0777);
			if (!fileCreated) {
				return fileCreated.moveError();
			}

			auto boundResult = vfs.nodeById(*fileCreated)
					.map([&](INode& node) {
						archiveFS->bind(node, filePath, entryPath, entry.get());

						vfs.updateNode(*fileCreated, node);
						return 0;
					});

			if (!boundResult) {
				return makeError(GenericError::IO, "Failed to bind a node to an archive entry");
			}
		}

		archive_read_data_skip(inArchive);
	}

	return none;
}


FilePermissions
ArchiveFS::defaultFilePermissions(NodeType) const noexcept {
	return FilePermissions(0644);
}


ArchiveFS::size_type
ArchiveFS::nextId() noexcept {
	return _idBase++;
}


kasofs::Result<INode>
ArchiveFS::createNode(NodeType type, kasofs::User owner, kasofs::FilePermissions perms) {
	if (kNodeType != type) {
		return makeError(GenericError::NXIO, "ArchiveFS::createNode");
	}

	INode node{type, owner, perms};
	node.dataSize = 0;
	node.vfsData = nextId();
	node.atime = nodeEpochTime();
	node.mtime = nodeEpochTime();

	return mv(node);
}

kasofs::Result<void>
ArchiveFS::bind(INode& node, std::filesystem::path path, std::filesystem::path entryName, archive_entry* entry) {
	auto maybeBoundEntry = _nameBind.try_emplace(node.vfsData, mv(path), mv(entryName));
	if (!maybeBoundEntry.second)
		return makeError(GenericError::EXIST, "ArchiveFS.bind-key");

	auto fileStats = archive_entry_stat(entry);
	node.atime = fileStats->st_atim.tv_sec;
	node.mtime = fileStats->st_mtim.tv_sec;
	node.dataSize = fileStats->st_size;
	node.owner = kasofs::User{fileStats->st_uid, fileStats->st_gid};

	return Ok();
}


kasofs::Result<void>
ArchiveFS::destroyNode(INode& node) {
	if (!isArchiveNode(node)) {
		return makeError(GenericError::NXIO, "ArchiveFS::destroyNode");
	}

	_nameBind.erase(node.vfsData);
	return Ok();
}


kasofs::Result<Filesystem::OpenFID>
ArchiveFS::open(kasofs::INode& node, kasofs::Permissions) {
	if (!isArchiveNode(node)) {
		return makeError(GenericError::NXIO, "ArchiveFS::open");
	}
	node.atime = nodeEpochTime();

	auto const id = node.vfsData;
	auto boundNameIt = _nameBind.find(id);
	if (boundNameIt == _nameBind.end()) {
		return makeError(GenericError::NOENT, "open: node name not bound");
	}

	// Open file:
	auto file = FileHandle{fopen(boundNameIt->second.filename.c_str(), "r"), &fclose};
	if (!file)
		return Solace::makeErrno();

	auto maybeArchive = ArchiveFS::ArchiveHandle{archive_read_new(), &archive_read_free};
	archive *const inArchive = maybeArchive.get();
	if (inArchive == nullptr) {
		return makeError(GenericError::IO, "archive_read_new");
	}

	archive_read_support_filter_all(inArchive);
	archive_read_support_format_all(inArchive);

	int const r = archive_read_open_FILE(inArchive, file.get());
	if (r != ARCHIVE_OK) {
		return makeErrno(inArchive);
	}

	bool found = false;
	auto entry = make_archiveEntry();
	while (!found && archive_read_next_header2(inArchive, entry.get()) == ARCHIVE_OK) {
		found = (boundNameIt->second.entryName == archive_entry_pathname(entry.get()));
		if (found)
			break;

		archive_read_data_skip(inArchive);
	}

	if (!found) {
		return makeError(GenericError::IO, "entry not found");
	}

	auto fileId = nextOpenId();
	_openArchives.try_emplace(fileId, mv(file), mv(maybeArchive));

	return fileId;
}


kasofs::Result<ArchiveFS::size_type>
ArchiveFS::read(OpenFID streamId, kasofs::INode& node, size_type offset, MutableMemoryView dest) {
	if (!isArchiveNode(node)) {
		return makeError(GenericError::NXIO, "ArchiveFS::read");
	}

	auto it = _openArchives.find(streamId);
	if (it == _openArchives.end())
		return makeError(GenericError::BADF, "ArchiveFS::read");

	auto& entry = it->second;
	if (entry.readOffset != offset) {
		return makeError(GenericError::IO, "read seek");
	}

	auto r = archive_read_data(entry.archive.get(), dest.dataAddress(), dest.size());
	if (r < 0) {
		return makeErrno(entry.archive.get());
	}

	entry.readOffset += r;

	return Ok<ArchiveFS::size_type>(r);
}


kasofs::Result<Filesystem::size_type>
ArchiveFS::write(OpenFID, kasofs::INode&, size_type, MemoryView) {
	return makeError(GenericError::BADF, "ArchiveFS::write not supported");
}


kasofs::Result<ArchiveFS::size_type>
ArchiveFS::seek(OpenFID, kasofs::INode& node, size_type offset, SeekDirection) {
	if (!isArchiveNode(node)) {
		return makeError(GenericError::NXIO, "ArchiveFS::close");
	}

	return Ok(offset);
}


kasofs::Result<void>
ArchiveFS::close(OpenFID streamId, kasofs::INode& node) {
	if (!isArchiveNode(node)) {
		return makeError(GenericError::NXIO, "ArchiveFS::close");
	}

	_openArchives.erase(streamId);
	return Ok();
}
