#include "LEDMatrix.h"

LEDMatrix::LEDMatrix(GridsEngine& engine) : gridsEngine(engine)
{
    startTimerHz(30); // Update at 30fps
    updatePattern();
}

LEDMatrix::~LEDMatrix()
{
    stopTimer();
}

void LEDMatrix::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Border
    g.setColour(juce::Colour(0xff202020));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, 1.0f);
    
    // Draw the 32x3 LED matrix - vertically centered with 10px padding
    const float startX = 20.0f;
    const float matrixHeight = rowSpacing * 2 + ledSize;
    const float startY = (bounds.getHeight() - matrixHeight) * 0.5f;
    
    // Draw labels
    g.setColour(juce::Colour(0x80ffffff));
    g.setFont(10.0f);
    
    // Row labels
    g.drawText("BD", 5, startY - 2, 25, 15, juce::Justification::left);
    g.drawText("SD", 5, startY + rowSpacing - 2, 25, 15, juce::Justification::left);
    g.drawText("HH", 5, startY + rowSpacing * 2 - 2, 25, 15, juce::Justification::left);
    
    // Draw LEDs in 4 groups of 8
    for (int group = 0; group < 4; ++group)
    {
        float groupX = startX + 25.0f + group * (8 * ledSpacing + 12.0f);
        
        // Group separator
        if (group > 0)
        {
            g.setColour(juce::Colour(0x30ffffff));
            float sepX = groupX - 8.0f;
            g.drawLine(sepX, startY - 8, sepX, startY + rowSpacing * 2 + ledSize + 3, 0.75f);
        }
        
        // Draw bar numbers for each group
        g.setColour(juce::Colour(0x60ffffff));
        g.setFont(8.0f);
        int barNum = group + 1;
        float textX = groupX + 3.5f * ledSpacing;
        g.drawText(juce::String(barNum), textX - 8, startY - 16, 25, 10, 
                  juce::Justification::centred);
        
        for (int step = 0; step < 8; ++step)
        {
            int globalStep = group * 8 + step;
            float x = groupX + step * ledSpacing;
            
            // BD row
            drawLED(g, x, startY, getBDColour(globalStep), 
                   bdPattern[globalStep], globalStep == currentStep);
            
            // SD row
            drawLED(g, x, startY + rowSpacing, getSDColour(globalStep), 
                   sdPattern[globalStep], globalStep == currentStep);
            
            // HH row
            drawLED(g, x, startY + rowSpacing * 2, getHHColour(globalStep), 
                   hhPattern[globalStep], globalStep == currentStep);
        }
    }
}

void LEDMatrix::resized()
{
    // Nothing special needed
}

void LEDMatrix::setCurrentStep(int step)
{
    if (currentStep != step)
    {
        currentStep = step % 32;
        repaint();
    }
}

void LEDMatrix::triggerReset(bool isRetrigger)
{
    isResetting = true;
    isRetriggerReset = isRetrigger;
    resetAnimationProgress = 0.0f;
    lastResetStep = currentStep;
    repaint();
}

void LEDMatrix::updatePattern()
{
    // Get the actual pattern from the Grids engine
    auto bdPatternData = gridsEngine.getBDPattern();
    auto sdPatternData = gridsEngine.getSDPattern();
    auto hhPatternData = gridsEngine.getHHPattern();
    
    // Get density values for thresholding
    float bdDensity = gridsEngine.getBDDensity();
    float sdDensity = gridsEngine.getSDDensity();
    float hhDensity = gridsEngine.getHHDensity();
    
    // Convert pattern data to boolean triggers based on density
    for (int i = 0; i < 32; ++i)
    {
        // Apply density threshold - higher values in pattern data mean more likely to trigger
        uint8_t bdThreshold = static_cast<uint8_t>(255 * (1.0f - bdDensity));
        uint8_t sdThreshold = static_cast<uint8_t>(255 * (1.0f - sdDensity));
        uint8_t hhThreshold = static_cast<uint8_t>(255 * (1.0f - hhDensity));
        
        bdPattern[i] = bdPatternData[i] > bdThreshold;
        sdPattern[i] = sdPatternData[i] > sdThreshold;
        hhPattern[i] = hhPatternData[i] > hhThreshold;
        
        // Accents from Grids: values > 200 are accented (matching GridsEngine logic)
        bdAccents[i] = bdPatternData[i] > 200 && bdPattern[i];
        sdAccents[i] = sdPatternData[i] > 200 && sdPattern[i];
        hhAccents[i] = hhPatternData[i] > 200 && hhPattern[i];
    }
    
    repaint();
}

