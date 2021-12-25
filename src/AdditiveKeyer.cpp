static const char *const HELP = "Additive keyer as plugin.";

#include "DDImage/PixelIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/DDMath.h"
#include "DDImage/NukeWrapper.h"

using namespace DD::Image;

class AdditiveKeyer : public PixelIop
{

    double _user_color[3];
    bool _enable_user_color;
    double _saturation;
    double _highlights;
    double _inverse_highlights;

public:
    void in_channels(int input, ChannelSet &mask) const override;
    AdditiveKeyer(Node *node) : PixelIop(node),
                                _enable_user_color(false),
                                _saturation(0.0f),
                                _highlights(0.0f),
                                _inverse_highlights(0.0f)

    {
        inputs(2);
        _user_color[0] = _user_color[1] = _user_color[2] = 0.0f;
    }

    const char *input_label(int n, char *) const override
    {
        switch (n)
        {
        case 0:
            return "Background";
        case 1:
            return "screen";
        default:
            return "mask";
        }
    }

    bool pass_transform() const override { return true; }
    void pixel_engine(const Row &in, int y, int x, int r, ChannelMask, Row &out) override;
    void knobs(Knob_Callback) override;
    static const Iop::Description d;
    const char *Class() const override { return d.name; }
    const char *node_help() const override { return HELP; }
    void _validate(bool) override;
};

static inline float y_convert_max(float r, float g, float b)
{
    if (g > r)
        r = g;
    if (b > r)
        r = b;
    return r;
}

void AdditiveKeyer::_validate(bool for_real)
{
    for (int i = 0; i < 3; i++)
    {
        _user_color[i] = knob("user_color")->get_value(i);
    }
    _inverse_highlights = 1 - (_highlights / 10.0f);
    copy_info();
    set_out_channels(Mask_All);
}

void AdditiveKeyer::in_channels(int input, ChannelSet &mask) const
{
}

void AdditiveKeyer::pixel_engine(const Row &in, int y, int x, int r,
                                 ChannelMask channels, Row &out)
{
    input0().get(y, x, r, channels, out);
    ChannelSet copied = channels;
    copied &= (channels);

    Row sceenInput(x, r);
    input1().get(y, x, r, copied, sceenInput);

    foreach (z, channels)
    {

        int idx = colourIndex(z);
        Channel rchan = brother(z, 0);
        Channel gchan = brother(z, 1);
        Channel bchan = brother(z, 2);

        const float *screenR = sceenInput[rchan];
        const float *screenG = sceenInput[gchan];
        const float *screenB = sceenInput[bchan];

        const float *bgA = in[rchan] + x;
        const float *bgG = in[gchan] + x;
        const float *bgB = in[bchan] + x;

        float *rOut = out.writable(rchan) + x;
        float *gOut = out.writable(gchan) + x;
        float *bOut = out.writable(bchan) + x;

        const float *END = screenR + (r - x);
        while (screenR < END)
        {
            float bgRvalue = *bgA++;
            float bgGvalue = *bgG++;
            float bgBvalue = *bgB++;

            float sR = *screenR++;
            float sG = *screenG++;
            float sB = *screenB++;

            float desatR = sR - _user_color[0];
            float desatG = sG - _user_color[1];
            float desatB = sB - _user_color[2];

            float y = y_convert_max(desatR, desatG, desatB);

            // float dR = clamp(lerp(y, desatR, _saturation), 0, 1000);
            // float dG = clamp(lerp(y, desatG, _saturation), 0, 1000);
            // float dB = clamp(lerp(y, desatB, _saturation), 0, 1000);

            *rOut++ = _inverse_highlights * (bgRvalue + _highlights * (y * bgRvalue));
            *gOut++ = _inverse_highlights * (bgGvalue + _highlights * (y * bgGvalue));
            *bOut++ = _inverse_highlights * (bgBvalue + _highlights * (y * bgBvalue));
        }
    }
}

void AdditiveKeyer::knobs(Knob_Callback f)
{
    Divider(f);
    Color_knob(f, _user_color, "user_color", "User Color");
    Bool_knob(f, &_enable_user_color, "Enable_user_color", "Enable User Color");
    SetFlags(f, Knob::STARTLINE);
    Divider(f);
    Double_knob(f, &_saturation, "saturation", "Saturation");
    Double_knob(f, &_highlights, "highlights", "highlights");
    Divider(f);
}

static Iop *build(Node *node) { return new NukeWrapper(new AdditiveKeyer(node)); }
const Iop::Description AdditiveKeyer::d("AdditiveKeyer", 0, build);
