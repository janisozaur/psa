#pragma once
#include <cstdint>

#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")

enum PAINT_QUADRANT_FLAGS
{
    PAINT_QUADRANT_FLAG_IDENTICAL = (1 << 0),
    PAINT_QUADRANT_FLAG_BIGGER = (1 << 7),
    PAINT_QUADRANT_FLAG_NEXT = (1 << 1),
};

#define MAX_PAINT_QUADRANTS 512
#define TUNNEL_MAX_COUNT 65


#pragma pack(push, 1)
/* size 0x12 */
struct attached_paint_struct
{
    uint32_t image_id; // 0x00
    union
    {
        uint32_t tertiary_colour;
        // If masked image_id is masked_id
        uint32_t colour_image_id;
    };
    uint16_t x;    // 0x08
    uint16_t y;    // 0x0A
    uint8_t flags; // 0x0C
    uint8_t pad_0D;
    attached_paint_struct* next; // 0x0E
};
assert_struct_size(attached_paint_struct, 0x12);

struct paint_string_struct
{
    uint16_t string_id;   // 0x00
    paint_string_struct* next; // 0x02
    uint16_t x;                // 0x06
    uint16_t y;                // 0x08
    uint32_t args[4];          // 0x0A
    uint8_t* y_offsets;        // 0x1A
};
assert_struct_size(paint_string_struct, 0x1e);
#pragma pack(pop)

struct paint_struct_bound_box
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t x_end;
    uint16_t y_end;
    uint16_t z_end;
};

struct TileElementBase
{
    uint8_t type;             // 0
    uint8_t flags;            // 1. Upper nibble: flags. Lower nibble: occupied quadrants (one bit per quadrant).
    uint8_t base_height;      // 2
    uint8_t clearance_height; // 3
};

struct TileElement : public TileElementBase
{
    uint8_t pad_04[4];
    uint8_t pad_08[8];
};

/* size 0x34 */
struct paint_struct
{
    uint32_t image_id; // 0x00
    union
    {
        uint32_t tertiary_colour; // 0x04
        // If masked image_id is masked_id
        uint32_t colour_image_id; // 0x04
    };
    paint_struct_bound_box bounds; // 0x08
    uint16_t x;                    // 0x14
    uint16_t y;                    // 0x16
    uint16_t quadrant_index;
    uint8_t flags;
    uint8_t quadrant_flags;
    attached_paint_struct* attached_ps; // 0x1C
    paint_struct* children;
    paint_struct* next_quadrant_ps; // 0x24
    uint8_t sprite_type;            // 0x28
    uint8_t var_29;
    uint16_t pad_2A;
    uint16_t map_x;           // 0x2C
    uint16_t map_y;           // 0x2E
    TileElement* tileElement; // 0x30 (or sprite pointer)
};
assert_struct_size(paint_struct, 0x34);

struct rct_drawpixelinfo
{
    uint8_t* bits{};
    int16_t x{};
    int16_t y{};
    int16_t width{};
    int16_t height{};
    int16_t pitch{}; // note: this is actually (pitch - width)
    uint16_t zoom_level{};

    void* DrawingEngine{};
};

union paint_entry
{
    paint_struct basic;
    attached_paint_struct attached;
    paint_string_struct string;
};

struct CoordsXY
{
    int32_t x = 0;
    int32_t y = 0;
};

struct support_height
{
    uint16_t height;
    uint8_t slope;
    uint8_t pad;
};

struct tunnel_entry
{
    uint8_t height;
    uint8_t type;
};

struct paint_session
{
    rct_drawpixelinfo DPI;
    paint_entry PaintStructs[4000];
    paint_struct* Quadrants[MAX_PAINT_QUADRANTS];
    paint_struct PaintHead;
    uint32_t ViewFlags;
    uint32_t QuadrantBackIndex;
    uint32_t QuadrantFrontIndex;
    const void* CurrentlyDrawnItem;
    paint_entry* EndOfPaintStructArray;
    paint_entry* NextFreePaintStruct;
    CoordsXY SpritePosition;
    paint_struct* LastRootPS;
    attached_paint_struct* UnkF1AD2C;
    uint8_t InteractionType;
    uint8_t CurrentRotation;
    support_height SupportSegments[9];
    support_height Support;
    paint_string_struct* PSStringHead;
    paint_string_struct* LastPSString;
    paint_struct* WoodenSupportsPrependTo;
    CoordsXY MapPosition;
    tunnel_entry LeftTunnels[TUNNEL_MAX_COUNT];
    uint8_t LeftTunnelCount;
    tunnel_entry RightTunnels[TUNNEL_MAX_COUNT];
    uint8_t RightTunnelCount;
    uint8_t VerticalTunnelHeight;
    const TileElement* SurfaceElement;
    TileElement* PathElementOnSameHeight;
    TileElement* TrackElementOnSameHeight;
    bool DidPassSurface;
    uint8_t Unk141E9DB;
    uint16_t WaterHeight;
    uint32_t TrackColours[4];
};
