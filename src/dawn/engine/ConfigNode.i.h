/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2016 (git@davedissian.com)
 */
#pragma once

#include "math/Vec2i.h"
#include "math/Vec3i.h"
#include "math/Vec4i.h"

namespace dw {

template <class T>
ConfigNode::ConfigNode(Context* context, const T& s) : ConfigNode(context, Converter<T>::encode(context, s)) {
}

template <> struct Converter<bool> {
    static ConfigNode encode(Context* context, const bool& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = value ? "true" : "false";
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, bool& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        rhs = node.mData.scalar == "true";
        return true;
    }
};

template <> struct Converter<int> {
    static ConfigNode encode(Context* context, const int& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = std::to_string(value);
        return out;
    }

    static bool decode(Log& logger, const ConfigNode& node, int& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        try {
            rhs = std::stoi(node.mData.scalar);
        } catch (std::exception& e) {
            logger.info << "Unable to convert value " << node.mData.scalar << " - Reason: " << e.what();
            return false;
        }

        return true;
    }
};

template <> struct Converter<uint> {
    static ConfigNode encode(Context* context, const uint& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = std::to_string(value);
        return out;
    }

    static bool decode(Log& logger, const ConfigNode& node, uint& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        try {
            rhs = std::stoul(node.mData.scalar);
        } catch (std::exception& e) {
            logger.info << "Unable to convert value " << node.mData.scalar << " - Reason: " << e.what();
            return false;
        }

        return true;
    }
};

template <> struct Converter<uint64_t> {
    static ConfigNode encode(Context* context, const uint64_t& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = std::to_string(value);
        return out;
    }

    static bool decode(Log& logger, const ConfigNode& node, uint64_t& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        try {
            rhs = std::stoull(node.mData.scalar);
        } catch (std::exception& e) {
            logger.info << "Unable to convert value " << node.mData.scalar << " - Reason: " << e.what();
            return false;
        }

        return true;
    }
};

template <> struct Converter<float> {
    static ConfigNode encode(Context* context, const float& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = std::to_string(value);
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, float& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        rhs = std::stof(node.mData.scalar);
        return true;
    }
};

template <> struct Converter<double> {
    static ConfigNode encode(Context* context, const double& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = std::to_string(value);
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, double& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        rhs = std::stod(node.mData.scalar);
        return true;
    }
};

template <> struct Converter<String> {
    static ConfigNode encode(Context* context, const String& value) {
        ConfigNode out{context};
        out.mType = NT_SCALAR;
        out.mData.scalar = value;
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, String& rhs) {
        if (node.mType != NT_SCALAR)
            return false;

        rhs = node.mData.scalar;
        return true;
    }
};

template <> struct Converter<Vec2> {
    static ConfigNode encode(Context* context, const Vec2& rhs) {
        ConfigNode out{context};
        out.push(ConfigNode(context, rhs.x));
        out.push(ConfigNode(context, rhs.y));
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, Vec2& rhs) {
        if (!node.isSequence() || node.size() != 2)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        return true;
    }
};

template <> struct Converter<Vec2i> {
    static ConfigNode encode(Context* context, const Vec2i& rhs) {
        ConfigNode out{context};
        out.push(ConfigNode(context, rhs.x));
        out.push(ConfigNode(context, rhs.y));
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, Vec2i& rhs) {
        if (!node.isSequence() || node.size() != 2)
            return false;

        rhs.x = node[0].as<int>();
        rhs.y = node[1].as<int>();
        return true;
    }
};

template <> struct Converter<Vec3> {
    static ConfigNode encode(Context* context, const Vec3& rhs) {
        ConfigNode out{context};
        out.push(ConfigNode(context, rhs.x));
        out.push(ConfigNode(context, rhs.y));
        out.push(ConfigNode(context, rhs.z));
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, Vec3& rhs) {
        if (!node.isSequence() || node.size() != 3)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        return true;
    }
};

template <> struct Converter<Vec3i> {
    static ConfigNode encode(Context* context, const Vec3i& rhs) {
        ConfigNode out{context};
        out.push(ConfigNode(context, rhs.x));
        out.push(ConfigNode(context, rhs.y));
        out.push(ConfigNode(context, rhs.z));
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, Vec3i& rhs) {
        if (!node.isSequence() || node.size() != 3)
            return false;

        rhs.x = node[0].as<int>();
        rhs.y = node[1].as<int>();
        rhs.z = node[2].as<int>();
        return true;
    }
};

template <> struct Converter<Vec4> {
    static ConfigNode encode(Context* context, const Vec4& rhs) {
        ConfigNode out{context};
        out.push(ConfigNode(context, rhs.x));
        out.push(ConfigNode(context, rhs.y));
        out.push(ConfigNode(context, rhs.z));
        out.push(ConfigNode(context, rhs.w));
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, Vec4& rhs) {
        if (!node.isSequence() || node.size() != 4)
            return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();
        rhs.z = node[2].as<float>();
        rhs.w = node[3].as<float>();
        return true;
    }
};

template <> struct Converter<Vec4i> {
    static ConfigNode encode(Context* context, const Vec4i& rhs) {
        ConfigNode out{context};
        out.push(ConfigNode(context, rhs.x));
        out.push(ConfigNode(context, rhs.y));
        out.push(ConfigNode(context, rhs.z));
        out.push(ConfigNode(context, rhs.w));
        return out;
    }

    static bool decode(Log&, const ConfigNode& node, Vec4i& rhs) {
        if (!node.isSequence() || node.size() != 4)
            return false;

        rhs.x = node[0].as<int>();
        rhs.y = node[1].as<int>();
        rhs.z = node[2].as<int>();
        rhs.w = node[3].as<int>();
        return true;
    }
};
}