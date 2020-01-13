#pragma once

#include <Tact/Macros.hpp>
#include <Tact/Util.hpp>
#include <memory>
#include <string>
#include <random>
#include <map>

namespace tact
{

///////////////////////////////////////////////////////////////////////////////

/// A signal that simple returns the time passed to it
struct Time {
    inline double sample(double t) const { return t; };
    constexpr double length() const { return INF; }
private:
    TACT_SERIALIZABLE
};

///////////////////////////////////////////////////////////////////////////////

/// A Signal that emits a constant value over time
class SYNTACTS_API Scalar
{
public:
    Scalar(double value = 1);
    double sample(double t) const;
    double length() const;
public:
    double value;
private:
    TACT_SERIALIZE(TACT_MEMBER(value));
};

///////////////////////////////////////////////////////////////////////////////

/// A Signal that increases or decreases over time
class SYNTACTS_API Ramp 
{
public:
    Ramp(double initial = 1, double rate = 0);
    Ramp(double initial, double final, double span);
    double sample(double t) const;
    double length() const;
public:
    double initial;
    double rate;
private:
    TACT_SERIALIZE(TACT_MEMBER(initial), TACT_MEMBER(rate));
};

///////////////////////////////////////////////////////////////////////////////

/// A signal that generates white noise
class SYNTACTS_API Noise
{
public:
    Noise();
    double sample(double t) const;
    double length() const;
private:
    TACT_SERIALIZABLE
};

///////////////////////////////////////////////////////////////////////////////

class SYNTACTS_API Expression {
public:
    Expression(const std::string& expr = "sin(2*pi*100*t)");
    Expression(const Expression& other);
    ~Expression();
    double sample(double t) const;
    double length() const;
    bool setExpression(const std::string& expr);
    const std::string& getExpression() const;
    bool operator=(const std::string& expr);
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
private:
    friend class cereal::access;
    template<class Archive>
    void save(Archive& archive) const 
    { 
        std::string expr = getExpression();
        archive(TACT_MEMBER(expr)); 
    }
    template<class Archive>
    void load(Archive& archive) 
    { 
        std::string expr;
        archive(TACT_MEMBER(expr)); 
        setExpression(expr); 
    }
};

///////////////////////////////////////////////////////////////////////////////

/// A signal that generates white noise
class SYNTACTS_API PolyBezier
{
public:
    /// Point
    struct Point {
        double t, y;
        TACT_SERIALIZE(TACT_MEMBER(t), TACT_MEMBER(y));
    };
    /// A point and left and right control points
    struct PointGroup {
        Point cpL, p, cpR;
        TACT_SERIALIZE(TACT_MEMBER(cpL), TACT_MEMBER(p), TACT_MEMBER(cpR));
    };
public:
    double sample(double t) const;
    double length() const;
    void solve();
public:
    std::vector<PointGroup> points;
    std::vector<Point> solution;
private:
    friend class cereal::access;
    template<class Archive>
    void save(Archive& archive) const 
    { 
        archive(TACT_MEMBER(points)); 
    }
    template<class Archive>
    void load(Archive& archive) 
    { 
        archive(TACT_MEMBER(points)); 
        solve();
    }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace tact