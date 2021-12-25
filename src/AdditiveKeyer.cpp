static const char *const HELP = "Additive keyer as plugin.";

#include "DDImage/PixelIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/NukeWrapper.h"

using namespace DD::Image;

class AdditiveKeyer : public PixelIop
{

public:
    void in_channels(int input, ChannelSet &mask) const override;
    AdditiveKeyer(Node *node) : PixelIop(node)
    {
    }
    bool pass_transform() const override { return true; }
    void pixel_engine(const Row &in, int y, int x, int r, ChannelMask, Row &out) override;
    void knobs(Knob_Callback) override;
    static const Iop::Description d;
    const char *Class() const override { return d.name; }
    const char *node_help() const override { return HELP; }
    void _validate(bool) override;
};

void AdditiveKeyer::_validate(bool for_real)
{
    copy_info();
    set_out_channels(Mask_All);
}

void AdditiveKeyer::in_channels(int input, ChannelSet &mask) const
{
}

void AdditiveKeyer::pixel_engine(const Row &in, int y, int x, int r,
                                 ChannelMask channels, Row &out)
{
    foreach (z, channels)
    {
        const float *inptr = in[z] + x;
        const float *END = inptr + (r - x);
        float *outptr = out.writable(z) + x;
        while (inptr < END)
            *outptr++ = *inptr++;
    }
}

void AdditiveKeyer::knobs(Knob_Callback f)
{
}

static Iop *build(Node *node) { return new NukeWrapper(new AdditiveKeyer(node)); }
const Iop::Description AdditiveKeyer::d("AdditiveKeyer", 0, build);
