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
    int _saturation_mode;
    double _saturation;
    double _highlights;
    double _negative;
    bool _enable_negative;

public:
    void in_channels(int input, ChannelSet &mask) const override;
    AdditiveKeyer(Node *node) : PixelIop(node),
                                _saturation_mode(0),
                                _saturation(0.0f),
                                _highlights(0.0f),
                                _negative(0.0f),
                                _enable_negative(false)

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
            float Rbgvalue = *bgA++;
            float Gbgvalue = *bgG++;
            float Bbgvalue = *bgB++;

            float sR = *screenR++;
            float sG = *screenG++;
            float sB = *screenB++;

            float Rminus = sR - _ref_color[0];
            float Gminus = sG - _ref_color[1];
            float Bminus = sB - _ref_color[2];

            float luma;
            switch (_saturation_mode)
            {
            case REC709:
                luma = y_convert_rec709(Rminus, Gminus, Bminus);
                break;
            case CCIR601:
                luma = y_convert_ccir601(Rminus, Gminus, Bminus);
                break;
            case AVERAGE:
                luma = y_convert_avg(Rminus, Gminus, Bminus);
                break;

            case MAXIMUM:
                luma = y_convert_max(Rminus, Gminus, Bminus);
                break;
            }

            float desatR = lerp(luma, Rminus, (float)_saturation);
            float desatG = lerp(luma, Gminus, (float)_saturation);
            float desatB = lerp(luma, Bminus, (float)_saturation);

            Vector3 brights;
            brights.x = _highlights * (clamp(desatR, 0, 1000) * Rbgvalue);
            brights.y = _highlights * (clamp(desatG, 0, 1000) * Gbgvalue);
            brights.z = _highlights * (clamp(desatB, 0, 1000) * Bbgvalue);

            Vector3 darks(0.0, 0.0, 0.0);
            if (_enable_negative)
            {
                darks.x = _negative * (clamp(desatR, -1000, 0) * Rbgvalue);
                darks.y = _negative * (clamp(desatG, -1000, 0) * Gbgvalue);
                darks.z = _negative * (clamp(desatB, -1000, 0) * Bbgvalue);
            }

            *rOut++ = Rbgvalue + (brights.x + darks.x);
            *gOut++ = Gbgvalue + (brights.y + darks.y);
            *bOut++ = Bbgvalue + (brights.z + darks.z);
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
    Double_knob(f, &_highlights, IRange(0.0, 5), "highlights", "highlights");
    Double_knob(f, &_negative, IRange(0.0, 5), "negative", "negative");
    Bool_knob(f, &_enable_negative, "_enable_negative", "enable _negative");
    SetFlags(f, Knob::STARTLINE);
    Divider(f);
}

static Iop *build(Node *node) { return new NukeWrapper(new AdditiveKeyer(node)); }
const Iop::Description AdditiveKeyer::d("AdditiveKeyer", 0, build);
