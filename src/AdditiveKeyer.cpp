static const char *const HELP = "Additive keyer as plugin.\n"
                                "This is combination of various AdditiveKeyer gizmos.\n\n"
                                "Author: 12/2021,  Falk Hofmann\n"
                                "Version: 1.0.0";

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

public:
    void in_channels(int input, ChannelSet &mask) const override;
    AdditiveKeyer(Node *node) : PixelIop(node),
                                _saturation_mode(0),
                                _saturation(0.0f),
                                _highlights(0.0f),
                                _negative(0.0f)

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
    // TODO return early if no second input
    input0().get(y, x, r, channels, out);
    ChannelSet copied = channels;
    copied &= (channels);

    //TODO define interest to allow caching
    Row sceenInput(x, r);
    input1().get(y, x, r, copied, sceenInput);

    foreach (z, channels)
    {

        int idx = colourIndex(z);
        Channel rchan = brother(z, 0);
        Channel gchan = brother(z, 1);
        Channel bchan = brother(z, 2);

        const float *screen_r = sceenInput[rchan];
        const float *screen_g = sceenInput[gchan];
        const float *screen_b = sceenInput[bchan];

        const float *bg_r = in[rchan] + x;
        const float *bg_g = in[gchan] + x;
        const float *bg_b = in[bchan] + x;

        float *rOut = out.writable(rchan) + x;
        float *gOut = out.writable(gchan) + x;
        float *bOut = out.writable(bchan) + x;

        const float *END = screen_r + (r - x);
        while (screen_r < END)
        {
            float red_bg = *bg_r++;
            float green_bg = *bg_g++;
            float blue_bg = *bg_b++;

            float red_screen = *screen_r++;
            float green_screen = *screen_g++;
            float blue_screen = *screen_b++;

            float red_minus = red_screen - _ref_color[0];
            float green_minus = green_screen - _ref_color[1];
            float blue_minus = blue_screen - _ref_color[2];

            float luma;
            switch (_saturation_mode)
            {
            case REC709:
                luma = y_convert_rec709(red_minus, green_minus, blue_minus);
                break;
            case CCIR601:
                luma = y_convert_ccir601(red_minus, green_minus, blue_minus);
                break;
            case AVERAGE:
                luma = y_convert_avg(red_minus, green_minus, blue_minus);
                break;

            case MAXIMUM:
                luma = y_convert_max(red_minus, green_minus, blue_minus);
                break;
            }

            float red_desat = lerp(luma, red_minus, (float)_saturation);
            float green_desat = lerp(luma, green_minus, (float)_saturation);
            float blue_desat = lerp(luma, blue_minus, (float)_saturation);

            Vector3 brights(_highlights * (clamp(red_desat, 0, 1000) * red_bg),
                            _highlights * (clamp(green_desat, 0, 1000) * green_bg),
                            _highlights * (clamp(blue_desat, 0, 1000) * blue_bg));

            Vector3 darks(0.0, 0.0, 0.0);
            if (_negative > 0.0f)
            {
                darks.x = _negative * (clamp(red_desat, -1000, 0) * red_bg);
                darks.y = _negative * (clamp(green_desat, -1000, 0) * green_bg);
                darks.z = _negative * (clamp(blue_desat, -1000, 0) * blue_bg);
            }

            *rOut++ = red_bg + (brights.x + darks.x);
            *gOut++ = green_bg + (brights.y + darks.y);
            *bOut++ = blue_bg + (brights.z + darks.z);
        }
    }
}

void AdditiveKeyer::knobs(Knob_Callback f)
{
    Divider(f);
    Color_knob(f, _ref_color, "ref_color", "ref color");
    Tooltip(f, "Pick the color of your FG element. This is mostly a green- or bluescreen.");
    SetFlags(f, Knob::STARTLINE);
    Divider(f);
    Enumeration_knob(f, &_saturation_mode, mode_names, "mode", "luminance math");
    Tooltip(f, "Choose a mode to apply the greyscale conversion.\n"
               "Note that maximum won't have any negative values.");
    Double_knob(f, &_saturation, IRange(0.0, 1), "saturation", "saturation");
    Tooltip(f, "Define the amount of how much saturation the FG should keep.");
    Double_knob(f, &_highlights, IRange(0.0, 5), "highlights", "highlights");
    Tooltip(f, "Multiplier for the highlights to add on top.");
    Double_knob(f, &_negative, IRange(0.0, 5), "negative", "negative");
    Tooltip(f, "Multiplier for the negative values to add on top.");
    SetFlags(f, Knob::STARTLINE);
    Divider(f);
}

static Iop *build(Node *node) { return new NukeWrapper(new AdditiveKeyer(node)); }
const Iop::Description AdditiveKeyer::d("AdditiveKeyer", 0, build);
