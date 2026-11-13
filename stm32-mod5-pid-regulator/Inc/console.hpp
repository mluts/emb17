#pragma once

#include "etl/delegate.h"
#include "etl/string_view.h"

namespace console {
// Handlers receive only the arg text, alredy stripped of the command word

using Handler = etl::delegate<void(etl::string_view)>;

void add(etl::string_view name, Handler handler);
void read_line();
} // namespace console
