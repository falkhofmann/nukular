static const char *const HELP = "Lightweight contrast node to match by default th color correction contrast behavior,"
                                "within the addition to adjust the pivot itself.\n\n"
                                "Author: 12/2021, Falk Hofmann\n"
                                "Version: 1.0.0";

#include "DDImage/PixelIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/NukeWrapper.h"
#include "DDImage/DDMath.h"
#include "DDImage/RGB.h"

using namespace DD::Image;

class Kontrast : public PixelIop
{

    float _value[4];
    double _pivot;

public:
    Kontrast(Node *node) : PixelIop(node)
    {
        _value[0] = _value[1] = _value[2] = _value[3] = 1.0;
        _pivot = 0.18f;
    }

    void in_channels(int input_number, ChannelSet &channels) const override{};

    void knobs(Knob_Callback f) override;
    void _validate(bool for_real) override
    {
        copy_info();
        for (unsigned i = 0; i < 4; i++)
        {
            if (_value[i])
            {
                set_out_channels(Mask_All);
                info_.black_outside(false);
                return;
            }
        }
        set_out_channels(Mask_None);
    }

    void pixel_engine(const Row &in, int y, int x, int r, ChannelMask channels, Row &out) override;
    static const Iop::Description d;

    const char *Class() const override { return d.name; }
    const char *node_help() const override { return HELP; }
};

void Kontrast::knobs(Knob_Callback f)
{
    AColor_knob(f, _value, IRange(0, 5), "value", "value");
    Tooltip(f, "Contrast value to \nin- or decreae contrast of the image.");
    Double_knob(f, &_pivot, IRange(0, 1), "pivot", "pivot");
    Tooltip(f, "The pivot for the contrast enhancement. 0.18 is default and matches the ColorCorrection behavior.");
}

void Kontrast::pixel_engine(const Row &in, int y, int x, int r,
                            ChannelMask channels, Row &out)
{
    foreach (z, channels)
    {
        const float c1 = _value[colourIndex(z)];
        const float *inptr = in[z] + x;
        const float *END = inptr + (r - x);
        float *outptr = out.writable(z) + x;
        while (inptr < END)
        {
            *outptr++ = pow(*inptr++ / _pivot, c1) * _pivot;
        }
    }
}

static Iop *build(Node *node) { return (new NukeWrapper(new Kontrast(node)))->channels(Mask_RGB); }
const Iop::Description Kontrast::d("Kontrast", 0, build);