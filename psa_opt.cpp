#include "structs.h"
#include "psa_openrct2.h"

template<uint8_t>
static bool check_bounding_box(const paint_struct_bound_box& initialBBox, const paint_struct_bound_box& currentBBox)
{
    return false;
}

template<> bool check_bounding_box<0>(const paint_struct_bound_box& initialBBox, const paint_struct_bound_box& currentBBox)
{
    const static uint8_t directions[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 };
    bool c1 = initialBBox.z_end >= currentBBox.z;
    bool c2 = initialBBox.y_end >= currentBBox.y;
    bool c3 = initialBBox.x_end >= currentBBox.x;
    bool c4 = initialBBox.z < currentBBox.z_end;
    bool c5 = initialBBox.y < currentBBox.y_end;
    bool c6 = initialBBox.x < currentBBox.x_end;
    uint8_t c_all = (c1 << 5) | (c2 << 4) | (c3 << 3) | (c4 << 2) | (c5 << 1) | (c6 << 0);

    return directions[c_all];
}

template<> bool check_bounding_box<1>(const paint_struct_bound_box& initialBBox, const paint_struct_bound_box& currentBBox)
{
    const static uint8_t directions[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool c1 = initialBBox.z_end >= currentBBox.z;
    bool c2 = initialBBox.y_end >= currentBBox.y;
    bool c3 = initialBBox.x_end >= currentBBox.x;
    bool c4 = initialBBox.z < currentBBox.z_end;
    bool c5 = initialBBox.y < currentBBox.y_end;
    bool c6 = initialBBox.x < currentBBox.x_end;
    uint8_t c_all = (c1 << 5) | (c2 << 4) | (c3 << 3) | (c4 << 2) | (c5 << 1) | (c6 << 0);
    return directions[c_all];
}

template<> bool check_bounding_box<2>(const paint_struct_bound_box& initialBBox, const paint_struct_bound_box& currentBBox)
{
    const static uint8_t directions[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool c1 = initialBBox.z_end >= currentBBox.z;
    bool c2 = initialBBox.y_end >= currentBBox.y;
    bool c3 = initialBBox.x_end >= currentBBox.x;
    bool c4 = initialBBox.z < currentBBox.z_end;
    bool c5 = initialBBox.y < currentBBox.y_end;
    bool c6 = initialBBox.x < currentBBox.x_end;
    uint8_t c_all = (c1 << 5) | (c2 << 4) | (c3 << 3) | (c4 << 2) | (c5 << 1) | (c6 << 0);
    return directions[c_all];
}

template<> bool check_bounding_box<3>(const paint_struct_bound_box& initialBBox, const paint_struct_bound_box& currentBBox)
{
    const static uint8_t directions[64] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
                                            1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    bool c1 = initialBBox.z_end >= currentBBox.z;
    bool c2 = initialBBox.y_end >= currentBBox.y;
    bool c3 = initialBBox.x_end >= currentBBox.x;
    bool c4 = initialBBox.z < currentBBox.z_end;
    bool c5 = initialBBox.y < currentBBox.y_end;
    bool c6 = initialBBox.x < currentBBox.x_end;
    uint8_t c_all = (c1 << 5) | (c2 << 4) | (c3 << 3) | (c4 << 2) | (c5 << 1) | (c6 << 0);
    return directions[c_all];
}

template<uint8_t _TRotation>
static paint_struct* paint_arrange_structs_helper_rotation(paint_struct* ps_next, uint16_t quadrantIndex, uint8_t flag)
{
    paint_struct* ps;
    paint_struct* ps_temp;
    do
    {
        ps = ps_next;
        ps_next = ps_next->next_quadrant_ps;
        if (ps_next == nullptr)
            return ps;
    } while (quadrantIndex > ps_next->quadrant_index);

    // Cache the last visited node so we don't have to walk the whole list again
    paint_struct* ps_cache = ps;

    ps_temp = ps;
    do
    {
        ps = ps->next_quadrant_ps;
        if (ps == nullptr)
            break;

        if (ps->quadrant_index > quadrantIndex + 1)
        {
            ps->quadrant_flags = PAINT_QUADRANT_FLAG_BIGGER;
        }
        else if (ps->quadrant_index == quadrantIndex + 1)
        {
            ps->quadrant_flags = PAINT_QUADRANT_FLAG_NEXT | PAINT_QUADRANT_FLAG_IDENTICAL;
        }
        else if (ps->quadrant_index == quadrantIndex)
        {
            ps->quadrant_flags = flag | PAINT_QUADRANT_FLAG_IDENTICAL;
        }
    } while (ps->quadrant_index <= quadrantIndex + 1);
    ps = ps_temp;

    while (true)
    {
        while (true)
        {
            ps_next = ps->next_quadrant_ps;
            if (ps_next == nullptr)
                return ps_cache;
            if (ps_next->quadrant_flags & PAINT_QUADRANT_FLAG_BIGGER)
                return ps_cache;
            if (ps_next->quadrant_flags & PAINT_QUADRANT_FLAG_IDENTICAL)
                break;
            ps = ps_next;
        }

        ps_next->quadrant_flags &= ~PAINT_QUADRANT_FLAG_IDENTICAL;
        ps_temp = ps;

        const paint_struct_bound_box& initialBBox = ps_next->bounds;

        while (true)
        {
            ps = ps_next;
            ps_next = ps_next->next_quadrant_ps;
            if (ps_next == nullptr)
                break;
            if (ps_next->quadrant_flags & PAINT_QUADRANT_FLAG_BIGGER)
                break;
            if (!(ps_next->quadrant_flags & PAINT_QUADRANT_FLAG_NEXT))
                continue;

            const paint_struct_bound_box& currentBBox = ps_next->bounds;

            const bool compareResult = check_bounding_box<_TRotation>(initialBBox, currentBBox);

            if (compareResult)
            {
                ps->next_quadrant_ps = ps_next->next_quadrant_ps;
                paint_struct* ps_temp2 = ps_temp->next_quadrant_ps;
                ps_temp->next_quadrant_ps = ps_next;
                ps_next->next_quadrant_ps = ps_temp2;
                ps_next = ps;
            }
        }

        ps = ps_temp;
    }
}

static paint_struct* paint_arrange_structs_helper(paint_struct* ps_next, uint16_t quadrantIndex, uint8_t flag, uint8_t rotation)
{
    switch (rotation)
    {
        case 0:
            return paint_arrange_structs_helper_rotation<0>(ps_next, quadrantIndex, flag);
        case 1:
            return paint_arrange_structs_helper_rotation<1>(ps_next, quadrantIndex, flag);
        case 2:
            return paint_arrange_structs_helper_rotation<2>(ps_next, quadrantIndex, flag);
        case 3:
            return paint_arrange_structs_helper_rotation<3>(ps_next, quadrantIndex, flag);
    }
    return nullptr;
}

/**
 *
 *  rct2: 0x00688217
 */
void paint_session_arrange_opt(paint_session* session)
{
    paint_struct* psHead = &session->PaintHead;

    paint_struct* ps = psHead;
    ps->next_quadrant_ps = nullptr;

    uint32_t quadrantIndex = session->QuadrantBackIndex;
    if (quadrantIndex != UINT32_MAX)
    {
        do
        {
            paint_struct* ps_next = session->Quadrants[quadrantIndex];
            if (ps_next != nullptr)
            {
                ps->next_quadrant_ps = ps_next;
                do
                {
                    ps = ps_next;
                    ps_next = ps_next->next_quadrant_ps;

                } while (ps_next != nullptr);
            }
        } while (++quadrantIndex <= session->QuadrantFrontIndex);

        paint_struct* ps_cache = paint_arrange_structs_helper(
            psHead, session->QuadrantBackIndex & 0xFFFF, PAINT_QUADRANT_FLAG_NEXT, session->CurrentRotation);

        quadrantIndex = session->QuadrantBackIndex;
        while (++quadrantIndex < session->QuadrantFrontIndex)
        {
            ps_cache = paint_arrange_structs_helper(ps_cache, quadrantIndex & 0xFFFF, 0, session->CurrentRotation);
        }
    }
}
