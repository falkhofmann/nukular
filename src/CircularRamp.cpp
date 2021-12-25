
static const char* const CLASS="CircularRamp";
static const char* const HELP="A radial gradient";

#include "DDImage/Iop.h"
#include "DDImage/Format.h"
#include "DDImage/Knobs.h"
#include "DDImage/Row.h"
#include "DDImage/DDMath.h"
#include <math.h>

using namespace DD::Image;
using namespace std;


class CircularRamp : public Iop
{
    float center[2];
    double rotate;
    float start_c[4];
    float end_c[4];

    Channel channel[4];
    FormatPair formats;

    double _radians;

public:
    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Description desc;

    CircularRamp(Node* node) : Iop(node)
    {
        inputs(0);
        const Format& format = input_format();
        center[0] = format.width()/2;
        center[1] = format.height()/2;

        start_c[0] = start_c[1] = start_c[2] = start_c[3] = 0.0f;
        end_c[0] = end_c[1] = end_c[2] = end_c[3] = 1.0f;

        channel[0] = Chan_Red;
        channel[1] = Chan_Green;
        channel[2] = Chan_Blue;
        channel[3] = Chan_Alpha;
        formats.format(0);

        rotate = 0;
    };

    void _validate(bool for_real)
    {
        bool non_zero = false;
        info_.black_outside(false);
        ChannelSet tchan;
        for (int i=0; i<4; i++)
        {
            info_.turn_on(channel[i]);
        }

        info_.full_size_format(*formats.fullSizeFormat());
        info_.format(*formats.format());
        info_.set(format());

        _radians = rotate * M_PI/180;
    }

    void engine(int y, int xx, int r, ChannelMask channels, Row& row)
    {
        for (int x=xx; x<r; x++)
        {
            float v_pos = (float) cos(_radians) *
                            (y - center[1]) + sin(_radians) * (x - center[0])
                            + center[1];
            float h_pos = (float) -sin(_radians) *
                            (y - center[1]) + cos(_radians) * (x - center[0])
                            + center[0];

            for (int z=0; z<4; z++){
                float* out = row.writable(channel[z]);
                float a = (1-(0.5+(atan2(h_pos - center[0], v_pos - center[1])/(M_PI*2)))) * start_c[z];
                float b = (0.5+(atan2(h_pos - center[0], v_pos - center[1])/(M_PI*2))) * end_c[z];
                out[x] = a+b;
            }
        }
    };


    void knobs(Knob_Callback f)
    {
        Text_knob(f, "<b>Transformation</b>");
        Format_knob(f, &formats, "Format");
        Tooltip(f, "Set the format you are want to create.");
        XY_knob(f, center, "center", "Center");
        Tooltip(f, "Center to draw the ramp.");
        Double_knob(f, &rotate, IRange(0, 360), "Rotate");
        Tooltip(f, "Degress the ramp should be rotated.");
        Divider(f, "");
        Text_knob(f, "<b>Colors</b>");
        SetFlags(f, Knob::STARTLINE);
        AColor_knob(f, start_c, "start_color", "Start");
        Tooltip(f, "Color at the gradients start.");
        AColor_knob(f, end_c, "end_color", "End");
        Tooltip(f, "Color at the gradients end.");
        Tab_knob(f, "Info");
        Text_knob(f, "Author", "Falk Hofmann");
        Text_knob(f, "Date", "31 Oct 2021");
        Text_knob(f, "Version", "1.0.0");
    }

};

static Iop* constructor(Node* node) { return new CircularRamp(node); }
const Iop::Description CircularRamp::desc(CLASS, "Draw/CircularRamp", constructor);