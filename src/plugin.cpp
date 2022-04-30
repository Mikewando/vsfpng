#include "fpng.h"
#include "p2p.h"
#include "p2p_api.h"

#include <VapourSynth4.h>
#include <VSHelper4.h>

#include <string>
#include <memory>

#ifdef _WIN32
#include "vsutf16.h"
#endif

//////////////////////////////////////////
// Shared

static std::string specialPrintf(const std::string &filename, int number) {
    std::string result;
    size_t copyPos = 0;
    size_t minWidth = 0;
    bool zeroPad = false;
    bool percentSeen = false;
    bool zeroPadSeen = false;
    bool minWidthSeen = false;

    for (size_t pos = 0; pos < filename.length(); pos++) {
        const char c = filename[pos];
        if (c == '%' && !percentSeen) {
            result += filename.substr(copyPos, pos - copyPos);
            copyPos = pos;
            percentSeen = true;
            continue;
        }
        if (percentSeen) {
            if (c == '0' && !zeroPadSeen) {
                zeroPad = true;
                zeroPadSeen = true;
                continue;
            }
            if (c >= '1' && c <= '9' && !minWidthSeen) {
                minWidth = c - '0';
                zeroPadSeen = true;
                minWidthSeen = true;
                continue;
            }
            if (c == 'd') {
                std::string num = std::to_string(number);
                if (minWidthSeen && minWidth > num.length())
                    num = std::string(minWidth - num.length(), zeroPad ? '0' : ' ') + num;
                result += num;
                copyPos = pos + 1;
            }
        }
        minWidth = 0;
        zeroPad = false;
        percentSeen = false;
        zeroPadSeen = false;
        minWidthSeen = false;
    }

    result += filename.substr(copyPos, filename.length() - copyPos);

    return result;
}

static bool fileExists(const std::string &filename) {
#ifdef _WIN32
    FILE * f = _wfopen(utf16_from_utf8(filename).c_str(), L"rb");
#else
    FILE * f = fopen(filename.c_str(), "rb");
#endif
    if (f)
        fclose(f);
    return !!f;
}

//////////////////////////////////////////
// Write

struct WriteData {
    VSNode *videoNode;
    VSNode *alphaNode;
    const VSVideoInfo* vi;
    std::string filename;
    int firstNum;
    int compression;
    bool overwrite;

    WriteData() : videoNode(nullptr), alphaNode(nullptr), compression(fpng::FPNG_ENCODE_SLOWER) {}
};

enum {
    RGB24_CHANNELS = 3,
    RGBA32_CHANNELS = 4
};

static const VSFrame *VS_CC writeGetFrame(int n, int activationReason, void *instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
    WriteData *d = static_cast<WriteData*>(instanceData);

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->videoNode, frameCtx);
        if (d->alphaNode) {
            vsapi->requestFrameFilter(n, d->alphaNode, frameCtx);
        }
    } else if (activationReason == arAllFramesReady) {
        const VSFrame* frame = vsapi->getFrameFilter(n, d->videoNode, frameCtx);

        std::string filename = specialPrintf(d->filename, n + d->firstNum);

        if (!d->overwrite && fileExists(filename)) {
            return frame;
        }

        const VSFrame* frames[] = {frame, frame, frame};

        int width = vsapi->getFrameWidth(frame, 0);
        int height = vsapi->getFrameHeight(frame, 0);

        const VSFrame *alphaFrame = nullptr;
        int alphaWidth = 0;
        int alphaHeight = 0;

        if (d->alphaNode) {
            alphaFrame = vsapi->getFrameFilter(n, d->alphaNode, frameCtx);
            alphaWidth = vsapi->getFrameWidth(alphaFrame, 0);
            alphaHeight = vsapi->getFrameHeight(alphaFrame, 0);

            if (width != alphaWidth || height != alphaHeight) {
                vsapi->setFilterError("Write: Mismatched dimension of the alpha clip", frameCtx);
                vsapi->freeFrame(frame);
                vsapi->freeFrame(alphaFrame);
                return nullptr;
            }
        }

        int channelCount = alphaFrame ? RGBA32_CHANNELS : RGB24_CHANNELS;

        uint8_t* imageBuffer = (uint8_t*)malloc(width * height * channelCount);
        p2p_buffer_param p = {};
        p.packing = alphaFrame ? p2p_rgba32_be : p2p_rgb24_be;
        p.width = width;
        p.height = height;
        p.dst[0] = imageBuffer;
        p.dst_stride[0] = width * channelCount;

        for (int plane = 0; plane < channelCount; plane++) {
            if (plane < RGB24_CHANNELS) {
                p.src[plane] = vsapi->getReadPtr(frame, plane);
                p.src_stride[plane] = vsapi->getStride(frame, plane);
            } else {
                p.src[plane] = vsapi->getReadPtr(alphaFrame, 0);
                p.src_stride[plane] = vsapi->getStride(alphaFrame, 0);
            }
        }

        p2p_pack_frame(&p, P2P_ALPHA_SET_ONE);

        bool write_status = fpng::fpng_encode_image_to_file(filename.c_str(), imageBuffer, width, height, channelCount, d->compression);

        vsapi->freeFrame(alphaFrame);
        free(imageBuffer);
        if (!write_status) {
            vsapi->freeFrame(frame);
            vsapi->setFilterError("Write: Frame could not be written. Ensure that output path exists and is writable", frameCtx);
        } else {
            return frame;
        }
    }

    return nullptr;
}

