// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include "common/common_funcs.h"
#include "common/vector_math.h"
#include "video_core/pica_types.h"

namespace Pica {

struct RasterizerRegs;
using AttributeBuffer = std::array<Common::Vec4<f24>, 16>;

class alignas(16) OutputVertex {
public:
    OutputVertex() = default;
    explicit OutputVertex(const RasterizerRegs& regs, const AttributeBuffer& output);

    // Store raw f24 values in arrays to control exact memory layout
    alignas(16) std::array<f24, 4> pos_raw;   // 16 bytes
    alignas(16) std::array<f24, 4> quat_raw;  // 16 bytes
    alignas(16) std::array<f24, 4> color_raw; // 16 bytes
    alignas(8) std::array<f24, 2> tc0_raw;    // 8 bytes
    alignas(8) std::array<f24, 2> tc1_raw;    // 8 bytes
    alignas(4) f24 tc0_w;                     // 4 bytes
    u32 pad1;                                 // 4 bytes
    alignas(8) std::array<f24, 3> view_raw;   // 12 bytes
    u32 pad2;                                 // 4 bytes
    alignas(8) std::array<f24, 2> tc2_raw;    // 8 bytes

    // Accessors that create vector types on demand
    Common::Vec4<f24> pos() const {
        return Common::Vec4<f24>{pos_raw[0], pos_raw[1], pos_raw[2], pos_raw[3]};
    }
    Common::Vec4<f24> quat() const {
        return Common::Vec4<f24>{quat_raw[0], quat_raw[1], quat_raw[2], quat_raw[3]};
    }
    Common::Vec4<f24> color() const {
        return Common::Vec4<f24>{color_raw[0], color_raw[1], color_raw[2], color_raw[3]};
    }
    Common::Vec2<f24> tc0() const {
        return Common::Vec2<f24>{tc0_raw[0], tc0_raw[1]};
    }
    Common::Vec2<f24> tc1() const {
        return Common::Vec2<f24>{tc1_raw[0], tc1_raw[1]};
    }
    Common::Vec3<f24> view() const {
        return Common::Vec3<f24>{view_raw[0], view_raw[1], view_raw[2]};
    }
    Common::Vec2<f24> tc2() const {
        return Common::Vec2<f24>{tc2_raw[0], tc2_raw[1]};
    }

    // Mutable accessors
    void set_pos(const Common::Vec4<f24>& v) {
        pos_raw = {v.x, v.y, v.z, v.w};
    }
    void set_quat(const Common::Vec4<f24>& v) {
        quat_raw = {v.x, v.y, v.z, v.w};
    }
    void set_color(const Common::Vec4<f24>& v) {
        color_raw = {v.x, v.y, v.z, v.w};
    }
    void set_tc0(const Common::Vec2<f24>& v) {
        tc0_raw = {v.x, v.y};
    }
    void set_tc1(const Common::Vec2<f24>& v) {
        tc1_raw = {v.x, v.y};
    }
    void set_view(const Common::Vec3<f24>& v) {
        view_raw = {v.x, v.y, v.z};
    }
    void set_tc2(const Common::Vec2<f24>& v) {
        tc2_raw = {v.x, v.y};
    }

private:
    template <class Archive>
    void serialize(Archive& ar, const u32) {
        ar & pos_raw;
        ar & quat_raw;
        ar & color_raw;
        ar & tc0_raw;
        ar & tc1_raw;
        ar & tc0_w;
        ar & view_raw;
        ar & tc2_raw;
    }
    friend class boost::serialization::access;
};

static_assert(std::is_standard_layout_v<OutputVertex>, "Structure is not standard layout");
static_assert(sizeof(OutputVertex) == 96, "OutputVertex has invalid size");
static_assert(alignof(OutputVertex) == 16, "OutputVertex has invalid alignment");

static_assert(offsetof(OutputVertex, pos_raw) == 0, "Invalid pos offset");
static_assert(offsetof(OutputVertex, quat_raw) == 16, "Invalid quat offset");
static_assert(offsetof(OutputVertex, color_raw) == 32, "Invalid color offset");
static_assert(offsetof(OutputVertex, tc0_raw) == 48, "Invalid tc0 offset");
static_assert(offsetof(OutputVertex, tc1_raw) == 56, "Invalid tc1 offset");
static_assert(offsetof(OutputVertex, tc0_w) == 64, "Invalid tc0_w offset");
static_assert(offsetof(OutputVertex, view_raw) == 72, "Invalid view offset");
static_assert(offsetof(OutputVertex, tc2_raw) == 88, "Invalid tc2 offset");

} // namespace Pica
