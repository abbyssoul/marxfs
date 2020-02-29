/*
*  Copyright (C) Ivan Ryabov - All Rights Reserved
*
*  Unauthorized copying of this file, via any medium is strictly prohibited.
*  Proprietary and confidential.
*
*  Written by Ivan Ryabov <abbyssoul@gmail.com>
*/
#include "model.hpp"

#include <solace/stringView.hpp>
#include <solace/version.hpp>
#include <solace/posixErrorDomain.hpp>
#include <solace/dictionary.hpp>
#include <solace/dialstring.hpp>
#include <solace/output_utils.hpp>

#include <clime/parser.hpp>

#include <kasofs/kasofs.hpp>
#include <apsio/simpleServer.hpp>


#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>

#include <iostream>  // std::cerr


using namespace Solace;


static auto const kAppName = StringLiteral{"marxfs"};
static auto const kAppVersion = Version{0, 0, 1, "dev"};


namespace marxfs {

struct AppOptions {
	Solace::StringView  pidFileName{};  // "/var/run/marxfs.pid"
	Solace::StringView  configFileName{"/etc/marxfs/marxfs.toml"};

	apsio::Server::BaseConfig	serverOptions;

	// Note: Default bind config exposes marxfs on tcp:564 which requires root privileges
	std::vector<DialString> binds = {{kProtocolTCP, "127.0.0.1", "564"}};
};


struct Configuration {
	apsio::Server::Config		serverConfig;
	std::vector<DialString>		binds;

	kasofs::Vfs vfs;
};


kasofs::User
systemUser() noexcept {
	return {getuid(), getgid()};
}


int errorExitCode(Error const& e) {
	if (e) {  // Oh well, crush and burn
		std::cerr << "Error: " << e << std::endl;
	}

	return EXIT_FAILURE;
}


apsio::Result<void>
buildVfs(Configuration& config, kasofs::VfsId fsId, std::filesystem::path const& filePath) {
	auto rootId = config.vfs.rootId();
	auto user = config.vfs.nodeById(rootId).get().owner;

	// create archive root:
	auto filename = filePath.filename();
	auto maybeArchiveRootId = config.vfs.createDirectory(rootId, filename.c_str(), user, 0777);
	if (!maybeArchiveRootId)
		return maybeArchiveRootId.moveError();

	auto res = mapToFs(config.vfs, user, *maybeArchiveRootId, fsId, filePath);
	if (!res) {
		return res.moveError();
	}

	return Ok();
}


struct BindParser {

	static auto parseBindOptions(StringView value) {
		Result<std::vector<DialString>, Error> result{types::okTag, in_place};

		value.split(",", [&result](StringView split, StringView::size_type i, StringView::size_type count) {
			if (!result)
				return;

			auto rest = tryParseDailString(split);
			if (!rest) {
				result = rest.moveError();
				return;
			}

			auto& targetCollection = *result;
			if (i == 0 && count != 0 && targetCollection.empty()) {
				targetCollection.reserve(count);
			}

			targetCollection.emplace_back(*rest);
		});

		return result;
	}

	Optional<Error> operator() (Optional<StringView> const& value, clime::Parser::Context const& /*cntx*/) {
		auto res = parseBindOptions(*value);
		if (!res) {
			return res.moveError();
		}

		auto& dialStrings = res.unwrap();
		binds.reserve(binds.size() + dialStrings.size());
		for (auto& ds : dialStrings) {
			binds.emplace_back(ds);
		}

		// No errors encontered
		return none;
	}

	bool hasCustomBinds() const {
		return !binds.empty();
	}

	auto bindOptions() {
		return std::move(binds);
	}

