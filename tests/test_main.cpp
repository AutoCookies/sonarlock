#include "sonarlock/app/cli.hpp"
#include "sonarlock/audio/fake_audio_backend.hpp"
#include "sonarlock/core/dsp_pipeline.hpp"
#include "sonarlock/core/dsp_primitives.hpp"
#include "sonarlock/core/motion_detection.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool approx(double a, double b, double eps = 1e-3) { return std::abs(a - b) <= eps; }

bool test_downmix_correctness() {
    const double sr = 48000.0;
    const double f0 = 19000.0;
    sonarlock::core::Nco nco(sr, f0);
    sonarlock::core::IirLowPass i_lp(sr, 500.0), q_lp(sr, 500.0);

    double mag_sum = 0.0;
    for (int n = 0; n < 4096; ++n) {
        double x = std::sin(2.0 * 3.14159265358979323846 * f0 * static_cast<double>(n) / sr);
        auto [c, s] = nco.next();
        double i = i_lp.process(x * c);
        double q = q_lp.process(x * (-s));
        if (n > 500) mag_sum += std::sqrt(i * i + q * q);
    }
    const double avg = mag_sum / (4096.0 - 500.0);
    return avg > 0.45 && avg < 0.55;
}

bool test_lowpass_attenuation_and_stability() {
    sonarlock::core::IirLowPass lp(1000.0, 50.0);
    double high_rms = 0.0, low_rms = 0.0;
    for (int n = 0; n < 4000; ++n) {
        const double t = n / 1000.0;
        const double in = std::sin(2.0 * 3.14159265358979323846 * 10.0 * t) + 0.8 * std::sin(2.0 * 3.14159265358979323846 * 200.0 * t);
        const double y = lp.process(in);
        if (!std::isfinite(y) || std::abs(y) > 10.0) return false;
        if (n > 1000) {
            low_rms += std::pow(std::sin(2.0 * 3.14159265358979323846 * 10.0 * t) - y, 2);
            high_rms += y * y;
        }
    }
    return low_rms < high_rms;
}

bool test_phase_unwrap_continuity() {
    sonarlock::core::PhaseTracker tracker;
    std::vector<double> phases{3.0, 3.1, -3.05, -2.9, -2.7};
    double prev = 0.0;
    bool first = true;
    for (double p : phases) {
        const double u = tracker.unwrap(std::cos(p), std::sin(p));
        if (!first && std::abs(u - prev) > 0.5) return false;
        prev = u;
        first = false;
    }
    return true;
}

bool test_feature_extraction_ordering() {
    sonarlock::core::AudioConfig cfg;
    cfg.dsp.duration_seconds = 2.0;

    sonarlock::audio::FakeAudioBackend b_static(sonarlock::core::FakeScenario::Static, 7);
    sonarlock::core::BasicDspPipeline p_static;
    sonarlock::core::RuntimeMetrics m_static;
    if (!b_static.run_session(cfg, p_static, m_static).ok()) return false;

    sonarlock::audio::FakeAudioBackend b_human(sonarlock::core::FakeScenario::Human, 7);
    sonarlock::core::BasicDspPipeline p_human;
    sonarlock::core::RuntimeMetrics m_human;
    if (!b_human.run_session(cfg, p_human, m_human).ok()) return false;

    return m_human.features.doppler_band_energy > m_static.features.doppler_band_energy;
}

bool test_detection_no_motion_never_triggered() {
    sonarlock::core::MotionDetector det({}, std::make_unique<sonarlock::core::DefaultMotionScorer>());
    for (int i = 0; i < 300; ++i) {
        sonarlock::core::MotionFeatures f{0.2, 0.005, 0.2, 2.0};
        auto ev = det.evaluate(f, i * 0.01);
        if (ev.state == sonarlock::core::DetectionState::Triggered) return false;
    }
    return true;
}

bool test_detection_human_triggers() {
    sonarlock::core::MotionDetector det({}, std::make_unique<sonarlock::core::DefaultMotionScorer>());
    bool triggered = false;
    for (int i = 0; i < 400; ++i) {
        const double t = i * 0.01;
        sonarlock::core::MotionFeatures f{0.3, (t > 0.4 && t < 1.2) ? 0.18 : 0.01, (t > 0.4 && t < 1.2) ? 18.0 : 2.0, 10.0};
        auto ev = det.evaluate(f, t);
        if (ev.state == sonarlock::core::DetectionState::Triggered || ev.state == sonarlock::core::DetectionState::Cooldown) {
            triggered = true;
            break;
        }
    }
    return triggered;
}

