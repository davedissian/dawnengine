/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.me.uk)
 */
#include "Base.h"
#include "core/io/FileSystem.h"
#include "core/App.h"
#include "core/Engine.h"
#include "core/GameSession.h"
#include "input/Input.h"
#include "renderer/Renderer.h"
#include "resource/ResourceCache.h"
#include "scene/SceneManager.h"
#include "script/LuaVM.h"

// Required for getBasePath/getPrefPath.
#if DW_PLATFORM == DW_WIN32
#include <Psapi.h>
#elif DW_PLATFORM == DW_MACOS
#include <CoreFoundation/CoreFoundation.h>
#define MAX_PATH 256
#elif DW_PLATFORM == DW_LINUX
#include <unistd.h>
#endif

#ifdef DW_EMSCRIPTEN
#include <emscripten.h>
#endif

namespace dw {
SessionId::SessionId(u32 session_index) : session_index_(session_index) {
}

u32 SessionId::index() const {
    return session_index_;
}

int Engine::runApp(UniquePtr<App> app, int argc, char** argv) {
    // TODO(David) Load config.

    CommandLine cmdline(argc, argv);
    Engine engine{std::move(app)};
    engine.setup(cmdline);
    auto exit_code = engine.run();

    // If we're running in the browser, engine.run() will return immediately. We should not shut
    // down in this case.
#ifndef DW_EMSCRIPTEN
    engine.shutdown();
#endif

    return exit_code;
}

Engine::Engine(UniquePtr<App> app)
    : Object{nullptr},
      initialised_{false},
      running_{true},
      save_config_on_exit_{true},
      headless_{false},
      app_(std::move(app)),
      log_file_{"engine.log"},
      config_file_{"engine.cfg"} {
}

Engine::~Engine() {
    shutdown();
}

void Engine::setup(const CommandLine& cmdline) {
    assert(!initialised_);

    cmdline_ = cmdline;

    // Create context.
    context_ = new Context(basePath(), "");
    app_->context_ = context_;
    app_->engine_ = this;

    // Initialise file system.
    context_->addModule<FileSystem>();

    // Initialise logging.
    context_->addModule<Logger>();
    // TODO(david): Add a file logger to prefPath + log_file_
#ifdef DW_DEBUG
    log().warn("NOTE: This is a debug build!");
#endif

    // Update working directory.
    context_->module<FileSystem>()->setWorkingDir(context_->basePath());

    // Print info.
    log().info("Initialising engine " DW_VERSION_STR);
    printSystemInfo();
    if (cmdline.flags.size() > 0) {
        log().info("Flags:");
        for (auto& flag : cmdline.flags) {
            log().info("\t%s", flag);
        }
    }
    if (cmdline.arguments.size() > 0) {
        log().info("Arguments:");
        for (auto& arg : cmdline.arguments) {
            log().info("\t%s %s", arg.first, arg.second);
        }
    }

    // Enable headless mode if the flag is passed.
    if (cmdline.flags.find("-headless") != cmdline.flags.end()) {
        headless_ = true;
        log().info("Running in headless mode.");
    }

    // Build window title.
    String window_title{app_->gameName()};
    window_title += " ";
    window_title += app_->gameVersion();
#ifdef DW_DEBUG
    window_title += " (debug)";
#endif

    // Load configuration
    if (context_->module<FileSystem>()->fileExists(config_file_)) {
        log().info("Loading configuration from %s", config_file_);
        context_->loadConfig(config_file_);
    } else {
        log().info("Configuration does not exist, creating %s", config_file_);
        context_->setDefaultConfig();
    }

    // Create the engine subsystems.
    auto* renderer = context_->addModule<Renderer>();
    auto renderer_result = Result<void>();
    if (!headless_) {
        bool use_multithreading = true;
#ifdef DW_EMSCRIPTEN
        use_multithreading = false;
#endif
        renderer_result = renderer->rhi()->init(
            rhi::RendererType::OpenGL, context_->config().at("window_width").get<u16>(),
            context_->config().at("window_height").get<u16>(), window_title, use_multithreading);
        context_->addModule<Input>();
    } else {
        renderer_result = renderer->rhi()->init(
            rhi::RendererType::Null, context_->config().at("window_width").get<u16>(),
            context_->config().at("window_height").get<u16>(), window_title, false);
    }
    if (!renderer_result) {
        log().error("Renderer failed to initialise: %s", renderer_result.error());
        std::abort();
    }
    context_->addModule<ResourceCache>();

    // Engine events and UI.
    event_system_ = makeUnique<EventSystem>(context_);
    if (!headless_) {
        context_->module<Input>()->registerEventSystem(event_system_.get());
    }
    ui_ = makeUnique<UserInterface>(context_, event_system_.get());

    /*
    auto* net = context_->addModule<Networking>();
    auto port_arg = cmdline.arguments.find("-p");
    u16 port = port_arg != cmdline.arguments.end() ? std::stoi(port_arg->second) : 40000;
    if (cmdline.flags.find("-host") != cmdline.flags.end()) {
        net->listen(port, 32);
    } else if (cmdline.arguments.find("-join") != cmdline.arguments.end()) {
        String ip = cmdline.arguments["-join"];
        net->connect(ip, port);
    }
    */

    // Set up built in entity systems.
    // auto& sm = *context_->module<SceneManager>();

    // Set input viewport size
    /*
    subsystem<Input>()->setViewportSize(subsystem<Renderer>()->getViewportSize());

    // Enumerate available video modes
    Vector<SDL_DisplayMode> displayModes = subsystem<Renderer>()->getDeviceDisplayModes();
    LOG << "Available video modes:";
    for (auto i = displayModes.begin(); i != displayModes.end(); i++) {
        LOG << "\t" << (*i).w << "x" << (*i).h << "@" << (*i).refresh_rate << "Hz"
            << " - Pixel Format: " << SDL_GetPixelFormatName((*i).format);
    }

    // TODO: move this to UI system
    SDL_StartTextInput();
     */

    // Display startup info
    log().info("Current Working Directory: %s", module<FileSystem>()->workingDir());

    // The engine is now initialised
    initialised_ = true;
    log().info("Engine initialised. Starting %s %s", app_->gameName(), app_->gameVersion());

    // Register delegate.
    event_system_->addListener(this, &Engine::onExit);

    // Initialise app.
    app_->init(cmdline);
}

void Engine::shutdown() {
    if (!initialised_) {
        return;
    }

    // Save config.
    if (save_config_on_exit_) {
        context_->saveConfig(config_file_);
    }

    // Shutdown app.
    app_->shutdown();

    ui_.reset();
    event_system_.reset();
    game_sessions_.clear();

    // Remove subsystems.
    context_->removeModule<ResourceCache>();
    context_->clearModules();

    // The engine is no longer initialised.
    initialised_ = false;
}

int Engine::run() {
    float time_per_update = 1.0f / 60.0f;
    time::TimePoint previous_time = time::beginTiming();
    double accumulated_time = 0.0;
    double frame_time_ = 0.0;
    auto main_loop = [&] {
        time::TimePoint current_time = time::beginTiming();
        frame_time_ = time::elapsed(previous_time, current_time);
        previous_time = current_time;
        accumulated_time += frame_time_;

        // Update game logic.
        while (accumulated_time >= time_per_update) {
            forEachSession([time_per_update](GameSession* session) {
                session->preUpdate();
                session->update(time_per_update);
                session->postUpdate();
            });
            accumulated_time -= time_per_update;
        }

        // Render a frame.
        double interpolation = accumulated_time / time_per_update;
        forEachSession([&](GameSession* session) {
            session->preRender();
            session->render(static_cast<float>(frame_time_), interpolation);
            session->postRender();
        });
        ui_->render();
        if (!context_->module<Renderer>()->frame()) {
            running_ = false;
        }
    };

#ifdef DW_EMSCRIPTEN
    // void emscripten_set_main_loop(em_callback_func func, int fps, int simulate_infinite_loop);
    emscripten_set_main_loop_arg(
        [](void* arg) { (*reinterpret_cast<decltype(main_loop)*>(arg))(); },
        reinterpret_cast<void*>(&main_loop), 0, 1);
#else
    while (running_) {
        main_loop();
    }
#endif

    return EXIT_SUCCESS;
}

SessionId Engine::addSession(UniquePtr<GameSession> session) {
    // TODO: Initialise session.
    game_sessions_.emplace_back(std::move(session));
    return {static_cast<u32>(game_sessions_.size()) - 1};
}

void Engine::replaceSession(SessionId session_id, UniquePtr<GameSession> session) {
    assert(game_sessions_.size() < session_id.index());
    // TODO: Initialise session.
    game_sessions_[session_id.index()] = std::move(session);
}

void Engine::removeSession(SessionId session_id) {
    game_sessions_[session_id.index()].reset();
}

const Set<String>& Engine::flags() const {
    return cmdline_.flags;
}

const HashMap<String, String>& Engine::arguments() const {
    return cmdline_.arguments;
}

void Engine::forEachSession(const Function<void(GameSession*)>& functor) {
    for (auto& session : game_sessions_) {
        functor(session.get());
    }
}

void Engine::printSystemInfo() {
#if DW_PLATFORM == DW_WIN32
    String platform = "Windows";
#elif DW_PLATFORM == DW_MACOS
    String platform = "macOS";
#elif DW_PLATFORM == DW_LINUX
#ifdef DW_EMSCRIPTEN
    String platform = "Emscripten";
#else
    String platform = "Linux";
#endif
#endif
    log().info("Platform: %s", platform);
    log().info("Base Path: %s", context()->basePath());
    log().info("Pref Path: %s", context()->prefPath());
    // TODO: more system info
}

#if DW_PLATFORM == DW_LINUX
namespace {
String readSymLink(const String& path) {
    char* retval = nullptr;
    size_t len = 64;
    ssize_t rc;

    while (true) {
        if (retval) {
            delete[] retval;
        }
        retval = new char[len + 1];
        rc = readlink(path.c_str(), retval, len);
        if (rc == -1) {
            break; /* not a symlink, i/o error, etc. */
        } else if (rc < static_cast<ssize_t>(len)) {
            retval[rc] = '\0'; /* readlink doesn't null-terminate. */
            String result{retval};
            delete[] retval;
            return result; /* we're good to go. */
        }

        len *= 2; /* grow buffer, try again. */
    }

    delete[] retval;
    return {};
}
}  // namespace
#endif

String Engine::basePath() const {
#if DW_PLATFORM == DW_WIN32
    u32 buffer_length = 128;
    char* path_array = NULL;
    u32 length = 0;

    // Get module filename.
    while (true) {
        path_array = new char[buffer_length];
        if (!path_array) {
            delete[] path_array;
            // TODO: Out of memory
            return "";
        }

        length = GetModuleFileNameEx(GetCurrentProcess(), NULL, path_array, buffer_length);
        if (length != buffer_length) {
            break;
        }

        // Buffer too small? Try again.
        buffer_length *= 2;
    }

    // If we failed to locate the exe, bail.
    if (length == 0) {
        delete[] path_array;
        // WIN_SetError("Couldn't locate our .exe");
        return "";
    }

    // Trim off the filename.
    String path{path_array};
    auto last_backslash = path.find_last_of('\\');
    assert(last_backslash != path.npos);
    return path.substr(0, last_backslash + 1);
#elif DW_PLATFORM == DW_MACOS
    CFBundleRef main_bundle = CFBundleGetMainBundle();
    CFURLRef resources_url = CFBundleCopyResourcesDirectoryURL(main_bundle);
    char path[MAX_PATH];
    if (!CFURLGetFileSystemRepresentation(resources_url, TRUE, (UInt8*)path, MAX_PATH)) {
        // TODO: error
    }
    CFRelease(resources_url);
    String str_path{path};
    str_path += "/";

    // For debugging, move from Resources folder to bin.
    str_path += "../../../";
    return str_path;
#elif DW_PLATFORM == DW_LINUX
#ifndef DW_EMSCRIPTEN
    String executable_path;

    // Is a Linux-style /proc filesystem available?
    if (access("/proc", F_OK) != 0) {
        // error.
        return "";
    }
    executable_path = readSymLink("/proc/self/exe");
    if (executable_path == "") {
        // Older kernels don't have /proc/self, try PID version.
        executable_path =
            readSymLink(tinyformat::format("/proc/%llu/exe", (unsigned long long)getpid()));
    }

    // Chop off filename.
    auto len = executable_path.find_last_of('/');
    executable_path = executable_path.substr(0, len);
    return executable_path;
#else
    // Use root directory for emscripten.
    return "/";
#endif
#endif
}

void Engine::onExit(const ExitEvent&) {
    running_ = false;
}
}  // namespace dw
