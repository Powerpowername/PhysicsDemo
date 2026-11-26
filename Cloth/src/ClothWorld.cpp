#include "ClothWorld.hpp"

#include <glm/geometric.hpp>

#include <cmath>

namespace
{
constexpr glm::vec3 gravity{0.0f, -9.81f, 0.0f};
constexpr float damping = 0.995f;
constexpr int solverIterations = 8;
}

void ClothWorld::create()
{
    particles_.resize(columns_ * rows_);
    constraints_.clear();

    reset(); // Measure each rest length on the undeformed grid.

    for (std::size_t row = 0; row < rows_; ++row)
    {
        for (std::size_t column = 0; column < columns_; ++column)
        {
            const auto here = index(column, row);
            if (column + 1 < columns_)
                addConstraint(here, index(column + 1, row), stretchCompliance_);
            if (row + 1 < rows_)
                addConstraint(here, index(column, row + 1), stretchCompliance_);

            // Diagonals resist shear; softer two-edge spans resist sharp folds.
            if (column + 1 < columns_ && row + 1 < rows_)
            {
                addConstraint(here, index(column + 1, row + 1), shearCompliance_);
                addConstraint(index(column + 1, row), index(column, row + 1), shearCompliance_);
            }
            if (column + 2 < columns_)
                addConstraint(here, index(column + 2, row), bendCompliance_);
            if (row + 2 < rows_)
                addConstraint(here, index(column, row + 2), bendCompliance_);
        }
    }
}

void ClothWorld::reset()
{
    time_ = 0.0f;
    for (std::size_t row = 0; row < rows_; ++row)
    {
        for (std::size_t column = 0; column < columns_; ++column)
        {
            auto& particle = particles_[index(column, row)];
            particle.position = {
                (static_cast<float>(column) - static_cast<float>(columns_ - 1) * 0.5f) * spacing_,
                topHeight_ - static_cast<float>(row) * spacing_,
                0.0f};
            particle.inverseMass = row == 0 ? 0.0f : 1.0f;
            particle.previousPosition = particle.position;
            particle.velocity = glm::vec3(0.0f);
        }
    }
}

void ClothWorld::step()
{
    time_ += fixedStep;
    for (auto& particle : particles_)
    {
        particle.previousPosition = particle.position;
        if (particle.inverseMass == 0.0f) continue;

        glm::vec3 acceleration = gravity;
        if (windEnabled_)
        {
            const float phase = time_ * 1.4f + particle.position.x * 1.8f
                + particle.position.y * 0.45f;
            acceleration += glm::vec3(1.2f * std::sin(phase * 0.7f),
                                      0.2f * std::cos(phase),
                                      4.0f * std::sin(phase));
        }
        particle.velocity += acceleration * fixedStep;
        particle.position += particle.velocity * fixedStep;
    }

    // Lambda accumulates across iterations of this step, not across time steps.
    for (auto& constraint : constraints_) constraint.lambda = 0.0f;
    for (int iteration = 0; iteration < solverIterations; ++iteration)
    {
        for (auto& constraint : constraints_) solveConstraint(constraint);
    }

    for (auto& particle : particles_)
    {
        if (particle.inverseMass == 0.0f) continue;
        // Constraint corrections are part of this step's motion, so derive velocity
        // from the final projected position instead of the unconstrained prediction.
        particle.velocity = damping * (particle.position - particle.previousPosition) / fixedStep;
    }
}

const glm::vec3& ClothWorld::position(std::size_t column, std::size_t row) const
{
    return particles_.at(index(column, row)).position;
}

std::size_t ClothWorld::index(std::size_t column, std::size_t row) const
{
    return row * columns_ + column;
}

void ClothWorld::addConstraint(std::size_t first, std::size_t second, float compliance)
{
    const float restLength = glm::distance(particles_[first].position, particles_[second].position);
    constraints_.push_back({first, second, restLength, compliance, 0.0f});
}

void ClothWorld::solveConstraint(DistanceConstraint& constraint)
{
    auto& first = particles_[constraint.first];
    auto& second = particles_[constraint.second];
    const glm::vec3 difference = second.position - first.position;
    const float length = glm::length(difference);
    const float inverseMassSum = first.inverseMass + second.inverseMass;
    if (length < 1.0e-6f || inverseMassSum == 0.0f) return;

    const float constraintValue = length - constraint.restLength;
    const float alpha = constraint.compliance / (fixedStep * fixedStep);
    const float deltaLambda = (-constraintValue - alpha * constraint.lambda)
        / (inverseMassSum + alpha);
    constraint.lambda += deltaLambda;
    const glm::vec3 direction = difference / length;
    first.position -= direction * (deltaLambda * first.inverseMass);
    second.position += direction * (deltaLambda * second.inverseMass);
}
