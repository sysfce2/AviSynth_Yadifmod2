#include "yadifmod2.h"
#include "myvshelper.h"


static const VSFrameRef* VS_CC
get_frame(int n, int activation_reason, void** instance_data, void**,
          VSFrameContext* frame_ctx, VSCore* core, const VSAPI* api)
{
    auto d = reinterpret_cast<YadifMod2*>(*instance_data);

    if (activation_reason == arInitial) {
        d->requestFrames(n, api, frame_ctx);
        return nullptr;
    }
    if (activation_reason != arAllFramesReady) {
        return nullptr;
    }
    return d->getFrame(n, core, api, frame_ctx);
}


static void VS_CC
free_filter(void* instance_data, VSCore* core, const VSAPI* api)
{
    auto d = reinterpret_cast<YadifMod2*>(instance_data);
    d->freeClips(api);
    delete d;
}


static void VS_CC
init_filter(VSMap* in, VSMap* out, void** instance_data, VSNode* node,
            VSCore* core, const VSAPI* api)
{
    auto d = reinterpret_cast<YadifMod2*>(*instance_data);
    api->setVideoInfo(d->getVideoInfo(), 1, node);
}


static void VS_CC
create_filter(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* api)
{
    VSNodeRef* clip = get_arg<VSNodeRef*>("clip", 0, in, api);
    VSNodeRef* edeint = get_arg<VSNodeRef*>("edeint", 0, in, api);

    try {
        int mode = get_arg("mode", 0, 0, in, api);
        validate(mode < 0 || mode > 3, "mode must be set to 0, 1, 2 or 3.");

        const VSVideoInfo& vi = *api->getVideoInfo(clip);
        validate(!is_constant_format(vi), "clip is not constant format");
        validate(is_half_precision(vi), "half precision is not supported.");

        if (edeint) {
            const VSVideoInfo& vied = *api->getVideoInfo(edeint);
            validate(!is_same_format(vi, vied),
                     "edeint clip's format does not mach.");
            validate(vied.numFrames != vi.numFrames * ((mode & 1) ? 2 : 1),
                     "edeint clip's number of frames doesn't match.");
        }

        int order = get_arg("order", 1, 0, in, api);
        validate(order < 0 || order > 1,
                 "order must be set to 0(BottomFieldFirst) or 1(TopFieldFirst).");

        int field = get_arg("field", -1, 0, in, api);
        validate(field < -1 || field > 1, "field must be set to -1, 0 or 1.");

        arch_t arch = get_arch(get_arg("opt", -1, 0, in, api));

        auto d = new YadifMod2(clip, edeint, order, field, mode, arch, core, api);

        api->createFilter(in, out, "yadifmod2", init_filter, get_frame,
                          free_filter, fmParallel, 0, d, core);


    } catch (std::runtime_error& e) {
        if (edeint) {
            api->freeNode(edeint);
        }
        api->freeNode(clip);
        auto msg = std::string("yadifmod2: ") + e.what();
        api->setError(out, msg.c_str());
    }
}


VS_EXTERNAL_API(void)
VapourSynthPluginInit(VSConfigPlugin conf, VSRegisterFunction reg, VSPlugin* p)
{
    conf("chikuzen.dest.not.have.his.own.domain.ym2", "ym2",
         "Yet Another Deinterlacing Filter mod2 for VapourSynth",
         VAPOURSYNTH_API_VERSION, 1, p);
    reg("yadifmod2",
        "clip:clip;order:int:opt;field:int:opt;mode:int:opt;edeint:clip:opt;"
        "opt:int:opt",
        create_filter, nullptr, p);
}