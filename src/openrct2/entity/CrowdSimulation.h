/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../common.h"
#include "../entity/EntityBase.h"
#include <vector>
#include <cstdint>

namespace OpenRCT2
{
    struct Guest;

    /**
     * Tipos de grupos de visitantes
     */
    enum class GuestGroupType : uint8_t
    {
        solo,           // Visitante individual
        couple,         // Pareja
        family,         // Familia con niños
        friends,        // Grupo de amigos
        tourGroup,      // Grupo turístico organizado
        schoolTrip,     // Excursión escolar
        count
    };

    /**
     * Tipos culturales que afectan el comportamiento
     */
    enum class GuestCultureType : uint8_t
    {
        local,          // Residente local - conoce bien el parque
        domesticTourist,// Turista nacional
        internationalTourist, // Turista internacional - más excitado pero se pierde fácil
        thrillSeeker,   // Buscador de emociones fuertes
        familyOriented, // Orientado a familias - prefiere atracciones suaves
        budgetTraveler, // Viajero económico - gasta menos
        luxuryVisitor,  // Visitante de lujo - gasta más en comodidades
        count
    };

    /**
     * Estados de ánimo colectivo para reacciones en cadena
     */
    enum class CollectiveMoodState : uint8_t
    {
        neutral,        // Estado normal
        euphoric,       // Euforia colectiva (atracción excelente)
        complaining,    // Reclamo colectivo (filas largas, precios altos)
        panicked,       // Pánico (accidente o cierre repentino)
        celebrating,    // Celebración (evento especial)
        angry,          // Enojo colectivo (vandalismo potencial)
        count
    };

    /**
     * Información de grupo de visitantes
     */
    struct GuestGroup
    {
        EntityId id;
        GuestGroupType type;
        std::vector<EntityId> members;
        EntityId leader;
        uint8_t cohesion; // Qué tan unido está el grupo (0-255)
        uint8_t patience; // Paciencia del grupo (0-255)
        bool splittingAllowed; // Permitir separación temporal
        
        GuestGroup()
            : id(EntityId::null)
            , type(GuestGroupType::solo)
            , leader(EntityId::null)
            , cohesion(200)
            , patience(150)
            , splittingAllowed(true)
        {
        }
        
        size_t getMemberCount() const { return members.size(); }
        bool hasMember(EntityId guestId) const;
        void addMember(EntityId guestId);
        void removeMember(EntityId guestId);
    };

    /**
     * Zona de influencia para reacciones en cadena
     */
    struct MoodZone
    {
        CoordsXY location;
        uint16_t radius; // Radio en tiles
        CollectiveMoodState currentMood;
        uint8_t intensity; // Intensidad del estado de ánimo (0-255)
        uint32_t duration; // Duración en ticks
        uint32_t creationTime; // Tick de creación
        EntityId sourceGuest; // Visitante que originó la emoción
        
        MoodZone()
            : location({ 0, 0 })
            , radius(0)
            , currentMood(CollectiveMoodState::neutral)
            , intensity(0)
            , duration(0)
            , creationTime(0)
            , sourceGuest(EntityId::null)
        {
        }
        
        bool isActive() const;
        void update(uint32_t currentTick);
    };

    /**
     * Sistema de gestión de multitudes mejorado
     */
    class CrowdSimulation
    {
    public:
        static CrowdSimulation& getInstance();
        
        // Inicialización y actualización
        void reset();
        void update();
        
        // Gestión de grupos
        GuestGroup* createGroup(GuestGroupType type);
        void dissolveGroup(EntityId groupId);
        GuestGroup* getGroup(EntityId groupId);
        GuestGroup* getGuestGroup(EntityId guestId);
        void assignGuestToGroup(EntityId guestId, EntityId groupId);
        void removeGuestFromGroup(EntityId guestId);
        
        // Cultura y comportamiento
        void setGuestCulture(EntityId guestId, GuestCultureType culture);
        GuestCultureType getGuestCulture(EntityId guestId) const;
        float getCulturalModifier(EntityId guestId, ModifierType type) const;
        
        // Reacciones en cadena
        void triggerCollectiveMood(
            const CoordsXY& location,
            uint16_t radius,
            CollectiveMoodState mood,
            uint8_t intensity,
            uint32_t duration,
            EntityId sourceGuest);
        void propagateMood(const CoordsXY& location, CollectiveMoodState mood, uint8_t intensity);
        CollectiveMoodState getAreaMood(const CoordsXY& location, uint16_t radius) const;
        
        // Notificaciones automáticas al staff
        void notifyStaffOfDisturbance(const CoordsXY& location, CollectiveMoodState mood);
        void requestSecurityResponse(const CoordsXY& location, uint8_t urgency);
        void requestEntertainerResponse(const CoordsXY& location, uint8_t priority);
        
        // Estadísticas y análisis
        uint32_t getTotalGroups() const;
        uint32_t getTotalGuestsInGroups() const;
        float getAverageGroupCohesion() const;
        std::vector<MoodZone> getActiveMoodZones() const;
        
        // Serialización para guardado/carga
        void serialise(DataSerialiser& stream);
        
    private:
        CrowdSimulation() = default;
        ~CrowdSimulation() = default;
        CrowdSimulation(const CrowdSimulation&) = delete;
        CrowdSimulation& operator=(const CrowdSimulation&) = delete;
        
        std::vector<GuestGroup> _groups;
        std::vector<MoodZone> _moodZones;
        std::unordered_map<EntityId, GuestCultureType> _guestCultures;
        std::unordered_map<EntityId, EntityId> _guestToGroup;
        
        uint32_t _nextGroupId;
        uint32_t _lastUpdateTick;
        
        void cleanupExpiredMoodZones();
        void updateGroupBehaviors();
        void processStaffNotifications();
    };

    // Tipos de modificadores culturales
    enum class ModifierType : uint8_t
    {
        spendingMultiplier,
        patienceModifier,
        excitementGain,
        nauseaTolerance,
        lostProbability,
        count
    };

} // namespace OpenRCT2
