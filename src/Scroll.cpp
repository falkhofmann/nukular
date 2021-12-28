#include "DDImage/Iop.h"
#include "DDImage/Row.h"
#include "DDImage/Pixel.h"
#include "DDImage/Knobs.h"

using namespace DD::Image;

static const char *const CLASS = "Scroll";
static const char *const HELP = "Scrolls the input by an integer number of pixels and loops it, for painting away tiling.\n\n"
                                "Author: 12/2021, Falk Hofmann, Julik Tarkhanov\n"
                                "Version: 1.0.0";

class Scroll : public Iop
{
    void _validate(bool);
    virtual void _request(int, int, int, int, ChannelMask, int);
    virtual void engine(int y, int x, int r, ChannelMask, Row &t);
    double x, y;
    bool _invert;
    int dx, dy;

    int _width, _height;

public:
    Scroll(Node *node) : Iop(node),
                         _invert(false)
    {
        x = y = 0;
    }

    signed int offsetCoord(signed int coord, int &total);
    virtual void knobs(Knob_Callback);
    const char *Class() const { return CLASS; }
    const char *node_help() const { return HELP; }
    static const Iop::Description d;
};

void Scroll::_validate(bool)
{
    // Figure out the integer translations. Floor(x+.5) is used so that
    // values always round the same way even if negative and even if .5

    copy_info();
    dx = int(floor(x + .5));
    dy = int(floor(y + .5));
    if (_invert)
    {
        dx *= -1;
        dy *= -1;
    }

    _width = input0().format().width() - 1;
    _height = input0().format().height() - 1;

    Box b(input0().info().x(), input0().info().y(), input0().info().r(), input0().info().t());
    info_.intersect(b);
}

signed int Scroll::offsetCoord(signed int coord, int &total)
{
    if (coord < 0)
    {
        coord = total + (coord % total);
    }
    if (coord > total)
    {
        coord = coord % total;
    }
    return coord;
}

void Scroll::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
    input0().request(channels, count);
}

void Scroll::engine(int y, int x, int r, ChannelMask channels, Row &out)
{
    Pixel pixel(channels);
    signed int theX, theY;

    theY = y - dy;
    theY = offsetCoord(theY, _height);

    for (; x < r; x++)
    {
        theX = x - dx;
        theX = offsetCoord(theX, _width);

        input0().at(theX, theY, pixel);
        foreach (z, channels)
        {
            float *destBuf = out.writable(z);
            *(destBuf + x) = pixel[z];
        }
    }
}

void Scroll::knobs(Knob_Callback f)
{
    XY_knob(f, &x, "scroll");
    Tooltip(f, "The number of pixels by which the canvas has to be offset. This is rounded to the nearest number of pixels, no filtering is done.");
    Bool_knob(f, &_invert, "invert", "invert");
    SetFlags(f, Knob::STARTLINE);
    Tooltip(f, "This does invert the translation.\n"
               "For easy wrapping around a paint node without any filtering.");
    Tab_knob(f, "Info");
    Text_knob(f, "Author", "Falk Hofmann, Julik Tarkhanov");
    Text_knob(f, "Date", "12/2021");
    Text_knob(f, "Version", "1.0.0");
}

static Iop *build(Node *node) { return new Scroll(node); }
const Iop::Description Scroll::d(CLASS, 0, build);