bool test_detection_pet_not_triggered() {
    sonarlock::core::MotionDetector det({}, std::make_unique<sonarlock::core::DefaultMotionScorer>());
    for (int i = 0; i < 400; ++i) {
        const double t = i * 0.01;
        sonarlock::core::MotionFeatures f{0.15, 0.04 + 0.01 * std::sin(2 * 3.14159265358979323846 * 6 * t), 6.0, 4.0};
        auto ev = det.evaluate(f, t);
        if (ev.state == sonarlock::core::DetectionState::Triggered) return false;
    }
    return true;
}

bool test_detection_cooldown_behavior() {
    sonarlock::core::DetectionConfig dc;
    dc.cooldown_ms = 500;
    sonarlock::core::MotionDetector det(dc, std::make_unique<sonarlock::core::DefaultMotionScorer>());

    sonarlock::core::DetectionState last = sonarlock::core::DetectionState::Idle;
    bool saw_cooldown = false;
    for (int i = 0; i < 250; ++i) {
        const double t = i * 0.01;
        sonarlock::core::MotionFeatures f{0.3, (t < 0.8) ? 0.2 : 0.01, (t < 0.8) ? 20.0 : 1.0, 12.0};
        auto ev = det.evaluate(f, t);
        if (ev.state == sonarlock::core::DetectionState::Cooldown) saw_cooldown = true;
        if (saw_cooldown && t < 1.2 && ev.state == sonarlock::core::DetectionState::Triggered) return false;
        last = ev.state;
    }
    return saw_cooldown && last != sonarlock::core::DetectionState::Triggered;
}

bool test_integration_fake_human_triggers() {
    sonarlock::core::AudioConfig cfg;
    cfg.scenario = sonarlock::core::FakeScenario::Human;
    cfg.dsp.duration_seconds = 2.0;

    sonarlock::audio::FakeAudioBackend backend(sonarlock::core::FakeScenario::Human, 7);
    sonarlock::core::BasicDspPipeline pipeline;
    sonarlock::core::RuntimeMetrics m;
    auto st = backend.run_session(cfg, pipeline, m);
    return st.ok() && m.triggered_count > 0;
}

bool test_integration_fake_static_no_trigger() {
    sonarlock::core::AudioConfig cfg;
    cfg.scenario = sonarlock::core::FakeScenario::Static;
    cfg.dsp.duration_seconds = 2.0;

    sonarlock::audio::FakeAudioBackend backend(sonarlock::core::FakeScenario::Static, 7);
    sonarlock::core::BasicDspPipeline pipeline;
    sonarlock::core::RuntimeMetrics m;
    auto st = backend.run_session(cfg, pipeline, m);
    return st.ok() && m.triggered_count == 0;
}

bool test_cli_parsing_flags_and_invalid() {
    sonarlock::app::CommandLine cmd;
    auto st = sonarlock::app::parse_args({"analyze", "--backend", "fake", "--scenario", "human", "--f0", "19000", "--lp-cutoff", "500", "--band-low", "20", "--band-high", "200", "--trigger-th", "0.6", "--release-th", "0.4", "--debounce-ms", "300", "--cooldown-ms", "3000", "--csv", "out.csv"}, cmd);
    if (!st.ok() || cmd.kind != sonarlock::app::CommandKind::Analyze) return false;
    if (cmd.config.scenario != sonarlock::core::FakeScenario::Human || cmd.csv_path != "out.csv") return false;
    st = sonarlock::app::parse_args({"analyze", "--scenario", "bad"}, cmd);
    return !st.ok();
}

} // namespace

int main() {
    struct T { const char* name; bool (*fn)(); };
    const std::vector<T> tests = {
        {"downmix", test_downmix_correctness},
        {"lowpass", test_lowpass_attenuation_and_stability},
        {"phase_unwrap", test_phase_unwrap_continuity},
        {"feature_order", test_feature_extraction_ordering},
        {"no_motion", test_detection_no_motion_never_triggered},
        {"human_trigger", test_detection_human_triggers},
        {"pet_no_trigger", test_detection_pet_not_triggered},
        {"cooldown", test_detection_cooldown_behavior},
        {"integration_human", test_integration_fake_human_triggers},
        {"integration_static", test_integration_fake_static_no_trigger},
        {"cli_parse", test_cli_parsing_flags_and_invalid},
    };

    for (const auto& t : tests) {
        if (!t.fn()) {
            std::cerr << "FAILED: " << t.name << '\n';
            return 1;
        }
    }
    std::cout << "All tests passed: " << tests.size() << '\n';
    return 0;
}
