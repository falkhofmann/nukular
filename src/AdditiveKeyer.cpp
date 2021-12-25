static const char *const HELP = "Additive keyer as plugin.";

#include "DDImage/PixelIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/DDMath.h"
#include "DDImage/RGB.h"
#include "DDImage/NukeWrapper.h"

using namespace DD::Image;

class AdditiveKeyer : public PixelIop
{

    double _ref_color[3];
    // bool _enable_ref_color;
    int _saturation_mode;
    double _saturation;
    double _highlights;
    double _inverse_highlights;
    bool _compensate;

public:
    void in_channels(int input, ChannelSet &mask) const override;
    AdditiveKeyer(Node *node) : PixelIop(node),
                                // _enable_ref_color(false),
                                _saturation_mode(0),
                                _saturation(0.0f),
                                _highlights(0.0f),
                                _inverse_highlights(0.0f),
                                _compensate(true)

    {
        inputs(2);
        _ref_color[0] = _ref_color[1] = _ref_color[2] = 0.0f;
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

enum
{
    REC709 = 0,
    CCIR601,
    AVERAGE,
    MAXIMUM
};

static const char *mode_names[] = {
    "Rec 709", "Ccir 601", "Average", "Maximum", nullptr};

static inline float y_convert_ccir601(float r, float g, float b)
{
    return r * 0.299f + g * 0.587f + b * 0.114f;
}

static inline float y_convert_avg(float r, float g, float b)
{
    return (r + g + b) / 3.0f;
}

static inline float y_convert_max(float r, float g, float b)
{
    if (g > r)
        r = g;
    if (b > r)
        r = b;
    return r;
}

static inline float y_convert_min(float r, float g, float b)
{
    if (g < r)
        r = g;
    if (b < r)
        r = b;
    return r;
}

void AdditiveKeyer::_validate(bool for_real)
{
    for (int i = 0; i < 3; i++)
    {
        _ref_color[i] = knob("ref_color")->get_value(i);
    }
    _inverse_highlights = 1.0f / _highlights;
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

            float minusR = (*screenR++) - _ref_color[0];
            float minusG = (*screenG++) - _ref_color[1];
            float minusB = (*screenB++) - _ref_color[2];

            float y;
            switch (_saturation_mode)
            {
            case REC709:
                y = y_convert_rec709(minusR, minusG, minusB);
                break;
            case CCIR601:
                y = y_convert_ccir601(minusR, minusG, minusB);
                break;
            case AVERAGE:
                y = y_convert_avg(minusR, minusG, minusB);
                break;

            case MAXIMUM:
                y = y_convert_max(minusR, minusG, minusB);
                break;
            }

            float desatR = clamp(lerp(y, minusR, _saturation), 0, 1000);
            float desatG = clamp(lerp(y, minusG, _saturation), 0, 1000);
            float desatB = clamp(lerp(y, minusB, _saturation), 0, 1000);

            float valRed = (bgRvalue + (_highlights * (desatR * bgRvalue)));
            float valGreen = (bgGvalue + (_highlights * (desatG * bgGvalue)));
            float valBlue = (bgBvalue + (_highlights * (desatB * bgBvalue)));

            if (_compensate)
            {
                valRed *= _inverse_highlights;
                valGreen *= _inverse_highlights;
                valBlue *= _inverse_highlights;
            }

            *rOut++ = valRed;
            *gOut++ = valGreen;
            *bOut++ = valBlue;
        }
    }
}

void AdditiveKeyer::knobs(Knob_Callback f)
{
    Divider(f);
    Color_knob(f, _ref_color, "ref_color", "ref color");
    // Bool_knob(f, &_enable_ref_color, "enable_ref_color", "enable user color");
    SetFlags(f, Knob::STARTLINE);
    Divider(f);
    Enumeration_knob(f, &_saturation_mode, mode_names, "mode", "luminance math");
    Tooltip(f, "Choose a mode to apply the greyscale conversion.");
    Double_knob(f, &_saturation, IRange(0.0, 1), "saturation", "saturation");
    Double_knob(f, &_highlights, IRange(0.0, 10), "highlights", "highlights");
    Bool_knob(f, &_compensate, "compensate", "compensate highlight");
    SetFlags(f, Knob::STARTLINE);
    Divider(f);
}

static Iop *build(Node *node) { return new NukeWrapper(new AdditiveKeyer(node)); }
const Iop::Description AdditiveKeyer::d("AdditiveKeyer", 0, build);
