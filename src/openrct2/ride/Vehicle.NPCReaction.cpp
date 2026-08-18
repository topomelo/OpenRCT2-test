/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../GameState.h"
#include "../entity/EntityRegistry.h"
#include "../entity/Guest.h"
#include "../entity/Peep.h"
#include "Vehicle.h"

using namespace OpenRCT2;

// ============================================================================
// NPC G-Force Reaction System
// ============================================================================

/**
 * Updates rider reactions based on current G-forces experienced.
 * This function analyzes the G-forces and triggers appropriate rider responses
 * without affecting the core physics calculations.
 */
void Vehicle::UpdateRiderReactionsToGForces(const GForces& gForces)
{
    if (num_peeps == 0 || ride.IsNull())
        return;

    auto curRide = GetRide();
    if (curRide == nullptr)
        return;

    // Only apply reactions when vehicle is in motion on track
    if (status != Status::travelling && status != Status::departing && status != Status::arriving)
        return;

    // Apply effects to each rider in the vehicle
    ApplyGForceEffectsToRiders(gForces);
}

/**
 * Applies G-force effects to all riders in the vehicle.
 * Effects include: excitement changes, nausea changes, thought generation,
 * and special actions (jumping, vomiting, screaming).
 */
void Vehicle::ApplyGForceEffectsToRiders(const GForces& gForces)
{
    auto curRide = GetRide();
    if (curRide == nullptr)
        return;

    // Process each peep in the vehicle
    for (int32_t i = 0; i < num_peeps; i++)
    {
        auto peepIndex = peep[i];
        if (peepIndex.IsNull())
            continue;

        auto peep = getGameState().entities.GetEntity<Guest>(peepIndex);
        if (peep == nullptr)
            continue;

        // Vertical G effects (positive = high G, negative = airtime)
        int32_t verticalG = gForces.verticalG;
        
        // Lateral G effects (side-to-side forces)
        int32_t lateralG = std::abs(gForces.lateralG);

        // High positive G forces (greater than 2G = ~160 units)
        if (verticalG > 160)
        {
            // Intense positive G - riders feel heavy
            peep->NauseaTarget += 8;
            peep->ExcitementTarget += 4;
            
            // Extreme G forces (>4G = ~320 units) cause more intense reactions
            if (verticalG > 320)
            {
                peep->NauseaTarget += 12;
                peep->EnergyTarget -= 5;
                
                // Small chance of vomiting at extreme G
                if ((peep->PeepId + getGameState().currentTicks) % 256 < 16)
                {
                    peep->InsertThought(PeepThoughtType::vomiting);
                }
            }
            
            // Generate thought about intensity
            if ((peep->PeepId + getGameState().currentTicks) % 512 < 32)
            {
                peep->InsertThought(PeepThoughtType::intensify);
            }
        }
        // Negative G forces (airtime, less than -0.5G = ~-40 units)
        else if (verticalG < -40)
        {
            // Mild airtime - excitement boost
            peep->ExcitementTarget += 8;
            
            // Strong airtime (less than -1G = ~-80 units)
            if (verticalG < -80)
            {
                peep->ExcitementTarget += 12;
                peep->NauseaTarget += 6;
                
                // Riders jump/scream during strong airtime
                if ((peep->PeepId + getGameState().currentTicks) % 128 < 48)
                {
                    peep->InsertThought(PeepThoughtType::great);
                }
            }
            
            // Very strong airtime (less than -2G = ~-160 units)
            if (verticalG < -160)
            {
                peep->NauseaTarget += 10;
                
                // Chance of screaming
                if ((peep->PeepId + getGameState().currentTicks) % 64 < 32)
                {
                    peep->InsertThought(PeepThoughtType::scream);
                }
            }
        }
        // Moderate G forces (normal range)
        else
        {
            // Slight excitement from normal riding
            if ((peep->PeepId + getGameState().currentTicks) % 256 < 16)
            {
                peep->ExcitementTarget += 2;
            }
        }

        // High lateral G forces (strong side-to-side movement)
        if (lateralG > 100)
        {
            peep->NauseaTarget += 6;
            
            // Very high lateral G (>2G lateral)
            if (lateralG > 200)
            {
                peep->NauseaTarget += 10;
                peep->ExcitementTarget += 3;
                
                // Thought about roughness
                if ((peep->PeepId + getGameState().currentTicks) % 384 < 48)
                {
                    peep->InsertThought(PeepThoughtType::rough);
                }
            }
        }

        // Update peep state to reflect new targets
        peep->WindowInvalidateFlags |= PEEP_INVALIDATE_PEEP_STATS;
    }
}

