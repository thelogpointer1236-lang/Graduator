#ifdef USE_STUB_ANGLEMETER_IMPLEMENTATIONS

#include "core/services/cameraProcessor/anglemeter/cast_anglemeter.h"
#include "core/services/ServiceLocator.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>

namespace {
constexpr double kAnglePeriodSec = 60.0;
constexpr double kHalfPeriodSec = kAnglePeriodSec / 2.0;
constexpr double kMinAngleDeg = 160.0;
constexpr double kMaxAngleDeg = -140.0;

struct ImageBuffer {
    std::unique_ptr<unsigned char[]> data;
    std::size_t sizeBytes = 0;
};

ImageBuffer &buffer() {
    static ImageBuffer instance;
    return instance;
}

qreal elapsedTimeSeconds() {
    if (auto *graduationService = ServiceLocator::instance().graduationService()) {
        return graduationService->getElapsedTimeSeconds();
    }
    return 0.0;
}
}

struct anglemeter_t {
    int width = 0;
    int height = 0;
    float (*transform)(float) = nullptr;
};

void anglemeterCreate(anglemeter_t **am_ptr) {
    if (!am_ptr) {
        return;
    }
    *am_ptr = new anglemeter_t();
}

void anglemeterDestroy(anglemeter_t *am) {
    delete am;
}

void anglemeterSetImageSize(anglemeter_t *am, int width, int height) {
    if (!am) {
        return;
    }
    am->width = width;
    am->height = height;
}

void anglemeterSetAngleTransformation(anglemeter_t *am, float (*func_ptr)(float)) {
    if (!am) {
        return;
    }
    am->transform = func_ptr;
}

void anglemeterRestoreState(anglemeter_t *am) {
    Q_UNUSED(am);
}

void anglemeterCreateImageBuffer(int width, int height) {
    auto &buf = buffer();
    const std::size_t size = static_cast<std::size_t>(std::max(1, width * height * 3));
    buf.data = std::make_unique<unsigned char[]>(size);
    buf.sizeBytes = size;
}

bool anglemeterIsImageBufferCreated() {
    return static_cast<bool>(buffer().data);
}

void anglemeterDestroyImageBuffer() {
    auto &buf = buffer();
    buf.data.reset();
    buf.sizeBytes = 0;
}

unsigned char *anglemeterCopyImage(const unsigned char *img) {
    auto &buf = buffer();
    if (!buf.data || buf.sizeBytes == 0) {
        return nullptr;
    }

    auto out = std::make_unique<unsigned char[]>(buf.sizeBytes);
    if (img) {
        std::copy(img, img + buf.sizeBytes, out.get());
    } else {
        std::fill_n(out.get(), buf.sizeBytes, 0);
    }

    return out.release();
}

void anglemeterFreeImage(const unsigned char *img) {
    delete[] img;
}

bool anglemeterGetArrowAngle(anglemeter_t *am, const unsigned char *img, float *angle) {
    Q_UNUSED(img);

    if (!am || !angle) {
        return false;
    }

    const double timestampSec = elapsedTimeSeconds();
    const double phase = std::fmod(timestampSec, kAnglePeriodSec);

    double computedAngle = 0.0;
    if (phase < kHalfPeriodSec) {
        computedAngle = kMinAngleDeg + (phase / kHalfPeriodSec) * (kMaxAngleDeg - kMinAngleDeg);
    } else {
        computedAngle = kMinAngleDeg + ((kAnglePeriodSec - phase) / kHalfPeriodSec) * (kMaxAngleDeg - kMinAngleDeg);
    }

    float result = static_cast<float>(computedAngle);
    if (am->transform) {
        result = am->transform(result);
    }

    *angle = result;
    return true;
}

#endif
