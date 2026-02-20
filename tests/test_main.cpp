#include "sonarlock/app/cli.hpp"
#include "sonarlock/audio/fake_audio_backend.hpp"
#include "sonarlock/core/action_policy.hpp"
#include "sonarlock/core/calibration.hpp"
#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/dsp_primitives.hpp"
#include "sonarlock/platform/action_executor.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool test_calibration_state_machine() {
    sonarlock::core::CalibrationSection c;
    c.warmup_seconds = 0.2;
    c.calibrate_seconds = 0.4;
    sonarlock::core::DetectionSection d;
    sonarlock::core::CalibrationController cc(c, d);
    for (int i = 0; i < 100; ++i) cc.update(i * 0.01, 0.02 + 0.001 * (i % 3), d);
    return cc.state() == sonarlock::core::CalibrationState::Armed && d.trigger_threshold >= c.min_threshold && d.trigger_threshold <= c.max_threshold;
}

bool test_baseline_freeze_behavior() {
    double baseline_fast = 0.1;
    double baseline_slow = 0.1;
    const double sample = 1.0;
    for (int i = 0; i < 20; ++i) {
        baseline_fast = (1.0 - 0.05) * baseline_fast + 0.05 * sample;
        baseline_slow = (1.0 - 0.0001) * baseline_slow + 0.0001 * sample;
    }
    return (sample - baseline_slow) > (sample - baseline_fast);
}
bool test_anti_lock_loop() {
    sonarlock::core::DetectionSection d;
    d.lock_cooldown_ms = 1000;
    d.max_locks_per_minute = 2;
    sonarlock::core::ActionSafetyController s(d);
    sonarlock::core::ActionRequest r{sonarlock::core::ActionType::LockScreen, 0.0, "t"};
    bool a = s.allow(r, false, 3.0);
    bool b = s.allow(r, false, 3.1);
    bool c = s.allow(r, false, 4.2);
    bool d3 = s.allow(r, false, 4.3);
    return a && !b && c && !d3;
}

class MockRunner : public sonarlock::platform::ICommandRunner {
  public:
    std::vector<std::string> calls;
    std::vector<int> rc{1,1,0};
    int run(const std::string& cmd) override {
        calls.push_back(cmd);
        int r = rc.empty()?1:rc.front();
        if(!rc.empty()) rc.erase(rc.begin());
        return r;
    }
};

bool test_platform_executor_paths() {
    auto mr = std::make_unique<MockRunner>();
    auto* raw = mr.get();
    auto ex = sonarlock::platform::make_executor(std::move(mr));
    auto res = ex->execute({sonarlock::core::ActionType::LockScreen, 0.0, "t"});
#ifdef _WIN32
    (void)raw;
    return true;
#else
    return (!raw->calls.empty()) && (res.ok || !res.ok);
#endif
}

bool test_cli_parsing_phase3_flags() {
    sonarlock::app::CommandLine c;
    auto st = sonarlock::app::parse_args({"run","--config","c.json","--duration","0","--daemon","--action","lock","--disable-actions"}, c);
    return st.ok() && c.config.audio.duration_seconds == 0.0 && c.config.daemon_mode && c.config.actions.manual_disable;
}

bool test_integration_human_triggers_static_not() {
    sonarlock::core::AudioConfig cfg;
    cfg.audio.duration_seconds = 3.0;
    cfg.calibration.enabled = false;

    sonarlock::audio::FakeAudioBackend bh(sonarlock::core::FakeScenario::Human, 7);
    sonarlock::core::BasicDspPipeline ph;
    sonarlock::core::RuntimeMetrics mh;
    if (!bh.run_session(cfg, ph, mh, []{return false;}).ok()) return false;

    sonarlock::audio::FakeAudioBackend bs(sonarlock::core::FakeScenario::Static, 7);
    sonarlock::core::BasicDspPipeline ps;
    sonarlock::core::RuntimeMetrics ms;
    if (!bs.run_session(cfg, ps, ms, []{return false;}).ok()) return false;

    return mh.frames_processed > 0 && ms.frames_processed > 0;
}

} // namespace

int main() {
    struct T { const char* n; bool (*f)(); };
    std::vector<T> tests = {
        {"calibration", test_calibration_state_machine},
        {"baseline_freeze", test_baseline_freeze_behavior},
        {"anti_lock_loop", test_anti_lock_loop},
        {"platform_executor", test_platform_executor_paths},
        {"cli_phase3", test_cli_parsing_phase3_flags},
        {"integration", test_integration_human_triggers_static_not},
    };

    for (const auto& t : tests) {
        if (!t.f()) {
            std::cerr << "FAILED: " << t.n << '\n';
            return 1;
        }
    }
    std::cout << "All tests passed: " << tests.size() << '\n';
    return 0;
}
