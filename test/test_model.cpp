/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
/*******************************************************************************
 * Unit Test Suit
 *	@file test/test_model.cpp
 *	@brief		Test suit for marxfs::Model
 ******************************************************************************/
#include "model.hpp"    // Class being tested.

#include <solace/output_utils.hpp>
#include <gtest/gtest.h>

using namespace Solace;
using namespace marxfs;


struct TestArchiveFS : public ::testing::Test {

	void SetUp() override {
		auto maybeFsId = _vfs.registerFilesystem<ArchiveFS>();
		ASSERT_TRUE(maybeFsId.isOk());

		_jsonFsId = *maybeFsId;
	}

	void TearDown() override {
		_vfs.unregisterFileSystem(_jsonFsId);
	}

protected:
	kasofs::User	_owner{0, 0};
	kasofs::Vfs		_vfs{_owner, kasofs::FilePermissions{0666}};
	kasofs::VfsId	_jsonFsId{0};
};



TEST_F(TestArchiveFS, modelPopulation) {

}

