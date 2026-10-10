#pragma once

#include "../ui/Feature.h"

namespace metro
{
std::unique_ptr<Feature> createBeatDisplay (AppContext&);
std::unique_ptr<Feature> createTempo (AppContext&);
std::unique_ptr<Feature> createTransport (AppContext&);
std::unique_ptr<Feature> createMeter (AppContext&);
std::unique_ptr<Feature> createSound (AppContext&);
std::unique_ptr<Feature> createSpeedTrainer (AppContext&);
std::unique_ptr<Feature> createPresets (AppContext&);
} // namespace metro