static void VS_CC writeFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
    WriteData *d = static_cast<WriteData *>(instanceData);
    vsapi->freeNode(d->videoNode);
    vsapi->freeNode(d->alphaNode);
    delete d;
}

static void VS_CC writeCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
    std::unique_ptr<WriteData> d(new WriteData());
    int err = 0;

    d->firstNum = vsapi->mapGetIntSaturated(in, "firstnum", 0, &err);
    if (d->firstNum < 0) {
        vsapi->mapSetError(out, "Write: Frame number offset can't be negative");
        return;
    }

    d->compression = vsapi->mapGetIntSaturated(in, "compression", 0, &err);
    if (err) {
        d->compression = 1;
    }
    if (d->compression < 0 || d->compression > fpng::FPNG_FORCE_UNCOMPRESSED) {
        vsapi->mapSetError(out, "Write: Compression must be one of: 0 (encode fast), 1 (default) (encode slower), or 2 (uncompressed)");
        return;
    }

    d->videoNode = vsapi->mapGetNode(in, "clip", 0, nullptr);
    d->vi = vsapi->getVideoInfo(d->videoNode);
    if ((d->vi->format.colorFamily != cfRGB) || d->vi->format.sampleType != stInteger || (d->vi->format.bitsPerSample != 8)) {
        vsapi->freeNode(d->videoNode);
        vsapi->mapSetError(out, "Write: Only RGB24 input is supported");
        return;
    }

    d->alphaNode = vsapi->mapGetNode(in, "alpha", 0, &err);
    d->filename = vsapi->mapGetData(in, "filename", 0, nullptr);
    d->overwrite = !!vsapi->mapGetInt(in, "overwrite", 0, &err);

    if (d->alphaNode) {
        const VSVideoInfo *alphaVi = vsapi->getVideoInfo(d->alphaNode);
        VSVideoFormat alphaFormat;
        vsapi->queryVideoFormat(&alphaFormat, cfGray, d->vi->format.sampleType, d->vi->format.bitsPerSample, 0, 0, core);

        if (d->vi->width != alphaVi->width || d->vi->height != alphaVi->height || alphaVi->format.colorFamily == cfUndefined ||
            !vsh::isSameVideoFormat(&alphaVi->format, &alphaFormat)) {
            vsapi->freeNode(d->videoNode);
            vsapi->freeNode(d->alphaNode);
            vsapi->mapSetError(out, "Write: Alpha clip dimensions and format don't match the main clip");
            return;
        }
    }

    if (!d->overwrite && specialPrintf(d->filename, 0) == d->filename) {
        // No valid digit substitution in the filename so error out to warn the user
        vsapi->freeNode(d->videoNode);
        vsapi->freeNode(d->alphaNode);
        vsapi->mapSetError(out, "Write: Filename string doesn't contain a number");
        return;
    }

    VSFilterDependency deps[] = {{ d->videoNode, rpStrictSpatial }, { d->alphaNode, rpStrictSpatial }};
    vsapi->createVideoFilter(out, "Write", d->vi, writeGetFrame, writeFree, fmParallel, deps, d->alphaNode ? 2 : 1, d.get(), core);
    d.release();
}

//////////////////////////////////////////
// Init

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi) {
    vspapi->configPlugin("tools.mike.fpng", "fpng", "fpng for vapoursynth", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin);
    vspapi->registerFunction("Write", "clip:vnode;filename:data;firstnum:int:opt;compression:int:opt;overwrite:int:opt;alpha:vnode:opt;", "clip:vnode;", writeCreate, nullptr, plugin);
    fpng::fpng_init();
}