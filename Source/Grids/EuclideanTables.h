#pragma once

#include <cstdint>

#ifdef ENABLE_EUCLIDEAN_MODE

namespace EuclideanTables {
    
    /**
     * Pre-computed Euclidean patterns using Björklund's algorithm
     * Matching Grids' implementation for consistency
     * 
     * Each pattern is a 32-bit value where bit i indicates if step i should trigger
     */
    
    // Helper to generate Euclidean patterns
    inline uint32_t computeEuclideanPattern(int hits, int steps) {
        if (hits >= steps) return 0xFFFFFFFF; // All hits
        if (hits == 0) return 0; // No hits
        
        uint32_t pattern = 0;
        int bucket = 0;
        
        // Bresenham-like algorithm for even distribution
        for (int i = 0; i < steps; ++i) {
            bucket += hits;
            if (bucket >= steps) {
                bucket -= steps;
                pattern |= (1 << i);
            }
        }
        
        return pattern;
    }
    
    // Generate lookup table at compile time
    // Indexed by [steps][hits] where steps is 1-32 and hits is 0-steps
    class PatternTable {
    public:
        PatternTable() {
            for (int steps = 1; steps <= 32; ++steps) {
                for (int hits = 0; hits <= steps; ++hits) {
                    patterns_[steps - 1][hits] = computeEuclideanPattern(hits, steps);
                }
            }
        }
        
        uint32_t getPattern(int steps, int hits) const {
            if (steps < 1 || steps > 32) return 0;
            if (hits < 0) return 0;
            if (hits > steps) return 0xFFFFFFFF;
            return patterns_[steps - 1][hits];
        }
        
    private:
        uint32_t patterns_[32][33]; // [steps-1][hits]
    };
    
    // Static instance for global access
    inline const PatternTable& getPatternTable() {
        static PatternTable table;
        return table;
    }
    
    // Convenience function to get a pattern
    inline uint32_t getPattern(int steps, int hits) {
        return getPatternTable().getPattern(steps, hits);
    }
    
    // Check if a step should trigger based on pattern
    inline bool shouldTrigger(uint32_t pattern, int step) {
        return (pattern >> (step % 32)) & 1;
    }
}

#endif // ENABLE_EUCLIDEAN_MODE