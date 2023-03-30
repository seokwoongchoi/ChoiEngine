#pragma once

enum class Intersection : uint
{
    Outside,
    Inside,
    Intersect
};

class Math final
{
public:
    static constexpr float PI = 3.14159265359f;
    static constexpr float PI_2 = 6.28318530718f;
    static constexpr float PI_DIV_2 = 1.57079632679f;
    static constexpr float PI_DIV_4 = 0.78539816339f;
    static constexpr float TO_DEG = 180.0f / PI;
    static constexpr float TO_RAD = PI / 180.0f;
	static constexpr float EPSILON = 0.000001f;

    static auto Random(const int& min, const int& max) -> const int;
    static auto Random(const float& min, const float& max) -> const float;

    static const float ToRadian(const float& degree) { return degree * TO_RAD; }
    static const float ToDegree(const float& radian) { return radian * TO_DEG; }

    template <typename T>
    static void  Clamp( T& value, const T& min, const T& max)
    {
		value=(value < min) ? min : (value > max) ? max : value;
    }

    template <typename T>
    static constexpr T Max(const T& lhs, const T& rhs)
    {
        return lhs > rhs ? lhs : rhs;        
    }

    template <typename T>
    static constexpr T Min(const T& lhs, const T& rhs)
    {
        return lhs < rhs ? lhs : rhs;
    }

    template <typename T>
    static constexpr T Abs(const T& value)
    {
        return value >= 0.0f ? value : -value;
    }

    template <typename T>
    static constexpr int Sign(const T& value)
    {
        return (T(0) < value) - (T(0) < value);
    }

    template <typename T>
    static constexpr bool Equals(const T& lhs, const T& rhs, const T& e = std::numeric_limits<T>::epsilon())
    {
        return lhs + e >= rhs && lhs - e <= rhs;
    }
};