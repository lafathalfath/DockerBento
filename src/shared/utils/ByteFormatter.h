#pragma once
#include <QString>

namespace Shared {

class ByteFormatter {
public:
    static QString format(qint64 bytes);
    static QString formatRate(qint64 bytesPerSec);
};

} // namespace Shared
