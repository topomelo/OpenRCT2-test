/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "CrowdSimulation.h"
#include "../entity/Guest.h"
#include "../entity/Staff.h"
#include "../world/Park.h"
#include "../scenario/Scenario.h"
#include <algorithm>
#include <random>

namespace OpenRCT2
{
    // Instancia singleton
    CrowdSimulation& CrowdSimulation::getInstance()
    {
        static CrowdSimulation instance;
        return instance;
    }

    void CrowdSimulation::reset()
    {
        _groups.clear();
        _moodZones.clear();
        _guestCultures.clear();
        _guestToGroup.clear();
        _nextGroupId = 1;
        _lastUpdateTick = 0;
    }

    void CrowdSimulation::update()
    {
        auto currentTick = gCurrentTicks;
        
        // Actualizar cada tick para reacciones en tiempo real
        cleanupExpiredMoodZones();
        updateGroupBehaviors();
        processStaffNotifications();
        
        _lastUpdateTick = currentTick;
    }

    GuestGroup* CrowdSimulation::createGroup(GuestGroupType type)
    {
        GuestGroup group;
        group.id = EntityId(_nextGroupId++);
        group.type = type;
        
        // Configurar parámetros según tipo de grupo
        switch (type)
        {
            case GuestGroupType::solo:
                group.cohesion = 0;
                group.patience = 150;
                group.splittingAllowed = true;
                break;
            case GuestGroupType::couple:
                group.cohesion = 220;
                group.patience = 140;
                group.splittingAllowed = false;
                break;
            case GuestGroupType::family:
                group.cohesion = 200;
                group.patience = 120;
                group.splittingAllowed = false;
                break;
            case GuestGroupType::friends:
                group.cohesion = 180;
                group.patience = 160;
                group.splittingAllowed = true;
                break;
            case GuestGroupType::tourGroup:
                group.cohesion = 240;
                group.patience = 100;
                group.splittingAllowed = false;
                break;
            case GuestGroupType::schoolTrip:
                group.cohesion = 250;
                group.patience = 80;
                group.splittingAllowed = false;
                break;
            default:
                break;
        }
        
        _groups.push_back(group);
        return &_groups.back();
    }

    void CrowdSimulation::dissolveGroup(EntityId groupId)
    {
        auto it = std::find_if(
            _groups.begin(), 
            _groups.end(), 
            [groupId](const GuestGroup& g) { return g.id == groupId; });
        
        if (it != _groups.end())
        {
            // Liberar todos los miembros
            for (auto memberId : it->members)
            {
                _guestToGroup.erase(memberId);
            }
            _groups.erase(it);
        }
    }

    GuestGroup* CrowdSimulation::getGroup(EntityId groupId)
    {
        auto it = std::find_if(
            _groups.begin(), 
            _groups.end(), 
            [groupId](const GuestGroup& g) { return g.id == groupId; });
        
        return (it != _groups.end()) ? &(*it) : nullptr;
    }

    GuestGroup* CrowdSimulation::getGuestGroup(EntityId guestId)
    {
        auto mapIt = _guestToGroup.find(guestId);
        if (mapIt != _guestToGroup.end())
        {
            return getGroup(mapIt->second);
        }
        return nullptr;
    }

    void CrowdSimulation::assignGuestToGroup(EntityId guestId, EntityId groupId)
    {
        GuestGroup* group = getGroup(groupId);
        if (group)
        {
            group->addMember(guestId);
            _guestToGroup[guestId] = groupId;
            
            // Si es el primer miembro, es el líder
            if (group->getMemberCount() == 1)
            {
                group->leader = guestId;
            }
        }
    }

    void CrowdSimulation::removeGuestFromGroup(EntityId guestId)
    {
        auto mapIt = _guestToGroup.find(guestId);
        if (mapIt != _guestToGroup.end())
        {
            EntityId groupId = mapIt->second;
            GuestGroup* group = getGroup(groupId);
            
            if (group)
            {
                group->removeMember(guestId);
                
                // Si era el líder, asignar nuevo líder
                if (group->leader == guestId && !group->members.empty())
                {
                    group->leader = group->members[0];
                }
                
                // Disolver grupo si está vacío
                if (group->members.empty())
                {
                    dissolveGroup(groupId);
                }
            }
            
            _guestToGroup.erase(mapIt);
        }
    }

    void CrowdSimulation::setGuestCulture(EntityId guestId, GuestCultureType culture)
    {
        _guestCultures[guestId] = culture;
    }

    GuestCultureType CrowdSimulation::getGuestCulture(EntityId guestId) const
    {
        auto it = _guestCultures.find(guestId);
        return (it != _guestCultures.end()) ? it->second : GuestCultureType::local;
    }

