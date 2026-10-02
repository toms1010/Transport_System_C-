#include "controllers/Controllers.hpp"

namespace gui {

Controllers::Controllers(AppContext& context)
    : context_(context),
      auth_(context),
      routes_(context),
      seats_(context),
      payments_(context),
      complaints_(context),
      notices_(context),
      students_(context),
      staff_(context) {}

}  // namespace gui
