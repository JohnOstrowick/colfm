#pragma once
// helpers/icons_data.h — only the compiled-in XML accessors (no parsing here)

#include <cstddef>
#include <string>
#include <string_view>

#include "icons_data_blob.h"  // provides: kIconsXmlBlob, kIconsXmlBlobLen

namespace IconsData {

inline std::string_view iconsXmlView() noexcept {
    return std::string_view(
        reinterpret_cast<const char*>(kIconsXmlBlob),
        static_cast<std::size_t>(kIconsXmlBlobLen)
    );
}

inline std::string iconsXmlString() {
    return std::string(
        reinterpret_cast<const char*>(kIconsXmlBlob),
        static_cast<std::size_t>(kIconsXmlBlobLen)
    );
}

} // namespace IconsData
