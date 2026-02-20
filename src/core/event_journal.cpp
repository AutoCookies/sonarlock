#include "sonarlock/core/event_journal.hpp"

#include <sstream>

namespace sonarlock::core {

EventJournal::EventJournal(std::size_t capacity) : capacity_(capacity) {}

void EventJournal::push(std::string event_json_line) {
    if (events_.size() >= capacity_) events_.pop_front();
    events_.push_back(std::move(event_json_line));
}

std::string EventJournal::dump_json_array(std::size_t max_items) const {
    std::ostringstream out;
    out << '[';
    const std::size_t start = (events_.size() > max_items) ? (events_.size() - max_items) : 0;
    bool first = true;
    for (std::size_t i = start; i < events_.size(); ++i) {
        if (!first) out << ',';
        first = false;
        out << events_[i];
    }
    out << ']';
    return out.str();
}

} // namespace sonarlock::core
