#pragma once

#include <deque>
#include <string>

namespace sonarlock::core {

class EventJournal {
  public:
    explicit EventJournal(std::size_t capacity = 128);
    void push(std::string event_json_line);
    std::string dump_json_array(std::size_t max_items) const;

  private:
    std::size_t capacity_;
    std::deque<std::string> events_;
};

} // namespace sonarlock::core
