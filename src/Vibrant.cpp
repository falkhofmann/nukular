/*
 * Vibrant.cpp
 * A Nuke plugin to add saturation based on weighting.
 *
 *  Created on: Oct 23, 2021
 *  Author: Falk Hofmann
 *  Version: 1.0.0
 *
 */

static const char *const HELP = "This node adds colorfulness to less saturated areas.\n\n"
                                "Author: 10/2021, Falk Hofmann\n"
                                "Version: 1.0.0";

#include "DDImage/PixelIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/NukeWrapper.h"
#include "DDImage/DDMath.h"
#include "DDImage/RGB.h"

using namespace DD::Image;

class Vibrant : public PixelIop
{

  double _vibrant;
  int mode;

public:
  Vibrant(Node *node) : PixelIop(node)
  {
    _vibrant = 1.0;
    mode = 0;
  }

  void in_channels(int input_number, ChannelSet &channels) const override
  {
    ChannelSet done;
    foreach (z, channels)
    {
      if (colourIndex(z) < 3)
      {
        if (!(done & z))
        {
          done.addBrothers(z, 3);
        }
      }
    }
    channels += done;
  }

  void knobs(Knob_Callback f) override;
  void _validate(bool for_real) override
  {
    set_out_channels(Mask_All);
    PixelIop::_validate(for_real);
  }

  void pixel_engine(const Row &in, int y, int x, int r, ChannelMask channels, Row &out) override;
  static const Iop::Description d;

  const char *Class() const override { return d.name; }
  const char *node_help() const override { return HELP; }
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

void Vibrant::knobs(Knob_Callback f)
{
  Double_knob(f, &_vibrant, IRange(0, 5), "vibrancy", "Vibrancy");
  Tooltip(f, "Adjust the amount of colorfulness. A value of 1 does not change the image.\n"
             "Values higher than 1 will add more color to less saturated areas, while Vibrancy values lower than "
             " 1 will reduce color of saturated areas");
  Enumeration_knob(f, &mode, mode_names, "mode", "luminance math");
  Tooltip(f, "Choose a mode to apply the greyscale conversion.");
}

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

static inline float calc_mask(float r, float g, float b)
{
  float mn = y_convert_min(r, g, b);
  float mx = y_convert_max(r, g, b);
  float t = clamp(1 - (MAX(mx, 1 - (mx - mn))));
  return t;
}

static inline float calc_value(float y, float cIn, float vib, float m)
{

  return (lerp(y, cIn, vib) * m) + ((1 - m) * cIn);
}

void Vibrant::pixel_engine(const Row &in, int y, int x, int r,
                           ChannelMask channels, Row &out)
{
  ChannelSet done;
  foreach (z, channels)
  {
    if (done & z)
      continue;

    if (colourIndex(z) >= 3 || _vibrant == 1.0)
    {
      out.copy(in, z, x, r);
      continue;
    }

    Channel rchan = brother(z, 0);
    done += rchan;
    Channel gchan = brother(z, 1);
    done += gchan;
    Channel bchan = brother(z, 2);
    done += bchan;

    const float *rIn = in[rchan] + x;
    const float *gIn = in[gchan] + x;
    const float *bIn = in[bchan] + x;

    float *rOut = out.writable(rchan) + x;
    float *gOut = out.writable(gchan) + x;
    float *bOut = out.writable(bchan) + x;

    const float *END = rIn + (r - x);

    float y;
    float fvibrance = float(_vibrant);

    float m;
    switch (mode)
    {
    case REC709:
      while (rIn < END)
      {
        y = y_convert_rec709(*rIn, *gIn, *bIn);
        m = calc_mask(*rIn, *gIn, *bIn);
        *rOut++ = calc_value(y, *rIn++, fvibrance, m);
        *gOut++ = calc_value(y, *gIn++, fvibrance, m);
        *bOut++ = calc_value(y, *bIn++, fvibrance, m);
      }
      break;
    case CCIR601:
      while (rIn < END)
      {
        y = y_convert_ccir601(*rIn, *gIn, *bIn);
        m = calc_mask(*rIn, *gIn, *bIn);
        *rOut++ = calc_value(y, *rIn++, fvibrance, m);
        *gOut++ = calc_value(y, *gIn++, fvibrance, m);
        *bOut++ = calc_value(y, *bIn++, fvibrance, m);
      }
      break;
    case AVERAGE:
      while (rIn < END)
      {
        y = y_convert_avg(*rIn, *gIn, *bIn);
        m = calc_mask(*rIn, *gIn, *bIn);
        m = calc_mask(*rIn, *gIn, *bIn);
        *rOut++ = calc_value(y, *rIn++, fvibrance, m);
        *gOut++ = calc_value(y, *gIn++, fvibrance, m);
        *bOut++ = calc_value(y, *bIn++, fvibrance, m);
      }
      break;
    case MAXIMUM:
      while (rIn < END)
      {
        y = y_convert_max(*rIn, *gIn, *bIn);
        m = calc_mask(*rIn, *gIn, *bIn);
        *rOut++ = calc_value(y, *rIn++, fvibrance, m);
        *gOut++ = calc_value(y, *gIn++, fvibrance, m);
        *bOut++ = calc_value(y, *bIn++, fvibrance, m);
      }
      break;
    }
  }
}

static Iop *build(Node *node) { return (new NukeWrapper(new Vibrant(node)))->channels(Mask_RGB); }
const Iop::Description Vibrant::d("Vibrant", 0, build);