// ============================================================================
// Enhanced Block Section Safety System
// ============================================================================

/**
 * Checks if there's sufficient safety distance to the preceding vehicle
 * in a block section. This prevents collisions by ensuring adequate
 * spacing before entering occupied blocks.
 * 
 * @param precedingVehicle The vehicle ahead in the same block section
 * @return true if safe distance is maintained, false if emergency braking needed
 */
bool Vehicle::CheckBlockSectionSafetyDistance(const Vehicle* precedingVehicle) const
{
    if (precedingVehicle == nullptr || velocity <= 0)
        return true;

    // Calculate distance to preceding vehicle
    int32_t dx = std::abs(x - precedingVehicle->x);
    int32_t dy = std::abs(y - precedingVehicle->y);
    int32_t dz = std::abs(z - precedingVehicle->z);
    
    // 3D distance approximation (faster than sqrt)
    int32_t distance = dx + dy + (dz / 2);

    // Calculate minimum safe distance based on current velocity
    // Higher speeds require more stopping distance
    int32_t currentSpeed = std::abs(velocity);
    
    // Base safe distance: 32 units minimum
    // Add velocity-dependent component: speed contributes to required distance
    int32_t safeDistance = 32 + (currentSpeed >> 12);
    
    // Additional safety margin for high-speed rides
    auto curRide = GetRide();
    if (curRide != nullptr)
    {
        auto rideEntry = GetRideEntry();
        if (rideEntry != nullptr)
        {
            // High-speed coasters need extra margin
            if (curRide->maxSpeed > 60_MPH)
            {
                safeDistance = (safeDistance * 3) / 2;
            }
        }
    }

    // Check if we're within the danger zone (less than safe distance)
    if (distance < safeDistance)
    {
        return false;
    }

    // Also check relative velocity - if we're closing in fast, need more distance
    int32_t relativeVelocity = velocity - precedingVehicle->velocity;
    if (relativeVelocity > 2_MPH)
    {
        // We're approaching the vehicle ahead
        // Require additional buffer based on closing speed
        int32_t closingBuffer = relativeVelocity >> 10;
        if (distance < safeDistance + closingBuffer)
        {
            return false;
        }
    }

    return true;
}

/**
 * Applies progressive emergency braking when vehicles are too close.
 * Braking force increases as distance decreases, providing smooth
 * deceleration rather than abrupt stops.
 * 
 * @param distanceToVehicle Current distance to the vehicle ahead
 * @param safeDistance Minimum safe distance that should be maintained
 */
void Vehicle::ApplyProgressiveEmergencyBraking(int32_t distanceToVehicle, int32_t safeDistance)
{
    if (distanceToVehicle >= safeDistance || velocity <= 0)
        return;

    // Calculate how critical the situation is (0.0 to 1.0)
    // 1.0 = collision imminent, 0.0 = at safe distance
    int32_t distanceRatio = (safeDistance - distanceToVehicle) * 256 / safeDistance;
    distanceRatio = std::clamp(distanceRatio, 0, 256);

    // Progressive braking force based on urgency
    // Minimum braking when just below safe distance
    // Maximum braking when very close to collision
    int32_t brakingForce = (minBrake * distanceRatio) >> 8;
    
    // If critically close (< 25% of safe distance), apply maximum braking
    if (distanceRatio > 192)
    {
        brakingForce = maxBrake;
        _vehicleMotionTrackFlags |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_AT_BLOCK_BRAKE;
    }

    // Apply braking force (negative acceleration)
    acceleration -= brakingForce;

    // Ensure we don't go backwards
    if (acceleration < -velocity)
    {
        acceleration = -velocity;
    }
}