void LEDMatrix::timerCallback()
{
    // Update current step from engine
    int newStep = gridsEngine.getCurrentStep();
    
    // Check if X/Y or density parameters have changed
    float currentX = gridsEngine.getX();
    float currentY = gridsEngine.getY();
    float currentBDDensity = gridsEngine.getBDDensity();
    float currentSDDensity = gridsEngine.getSDDensity();
    float currentHHDensity = gridsEngine.getHHDensity();
    
    bool parametersChanged = (std::abs(currentX - lastX) > 0.001f || 
                              std::abs(currentY - lastY) > 0.001f ||
                              std::abs(currentBDDensity - lastBDDensity) > 0.001f ||
                              std::abs(currentSDDensity - lastSDDensity) > 0.001f ||
                              std::abs(currentHHDensity - lastHHDensity) > 0.001f);
    
    // Update patterns when parameters change, step changes, or periodically
    static int updateCounter = 0;
    if (newStep != currentStep || parametersChanged || ++updateCounter > 15)
    {
        currentStep = newStep;
        updateCounter = 0;
        lastX = currentX;
        lastY = currentY;
        lastBDDensity = currentBDDensity;
        lastSDDensity = currentSDDensity;
        lastHHDensity = currentHHDensity;
        updatePattern();
    }
    
    // Update reset animation
    if (isResetting)
    {
        resetAnimationProgress += 0.08f; // Animate over ~12 frames
        if (resetAnimationProgress >= 1.0f)
        {
            isResetting = false;
            resetAnimationProgress = 0.0f;
        }
    }
    
    repaint();
}

juce::Colour LEDMatrix::getBDColour(int step) const
{
    if (bdPattern[step])
    {
#ifdef ENABLE_VELOCITY_SYSTEM
        // Vary brightness based on accent for velocity visualization
        if (bdAccents[step])
            return juce::Colour(0xffff6666); // Much brighter red for accented
        else
            return juce::Colour(0xff991111); // Much dimmer red for normal
#else
        if (bdAccents[step])
            return juce::Colour(0xffff4444); // Bright red for accented
        else
            return juce::Colour(0xffcc2222); // Red for normal
#endif
    }
    return juce::Colour(0xff331111); // Dark red for off
}

juce::Colour LEDMatrix::getSDColour(int step) const
{
    if (sdPattern[step])
    {
#ifdef ENABLE_VELOCITY_SYSTEM
        // Vary brightness based on accent for velocity visualization
        if (sdAccents[step])
            return juce::Colour(0xff66ff66); // Much brighter green for accented
        else
            return juce::Colour(0xff119911); // Much dimmer green for normal
#else
        if (sdAccents[step])
            return juce::Colour(0xff44ff44); // Bright green for accented
        else
            return juce::Colour(0xff22cc22); // Green for normal
#endif
    }
    return juce::Colour(0xff113311); // Dark green for off
}

juce::Colour LEDMatrix::getHHColour(int step) const
{
    if (hhPattern[step])
    {
#ifdef ENABLE_VELOCITY_SYSTEM
        // Vary brightness based on accent for velocity visualization
        if (hhAccents[step])
            return juce::Colour(0xffffff66); // Much brighter yellow for accented
        else
            return juce::Colour(0xff999911); // Much dimmer yellow for normal
#else
        if (hhAccents[step])
            return juce::Colour(0xffffff44); // Bright yellow for accented
        else
            return juce::Colour(0xffcccc22); // Yellow for normal
#endif
    }
    return juce::Colour(0xff333311); // Dark yellow for off
}

void LEDMatrix::drawLED(juce::Graphics& g, float x, float y, juce::Colour colour, bool isActive, bool isCurrent)
{
    // Apply reset animation effects
    if (isResetting)
    {
        if (isRetriggerReset)
        {
            // Sweep animation for retrigger - brighten as the sweep passes
            float ledPosition = (x - 45.0f) / (32 * ledSpacing); // Normalize position 0-1
            float sweepPosition = resetAnimationProgress;
            float distance = std::abs(ledPosition - sweepPosition);
            
            if (distance < 0.15f) // Near the sweep
            {
                float brightness = 1.0f - (distance / 0.15f);
                colour = colour.brighter(brightness * 2.0f);
                
                // Add glow effect
                g.setColour(juce::Colour(0xffff8833).withAlpha(brightness * 0.4f));
                g.fillEllipse(x - 3, y - 3, ledSize + 6, ledSize + 6);
            }
        }
        else
        {
            // Flash animation for transparent reset
            float flashIntensity = 1.0f - resetAnimationProgress;
            if (flashIntensity > 0.5f && isActive)
            {
                colour = colour.brighter(flashIntensity);
            }
        }
    }
    
    // Draw LED shadow
    if (isActive)
    {
        g.setColour(colour.withAlpha(0.3f));
        g.fillEllipse(x - 1, y + 1, ledSize + 2, ledSize + 2);
    }
    
    // Draw LED
    g.setColour(colour);
    g.fillEllipse(x, y, ledSize, ledSize);
    
    // Draw highlight for active LEDs
    if (isActive)
    {
        g.setColour(colour.brighter(0.5f).withAlpha(0.6f));
        g.fillEllipse(x + 2, y + 2, ledSize - 4, ledSize - 4);
    }
    
    // Draw current step indicator
    if (isCurrent)
    {
        g.setColour(juce::Colour(0x80ffffff));
        g.drawEllipse(x - 2, y - 2, ledSize + 4, ledSize + 4, 2.0f);
    }
}