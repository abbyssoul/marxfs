/*
*  Copyright (C) 2020 Ivan Ryabov
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/
#pragma once
#ifndef MARXFS_SRC_ERRORDOMAIN_HPP
#define MARXFS_SRC_ERRORDOMAIN_HPP

#include <solace/errorDomain.hpp>
#include <solace/error.hpp>

#include <archive.h>


namespace marxfs {


extern const Solace::AtomValue kArchiveErrorCatergory;


[[nodiscard]]
Solace::Error makeErrno(archive* arc) noexcept;

}  // namespace marxfs
#endif  // MARXFS_SRC_ERRORDOMAIN_HPP