	std::vector<DialString> binds;
};




Result<Configuration, Error>
configure(std::vector<StringView> const& targetFiles, AppOptions options) {
	auto user = systemUser();
	auto config = Configuration {
		{options.serverOptions},
		std::move(options.binds),
		{user, kasofs::FilePermissions{0777}}
	};

	// Register ArchiveFS driver:
	auto maybeArchiveDriverId = config.vfs.registerFilesystem<ArchiveFS>();
	if (!maybeArchiveDriverId) {
		return maybeArchiveDriverId.moveError();
	}

	for (auto fileName : targetFiles) {
		std::filesystem::path filePath{fileName.begin(), fileName.end()};
		auto buildFsResult = buildVfs(config, *maybeArchiveDriverId, filePath);
		if (!buildFsResult) {
			return buildFsResult.moveError();
		}
	}

	return config;
}


apsio::Server::Config
configureListener(Configuration const& config) {
	apsio::Auth::Policy::ACL acl{{"*", "*"}, std::make_unique<apsio::Auth::Strategy>()};
	acl.strategy->isRequired = false;

	auto maybePolicy = makeArrayOf<apsio::Auth::Policy::ACL>(mv(acl));

	apsio::Server::Config listenerConfig{};
	listenerConfig.authPolicy = apsio::Auth::Policy{maybePolicy.moveResult()};
	listenerConfig.backlog = config.serverConfig.backlog;
	listenerConfig.maxConnections = config.serverConfig.maxConnections;
	listenerConfig.maxMessageSize = config.serverConfig.maxMessageSize;

	return listenerConfig;
}


int
run(Result<Configuration, Error>&& maybeConfig) {
	if (!maybeConfig) {
		return errorExitCode(maybeConfig.getError());
	}

	auto& config = *maybeConfig;
	auto iocontext = asio::io_context{};
	asio::signal_set stopSignals{iocontext, SIGINT, SIGTERM};
	stopSignals.async_wait([&] (asio::error_code const& error, int signal_number) {
		if (error) {
			std::cerr << "Error waiting for a Signal: " << error.message() << std::endl;
			return;
		}

		// Note: It would be nice to know how many clients we dropped.
		std::clog << "Terminate signal " << signal_number << " received. stopping" << std::endl;
		iocontext.stop();
	});


	auto server = apsio::SimpleServer{iocontext, (*maybeConfig).vfs};

	std::vector<std::shared_ptr<apsio::Server::ConnectionListener>> listeners;
	listeners.reserve(config.binds.size());

	for (auto const& host : config.binds) {
		auto maybeLstener = server.listen(host, configureListener(config));
		if (!maybeLstener) {
			std::cerr << "Error: " << maybeLstener.getError() << std::endl;
			return EXIT_FAILURE;
		}

		listeners.emplace_back(maybeLstener.moveResult());
		std::clog << "Listening on '" << host << "'\n";
	}

	// Spin the loop
	iocontext.run();

	// And we done here
	return EXIT_SUCCESS;
}

}  // namespace marxfs


// Application entry point
int main(const int argc, const char *argv[]) {
	marxfs::AppOptions options;
	std::vector<StringView> files{};

	auto bindParser = marxfs::BindParser{};
	auto argsParsed = clime::Parser(kAppName, {
				clime::Parser::printVersion(kAppName, kAppVersion),
				clime::Parser::printHelp(),
				// {{"c", "config"},	"Config file to use", &options.configFileName},
				{{"P", "pidfile"},	"Path for the daemon PID file", &options.pidFileName},

				{{"maxCon"},	"Max number of concurrent connections", &options.serverOptions.maxConnections},
				{{"msize"},		"Max message payload size", &options.serverOptions.maxMessageSize},
				{{"backlog"},	"Max number of pending connections", &options.serverOptions.backlog},

				{{"H", "host"},	"Address(s) on which to listen for conections",
				 clime::Parser::ArgumentValue::Required, std::ref(bindParser)},
			})
			.arguments({
			   {"*", "JSON file", [&files](Solace::StringView arg, clime::Parser::Context const&) -> Optional<Error> {
					files.emplace_back(arg);
					return none;
				}}
			})
			.parse(argc, argv);

	if (argsParsed && bindParser.hasCustomBinds()) {
		options.binds = bindParser.bindOptions();
	}

	if (argsParsed && files.empty()) {
		std::cerr << "No files given\n";
		return EXIT_FAILURE;
	}

	return (argsParsed)
			? marxfs::run(marxfs::configure(files, options))
			: marxfs::errorExitCode(argsParsed.getError());
}
