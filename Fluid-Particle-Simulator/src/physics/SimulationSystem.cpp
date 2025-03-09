#include "SimulationSystem.h"
#include <iostream>
#include <algorithm>

SimulationSystem::SimulationSystem(const glm::vec2& bottomLeft, const glm::vec2& topRight, unsigned int particleRadius)
    : m_bottomLeft(bottomLeft), m_topRight(topRight), m_ParticleRadius(particleRadius),
    m_Zoom(1.0f), m_WindowWidth(800) // Default values
{
    m_SimHeight = std::abs(topRight.y - bottomLeft.y);
    m_SimWidth = std::abs(topRight.x - bottomLeft.x);
}

SimulationSystem::~SimulationSystem()
{
    // Destructor implementation (empty for now)
}

void SimulationSystem::AddParticle(const glm::vec2& position, float mass)
{
    Particle newParticle(position, mass);
    m_Particles.push_back(newParticle);
}

void SimulationSystem::AddParticleGrid(int rows, int cols, const glm::vec2& bottomLeft,
    const glm::vec2& topRight, const glm::vec2& spacing,
    float mass)
{
    // Validate input parameters
    if (rows <= 0 || cols <= 0) {
        std::cerr << "Error: Cannot create grid with " << rows << " rows and "
            << cols << " columns. Values must be positive." << std::endl;
        return;
    }

    // Ensure grid boundaries are within simulation bounds
    glm::vec2 gridBottomLeft = glm::max(bottomLeft, m_bottomLeft + glm::vec2(m_ParticleRadius));
    glm::vec2 gridTopRight = glm::min(topRight, m_topRight - glm::vec2(m_ParticleRadius));

    // Check if we have enough space for at least one particle
    if (gridBottomLeft.x >= gridTopRight.x || gridBottomLeft.y >= gridTopRight.y) {
        std::cerr << "Error: Not enough space within bounds to create particle grid" << std::endl;
        return;
    }

    // Calculate total space needed for the grid
    float requiredWidth, requiredHeight;

    // Calculate total space with particle sizes included
    requiredWidth = (cols * (2 * m_ParticleRadius)); // Space for all particles
    if (cols > 1) {
        requiredWidth += ((cols - 1) * spacing.x); // Add spacing between particles
    }

    requiredHeight = (rows * (2 * m_ParticleRadius)); // Space for all particles
    if (rows > 1) {
        requiredHeight += ((rows - 1) * spacing.y); // Add spacing between particles
    }

    // Check if the grid with specified spacing will fit
    if (requiredWidth > (gridTopRight.x - gridBottomLeft.x) ||
        requiredHeight > (gridTopRight.y - gridBottomLeft.y)) {
        std::cerr << "Error: Grid with " << rows << "x" << cols << " particles (radius " << m_ParticleRadius << ")"
            << " and spacing " << spacing.x << "x" << spacing.y
            << " requires " << requiredWidth << "x" << requiredHeight << " units, "
            << "but only " << (gridTopRight.x - gridBottomLeft.x) << "x"
            << (gridTopRight.y - gridBottomLeft.y) << " units are available." << std::endl;
        return;
    }

    // Calculate available space
    glm::vec2 availableSpace = gridTopRight - gridBottomLeft;

    std::cout << availableSpace.x << " " << availableSpace.y << std::endl;

    // Ensure that the number of particles can fit 
    if ((availableSpace.y / (2 * m_ParticleRadius)) * (availableSpace.x / (2 * m_ParticleRadius)) < rows * cols)
    {
        std::cerr << "Error: Too many particles to fit inside the desired grid" << std::endl;
        return;
    }

    // Determine final spacing between particles
    glm::vec2 finalSpacing;

    // If only one row/column, position it in the center
    if (cols == 1) {
        finalSpacing.x = 0;
    }
    else {
        // Use either user-specified spacing or evenly distribute
        finalSpacing.x = (spacing.x > 0) ?
            spacing.x : availableSpace.x / (cols - 1);
    }

    if (rows == 1) {
        finalSpacing.y = 0;
    }
    else {
        finalSpacing.y = (spacing.y > 0) ?
            spacing.y : availableSpace.y / (rows - 1);
    }

    // Verify minimum spacing required for particles not to overlap
    float minRequiredSpacing = 2 * m_ParticleRadius;
    if ((cols > 1 && finalSpacing.x < minRequiredSpacing) ||
        (rows > 1 && finalSpacing.y < minRequiredSpacing)) {
        std::cerr << "Error: Particles would overlap with calculated spacing of "
            << finalSpacing.x << "x" << finalSpacing.y << ". "
            << "Minimum spacing needed: " << minRequiredSpacing << " units." << std::endl;
        return;
    }

    // Calculate grid dimensions with the final spacing
    glm::vec2 gridSize(finalSpacing.x * (cols - 1), finalSpacing.y * (rows - 1));

    // Center the grid within the available space if it's smaller than available area
    glm::vec2 startPos = gridBottomLeft;
    if (gridSize.x < availableSpace.x) {
        startPos.x += (availableSpace.x - gridSize.x) / 2.0f;
    }
    if (gridSize.y < availableSpace.y) {
        startPos.y += (availableSpace.y - gridSize.y) / 2.0f;
    }

    // Create the grid with final spacing
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            glm::vec2 position = startPos + glm::vec2(col * finalSpacing.x, row * finalSpacing.y);
            AddParticle(position, mass);
        }
    }

    std::cout << "Created grid with " << rows << "x" << cols << " particles" << std::endl;
}

glm::mat4 SimulationSystem::GetViewMatrix(float aspectRatio, float simulationBorderOffset) const
{
    // Calculate the width and height of the simulation
    float simWidth = m_topRight.x - m_bottomLeft.x;
    float simHeight = m_topRight.y - m_bottomLeft.y;

    // Calculate the center of the simulation
    glm::vec2 center = (m_topRight + m_bottomLeft) * 0.5f;

    // ad y offest to create border between simulation and window
    center.y -= simulationBorderOffset;

    // Adjust for zoom factor
    float viewWidth = simWidth / m_Zoom;
    float viewHeight = viewWidth / aspectRatio;

    // Create orthographic projection matrix centered on the simulation
    glm::mat4 projection = glm::ortho(
        center.x - viewWidth / 2, center.x + viewWidth / 2,   // left, right
        center.y - viewHeight / 2, center.y + viewHeight / 2, // bottom, top
        -1.0f, 1.0f                                           // near, far
    );


    return projection;
}

float SimulationSystem::PixelToSimulationDistance(float pixelDistance) const
{
    // Calculate the simulation width in simulation units
    float simWidth = m_topRight.x - m_bottomLeft.x;

    // Calculate view width based on zoom level
    float viewWidth = simWidth / m_Zoom;

    // Calculate simulation units per pixel
    float scale = viewWidth / static_cast<float>(m_WindowWidth);

    // Convert pixel distance to simulation distance
    return pixelDistance * scale;
}
