
static const char *const CLASS = "CircularRays";
static const char *const HELP = "Radial distributed rays.";

#include "DDImage/Iop.h"
#include "DDImage/Format.h"
#include "DDImage/Knobs.h"
#include "DDImage/Row.h"
#include "DDImage/DDMath.h"
#include <math.h>

using namespace DD::Image;
using namespace std;

class CircularRays : public Iop
{
    Vector2 _center;
    double _amount;
    double _rotate;
    float _color[4];

    Channel channel[4];
    FormatPair formats;

    double _radians;

public:
    const char *Class() const { return CLASS; }
    const char *node_help() const { return HELP; }
    static const Description desc;

    CircularRays(Node *node) : Iop(node)
    {
        inputs(0);
        const Format &format = input_format();
        _center.x = format.width() / 2;
        _center.y = format.height() / 2;

        _color[0] = _color[1] = _color[2] = _color[3] = 1.0f;

        channel[0] = Chan_Red;
        channel[1] = Chan_Green;
        channel[2] = Chan_Blue;
        channel[3] = Chan_Alpha;
        formats.format(0);

        _amount = 10;
        _rotate = 0;
    };

    void _validate(bool for_real)
    {
        bool non_zero = false;
        info_.black_outside(false);
        ChannelSet tchan;
        for (int i = 0; i < 4; i++)
        {
            info_.turn_on(channel[i]);
        }

        info_.full_size_format(*formats.fullSizeFormat());
        info_.format(*formats.format());
        info_.set(format());

        _radians = _rotate * M_PI / 180;
    }

    void engine(int y, int xx, int r, ChannelMask channels, Row &row)
    {
        for (int x = xx; x < r; x++)
        {
            float v_pos = (float)cos(_radians) *
                              (y - _center.y) +
                          sin(_radians) * (x - _center.x) + _center.y;
            float h_pos = (float)-sin(_radians) *
                              (y - _center.y) +
                          cos(_radians) * (x - _center.x) + _center.x;

            for (int z = 0; z < 4; z++)
            {
                float *out = row.writable(channel[z]);
                float a = (sin(atan2(h_pos - _center.x, v_pos - _center.y) * _amount)) * _color[z];
                out[x] = a;
            }
        }
    };

    void knobs(Knob_Callback f)
    {
        Text_knob(f, "<b>Transformation</b>");
        Format_knob(f, &formats, "Format");
        Tooltip(f, "Set the format you are want to create.");
        XY_knob(f, &_center[0], "center", "Center");
        Tooltip(f, "Center to draw the rays.");
        Double_knob(f, &_amount, IRange(1, 500), "amount", "amount");
        Tooltip(f, "Amount of rays to be created.");
        Double_knob(f, &_rotate, IRange(0, 360), "Rotate");
        Tooltip(f, "Degress the rays should be rotated.");
        Text_knob(f, "<b>Colors</b>");
        SetFlags(f, Knob::STARTLINE);
        AColor_knob(f, _color, "color", "color");
        Tooltip(f, "Color of rays.");

        Tab_knob(f, "Info");
        Text_knob(f, "Author", "Falk Hofmann");
        Text_knob(f, "Date", "Dez 2021");
        Text_knob(f, "Version", "1.0.0");
    }
};

static Iop *constructor(Node *node) { return new CircularRays(node); }
const Iop::Description CircularRays::desc(CLASS, "Draw/CircularRays", constructor);