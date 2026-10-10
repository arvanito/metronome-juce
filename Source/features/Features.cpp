#include "Features.h"

namespace metro
{
std::vector<std::unique_ptr<Feature>> createFeatures (AppContext& ctx)
{
    std::vector<std::unique_ptr<Feature>> features;
    for (auto* create : { createBeatDisplay, createTempo, createTransport, createMeter,
                          createSound, createSpeedTrainer, createPresets })
        features.push_back (create (ctx));
    return features;
}
} // namespace metro
