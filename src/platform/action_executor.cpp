#include "sonarlock/platform/action_executor.hpp"

#include <cstdlib>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace sonarlock::platform {

int SystemCommandRunner::run(const std::string& cmd) { return std::system(cmd.c_str()); }

namespace {

class LinuxActionExecutor final : public IActionExecutor {
  public:
    explicit LinuxActionExecutor(std::unique_ptr<ICommandRunner> runner) : runner_(std::move(runner)) {}

    ActionResult execute(const sonarlock::core::ActionRequest& req) override {
        using sonarlock::core::ActionType;
        if (req.type == ActionType::None) return {true, "none"};
        if (req.type == ActionType::Beep) return {true, "soft"};
        if (req.type == ActionType::Notify) return {true, "notify"};

#if defined(_WIN32)
        (void)req;
        return {false, "not-linux"};
#else
        const char* cmds[] = {"loginctl lock-session", "gnome-screensaver-command -l", "xdg-screensaver lock"};
        for (auto* c : cmds) {
            if (runner_->run(c) == 0) return {true, std::string("lock-ok:") + c};
        }
        return {false, "lock commands failed"};
#endif
    }

  private:
    std::unique_ptr<ICommandRunner> runner_;
};

class WindowsActionExecutor final : public IActionExecutor {
  public:
    ActionResult execute(const sonarlock::core::ActionRequest& req) override {
        using sonarlock::core::ActionType;
        if (req.type == ActionType::None) return {true, "none"};
        if (req.type == ActionType::Beep) return {true, "soft"};
        if (req.type == ActionType::Notify) return {true, "notify"};
#if defined(_WIN32)
        return {LockWorkStation() != 0, "LockWorkStation"};
#else
        return {false, "not-windows"};
#endif
    }
};

} // namespace

std::unique_ptr<IActionExecutor> make_executor(std::unique_ptr<ICommandRunner> runner) {
#if defined(_WIN32)
    (void)runner;
    return std::make_unique<WindowsActionExecutor>();
#else
    return std::make_unique<LinuxActionExecutor>(std::move(runner));
#endif
}

} // namespace sonarlock::platform
