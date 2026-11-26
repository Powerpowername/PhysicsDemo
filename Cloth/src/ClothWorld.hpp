#pragma once

#include <glm/vec3.hpp>

#include <cstddef>
#include <vector>

class ClothWorld
{
public:
    static constexpr float fixedStep = 1.0f / 120.0f;

    void create();
    void reset();
    void step();

    void setWindEnabled(bool enabled) { windEnabled_ = enabled; }
    bool windEnabled() const { return windEnabled_; }

    std::size_t columns() const { return columns_; }
    std::size_t rows() const { return rows_; }
    const glm::vec3& position(std::size_t column, std::size_t row) const;

private:
    struct Particle
    {
        glm::vec3 position{0.0f};
        glm::vec3 previousPosition{0.0f};
        glm::vec3 velocity{0.0f};
        float inverseMass = 1.0f;
    };

    struct DistanceConstraint
    {
        std::size_t first = 0;
        std::size_t second = 0;
        float restLength = 0.0f;
        float compliance = 0.0f;
        float lambda = 0.0f;
    };

    static constexpr std::size_t columns_ = 18;
    static constexpr std::size_t rows_ = 13;
    static constexpr float spacing_ = 0.20f;
    static constexpr float topHeight_ = 3.10f;
    static constexpr float stretchCompliance_ = 2.0e-7f;
    static constexpr float shearCompliance_ = 1.0e-6f;
    static constexpr float bendCompliance_ = 3.0e-5f;

    std::size_t index(std::size_t column, std::size_t row) const;
    void addConstraint(std::size_t first, std::size_t second, float compliance);
    void solveConstraint(DistanceConstraint& constraint);

    std::vector<Particle> particles_;
    std::vector<DistanceConstraint> constraints_;
    bool windEnabled_ = true;
    float time_ = 0.0f;
};
