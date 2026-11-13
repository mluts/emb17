#include "console.hpp"
#include "console.h"
#include "etl/flat_map.h"
#include "etl/queue_spsc_atomic.h"
#include "etl/string.h"

namespace {
constexpr size_t RX_QUEUE_SIZE = 128;
constexpr size_t LINE_SIZE = 64;
constexpr size_t MAX_COMMANDS = 24;
volatile bool rx_dropped = false;

etl::queue_spsc_atomic<uint8_t, RX_QUEUE_SIZE> rx_queue;
etl::flat_map<etl::string_view, console::Handler, MAX_COMMANDS> commands;
etl::string<LINE_SIZE> line;

bool overflow = false;
} // namespace

void prompt() {
  printf("> ");
  fflush(stdout);
}

void run(etl::string_view text) {
  if (text.empty()) {
    prompt();
    return;
  }

  size_t space_pos = text.find(' ');
  etl::string_view name = text.substr(0, space_pos);
  etl::string_view arg;

  if (space_pos != etl::string_view::npos) {
    arg = text.substr(space_pos);

    size_t first = arg.find_first_not_of(' ');

    arg = (first == etl::string_view::npos) ? etl::string_view()
                                            : arg.substr(first);
  }

  auto it = commands.find(name);
  if (it == commands.end()) {
    printf("\r\nUnknown command: \"%.*s\"\r\n", (int)name.size(), name.data());
  } else {
    it->second(arg);
  }

  prompt();
}

void console::add(etl::string_view name, Handler handler) {
  if (!commands.full()) {
    commands[name] = handler;
  }
}

void console::read_line() {
  uint8_t c;

  while (rx_queue.pop(c)) {
    if (c == '\r' || c == '\n') {
      if (overflow) {
        printf("\r\nerror: line too long\r\n");
        prompt();
      } else {
        run(etl::string_view(line.data(), line.size()));
      }
      line.clear();
      overflow = false;
      rx_dropped = false;
    } else if (!line.full()) {
      line.push_back(static_cast<char>(c));
    } else {
      overflow = true;
    }
  }
}

extern "C" void USBD_CDC_console_receive(uint8_t *Buf, uint32_t *Len) {
  for (uint32_t i = 0; i < *Len; i++) {
    if (!rx_queue.push(Buf[i])) {
      rx_dropped = true;
    }
  }
}
