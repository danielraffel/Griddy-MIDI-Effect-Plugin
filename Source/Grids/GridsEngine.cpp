#include "GridsEngine.h"

GridsEngine::GridsEngine() {
    // Seed random number generator
    std::random_device rd;
    rng_.seed(rd());
    reset();
}

void GridsEngine::reset() {
    currentStep_ = 0;
    swingCounter_ = 0;
    bdTrigger_ = false;
    sdTrigger_ = false;
    hhTrigger_ = false;
    bdAccent_ = false;
    sdAccent_ = false;
    hhAccent_ = false;
}

void GridsEngine::tick() {
    // Simply evaluate and advance - swing is now handled in the processor
    evaluateDrums();
    currentStep_ = (currentStep_ + 1) % 32;
}

uint8_t GridsEngine::readDrumMap(int instrument, int step) {
    // Convert X/Y to grid coordinates
    float scaledX = x_ * 4.0f;  // 0-4 range for 5x5 grid
    float scaledY = y_ * 4.0f;
    
    // Find the four nearest nodes
    int x0 = static_cast<int>(scaledX);
    int y0 = static_cast<int>(scaledY);
    int x1 = std::min(x0 + 1, 4);
    int y1 = std::min(y0 + 1, 4);
    
    // Calculate interpolation factors
    float fx = scaledX - x0;
    float fy = scaledY - y0;
    
    // Get node indices
    int node00 = y0 * 5 + x0;
    int node01 = y0 * 5 + x1;
    int node10 = y1 * 5 + x0;
    int node11 = y1 * 5 + x1;
    
    // Calculate pattern offset for this instrument and step
    // Pattern layout: BD (0-31), SD (32-63), HH (64-95)
    int offset = instrument * 32 + step;
    
    // Read values from the four nearest nodes
    uint8_t v00 = (*grids::node_table[node00])[offset];
    uint8_t v01 = (*grids::node_table[node01])[offset];
    uint8_t v10 = (*grids::node_table[node10])[offset];
    uint8_t v11 = (*grids::node_table[node11])[offset];
    
    // Bilinear interpolation
    float v0 = v00 * (1.0f - fx) + v01 * fx;
    float v1 = v10 * (1.0f - fx) + v11 * fx;
    float result = v0 * (1.0f - fy) + v1 * fy;
    
    return static_cast<uint8_t>(result);
}

void GridsEngine::evaluateDrums() {
    // Read pattern values for current step
    uint8_t bdValue = readDrumMap(0, currentStep_);
    uint8_t sdValue = readDrumMap(1, currentStep_);
    uint8_t hhValue = readDrumMap(2, currentStep_);
    
    // Apply density thresholds
    bdTrigger_ = applyDensity(bdValue, bdDensity_);
    sdTrigger_ = applyDensity(sdValue, sdDensity_);
    hhTrigger_ = applyDensity(hhValue, hhDensity_);
    
    // Apply chaos only if density > 0 (don't add ghost notes when density is zero)
    if (chaos_ > 0.0f) {
        if (bdDensity_ > 0.0f) bdTrigger_ = applyChaos(bdTrigger_);
        if (sdDensity_ > 0.0f) sdTrigger_ = applyChaos(sdTrigger_);
        if (hhDensity_ > 0.0f) hhTrigger_ = applyChaos(hhTrigger_);
    }
    
    // Determine accents (values > 200 are accented)
    bdAccent_ = bdValue > 200 && bdTrigger_;
    sdAccent_ = sdValue > 200 && sdTrigger_;
    hhAccent_ = hhValue > 200 && hhTrigger_;
}

bool GridsEngine::applyDensity(uint8_t value, float density) {
    // Scale the threshold based on density
    // At density 0.0, nothing triggers
    // At density 1.0, all non-zero values trigger
    if (density <= 0.0f) {
        return false;  // No triggers at zero density
    }
    
    uint8_t threshold = static_cast<uint8_t>((1.0f - density) * 254.0f);
    return value > threshold;
}

bool GridsEngine::applyChaos(bool trigger) {
    float random = randomDist_(rng_);
    
    if (trigger) {
        // Chaos can remove triggers
        return random > (chaos_ * 0.3f);
    } else {
        // Chaos can add ghost notes
        return random < (chaos_ * 0.1f);
    }
}

std::array<uint8_t, 32> GridsEngine::getBDPattern() const {
    std::array<uint8_t, 32> pattern;
    for (int i = 0; i < 32; i++) {
        pattern[i] = const_cast<GridsEngine*>(this)->readDrumMap(0, i);
    }
    return pattern;
}

std::array<uint8_t, 32> GridsEngine::getSDPattern() const {
    std::array<uint8_t, 32> pattern;
    for (int i = 0; i < 32; i++) {
        pattern[i] = const_cast<GridsEngine*>(this)->readDrumMap(1, i);
    }
    return pattern;
}

std::array<uint8_t, 32> GridsEngine::getHHPattern() const {
    std::array<uint8_t, 32> pattern;
    for (int i = 0; i < 32; i++) {
        pattern[i] = const_cast<GridsEngine*>(this)->readDrumMap(2, i);
    }
    return pattern;
}