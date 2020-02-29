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
#include "errorDomain.hpp"
#include <solace/string.hpp>

using namespace Solace;
using namespace marxfs;


AtomValue const marxfs::kArchiveErrorCatergory = atom("archfs");


namespace /*anonimous*/ {

struct MarxfsErrorDomain final : public ErrorDomain {

	StringView name() const noexcept override { return "marxfs"; }

	String message(int /*code*/) const noexcept override {
		return String{};
	}
};


MarxfsErrorDomain errorDomain{};
auto const rego_generic = registerErrorDomain(kArchiveErrorCatergory, errorDomain);

}  // namespace


Error
marxfs::makeErrno(archive* arc) noexcept {
	return Error{kArchiveErrorCatergory, archive_errno(arc), archive_error_string(arc)};
}
