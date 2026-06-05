#include "ByteFormatter.h"

namespace Shared {

QString ByteFormatter::format(qint64 bytes) {
    if (bytes < 0) return "N/A";
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
}

QString ByteFormatter::formatRate(qint64 bytesPerSec) {
    return format(bytesPerSec) + "/s";
}

} // namespace Shared
