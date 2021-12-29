// Copyright (c) 2009 The Foundry Visionmongers Ltd.  All Rights Reserved.

static const char *const CLASS = "Toon";

static const char *const HELP = "Toon shader";

#include "DDImage/Iop.h"
#include "DDImage/NukeWrapper.h"

#include "DDImage/Row.h"
#include "DDImage/Tile.h"
#include "DDImage/Knobs.h"
#include "DDImage/Blur.h"

using namespace DD::Image;
using namespace std;

enum
{
    BOX = 0,
    TRIANGLE,
    QUADRATIC,
    GAUSSIAN
};
static const char *mode_names[] = {
    "Box", "Triangle", "Quadratic", "Gaussian", nullptr};

class Toon : public Iop
{
    double _blur_size[2];
    int _blur_mode;

public:
    int maximum_inputs() const { return 1; }
    int minimum_inputs() const { return 1; }

    Toon(Node *node) : Iop(node)
    {
        _blur_size[0] = _blur_size[1] = 10.0f, 10.0f;
        _blur_mode = 3;
    }
    ~Toon() {}

    void _validate(bool);
    void _request(int x, int y, int r, int t, ChannelMask channels, int count);

    void engine(int y, int x, int r, ChannelMask channels, Row &out);

    const char *Class() const { return CLASS; }
    const char *node_help() const { return HELP; }
    void knobs(Knob_Callback f) override;

    //! Information to the plug-in manager of DDNewImage/Nuke.

    static const Iop::Description description;
};

void Toon::_validate(bool for_real)
{
    copy_info(); // copy bbox channels etc from input0, which will validate it.
    info_.pad(-_blur_size[0], -_blur_size[1], _blur_size[0], _blur_size[1]);

    Iop *blur = dynamic_cast<DD::Image::Iop *>(Iop::create("Blur"));
    // Connect it to our op-s first input
    blur->set_input(0, input(0));

    // set_field is a special method that is guaranteed to work even if knobs are not created for op
    // Use this for setting the blur parameters
    blur->set_field("size", _blur_size, sizeof(double) * 2);
    blur->set_field("filter", &_blur_mode, sizeof(int));

    // Hook our op to blur so that it pulls data from it instead of original input0
    this->set_input(0, blur);

    // Validate input of our knob, which is now the new blur op
    input(0)->validate(for_real);
}

void Toon::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
    input(0)->request(x - _blur_size[0], y - _blur_size[1], r + _blur_size[0], t + _blur_size[1], channels, count);
}

void Toon::engine(int y, int x, int r,
                  ChannelMask channels, Row &row)
{
    if (aborted())
    {
        std::cerr << "Aborted!";
        return;
    }

    row.get(input0(), y, x, r, channels);

    foreach (z, channels)
    {
        const float *input0 = row[z] + x;
        float *outptr = row.writable(z) + x;
        const float *end = outptr + (r - x);

        while (outptr < end)
        {
            *outptr++ = *input0++;
        }
    }
}

void Toon::knobs(Knob_Callback f)
{
    WH_knob(f, _blur_size, "soften", "soften");
    Enumeration_knob(f, &_blur_mode, mode_names, "filter", "filter");
}

static Iop *ToonCreate(Node *node) { return new Toon(node); }
const Iop::Description Toon::description(CLASS, 0, ToonCreate);
