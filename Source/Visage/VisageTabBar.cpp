#include "VisageTabBar.h"

//==============================================================================
VisageTabBar::VisageTabBar()
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    
    DBG("VisageTabBar constructor called");
}

//==============================================================================
void VisageTabBar::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(borderColor);
    
    // Draw tabs
    for (int i = 0; i < tabs.size(); ++i)
    {
        auto tabBounds = getTabBounds(i);
        
        // Determine tab color
        juce::Colour tabColor;
        if (i == activeTabIndex)
        {
            tabColor = activeTabColor;
        }
        else if (i == hoveredTabIndex)
        {
            tabColor = hoveredTabColor;
        }
        else
        {
            tabColor = inactiveTabColor;
        }
        
        // Draw tab background with rounded top corners
        juce::Path tabPath;
        const float cornerRadius = 6.0f;
        
        tabPath.startNewSubPath(tabBounds.getX(), tabBounds.getBottom());
        tabPath.lineTo(tabBounds.getX(), tabBounds.getY() + cornerRadius);
        tabPath.addArc(tabBounds.getX(), tabBounds.getY(), cornerRadius * 2, cornerRadius * 2, 
                       juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
        tabPath.lineTo(tabBounds.getRight() - cornerRadius, tabBounds.getY());
        tabPath.addArc(tabBounds.getRight() - cornerRadius * 2, tabBounds.getY(), cornerRadius * 2, cornerRadius * 2, 
                       juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
        tabPath.lineTo(tabBounds.getRight(), tabBounds.getBottom());
        tabPath.closeSubPath();
        
        g.setColour(tabColor);
        g.fillPath(tabPath);
        
        // Draw subtle border for inactive tabs
        if (i != activeTabIndex)
        {
            g.setColour(borderColor.brighter(0.1f));
            g.strokePath(tabPath, juce::PathStrokeType(1.0f));
        }
        
        // Draw tab text
        g.setColour(i == activeTabIndex ? activeTextColor : textColor);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        
        auto textBounds = tabBounds.reduced(tabPadding, 0);
        g.drawText(tabs[i].name, textBounds, juce::Justification::centred);
    }
}

//==============================================================================
void VisageTabBar::resized()
{
    // Tabs will be resized dynamically based on content
}

//==============================================================================
void VisageTabBar::mouseDown(const juce::MouseEvent& e)
{
    int tabIndex = getTabIndexAtPosition(e.x, e.y);
    if (tabIndex >= 0 && tabIndex < tabs.size())
    {
        selectTab(tabIndex);
    }
}

//==============================================================================
void VisageTabBar::mouseMove(const juce::MouseEvent& e)
{
    int newHoveredIndex = getTabIndexAtPosition(e.x, e.y);
    
    if (newHoveredIndex != hoveredTabIndex)
    {
        hoveredTabIndex = newHoveredIndex;
        repaint();
    }
}

//==============================================================================
void VisageTabBar::mouseExit(const juce::MouseEvent& e)
{
    if (hoveredTabIndex != -1)
    {
        hoveredTabIndex = -1;
        repaint();
    }
}

//==============================================================================
bool VisageTabBar::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    if (tabs.isEmpty())
        return false;
    
    if (key == juce::KeyPress::leftKey)
    {
        int newIndex = (activeTabIndex - 1 + tabs.size()) % tabs.size();
        selectTab(newIndex);
        return true;
    }
    else if (key == juce::KeyPress::rightKey)
    {
        int newIndex = (activeTabIndex + 1) % tabs.size();
        selectTab(newIndex);
        return true;
    }
    else if (key == juce::KeyPress::homeKey)
    {
        selectTab(0);
        return true;
    }
    else if (key == juce::KeyPress::endKey)
    {
        selectTab(tabs.size() - 1);
        return true;
    }
    
    return false;
}

//==============================================================================
void VisageTabBar::addTab(const juce::String& name, const juce::String& id)
{
    tabs.add(Tab(name, id));
    
    // If this is the first tab, make it active
    if (tabs.size() == 1)
    {
        activeTabIndex = 0;
    }
    
    resized();
    repaint();
}

//==============================================================================
void VisageTabBar::removeTab(const juce::String& id)
{
    for (int i = 0; i < tabs.size(); ++i)
    {
        if (tabs[i].id == id)
        {
            tabs.remove(i);
            
            // Adjust active tab index if necessary
            if (activeTabIndex >= tabs.size())
                activeTabIndex = juce::jmax(0, tabs.size() - 1);
            
            resized();
            repaint();
            break;
        }
    }
}

//==============================================================================
void VisageTabBar::clearTabs()
{
    tabs.clear();
    activeTabIndex = 0;
    hoveredTabIndex = -1;
    repaint();
}

//==============================================================================
void VisageTabBar::setActiveTab(const juce::String& id)
{
    for (int i = 0; i < tabs.size(); ++i)
    {
        if (tabs[i].id == id)
        {
            selectTab(i);
            break;
        }
    }
}

//==============================================================================
void VisageTabBar::setActiveTab(int index)
{
    if (index >= 0 && index < tabs.size())
    {
        selectTab(index);
    }
}

//==============================================================================
juce::String VisageTabBar::getActiveTabId() const
{
    if (activeTabIndex >= 0 && activeTabIndex < tabs.size())
        return tabs[activeTabIndex].id;
    
    return {};
}

//==============================================================================
int VisageTabBar::getActiveTabIndex() const
{
    return activeTabIndex;
}

//==============================================================================
void VisageTabBar::setTabHeight(int height)
{
    tabHeight = height;
    repaint();
}

//==============================================================================
int VisageTabBar::getTabIndexAtPosition(int x, int y) const
{
    if (y < 0 || y >= tabHeight || tabs.isEmpty())
        return -1;
    
    for (int i = 0; i < tabs.size(); ++i)
    {
        auto tabBounds = getTabBounds(i);
        if (tabBounds.contains(x, y))
            return i;
    }
    
    return -1;
}

//==============================================================================
juce::Rectangle<int> VisageTabBar::getTabBounds(int tabIndex) const
{
    if (tabIndex < 0 || tabIndex >= tabs.size())
        return {};
    
    // Calculate tab width - distribute evenly across available width
    int totalWidth = getWidth();
    int tabWidth = totalWidth / juce::jmax(1, tabs.size());
    
    // Account for spacing between tabs
    int actualTabWidth = tabWidth - tabSpacing;
    if (tabIndex == tabs.size() - 1)
        actualTabWidth = totalWidth - (tabIndex * tabWidth); // Last tab takes remaining space
    
    int x = tabIndex * tabWidth;
    
    return juce::Rectangle<int>(x, 0, actualTabWidth, tabHeight);
}

//==============================================================================
void VisageTabBar::selectTab(int index)
{
    if (index == activeTabIndex || index < 0 || index >= tabs.size())
        return;
    
    activeTabIndex = index;
    repaint();
    
    // Notify listeners
    if (onTabChanged && activeTabIndex < tabs.size())
    {
        onTabChanged(tabs[activeTabIndex].id, activeTabIndex);
    }
}