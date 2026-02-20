#pragma once

#include "sonarlock/core/types.hpp"

#include <memory>
#include <string>

namespace sonarlock::platform {

struct ActionResult {
    bool ok{true};
    std::string message{"noop"};
};

class IActionExecutor {
  public:
    virtual ~IActionExecutor() = default;
    virtual ActionResult execute(const sonarlock::core::ActionRequest& req) = 0;
};

class ICommandRunner {
  public:
    virtual ~ICommandRunner() = default;
    virtual int run(const std::string& cmd) = 0;
};

class SystemCommandRunner final : public ICommandRunner {
  public:
    int run(const std::string& cmd) override;
};

std::unique_ptr<IActionExecutor> make_executor(std::unique_ptr<ICommandRunner> runner = std::make_unique<SystemCommandRunner>());

} // namespace sonarlock::platform
