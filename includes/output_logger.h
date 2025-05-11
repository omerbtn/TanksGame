#pragma once

#include <fstream>
#include <iostream>

#include "ActionRequest.h"
#include "tank.h"

class OutputLogger
{
public:
    OutputLogger(const std::string& filename = "");
    void logAction(int player, int step, ActionRequest action, bool valid);
    void logResult(const Tank &t1, const Tank &t2, int step);
    std::string action_to_string(ActionRequest action) const;
    
private:
    std::ofstream out_;
    bool valid_ = false;
};