    float CrowdSimulation::getCulturalModifier(EntityId guestId, ModifierType type) const
    {
        GuestCultureType culture = getGuestCulture(guestId);
        
        // Modificadores basados en cultura
        switch (culture)
        {
            case GuestCultureType::local:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 0.9f;
                    case ModifierType::patienceModifier: return 1.2f;
                    case ModifierType::excitementGain: return 0.8f;
                    case ModifierType::nauseaTolerance: return 1.0f;
                    case ModifierType::lostProbability: return 0.3f;
                    default: return 1.0f;
                }
                
            case GuestCultureType::domesticTourist:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 1.1f;
                    case ModifierType::patienceModifier: return 1.0f;
                    case ModifierType::excitementGain: return 1.1f;
                    case ModifierType::nauseaTolerance: return 1.0f;
                    case ModifierType::lostProbability: return 0.5f;
                    default: return 1.0f;
                }
                
            case GuestCultureType::internationalTourist:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 1.3f;
                    case ModifierType::patienceModifier: return 0.9f;
                    case ModifierType::excitementGain: return 1.3f;
                    case ModifierType::nauseaTolerance: return 0.9f;
                    case ModifierType::lostProbability: return 0.8f;
                    default: return 1.0f;
                }
                
            case GuestCultureType::thrillSeeker:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 1.0f;
                    case ModifierType::patienceModifier: return 0.8f;
                    case ModifierType::excitementGain: return 1.5f;
                    case ModifierType::nauseaTolerance: return 1.5f;
                    case ModifierType::lostProbability: return 0.4f;
                    default: return 1.0f;
                }
                
            case GuestCultureType::familyOriented:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 1.2f;
                    case ModifierType::patienceModifier: return 1.3f;
                    case ModifierType::excitementGain: return 0.7f;
                    case ModifierType::nauseaTolerance: return 0.8f;
                    case ModifierType::lostProbability: return 0.6f;
                    default: return 1.0f;
                }
                
            case GuestCultureType::budgetTraveler:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 0.6f;
                    case ModifierType::patienceModifier: return 1.1f;
                    case ModifierType::excitementGain: return 1.0f;
                    case ModifierType::nauseaTolerance: return 1.0f;
                    case ModifierType::lostProbability: return 0.5f;
                    default: return 1.0f;
                }
                
            case GuestCultureType::luxuryVisitor:
                switch (type)
                {
                    case ModifierType::spendingMultiplier: return 1.8f;
                    case ModifierType::patienceModifier: return 0.7f;
                    case ModifierType::excitementGain: return 0.9f;
                    case ModifierType::nauseaTolerance: return 1.1f;
                    case ModifierType::lostProbability: return 0.4f;
                    default: return 1.0f;
                }
                
            default:
                return 1.0f;
        }
    }

    void CrowdSimulation::triggerCollectiveMood(
        const CoordsXY& location,
        uint16_t radius,
        CollectiveMoodState mood,
        uint8_t intensity,
        uint32_t duration,
        EntityId sourceGuest)
    {
        MoodZone zone;
        zone.location = location;
        zone.radius = radius;
        zone.currentMood = mood;
        zone.intensity = intensity;
        zone.duration = duration;
        zone.creationTime = gCurrentTicks;
        zone.sourceGuest = sourceGuest;
        
        _moodZones.push_back(zone);
        
        // Notificar staff inmediatamente según tipo de estado de ánimo
        notifyStaffOfDisturbance(location, mood);
    }

    void CrowdSimulation::propagateMood(const CoordsXY& location, CollectiveMoodState mood, uint8_t intensity)
    {
        // Propagar el estado de ánimo a visitantes cercanos
        // Esto se integra con la lógica de actualización de Guests
        
        // Buscar todos los guests en el área
        // (Esta lógica se conecta con el sistema de entidades existente)
        
        // Aplicar modificadores de felicidad/enojo según intensidad
        // La implementación completa requiere acceso al gestor de entidades
    }

    CollectiveMoodState CrowdSimulation::getAreaMood(const CoordsXY& location, uint16_t radius) const
    {
        CollectiveMoodState dominantMood = CollectiveMoodState::neutral;
        uint8_t maxIntensity = 0;
        
        for (const auto& zone : _moodZones)
        {
            if (!zone.isActive())
                continue;
                
            auto dx = abs(zone.location.x - location.x);
            auto dy = abs(zone.location.y - location.y);
            auto distance = sqrt(dx * dx + dy * dy);
            
            if (distance <= (zone.radius + radius))
            {
                if (zone.intensity > maxIntensity)
                {
                    maxIntensity = zone.intensity;
                    dominantMood = zone.currentMood;
                }
            }
        }
        
        return dominantMood;
    }

    void CrowdSimulation::notifyStaffOfDisturbance(const CoordsXY& location, CollectiveMoodState mood)
    {
        switch (mood)
        {
            case CollectiveMoodState::angry:
            case CollectiveMoodState::panicked:
                // Solicitar seguridad inmediata
                requestSecurityResponse(location, 255);
                break;
                
            case CollectiveMoodState::euphoric:
            case CollectiveMoodState::celebrating:
                // Enviar animadores para aprovechar el momento
                requestEntertainerResponse(location, 200);
                break;
                
            case CollectiveMoodState::complaining:
                // Seguridad preventiva si la intensidad es alta
                // También notificar al manager del parque
                break;
                
            default:
                break;
        }
    }

    void CrowdSimulation::requestSecurityResponse(const CoordsXY& location, uint8_t urgency)
    {
        // Buscar guards de seguridad cercanos y redirigirlos
        // Esta función se integra con el sistema de Staff existente
        
        // Iterar sobre todo el staff de seguridad
        // Calcular distancia a la ubicación del disturbio
        // Asignar nueva ruta hacia la ubicación si están libres
        
        // La implementación completa requiere acceso a getAllEntities<Staff>()
    }

    void CrowdSimulation::requestEntertainerResponse(const CoordsXY& location, uint8_t priority)
    {
        // Buscar entertainers cercanos y redirigirlos
        // Similar a requestSecurityResponse pero para entertainers
        
        // Los entertainers aumentan la felicidad en áreas con euforia
        // para maximizar el efecto positivo
    }

    uint32_t CrowdSimulation::getTotalGroups() const
    {
        return static_cast<uint32_t>(_groups.size());
    }

    uint32_t CrowdSimulation::getTotalGuestsInGroups() const
    {
        uint32_t total = 0;
        for (const auto& group : _groups)
        {
            total += static_cast<uint32_t>(group.getMemberCount());
        }
        return total;
    }

    float CrowdSimulation::getAverageGroupCohesion() const
    {
        if (_groups.empty())
            return 0.0f;
            
        uint32_t totalCohesion = 0;
        for (const auto& group : _groups)
        {
            totalCohesion += group.cohesion;
        }
        
        return static_cast<float>(totalCohesion) / _groups.size();
    }

    std::vector<MoodZone> CrowdSimulation::getActiveMoodZones() const
    {
        std::vector<MoodZone> active;
        for (const auto& zone : _moodZones)
        {
            if (zone.isActive())
            {
                active.push_back(zone);
            }
        }
        return active;
    }

    void CrowdSimulation::serialise(DataSerialiser& stream)
    {
        stream << _groups;
        stream << _moodZones;
        stream << _guestCultures;
        stream << _guestToGroup;
        stream << _nextGroupId;
        stream << _lastUpdateTick;
    }

    void CrowdSimulation::cleanupExpiredMoodZones()
    {
        auto currentTick = gCurrentTicks;
        
        _moodZones.erase(
            std::remove_if(
                _moodZones.begin(),
                _moodZones.end(),
                [currentTick](const MoodZone& zone)
                {
                    return (currentTick - zone.creationTime) > zone.duration;
                }),
            _moodZones.end());
    }

    void CrowdSimulation::updateGroupBehaviors()
    {
        // Actualizar comportamientos de grupo basados en estado actual
        // - Verificar cohesión del grupo
        // - Ajustar paciencia según experiencias recientes
        // - Decidir si el grupo se separa temporalmente
        
        for (auto& group : _groups)
        {
            // Reducir paciencia con el tiempo en filas
            // Aumentar cohesión si están disfrutando juntos
            // Permitir separación si la paciencia es muy baja
        }
    }

    void CrowdSimulation::processStaffNotifications()
    {
        // Procesar cola de notificaciones al staff
        // Esta función se llama periódicamente para evitar sobrecarga
    }

    // Implementaciones de métodos de GuestGroup
    bool GuestGroup::hasMember(EntityId guestId) const
    {
        return std::find(members.begin(), members.end(), guestId) != members.end();
    }

    void GuestGroup::addMember(EntityId guestId)
    {
        if (!hasMember(guestId))
        {
            members.push_back(guestId);
        }
    }

    void GuestGroup::removeMember(EntityId guestId)
    {
        members.erase(
            std::remove(members.begin(), members.end(), guestId),
            members.end());
    }

    // Implementaciones de métodos de MoodZone
    bool MoodZone::isActive() const
    {
        auto currentTick = gCurrentTicks;
        return (currentTick - creationTime) < duration;
    }

    void MoodZone::update(uint32_t currentTick)
    {
        // Actualizar intensidad basada en tiempo transcurrido
        // El estado de ánimo se desvanece gradualmente
        if (isActive())
        {
            uint32_t elapsed = currentTick - creationTime;
            float fadeFactor = 1.0f - (static_cast<float>(elapsed) / duration);
            intensity = static_cast<uint8_t>(255 * fadeFactor);
        }
    }

} // namespace OpenRCT2
